#include <WiFi.h>
#include <PS4Controller.h>
#include <HardwareSerial.h>
#include "driver/spi_master.h"
#include "driver/spi_slave.h"
#include <Arduino.h>
#include <SPI.h>

// 通信バッファ
uint8_t* spi_master_tx_buf;
uint8_t* spi_master_rx_buf;

// HSPI の端子設定
static const uint8_t SPI_MASTER_CS = 15;
static const uint8_t SPI_MASTER_CLK = 14;
static const uint8_t SPI_MASTER_MOSI = 13;
static const uint8_t SPI_MASTER_MISO = 12;

// SPI 通信周波数
static const uint32_t SPI_CLK_HZ = 500000;



// 通信サイズ
static const uint32_t TRANS_SIZE = 6;

// SPI 通信に使用するバッファの初期化
void spi_buf_init() {
  spi_master_tx_buf = (uint8_t*)heap_caps_malloc(TRANS_SIZE + 2, MALLOC_CAP_DMA);
  spi_master_rx_buf = (uint8_t*)heap_caps_malloc(TRANS_SIZE + 2, MALLOC_CAP_DMA);

  //for (uint32_t i = 0; i < TRANS_SIZE; i++) {
  //  spi_master_tx_buf[i] = i & 0xFF;
  //}
  memset(spi_master_tx_buf, 0, TRANS_SIZE);
  memset(spi_master_rx_buf, 0, TRANS_SIZE);
}

#if 0
void spi_init() {
  spi_buf_init();
  SPI.begin(SPI_MASTER_CLK, SPI_MASTER_MISO, SPI_MASTER_MOSI);//, SPI_MASTER_CS);
  pinMode(SPI_MASTER_CS, INPUT_PULLUP);
  //SPI.beginTransaction(SPISettings(SPI_CLK_HZ, MSBFIRST, SPI_MODE3));
//  SPI.setFrequency(SPI_CLK_HZ); //SSD1331 のSPI Clock Cycle Time 最低150ns
//  SPI.setDataMode(SPI_MODE3);
}

void send_spi_sub() {
#define SPI_MODE SPI_MODE0  // SPI_MODE3
  SPI.beginTransaction(SPISettings(SPI_CLK_HZ, MSBFIRST, SPI_MODE));
  digitalWrite(SPI_MASTER_CS, LOW); // SSをアクティブにする
  SPI.transfer(spi_master_tx_buf, TRANS_SIZE);
  digitalWrite(SPI_MASTER_CS, HIGH); // SSを非アクティブにして通信を終了
  SPI.endTransaction();
}
#else

// SPI マスタの設定
spi_transaction_t spi_master_trans;
spi_device_interface_config_t spi_master_cfg;
spi_device_handle_t spi_master_handle;
spi_bus_config_t spi_master_bus;

// HSPI の初期化
void spi_master_init() {
  spi_master_trans.flags = 0;
  spi_master_trans.length = 8 * TRANS_SIZE;
  spi_master_trans.rx_buffer = spi_master_rx_buf;
  spi_master_trans.tx_buffer = spi_master_tx_buf;

  spi_master_cfg.mode = SPI_MODE0;  // MODE3 bug
  spi_master_cfg.clock_speed_hz = SPI_CLK_HZ;
  spi_master_cfg.spics_io_num = SPI_MASTER_CS;
  spi_master_cfg.queue_size = 1;  // キューサイズ
  spi_master_cfg.flags = SPI_DEVICE_NO_DUMMY;
  spi_master_cfg.queue_size = 1;
  spi_master_cfg.pre_cb = NULL;
  spi_master_cfg.post_cb = NULL;

  spi_master_bus.sclk_io_num = SPI_MASTER_CLK;
  spi_master_bus.mosi_io_num = SPI_MASTER_MOSI;
  spi_master_bus.miso_io_num = SPI_MASTER_MISO;
  spi_master_bus.max_transfer_sz = TRANS_SIZE;

  ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &spi_master_bus, 0));  // not DMA
  ESP_ERROR_CHECK(
    spi_bus_add_device(VSPI_HOST, &spi_master_cfg, &spi_master_handle));
}

void spi_init() {
  spi_buf_init();
  spi_master_init();
}

void send_spi_sub() {
  spi_transaction_t* spi_trans = (spi_transaction_t*)spi_master_tx_buf;

  // マスターの送信開始
  ESP_ERROR_CHECK(spi_device_queue_trans(spi_master_handle, &spi_master_trans,
                                         portMAX_DELAY));

  // 通信完了待ち
  spi_device_get_trans_result(spi_master_handle, &spi_trans, portMAX_DELAY);
}
#endif

void setup() {
  Serial.begin(115200);
  PS4.begin("60:45:2e:4b:1f:d0");
  // WiFi.begin(ssid, password);
  // #if 0
  //   while (WiFi.status() != WL_CONNECTED) {
  //      delay(1000);
  //      Serial.println("つないでるぜ");
  //   }
  // #endif
  Serial.println("つながったぜ");
  spi_init();
}


extern void data_send(char* buf, char flag, int val);
/////////////////////////////////
int tien = 20;
int sw = 0;
int souryou = 0;
// int souryou2 = 20;
/////////////////////////////////
void loop() {
  if (PS4.isConnected()) {
    /////if (PS4.ボタンのなまえ()) print_and_udp(送る文字,引数があるか*あったら1*、引数、送る量）
    sw = 0;
    if (PS4.Right()) data_send("D", 0, 0, souryou);
    if (PS4.Left()) data_send("A", 0, 0, souryou);
    if (PS4.Up()) data_send("W", 0, 0, souryou);
    if (PS4.Down()) data_send("S", 0, 0, souryou);
    if (PS4.Circle()) data_send("d", 0, 0, souryou);
    if (PS4.Square()) data_send("a", 0, 0, souryou);
    if (PS4.Triangle()) data_send("w", 0, 0, souryou);
    if (PS4.Cross()) data_send("s", 0, 0, souryou);
    if (PS4.L1()) data_send("y", 0, 0, souryou);
    if (PS4.R1()) data_send("i", 0, 0, souryou);
    if (PS4.L3()) data_send("r", 0, 0, souryou);
    if (PS4.R3()) data_send("l", 0, 0, souryou);

    if (PS4.R2()) {
      int R2 = PS4.R2Value();
      if (R2 > 90) data_send("9", 1, R2, souryou);
    }
    if (PS4.L2()) {
      int L2 = PS4.L2Value();
      if (L2 > 90) data_send("7", 2, L2, souryou);
    }

    int gosa = 12;

    // int Rx = PS4.RStickY();
    // if (Rx < -80 && Rx > -130) {
    //   data_send("z", 3, Rx, souryou);
    // } else if (Rx < -40 && Rx > -80) {
    //   data_send("2", 3, Rx, souryou);
    // } else if (Rx < -12 && Rx > -40) {
    //   data_send("3", 3, Rx, souryou);
    // } else if (Rx < 40 && Rx > 12) {
    //   data_send("4", 3, Rx, souryou);
    // } else if (Rx < 80 && Rx > 40) {
    //   data_send("5", 3, Rx, souryou);
    // } else if (Rx < 130 && Rx > 80) {
    //   data_send("n", 3, Rx, souryou);
    // }
    // int Ly = PS4.LStickY();
    // if (Ly < gosa && Ly > -gosa) {
    //   print_and_udp("l", 1, 0, souryou);
    // } else {
    //   print_and_udp("l", 1, Ly, souryou);
    // }

    // int Lx = PS4.LStickX();
    // if (Lx < -80 && Lx > -130) {
    //   data_send("z", 4, Lx, souryou);
    // } else if (Lx < -40 && Lx > -80) {
    //   data_send("x", 4, Lx, souryou);
    // } else if (Lx < -12 && Lx > -40) {
    //   data_send("c", 4, Lx, souryou);
    // } else if (Lx < 40 && Lx > 12) {
    //   data_send("v", 4, Lx, souryou);
    // } else if (Lx < 80 && Lx > 40) {
    //   data_send("b", 4, Lx, souryou);
    // } else if (Lx < 130 && Lx > 80) {
    //   data_send("n", 4, Lx, souryou);
    // }

    // int Ly = PS4.LStickY();
    // if (Ly < -80 && Ly > -130) {
    //   data_send("Z", 5, Ly, souryou);
    // } else if (Ly < -40 && Ly > -80) {
    //   data_send("X", 5, Ly, souryou);
    // } else if (Ly < -12 && Ly > -40) {
    //   data_send("C", 5, Ly, souryou);
    // } else if (Ly < 40 && Ly > 12) {
    //   data_send("V", 5, Ly, souryou);
    // } else if (Ly < 80 && Ly > 40) {
    //   data_send("B", 5, Ly, souryou);
    // } else if (Ly < 130 && Ly > 80) {
    //   data_send("N", 5, Ly, souryou);
    // }
    int Rx = PS4.RStickY();
    int Ra;
    if (Rx < -80 && Rx > -130) {
      Ra = 2;
    } else if (Rx < 130 && Rx > 80) {
      Ra = 1;
    }else{
      Ra = 0;
    }
    int Ly = PS4.LStickY();
    if (Ly < -80 && Ly > -130) {
      if (Ra == 1) {
        data_send("X", 3, Rx, souryou);
      } else if (Ra == 2) {
        data_send("C", 3, Rx, souryou);
      } else {
        data_send("Z", 5, Ly, souryou);
      }
    } else if (Ly < 130 && Ly > 80) {
      if (Ra == 1) {
        data_send("x", 3, Rx, souryou);
      } else if (Ra == 2) {
        data_send("c", 3, Rx, souryou);
      } else {
        data_send("z", 5, Ly, souryou);
      }
    } else {
      if (Ra == 1) {
        data_send("n", 3, Rx, souryou);
      } else if (Ra == 2) {
        data_send("N", 3, Rx, souryou);
      } 
    }



    if (sw == 0) {
      //   data_send("h", 0, 0, souryou);
    }
  } else {
    //print_and_udp("No", 0, 0, souryou);
  }
  delay(tien);  // 20ms
}

// void data_send(char* buf, char flag, int val, int val2) {

//   sw = 1; spi_master_tx_buf[0] = buf[0];
//   spi_master_tx_buf[1] = 0;
//   spi_master_tx_buf[2] = flag;
//   spi_master_tx_buf[3] = val >> 8;
//   spi_master_tx_buf[4] = val & 0xff;
//   spi_master_tx_buf[5] = val2;
//   send_spi_sub();
//   Serial.printf(buf);
//   //Serial.println("send done");
// }
void data_send(char* buf, char flag, int val, int val2) {
  sw = 1;
  spi_master_tx_buf[0] = 0x55;
  spi_master_tx_buf[1] = buf[0];
  spi_master_tx_buf[2] = flag;
  spi_master_tx_buf[3] = val >> 8;
  spi_master_tx_buf[4] = val & 0xff;
  spi_master_tx_buf[5] = val2;
  send_spi_sub();
}
