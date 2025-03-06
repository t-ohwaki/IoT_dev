// M5core2 shot sensor BLEnode
// M5core2内蔵の加速度センサーにより、プレスのショットを検出し
// 単位時間あたりのショット数をBLE通信でアドバタイズする

#include <M5Core2.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_sleep.h"
#define THRESH  0.08.    // ショット検出の加速変化の閾値
#define T_PERIOD     1   // Transmission period
#define S_PERIOD     1  // Silent period
RTC_DATA_ATTR static uint8_t seq; // remember number of boots in RTC Memory
float accX = 0.0F;
float accY = 0.0F;
float accZ = 0.0F;
float a = 1;
float pre_accZ = 1.0F;
uint8_t mac[6];        /* MAC address 表示処理　*/
char devKey[20];       /* MAC address 表示処理　*/
int count =0 ;
int count_sum = 0 ;

void setAdvData(BLEAdvertising *pAdvertising) {
       uint16_t u_accX = (uint16_t)(accX * 100);
       uint16_t u_accY = (uint16_t)(accY * 100);
       uint16_t u_accZ = (uint16_t)(accZ * 100);



    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    oAdvertisementData.setFlags(0x06); // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

    std::string strServiceData = "";
    strServiceData += (char)0x0a;   // 長さ
    strServiceData += (char)0xff;   // AD Type 0xFF: Manufacturer specific data
    strServiceData += (char)0xff;   // Test manufacture ID low byte
    strServiceData += (char)0xff;   // Test manufacture ID high byte
    strServiceData += (char)seq;                   // シーケンス番号
    strServiceData += (char)(count & 0xff);         // ショット数の下位バイト
    strServiceData += (char)((count >> 8) & 0xff);  // ショット数の上位バイト
    strServiceData += (char)0xff;        // 未使用
    strServiceData += (char)0xff;        // 未使用
    strServiceData += (char)0xff;        // 未使用
    strServiceData += (char)0xff;       // 　未使用

    oAdvertisementData.addData(strServiceData);
    pAdvertising->setAdvertisementData(oAdvertisementData);
}

void setup() {
  M5.begin();
//  M5.Power.begin();
  M5.IMU.Init();
  M5.Lcd.setTextSize(3);
                               /* MAC ADDRESS 表示処理追加 ここから */
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(devKey,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]+2) ;
    Serial.println(devKey);
                           /* MAC ADDRESS 表示処理追加 ここまで */
}

void loop() {
  count = 0 ;
  for (int i=0; i<=150; i++) {
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.IMU.getAccelData(&accX, &accY, &accZ);
    M5.Lcd.setCursor(0, 0, 1);
    M5.Lcd.setTextSize(3);
    M5.Lcd.printf("Xacc: %5.2fG\r\n", accX);
    M5.Lcd.printf("Yacc: %5.2fG\r\n", accY);
    M5.Lcd.printf("Zacc: %5.2fG\r\n", accZ);
    M5.Lcd.setTextSize(5);
    M5.Lcd.printf("N=%d\r\n",count_sum);
 
  // ショット検出　→　BLE データ送信
  if ((accZ-pre_accZ) > THRESH) {
    count ++ ;
    count_sum ++ ;
    }
    pre_accZ = accZ ;
   delay(400);
  }
  // BLE 送信
       BLEDevice::init("shot-Sensor-Node-XX");         // デバイスを初期化　XXはセンサー番号
   BLEServer *pServer = BLEDevice::createServer();    // サーバーを生成
    BLEAdvertising *pAdvertising = pServer->getAdvertising(); // アドバタイズオブジェクトを取得
    setAdvData(pAdvertising);                          // アドバタイジングデーターをセット
    M5.Lcd.printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]+2);                             // MAC address 表示追加
    pAdvertising->start();                             // アドバタイズ起動
    Serial.println("Advertizing started...");
    delay(T_PERIOD * 1000);                            // T_PERIOD秒アドバタイズする
    pAdvertising->stop();                              // アドバタイズ停止
    delay(S_PERIOD * 1000);
    seq++;    
  }
  

  