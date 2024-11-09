#include "mbed.h"
#include "rtos.h"
#include "FIRSTPENGUIN.HPP"
#include <SPI.h>

BufferedSerial pc(USBTX, USBRX, 115200); // ok: Serial communication with the PC

int leftJoystickX;
int leftJoystickY;
int rightJoystickX;
int left1;
int right1;
int mo1 = 0;
int mo2 = 0;

int ch1 = 0;
int16_t output;

int maxMotorSpeed = 10000; // maxMotorSpeedをグローバル変数として宣言
constexpr uint32_t can_id = 30;

CANMessage msgr;

CAN can{PA_11, PA_12, (int)1e6}; // CAN1

FirstPenguin penguin{can_id, can}; // FirstPenguinクラスのインスタンス

DigitalIn button1(PC_13);

// LEDピン定義
DigitalOut led(LED1);

// SPIピン定義
constexpr PinName SPI_MOSI1 = PA_7;
constexpr PinName SPI_MISO1 = PA_6;
constexpr PinName SPI_SCLK1 = PA_5;
constexpr PinName SPI_CS1 = PA_4;

SPISlave spi_dev(SPI_MOSI1, SPI_MISO1, SPI_SCLK1, SPI_CS1); // mosi, miso, sclk, ssel

void spi_setup()
{
    spi_dev.format(8, 0);      // 8bit, MODE0
    spi_dev.frequency(500000); // 500kHz
    spi_dev.reply(0x00);       // SPI first reply
    printf("spi setup done\n");
}

int spi_flag = 0;
int spi_count = 0;
char spi_recv_buf[16];
#define SPI_LEN 6
DigitalIn spi_cs_pin(SPI_CS1);
void recvSPIdata()
{
    int off, i;
    spi_setup();
    printf("spi receive started\n");
    char buf[16];
    while (1)
    {
        for (off = 0; off < SPI_LEN; off++)
        {
            while (!spi_dev.receive())
            {
                if (off > 0 && spi_cs_pin)
                {
                    // printf("spi abort\n");
                    goto out;
                }
            }
            buf[off] = spi_dev.read(); // Read byte from master
            spi_dev.reply(off);        // Make this the next reply
        }
    out:
        if (off == SPI_LEN)
        {
            for (i = 0; i < SPI_LEN; i++)
            {
                spi_recv_buf[i] = buf[i];
            }
            /*
                spi_recv_buf[0] = (buf[0] << 7) | (buf[1] >> 1) ;
                spi_recv_buf[1] = (buf[1] << 7) | (buf[2] >> 1) ;
                spi_recv_buf[2] = (buf[2] << 7) | (buf[3] >> 1) ;
                spi_recv_buf[3] = (buf[3] << 7) | (buf[4] >> 1) ;
                spi_recv_buf[4] = (buf[4] << 7) | (buf[5] >> 1) ;
                spi_recv_buf[5] = (buf[5] == 0);
            */
            spi_recv_buf[0] = buf[0] & 0x7F; // 最上位ビットをクリア
            spi_recv_buf[1] = buf[1];
            spi_recv_buf[2] = buf[2];
            spi_recv_buf[3] = buf[3];
            spi_recv_buf[4] = buf[4];
            spi_recv_buf[5] = buf[5];
            // for (i=0; i<SPI_LEN-1; i++)
            {
                //    spi_recv_buf[i] = buf[i+1];
            }
            // spi_recv_buf[5] = buf[0];
            spi_flag++;
            spi_count++;
        }
        if (spi_flag)
        {
            char *p = spi_recv_buf;
            printf("spi received cnt:%d %x %x %x %x %x %x\n",
                   spi_count, p[0], p[1], p[2], p[3], p[4], p[5]);
        }
        ThisThread::sleep_for(10ms);
    }
}

void sendCANmsg()
{
    printf("CAN sender started\n");
    while (1)
    {
        static int16_t save, save1;
        save1 = left1 + right1;
        if (save != save1)
        {
            // printf("L:%x R:%x\n", left1, right1);
            save = save1;
        }

        // Read feedback from CAN
        if (can.read(msgr))
        {
            penguin.read(msgr);
        }

        // ジョイスティック入力に基づくモーター制御
        penguin.pwm[0] = left1;  // -leftJoystickX * 0.4 - leftJoystickY * 0.4 - rightJoystickX;
        penguin.pwm[1] = mo1;    // leftJoystickX * 0.4 - leftJoystickY * 0.4 - rightJoystickX;
        penguin.pwm[2] = right1; // leftJoystickX * 0.4 + leftJoystickY * 0.4 - rightJoystickX;
        penguin.pwm[3] = mo2;    // leftJoystickY * 0.4 - leftJoystickX * 0.4 - rightJoystickX;

        // PWM値が最大速度を超えないようにする
        for (int i = 0; i < 4; ++i)
        {
            if (penguin.pwm[i] > maxMotorSpeed)
                penguin.pwm[i] = maxMotorSpeed;
        }
        //    int number_r = right1, number_R = right1, number_L = left1;
        //    pwm0[0] = static_cast<int16_t>(((number_r - number_R) * 90 - number_L * 30));
        //    pwm0[1] = static_cast<int16_t>(((number_r + number_R) * 90 - number_L * 30));
        //    pwm0[2] = static_cast<int16_t>(-((number_r + number_R) * 90 + number_L * 30));
        //    pwm0[3] = static_cast<int16_t>(-((number_r - number_R) * 90 + number_L * 30));

        penguin.send();
        ThisThread::sleep_for(10ms);
    }
}

void serialRead()
{
    static int8_t mode = 0;
    int8_t change = 0, i;
    char buf[20] = {0};
    if (spi_flag > 0)
    {
        if (buf[0] == 0x55)
        {
            for (i = 0; i < SPI_LEN; i++)
            {
                buf[i] = spi_recv_buf[i];
            }
            // buf++;
            for (int j = 0; j < SPI_LEN - 1; j++)
            {
                buf[j] = buf[j + 1];
            }
        }
        else
        {
            buf[0] = 0;
            // printf("spi 0x55 not found\n");
        }
        spi_flag = 0;
        change = 1;
    }

    if (pc.readable())
    {
        pc.read(buf, sizeof(buf));
        printf("pc.read %x\n", buf[0]);
        change = 1;
    }
    if (change > 0)
    {
        // if (*buf == 'a')
        // {
        //     mode = 1;
        // }
        // else if (*buf == 's')
        // {
        //     mode = 2;
        // }
        // else if (*buf == 'd')
        // {
        //     mode = 3;
        // }
        // else if (*buf == 'z')
        // {
        //     mode = 4;
        // }
        // else if (*buf == 'x')
        // {
        //     mode = 5;
        // }
        // else if (*buf == 'c')
        // {
        //     mode = 6;
        // }
        // else if (*buf == 'h')
        // {
        //     mode = 7;
        // }
        if (spi_recv_buf[1] == 'a')
        {
            mode = 1;
        }
        else if (spi_recv_buf[1] == 's')
        {
            mode = 2;
        }
        else if (spi_recv_buf[1] == 'd')
        {
            mode = 3;
        }
        else if (spi_recv_buf[1] == 'A')
        {
            mode = 4;
        }
        else if (spi_recv_buf[1] == 'S')
        {
            mode = 5;
        }
        else if (spi_recv_buf[1] == 'D')
        {
            mode = 6;
        }
        else if (spi_recv_buf[1] == 'h')
        {
            mode = 7;
        }
        else if (spi_recv_buf[1] == 'X')    
        {
            mode = 8;
        }
        else if (spi_recv_buf[1] == 'C')
        {
            mode = 9;
        }

        else if (spi_recv_buf[1] == 'Z')
        {
            mode = 10;
        }
        else if (spi_recv_buf[1] == 'x')
        {
            mode = 11;
        }
        else if (spi_recv_buf[1] == 'c')
        {
            mode = 12;
        }
        else if (spi_recv_buf[1] == 'z')
        {
            mode = 13;
        }
        else if (spi_recv_buf[1] == 'n')
        {
            mode = 14;
        }
        else if (spi_recv_buf[1] == 'N')
        {
            mode = 15;
        }
        else if (spi_recv_buf[1] == 'y')
        {
            mode = 16;
        }
        else if (spi_recv_buf[1] == 'i')
        {
            mode = 17;
        }
        
        else
        {   
            
            // 何もしない
        }
        printf("buf: %d", spi_recv_buf[1]);
        change = 1;
    }
    if (button1 == 0)
    { // push
        printf("button1 %d %d %d\n", mode, left1, right1);
        mode += 1;
        if (mode > 6)
            mode = 0;
        change = 1;
    }
    if (change > 0)
    {
            
        switch (mode)
        {
        case 0:
            break;
        case 1:
            left1 = 6380;
            break;
        case 2:
            left1 = 0;
            break;
        case 3:
            left1 = -6380;
            break;
        case 4:
            right1 = 16380;
            break;
        case 5:
            right1 = 0;
            break;
        case 6:
            right1 = -16380;
            break;
        case 8:
            mo1 = 6380;
            mo2 = 6380;
            break;
        case 9:
            mo1 = -6380;
            mo2 = 6380;
            break;
        case 10:
            mo1 = 0;
            mo2 = 6380;
            break;
        case 11:
            mo1 = 6380;
            mo2 = -6380;
            break;
        case 12:
            mo1 = -6380;
            mo2 = -6380;
            break;
        case 13:
            mo1 = 0;
            mo2 = -6380;
            break;
        case 14:
            mo1 = 6380;
            mo2 = 0;
            break;
        case 15:
            mo1 = -6380;
            mo2 = 0;
            break;
        case 16:
            mo1 = 16380;
            mo2 = -16380;
            break;
        case 17:
            mo1 = -16380;
            mo2 = 16380;
            break;

        case 7:
            break;
        }
        change = 0;
        ch1 = 0;
    }
    else
    {

        if (ch1 > 10)
        {
            left1 = 0;
            right1 = 0;
            mo1 = 0;
            mo2 = 0;
        }
        else
        {
            ch1++;
        }
    }
}

// void processPS4Input(const char *input)
// {
//     if (strcmp(input, "UP") == 0)
//     {
//         left1 = 16380;
//     }
//     else if (strcmp(input, "DOWN") == 0)
//     {
//         left1 = 0;
//     }
//     else if (strcmp(input, "LEFT") == 0)
//     {
//         right1 = 16380;
//     }
//     else if (strcmp(input, "RIGHT") == 0)
//     {
//         right1 = 0;
//     }
//     else if (strcmp(input, "CIRCLE") == 0)
//     {
//         // 例: CIRCLEボタンが押されたときの処理
//     }
//     else if (strcmp(input, "SQUARE") == 0)
//     {
//         // 例: SQUAREボタンが押されたときの処理
//     }
//     else if (strcmp(input, "TRIANGLE") == 0)
//     {
//         // 例: TRIANGLEボタンが押されたときの処理
//     }
//     else if (strcmp(input, "CROSS") == 0)
//     {
//         // 例: CROSSボタンが押されたときの処理
//     }
// }

Thread canThread; // Threadインスタンスの宣言
Thread spiThread; // Threadインスタンスの宣言

int main()
{
    printf("USB TX:%d RX:%d PA_11:%d PA_12:%d PB_13:%d PB_12:%d\n", USBTX, USBRX, PA_11, PA_12, PB_13, PB_12);
    printf("main1 started\n");

    wait_us(1000 * 100);          // 0.1sec
    canThread.start(recvSPIdata); // SPIデータ受信スレッドを開始
    wait_us(1000 * 100);          // 0.1sec
    spiThread.start(sendCANmsg);  // CANメッセージ送信スレッドを開始
    wait_us(1000 * 100);          // 0.1sec
    while (1)
    {
        serialRead();
        ThisThread::sleep_for(10ms);
    }
}
