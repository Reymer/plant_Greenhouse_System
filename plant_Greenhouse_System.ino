#define BLYNK_TEMPLATE_ID           "TMPLIUaHyBIk"
#define BLYNK_DEVICE_NAME           "IOT"
#define BLYNK_AUTH_TOKEN            "dCwff423jU3AjbcYrtxliwPI34ShTJPV"
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>

unsigned long previousMillis = 0;        // 時間存值 = 0
const long interval = 2000;              // 間隔 = 2秒
int Temp_con = 0;
int Light_con = 0;
int Motor_con = 0;
float humidity, temp_f;    // 從 DHT-11 讀取的值
static char ftemp[7];
char auth[] = BLYNK_AUTH_TOKEN;

//wifi connect
char ssid[] = "Slio";
char pass[] = "94870678";

#define DHTPIN D5 
#define DHTTYPE DHT11
#define RELAY_LIGHT D3 
#define RELAY_FAN D4
#define RELAY_MOTOR D6

WidgetLED FAN(V0);
WidgetLED LIGHT(V10);
WidgetLED MOTOR(V11);

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
BH1750 lightMeter;

// 設定V8為可寫入溫度變數
BLYNK_WRITE(V8)
{
  Temp_con = param.asInt(); 

  Serial.print(" The Temp_con value is: ");
  Serial.println(Temp_con);
  Serial.println();

}

// 設定V9為可寫入亮度變數
BLYNK_WRITE(V9)
{
  Light_con = param.asInt(); 

  Serial.print(" The Light_con value is: ");
  Serial.println(Light_con);
  Serial.println();

}

// 設定V12為可寫入土壤溼度值變數
BLYNK_WRITE(V12)
{
  Motor_con = param.asInt(); 

  Serial.print(" The Motor_con value is: ");
  Serial.println(Motor_con);
  Serial.println();

}

void sendSensor()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  // (ltux)值大於所設定(Temp_con)的值啟動風扇
  if (Temp_con > t){
    digitalWrite(RELAY_FAN, LOW);
    FAN.off();
  }else{
    digitalWrite(RELAY_FAN, HIGH);
    FAN.on();
  }

  Serial.println(h);
  Serial.println(t);
  Blynk.virtualWrite(V5, t);
  Blynk.virtualWrite(V6, h);
}

void lightsensor(){
  float lux = lightMeter.readLightLevel();
  if (isnan(lux)) {
    Serial.println("Failed to read from light sensor!");
    return;
  }
  // (lux)值小於所設定(Light_con)的值啟動植物燈
  if (Light_con < lux){
    digitalWrite(RELAY_LIGHT, LOW);
    LIGHT.off();
  }else{
    digitalWrite(RELAY_LIGHT, HIGH);
    LIGHT.on();
  }
  Blynk.virtualWrite(V7, lux);
}

 

//Thing Speak
const char* apiKey = "29SY6SR78EC65Q4K"; //Thing Speak WRITE API KEY
const char* resource = "/update?api_key=";
const char* server = "api.thingspeak.com"; // Thing Speak API server 
// 將資料寫入thingspeak
void thingspeak(){
  // 量測間等待至少 2 秒
  unsigned long currentMillis = millis(); //目前時間
 
  if(currentMillis - previousMillis >= interval) { //如果目前時間減去存值0秒大於2秒的，
   
    previousMillis = currentMillis; // 將最後讀取感測值的時間紀錄下來 

    // 讀取溫度大約 250 微秒!
    humidity = dht.readHumidity();          // 讀取濕度(百分比)
    temp_f = dht.readTemperature(true);     // 讀取溫度(華氏)
    
    // 檢查兩個值是否為空值
    if (isnan(humidity) || isnan(temp_f)) {
       Serial.println("Failed to read from DHT sensor!");
       return;
    }
  }

  Serial.println(temp_f);
  Serial.println(humidity);

  float tempe = (temp_f-32)*5/9;   
  dtostrf(tempe, 6, 2, ftemp);
  dtostrf(humidity, 6, 2, ftemp);

  // 除錯用
  Serial.println(tempe);
  Serial.println(humidity);
  
  Serial.print("Connecting to "); 
  Serial.print(server);
  WiFiClient client;

  // 使用 80 Port 連線
  if (client.connect(server, 80)) {
    Serial.println(F("connected"));
  } 
  else  {
    Serial.println(F("connection failed"));
    return;
  }
   
  Serial.print("Request resource: "); 
  Serial.println(resource);
  client.print(String("GET ") + resource + apiKey + "&field1=" + tempe + "&field2=" + humidity +
                  " HTTP/1.1\r\n" +
                  "Host: " + server + "\r\n" + 
                  "Connection: close\r\n\r\n");
                  
  int timeout = 5 * 10; // 5秒             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  
  if(!client.available()) {
     Serial.println("No response, going back to sleep");
  }
  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\nclosing connection");
  client.stop();
}

// 設定預設值
void setup()
{
  // Debug console
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  WiFi.begin(ssid, pass);
  // 等待連線，並從 Console顯示 IP
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  dht.begin();
  Wire.begin();
  lightMeter.begin();
  pinMode(A0, INPUT);

  pinMode(RELAY_LIGHT, OUTPUT); 
  pinMode(RELAY_FAN, OUTPUT); 
  pinMode(RELAY_MOTOR, OUTPUT); 

  digitalWrite(RELAY_LIGHT, LOW);
  digitalWrite(RELAY_FAN, LOW);
  digitalWrite(RELAY_MOTOR, LOW);

  // 時間行程
  timer.setInterval(1000L, sendSensor);
  timer.setInterval(1000L, lightsensor);
  timer.setInterval(1000L, soilmoisturesensor);
  timer.setInterval(1000L, thingspeak);
}

void loop()
{
  Blynk.run();
  timer.run();
}