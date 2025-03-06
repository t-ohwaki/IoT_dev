/* 環境センサーで測定した温度、湿度などのデーターをBluetooth Low Energy (BLE) でIoTゲートウェイに送信する */
/*　M5StickC_ENV環境センサーノードのサンプルプログラム
// 参考コード　M5StickC PlusでENV II （環境センサユニット）を動作させる（Qiita）
// M5StickC PlusでM5Stack用環境センサユニット ver.2（ENV II）の値を取得する

#include <M5StickCPlus.h>    /* M5StickCPlusの場合 */
// #include <M5StickC.h>　   /* M5StickC の場合  */
#include <Wire.h>
#include "Adafruit_Sensor.h"
#include <Adafruit_BMP280.h>
#include "UNIT_ENV.h"

#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_sleep.h"

#define T_PERIOD     10   // Transmission period
#define S_PERIOD     10  // Silent period
RTC_DATA_ATTR static uint8_t seq; // remember number of boots in RTC Memory
uint8_t mac[6];        /* MAC address 表示処理　*/
char devKey[20];       /* MAC address 表示処理　*/
// #ifdef ARDUINO_M5Stack_Core_ESP32
// #define SDA 21
// #define SCL 22
// #else
// #define SDA 12
// #define SCL 14
// #endif
SHT3X sht30;
QMP6988 qmp6988;
float tmp =0.0;
float hum =0.0;
float prs =0.0;

void setAdvData(BLEAdvertising *pAdvertising) {
     if (sht30.get() == 0){
        tmp = sht30.cTemp ;
        hum = sht30.humidity;
     }else{
      tmp = 0.0 ;
      hum = 0.0 ;
     }
     uint16_t temp = (uint16_t)(tmp * 100);
     uint16_t humid = (uint16_t)(hum * 100);
     prs = qmp6988.calcPressure()/100;
     uint16_t pressure =(uint16_t)(prs*100);
 
    double vbat = M5.Axp.GetVbatData() * 1.1 / 1000;  // バッテリー電圧を取得 ----D

    M5.Lcd.setCursor(0, 0, 1);
    M5.Lcd.printf("temp: %.1f'C\r\n", tmp);
    M5.Lcd.printf("humid:%.1f%%\r\n", hum);
    M5.Lcd.printf("press:%.1fhPa\r\n", prs);
    M5.Lcd.printf("vbat: %4.2fV\r\n", vbat);

 //   bme280.get_sensor_data(&data);
 //   Serial.printf("temp: %.1f, humid: %.1f, press: %.1f\r\n", data.temperature, data.humidity, data.pressure / 100);
 //   uint16_t temp = (uint16_t)(data.temperature * 100);
 //   uint16_t humid = (uint16_t)(data.humidity * 100);
 //   uint16_t press = (uint16_t)(data.pressure / 10);

    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

    oAdvertisementData.setFlags(0x06); // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

    std::string strServiceData = "";
    strServiceData += (char)0x0a;   // 長さ
    strServiceData += (char)0xff;   // AD Type 0xFF: Manufacturer specific data
    strServiceData += (char)0xff;   // Test manufacture ID low byte
    strServiceData += (char)0xff;   // Test manufacture ID high byte
    strServiceData += (char)seq;                   // シーケンス番号
    strServiceData += (char)(temp & 0xff);         // 温度の下位バイト
    strServiceData += (char)((temp >> 8) & 0xff);  // 温度の上位バイト
    strServiceData += (char)(humid & 0xff);        // 湿度の下位バイト
    strServiceData += (char)((humid >> 8) & 0xff); // 湿度の上位バイト
    strServiceData += (char)(pressure & 0xff);        // 気圧の下位バイト
    strServiceData += (char)((pressure >> 8) & 0xff); // 気圧の上位バイト

    oAdvertisementData.addData(strServiceData);
    pAdvertising->setAdvertisementData(oAdvertisementData);
}

void setup() {
    M5.begin();
    M5.Axp.ScreenBreath(10);    // 画面の輝度を少し下げる ----B
    M5.Lcd.setRotation(3);      // 左を上にする         ----C
    M5.Lcd.setTextSize(2);      // 文字サイズを2にする
    M5.Lcd.fillScreen(BLACK);   // 背景を黒にする
                             /* MAC ADDRESS 表示処理追加 ここから */
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(devKey,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]+2) ;
    Serial.println(devKey);
                           /* MAC ADDRESS 表示処理追加 ここまで */
    Wire.begin(); 
    // I2Cを初期化する
    while (!qmp6988.init(0x76)) {  // BMP280を初期化する
        M5.Lcd.println("QMP6988 init fail");
    }

    Serial.begin(115200);
    Serial.printf("start IoT %d\n",seq);

 //   bme280.begin(); // BME280の初期化

    BLEDevice::init("Env-Sensor-Node-10");                  // デバイスを初期化
    BLEServer *pServer = BLEDevice::createServer();    // サーバーを生成

    BLEAdvertising *pAdvertising = pServer->getAdvertising(); // アドバタイズオブジェクトを取得
    setAdvData(pAdvertising);                          // アドバタイジングデーターをセット
    M5.Lcd.printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]+2);   // MAC address 表示追加
    pAdvertising->start();                             // アドバタイズ起動
    Serial.println("Advertizing started...");
    delay(T_PERIOD * 1000);                            // T_PERIOD秒アドバタイズする
    pAdvertising->stop();                              // アドバタイズ停止

    seq++;                                             // シーケンス番号を更新

    Serial.printf("enter deep sleep\n");
    delay(10);
    esp_deep_sleep(1000000LL * S_PERIOD);              // S_PERIOD秒Deep Sleepする
    Serial.printf("in deep sleep\n");
}

void loop() {
}
