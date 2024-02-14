// driver for the GSL1680 touch panel
// Information gleaned from https://github.com/rastersoft/gsl1680.git and various other sources
// firmware for the specific panel was found here:- http://www.buydisplay.com/default/5-inch-tft-lcd-module-800x480-display-w-controller-i2c-serial-spi
// As was some test code.
// This is for that 800X480 display and the 480x272 from buydisplay.com

/*
Pin outs
the FPC on the touch panel is six pins, pin 1 is to the left pin 6 to the right with the display facing up

pin | function  | Teensy LC
-----------------------------
1   | SCL       | A5 19
2   | SDA       | A4 18
3   | VDD (3v3) | 3v3
4   | Wake      | A7 21
5   | Int       | A8 22
6   | Gnd       | gnd
*/
#include <Wire.h>
#include "Arduino.h"

// Pins
#define WAKE 21
#define INTRPT 22

#define SCREEN_MAX_X 		800
#define SCREEN_MAX_Y 		480

#define GSLX680_I2C_ADDR 	0x40

#define GSL_DATA_REG		0x80
#define GSL_STATUS_REG		0xe0
#define GSL_PAGE_REG		0xf0

#define delayus delayMicroseconds

struct _coord { uint32_t x, y; uint8_t finger; };

struct _ts_event {
	uint8_t  n_fingers;
	struct _coord coords[5];
};

struct _ts_event ts_event;

static inline void wiresend(uint8_t x)
{
#if ARDUINO >= 100
	Wire.write((uint8_t)x);
#else
	Wire.send(x);
#endif
}

static inline uint8_t wirerecv(void)
{
#if ARDUINO >= 100
	return Wire.read();
#else
	return Wire.receive();
#endif
}

bool i2c_write(uint8_t reg, uint8_t *buf, int cnt)
{
#if 0
	Serial.print("i2c write: "); Serial.println(reg, HEX);
	for(int i = 0; i < cnt; i++) {
		Serial.print(buf[i], HEX); Serial.print(",");
	}
	Serial.println();
#endif

	Wire.beginTransmission(GSLX680_I2C_ADDR);
	wiresend(reg);
	for(int i = 0; i < cnt; i++) {
		wiresend(buf[i]);
	}
	int r = Wire.endTransmission();
	if(r != 0) {
        // Serial.print("i2c write error: ");
        // Serial.print(r);
        // Serial.print(" ");
        // Serial.println(reg, HEX);
    }
	return r == 0;
}

int i2c_read(uint8_t reg, uint8_t *buf, int cnt)
{
	Wire.beginTransmission(GSLX680_I2C_ADDR);
	wiresend(reg);
	int r = Wire.endTransmission();
	if(r != 0) { Serial.print("i2c read error: "); Serial.print(r); Serial.print(" "); Serial.println(reg, HEX); }

	int n = Wire.requestFrom(GSLX680_I2C_ADDR, cnt);
	if(n != cnt) { Serial.print("i2c read error: did not get expected count "); Serial.print(n); Serial.print(" - "); Serial.println(cnt); }

	for(int i = 0; i < n; i++) {
		buf[i] = wirerecv();
	}
	return n;
}

void clr_reg(void)
{
	uint8_t buf[4];

	buf[0] = 0x88;
	i2c_write(0xe0, buf, 1);
	delay(20);

	buf[0] = 0x01;
	i2c_write(0x80, buf, 1);
	delay(5);

	buf[0] = 0x04;
	i2c_write(0xe4, buf, 1);
	delay(5);

	buf[0] = 0x00;
	i2c_write(0xe0, buf, 1);
	delay(20);
}

void reset_chip()
{
	uint8_t buf[4];

	buf[0] = 0x88;
	i2c_write(GSL_STATUS_REG, buf, 1);
	delay(20);

	buf[0] = 0x04;
	i2c_write(0xe4, buf, 1);
	delay(10);

	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	i2c_write(0xbc, buf, 4);
	delay(10);
}

void startup_chip(void)
{
	uint8_t buf[4];

	buf[0] = 0x00;
	i2c_write(0xe0, buf, 1);
}

void init_chip()
{
	digitalWrite(WAKE, HIGH);
	delay(50);
	digitalWrite(WAKE, LOW);
	delay(50);
	digitalWrite(WAKE, HIGH);
	delay(30);

	delay(500);

	// CTP startup sequence
	clr_reg();
	reset_chip();
}

int read_data(void)
{

	//Serial.println("reading data...");
	uint8_t touch_data[24] = {0};
	int n = i2c_read(GSL_DATA_REG, touch_data, 24);
	//Serial.print("read: "); Serial.println(n);
    if(n != 24) return -1;
	ts_event.n_fingers = touch_data[0];
	for(int i = 0; i < ts_event.n_fingers; i++) {
		ts_event.coords[i].x = ( (((uint32_t)touch_data[(i * 4) + 5]) << 8) | (uint32_t)touch_data[(i * 4) + 4] ) & 0x00000FFF; // 12 bits of X coord
		ts_event.coords[i].y = ( (((uint32_t)touch_data[(i * 4) + 7]) << 8) | (uint32_t)touch_data[(i * 4) + 6] ) & 0x00000FFF;
		ts_event.coords[i].finger = (uint32_t)touch_data[(i * 4) + 7] >> 4; // finger that did the touch
	}

	return ts_event.n_fingers;
}

bool is_fw_loaded()
{
    uint8_t buf[4];
    buf[0]= 0x00;
    i2c_write(GSL_PAGE_REG, buf, 1);
    i2c_read(0x00, buf, 4);
    i2c_read(0x00, buf, 4);
    //printf("%02X, %02X, %02X,%02X\r\n", buf[0], buf[1], buf[2], buf[3]);
    return (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x00 && buf[3] == 0x01);
}

void gsl_load_fw(uint8_t addr, uint8_t Wrbuf[4])
{
	i2c_write(addr, Wrbuf, 4);
}

void gsl_final_setup()
{
	reset_chip();
	startup_chip();
}

extern "C" void add_touch_event(struct _ts_event*);
void handle_read_irq()
{
    int n = read_data();
    if(n >= 0) {
        add_touch_event(&ts_event);
    }
}

void gsl_setup()
{
	pinMode(WAKE, OUTPUT);
	digitalWrite(WAKE, LOW);
	pinMode(INTRPT, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(INTRPT), handle_read_irq, FALLING);
	delay(100);
	Wire.begin();
	init_chip();

#if 0
	uint8_t buf[4];
	int n = i2c_read(0xB0, buf, 4);
	Serial.print(buf[0], HEX); Serial.print(","); Serial.print(buf[1], HEX); Serial.print(","); Serial.print(buf[2], HEX); Serial.print(","); Serial.println(buf[3], HEX);
#endif
}
