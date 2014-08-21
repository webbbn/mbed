#include "mbed.h"

#include "MPU9250.h"

const uint8_t LED_ON = 0;
const uint8_t LED_OFF = 1;

int main() {
    DigitalOut red(LEDR);
    DigitalOut green(LEDG);
    DigitalOut blue(LEDB);
    Serial ser(USART3_TX, USART3_RX);
    SPI fl_spi(FLASH_MOSI, FLASH_MISO, FLASH_SCLK); // mosi, miso, sclk
    DigitalOut fl_cs(FLASH_CS);
    SPI sen_spi(SEN_MOSI, SEN_MISO, SEN_SCLK); // mosi, miso, sclk
    DigitalOut mpu_cs(MPU9250_CS);
    DigitalOut baro_cs(BARO_CS);
    mpu9250_spi mpu9250(sen_spi, MPU9250_CS);

    ser.baud(38400);

    wait(0.2);
    sen_spi.format(8,0);
    sen_spi.frequency(10000000);
    fl_spi.format(8,0);
    fl_spi.frequency(10000000);
    ser.printf("Hello World!\n\r");

    fl_cs=0;
    wait_us(10);
    fl_cs=1;

    // read JEDEC-ID
    ser.printf("JEDEC-ID\n\r");
    fl_cs=0;
    fl_spi.write(0x9F); // read JEDEC ID
    ser.printf("%x\n\r", fl_spi.write(0));
    ser.printf("%x\n\r", fl_spi.write(0));
    ser.printf("%x\n\r", fl_spi.write(0));
    fl_cs=1;

    ser.printf("Testing Write\n\r");

    // send WREN command
    fl_cs=0;
    fl_spi.write(0x06); // WREN
    fl_cs=1;
    wait_us(10);

    // write first byte
    fl_cs=0;
    fl_spi.write(0x02); // write
    fl_spi.write(0);// // 3 bytes for address, msb first
    fl_spi.write(0);
    fl_spi.write(0);
    fl_spi.write('A');
    fl_cs=1;
    wait_us(100);

    // write second byte
    fl_cs=0;
    fl_spi.write(0x02);
    fl_spi.write(0);
    fl_spi.write(0);
    fl_spi.write(1);
    fl_spi.write('B');
    fl_cs=1;
    wait_us(100);

    // write third byte (string end marker)
    fl_cs=0;
    fl_spi.write(0x02);
    fl_spi.write(0);
    fl_spi.write(0);
    fl_spi.write(2);
    fl_spi.write(0);
    fl_cs=1;
    wait_us(100);

    // read first byte
    fl_cs=0;
    fl_spi.write(0x03);  // read command
    fl_spi.write(0); // 3 byte address
    fl_spi.write(0);
    fl_spi.write(0);
    ser.printf("%c\n\r", fl_spi.write(0));
    fl_cs=1;
    wait_us(10);

    // read second byte
    fl_cs=0;
    fl_spi.write(0x03);
    fl_spi.write(0);
    fl_spi.write(0);
    fl_spi.write(1);
    ser.printf("%c\n\r", fl_spi.write(0));
    fl_cs=1;
    wait_us(10);

    // read third byte
    fl_cs=0;
    fl_spi.write(0x03);
    fl_spi.write(0);
    fl_spi.write(0);
    fl_spi.write(2);
    ser.printf("%c\n\r", fl_spi.write(0));
    fl_cs=1;
    wait(10);

    ser.printf("Testing Baro\n\r");

    // Reset the baro.
    wait(0.2);
    baro_cs=0;
    sen_spi.write(0x1E);
    wait(0.2);

    wait(0.2);
    baro_cs=0;
    sen_spi.write(0xA2);
    ser.printf("%x\n\r", sen_spi.write(0x0));
    ser.printf("%x\n\r", sen_spi.write(0x0));
    baro_cs=1;

    wait(0.2);
    baro_cs=0;
    sen_spi.write(0xA3);
    ser.printf("%x\n\r", sen_spi.write(0x0));
    ser.printf("%x\n\r", sen_spi.write(0x0));
    baro_cs=1;
    wait(10);

    ser.printf("Testing MPU\n\r");
    mpu9250.init(0, 0);
    ser.printf("MPU-9250 whoami: %x\n\r", mpu9250.whoami());

    while(1) {
	red = LED_ON;
	wait(1.0);
	red = LED_OFF;
	green = LED_ON;
	wait(1.0);
	green = LED_OFF;
	blue = LED_ON;
	wait(1.0);
	red = LED_ON;
	green = LED_ON;
	blue = LED_ON;
	wait(2.0);
	red = LED_OFF;
	green = LED_OFF;
	blue = LED_OFF;
	wait(2.0);
	ser.printf("Hello World!\n\r");
    }
}
