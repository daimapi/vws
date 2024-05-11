//UTF-8
////////////////////////////////////////////////////////environment setting
#include <PMS.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include "WiFi.h"
#include "ThingSpeak.h"
//#include <SoftwareSerial.h> 
//SoftwareSerial Serial1(2, 3); // 將Arduino Pin2設定為RX, Pin3設定為TX
PMS pms(Serial2);
PMS::DATA data;
int ledpin = 2;
Adafruit_BMP280 bmp;
const char *ssid     = "omg u sus"; //ssid:網路名稱
const char *password = "uc2s33tuc465zix"; //pasword：網路密碼
int i = 0;

WiFiClient client;
#define CHANNEL_ID 2544455
#define CHANNEL_WRITE_API_KEY "3ZEDL3WPV2JS3NNN"
///////////////////////////////////////////////////////
void connectwifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  Serial.println(String("Connecting to ")+ssid);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(ledpin,HIGH);
    delay(300);
    digitalWrite(ledpin,LOW);
    delay(300);
  }

  Serial.print("\nIP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("WiFi status:");
  WiFi.printDiag(Serial); //顯示WiFi連線狀態資訊：工作模式、Channel、SSID、Passphrase、BSSID
  Serial.println("WiFi RSSI:");
  Serial.println(WiFi.RSSI());
}
void setup()
{
  Serial.begin(9600);   
  Serial2.begin(9600);//for pms
  
  pms.passiveMode();
  
  unsigned status;
  status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    while (1) delay(10);
  }else{
    Serial.println("Good");
  }
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                Adafruit_BMP280::SAMPLING_X1,     /* Temp. oversampling */
                Adafruit_BMP280::SAMPLING_X1,    /* Pressure oversampling */
                Adafruit_BMP280::FILTER_OFF,      /* Filtering. */
                Adafruit_BMP280::STANDBY_MS_1); /* Standby time. */

  pms.sleep();
  connectwifi();
  ThingSpeak.begin(client);
}
void loop()
{
  if (Serial.available()>0 && Serial.read()=='1')
  {
    digitalWrite(ledpin,HIGH);
    Serial.println("o");
    BMPreq();
    PMSreq();
    ThingSpeak.setField(8, WiFi.RSSI());
    ThingSpeak.writeFields(CHANNEL_ID, CHANNEL_WRITE_API_KEY);
    Serial.println("d");
    digitalWrite(ledpin,LOW);
  }
}

void PMSreq()
{
  pms.wakeUp();
  
  delay(30000);
  pms.requestRead();
  if (pms.readUntil(data))
  {
    Serial.print(F("HUM (%): "));
    Serial.println(data.AMB_HUM/10);
    ThingSpeak.setField(4, data.AMB_HUM/10);
    
    Serial.print(F("PM 1.0 (ug/m^3): "));
    Serial.println(data.PM_AE_UG_1_0);
    ThingSpeak.setField(1, data.PM_AE_UG_1_0);
    
    Serial.print(F("PM 2.5 (ug/m^3): "));
    Serial.println(data.PM_AE_UG_2_5);
    ThingSpeak.setField(2, data.PM_AE_UG_2_5);

    Serial.print(F("PM 10 (ug/m^3): "));
    Serial.println(data.PM_AE_UG_10_0);
    ThingSpeak.setField(3, data.PM_AE_UG_10_0);

    Serial.print(F("HCHO (mg/m^3): "));
    Serial.println(data.AMB_HCHO);
    ThingSpeak.setField(7, data.AMB_HCHO);
  }
  delay(1000);
  pms.sleep();
}

void BMPreq()
{
  if (bmp.takeForcedMeasurement()) {
    Serial.print(F("Temperature = "));
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");
    ThingSpeak.setField(5, round(bmp.readTemperature()));

    Serial.print(F("Pressure = "));
    Serial.print(bmp.readPressure()/100);
    Serial.println(" hPa");
    ThingSpeak.setField(6, round(bmp.readPressure()/100));

    //Serial.print(F("Approx altitude = "));
    //Serial.print(bmp.readAltitude(1013.25)); /* Adjusted to local forecast! */
    //Serial.println(" m");

    Serial.println();
  } else {
    Serial.println("Forced measurement failed!");
  }
}
