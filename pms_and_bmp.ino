// UTF-8
//esp32 board mngr 2.0.7 passed
////////////////////////////////////////////////////////environment setting
#include <PMS.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include "WiFi.h"
#include "ThingSpeak.h"
#include "time.h"
#include <ESP32Time.h>
#include "token.h"
ESP32Time rtc;
PMS pms(Serial2);
PMS::DATA data;
Adafruit_BMP280 bmp;
WiFiClient client;


const int time_array[] = {7200, 72000, 50400, 28800};
//const int time_array[] = {6240};
///////////////////////////////////////////////////////
void connectwifi() {
  WiFi.mode(WIFI_STA);
  delay(100);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    Serial.println(String("Connecting to ") + ssid);
    if (pw == "") {
      WiFi.begin(ssid);
    } 
    else {
      WiFi.begin(ssid, pw);
    }
    int c = 0;
    while (WiFi.status() != WL_CONNECTED && c < 2000) {
      c += 1;
      delay(10);
      Serial.print(".");
    }
  }
  Serial.println("");
  Serial.println(WiFi.macAddress());
  // how to sleep ???
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nIP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("WiFi status:");
    WiFi.printDiag(Serial); // 顯示WiFi連線狀態資訊：工作模式、Channel、SSID、Passphrase、BSSID
    Serial.println("WiFi RSSI:");
    Serial.println(WiFi.RSSI());
    Serial.println("");
  }
  else
    Serial.println("connection failed");
}

int sleeptime() {
  struct tm now = rtc.getTimeStruct();
  int nowt = now.tm_hour * 3600 + now.tm_min * 60 + now.tm_sec;
  Serial.print("nowt : ");
  Serial.println(nowt);
  int tarray[sizeof(time_array)/sizeof(time_array[0])] = {};
  for (int i = 0; i < (sizeof(time_array)/sizeof(time_array[0])); i++) {
      tarray[i] = time_array[i];
  }
  
  for (int i = 0; i < (sizeof(tarray)/sizeof(tarray[0])); i++) {
      tarray[i] = tarray[i] - nowt;
      if (tarray[i] < 0) {
          tarray[i] = tarray[i] + 86400;
      }
  }
  int minVal = tarray[0];
  for (int i = 0; i < (sizeof(tarray)/sizeof(tarray[0])); i++) {
      minVal = min(tarray[i], minVal);
  }
  return minVal;
}

void PMSreq() {
  // pms.wakeUp();

  // delay(30000);
  pms.requestRead();
  if (pms.readUntil(data)) {
    Serial.print(F("HUM (%): "));
    Serial.println(float(data.AMB_HUM)/10);
    ThingSpeak.setField(4, data.AMB_HUM);

    Serial.print(F("PM 1.0 (ug/m^3): "));
    Serial.println(data.PM_AE_UG_1_0);
    ThingSpeak.setField(1, data.PM_AE_UG_1_0);

    Serial.print(F("PM 2.5 (ug/m^3): "));
    Serial.println(data.PM_AE_UG_2_5);
    ThingSpeak.setField(2, data.PM_AE_UG_2_5);

    Serial.print(F("PM 10 (ug/m^3): "));
    Serial.println(data.PM_AE_UG_10_0);
    ThingSpeak.setField(3, data.PM_AE_UG_10_0);

    //Serial.print(F("HCHO (mg/m^3): "));
    //Serial.println(data.AMB_HCHO);
    //ThingSpeak.setField(7, data.AMB_HCHO);
  }
  delay(1000);
  pms.sleep();
}

void BMPreq() {
  if (bmp.takeForcedMeasurement()) {
    Serial.print(F("Temperature = "));
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");
    ThingSpeak.setField(5, round(bmp.readTemperature()*10));

    Serial.print(F("Pressure = "));
    Serial.print(bmp.readPressure() / 100);
    Serial.println(" hPa");
    ThingSpeak.setField(6, round(bmp.readPressure() / 10));
    // Serial.print(F("Approx altitude = "));
    // Serial.print(bmp.readAltitude(1013.25)); /* Adjusted to local forecast! */
    // Serial.println(" m");
  }
  else {
    Serial.println("Forced measurement failed!");
  }
}

void NTPsync() {
  struct tm t;
  configTime(8 * 3600L, 0, "time.stdtime.gov.tw", "time.google.com");
  while (!getLocalTime(&t)) {
    Serial.println("getLocalTime Error");
    delay(500);
  }
  rtc.setTimeStruct(t);
  // rtc.adjust(DateTime(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec));
  Serial.print("NTPsynced : ");
  Serial.println(&t, "%A, %B %d %Y %H:%M:%S");
}

void setup() {
  pms.passiveMode();
  pinMode(2, OUTPUT);
  pinMode(15, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15,0);
  digitalWrite(2,LOW);
  Serial.begin(9600);
  Serial2.begin(9600); // for pms
  delay(1000);
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  Serial.print("hello world : ");
  Serial.println(wakeup_reason);
  Serial.println(esp_reset_reason());
  Serial.print("batt (V) : ");
  float batt = analogRead(34);
  Serial.println(batt/2048*3.3);
  
  Serial.println("bmp waking up");
  pms.wakeUp();
  int start_t = millis();
  Serial.println("pms waking up");
  unsigned status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    while (1)
      delay(10);
  }
  else {
    Serial.println("bmp init pass");
    bmp.setSampling(Adafruit_BMP280::MODE_FORCED,   /* Operating Mode. */
                Adafruit_BMP280::SAMPLING_X1,   /* Temp. oversampling */
                Adafruit_BMP280::SAMPLING_X1,   /* Pressure oversampling */
                Adafruit_BMP280::FILTER_OFF,    /* Filtering. */
                Adafruit_BMP280::STANDBY_MS_1); /* Standby time. */
  }
  connectwifi();
  ThingSpeak.begin(client);
  int inter = millis()-start_t;
  Serial.println(inter);
  if (inter < 30000) delay(30000 - inter);
  Serial.println("pms init pass");
  
  Serial.println("start process");
  BMPreq();
  PMSreq();
  ThingSpeak.setField(7, round(batt/2048*330));
  //ThingSpeak.writeFields(CHANNEL_ID, CHANNEL_WRITE_API_KEY);
  
  struct tm nowt = rtc.getTimeStruct();
  Serial.println(&nowt, "%A, %B %d %Y %H:%M:%S");
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER || wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    ThingSpeak.writeFields(CHANNEL_ID, CHANNEL_WRITE_API_KEY);
    Serial.println("sent");
    if (rtc.getHour(true) == 2 || wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) NTPsync(); // sync every 02:00 (also ntpsync while ext isnt essential)
    else rtc.setTime(rtc.getEpoch() + 8*3600ULL);
  }
  else {
    NTPsync(); //hardware reset( cpu + rtc )
  }
  
  Serial.println("end process");
  int sleep_time = sleeptime();
  if (sleep_time == 0) {
    sleep_time++;
  }
  Serial.print("byebye for : ");
  Serial.println(sleep_time);
  Serial.flush(); 
  esp_sleep_enable_timer_wakeup(sleep_time * 1000000ULL);
  esp_deep_sleep_start();
  //ESP.deepSleep(sleep_time * 1000000);
}

void loop() {}
