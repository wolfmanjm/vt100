#include <SPI.h>
#include <RA8875.h>
#include "tinyflash.h"
#include <EEPROM.h>

//#define TEST
//#define DEBUG
#define LED 13

#define FW_SOURCE_LEN 5478

#define RA8875_CS 10
#define RA8875_RESET 255 //any pin, if you want to disable just set at 255 or not use at all
//RA8875(CSp,RSTp,mosi_pin,sclk_pin,miso_pin);
RA8875 tft = RA8875(RA8875_CS, RA8875_RESET, 11, 14, 12);
/* Teensy LC
    No reset
    CS   10
    mosi 11
    miso 12
    sclk 14 (alt)
*/

/* Uses a W25Q80BV flash to store the touch screen firmware (as not enough in teensy)
    On SPI1 as cannot share with TFT
    CS   6
    miso 1
    mosi 2
    sclk 20

    #define FLASHFW to flash the firmware
    #define VERIFYFW to check it
    Make sure both are undefined to run program
*/
TinyFlash flash;
uint32_t capacity = 0;
bool has_touch = false;
uint16_t screen_width, screen_height;
uint16_t char_width, char_height;

// externals
void gsl_final_setup();
void gsl_setup();
void gsl_load_fw(uint8_t addr, uint8_t Wrbuf[4]);

void load_touch_fw()
{
    gsl_setup();

#ifdef DEBUG
    Serial.println("Loading Touch FIRMWARE");
#endif

    tft.println("Loading Touch FIRMWARE...");
    if(!flash.beginRead(0)) {
#ifdef DEBUG
        Serial.println("beginread failed");
#endif
        tft.println("flash read failed");
        digitalWrite(LED, 0);
        return;
    }

    uint16_t source_len = FW_SOURCE_LEN;

    for (uint32_t source_line = 0; source_line < source_len; source_line++) {
        uint8_t addr = flash.readNextByte();
        flash.readNextByte();
        flash.readNextByte();
        flash.readNextByte();

        uint8_t buf[4];
        buf[0] = flash.readNextByte();
        buf[1] = flash.readNextByte();
        buf[2] = flash.readNextByte();
        buf[3] = flash.readNextByte();
        gsl_load_fw(addr, buf);
    }
    flash.endRead();
#ifdef DEBUG
    Serial.println("Touch Firmware loaded ok");
#endif
    tft.println("...Loaded Touch FIRMWARE");
    gsl_final_setup();
}

int baudrate = 9600;
int rotation = 0; // 1 is portrait
int font_size = 0;
uint16_t text_color = RA8875_GREEN;
bool lfcrlf = true; // convert lf to crlf

void kbd_setup();

void setup()
{
#ifdef DEBUG
    Serial.begin (115200);   // debugging
    // wait up to 5 seconds for Arduino Serial Monitor
    unsigned long startMillis = millis();
    while (!Serial && (millis() - startMillis < 5000)) ;
    Serial.println("Starting Terminal.");
#endif

    // read config settings stored in eeprom
    // first time settings are set will write the defaults and any changes
    uint8_t value = EEPROM.read(0);
    if(value == 0xA5) {
        // configs have been stored
        value= EEPROM.read(1);
        switch(value) {
            case 0: baudrate = 1200; break;
            case 1: baudrate = 2400; break;
            case 2: baudrate = 4800; break;
            case 3: baudrate = 9600; break;
            case 4: baudrate = 19200; break;
            case 5: baudrate = 115200; break;
            default: baudrate = 9600; break;
        }
        rotation = EEPROM.read(2);
        font_size = EEPROM.read(3);
        text_color = (EEPROM.read(5) << 8) | EEPROM.read(4);
        lfcrlf = EEPROM.read(6) != 0;
    }
#ifdef DEBUG
    else {
        Serial.println("No settings found in EEPROM");
    }
#endif

    Serial1.setRX(3);
    Serial1.setTX(4);
    Serial1.begin(baudrate, SERIAL_8N1); // for I/O
    // Serial1.println("Hello world!");

    tft.begin(RA8875_800x480);
    tft.setRotation(rotation);

    tft.setFontScale(font_size); //font x1

    screen_width = tft.width();
    screen_height = tft.height();
    char_width = tft.getFontWidth();
    char_height = tft.getFontHeight();

#ifdef DEBUG
    Serial.printf("Screen width: %u, height: %u\n", screen_width, screen_height);
    Serial.printf("Font width: %u, height: %u\n", char_width, char_height);
    Serial.printf("Line lengths: %u x %u\n", screen_width / char_width, screen_height / char_height);
#endif

    tft.printf("Screen width: %u, height: %u\n", screen_width, screen_height);
    tft.printf("Font width: %u, height: %u\n", char_width, char_height);
    tft.printf("Line lengths: %u x %u\n", screen_width / char_width, screen_height / char_height);

    tft.showCursor(UNDER, true);

    //now set a text color, background transparent
    tft.println("Starting up...");

    // setup the micro keyboard
    kbd_setup();

    pinMode(LED, OUTPUT);
    digitalWrite(LED, 0);
    capacity = flash.begin();
#ifdef DEBUG
    Serial.println(capacity);     // Chip size to host
#endif
    if(capacity > 0) {
        digitalWrite(LED, 1);
        load_touch_fw();
        has_touch = true;

    } else {
#ifdef DEBUG
        Serial.println("Bad flash");
#endif
        tft.println("Unable to setup Flash");
        digitalWrite(LED, 0);
        has_touch = false;
    }

    // set text color to green
    tft.setTextColor(text_color);
    tft.setCursor(0, 0);
    tft.fillWindow(RA8875_BLACK); //fill window black
    tft.sleep(false);
    tft.displayOn(true);

#if 0
    tft.println("Once upon a midnight dreary, while I pondered, weak and weary,");
    tft.println("Over many a quaint and curious volume of forgotten lore,");
    tft.println("While I nodded, nearly napping, suddenly there came a tapping,");
    tft.println("As of some one gently rapping, rapping at my chamber door.");
    tft.println("'Tis some visitor,' I muttered, 'tapping at my chamber door Only this, and nothing more.'");
    tft.println("");
    tft.println("Ah, distinctly I remember it was in the bleak December,");
    tft.println("And each separate dying ember wrought its ghost upon the floor.");
    tft.println("Eagerly I wished the morrow;- vainly I had sought to borrow");
    tft.println("From my books surcease of sorrow- sorrow for the lost Lenore-");
    tft.println("For the rare and radiant maiden whom the angels name Lenore-");
    tft.println("Nameless here for evermore.");
    tft.println("");
    tft.println("Once upon a midnight dreary, while I pondered, weak and weary,");
    tft.println("Over many a quaint and curious volume of forgotten lore,");
    tft.println("While I nodded, nearly napping, suddenly there came a tapping,");
    tft.println("As of some one gently rapping, rapping at my chamber door.");
    tft.println("'Tis some visitor,' I muttered, 'tapping at my chamber door Only this, and nothing more.'");
    tft.println("");
    tft.println("Ah, distinctly I remember it was in the bleak December,");
    tft.println("And each separate dying ember wrought its ghost upon the floor.");
    tft.println("Eagerly I wished the morrow;- vainly I had sought to borrow");
    tft.println("From my books surcease of sorrow- sorrow for the lost Lenore-");
    tft.println("For the rare and radiant maiden whom the angels name Lenore-");
    tft.println("Nameless here for evermore.");
#endif
}

struct _coord {
    uint32_t x, y;
    uint8_t finger;
};

struct _ts_event {
    uint8_t  n_fingers;
    struct _coord coords[5];
};

#include "RingBuffer.h"
using touch_event_t = struct _ts_event;
template<class T> using touch_events_t = RingBuffer<T, 16>;
touch_events_t<touch_event_t> touch_events;
enum touch_state_t { UP, DOWN };
touch_state_t touch_state = UP;
bool moved = false;

extern "C" void add_touch_event(struct _ts_event *e)
{
    touch_events.push_back(*e);
}

int tog = 0;
uint32_t fc = 0, time = 0;
int max_depth = 0;
uint32_t lastx, lasty;
struct _coord last_coords[5] = {0};
int nf = 0;
bool last_finger_down[5] = {false};

// wait for char and return it
char getcharw()
{
#ifdef DEBUG
    while(true) {
        if (Serial1.available()) {
            return Serial1.read();
        }
        if (Serial.available()) {
            return Serial.read();
        }
    }
#else
    while (!Serial1.available()) ;
    return Serial1.read();
#endif
}

void clearScreen()
{
    tft.fillWindow(RA8875_BLACK);
}

void scroll_up()
{
    tft.BTE_move(0, char_height, screen_width, screen_height - char_height, 0, 0);
    delay(50);
    tft.fillRect(0, screen_height - char_height, screen_width, char_height, RA8875_BLACK);
}

void scroll_down()
{
    tft.BTE_move(screen_width - 1, screen_height - char_height - 1, screen_width, screen_height - char_height, screen_width - 1, screen_height - 1, 0, 0, false, RA8875_BTEROP_SOURCE, false, true);
    delay(50);
    tft.fillRect(0, 0, screen_width, char_height, RA8875_BLACK);
}

// extern
char process_key(bool wait);
bool local_echo= true;

// do some basic VT100/ansi escape sequence handling
void process(char data)
{
    int16_t currentX, currentY;
    if (data == '\r') {
        // start of current line
        tft.getCursor(currentX, currentY);
        tft.setCursor(0, currentY);

    } else if (data == '\n') {
        // next line, potentially scroll
        tft.getCursor(currentX, currentY);
        int16_t x= currentX;
        int16_t y = currentY + char_height;
        if(y >= screen_height) {
            scroll_up();
            y = screen_height - char_height;
        }
        if(lfcrlf) x= 0; // if LF is converted to CRLF
        tft.setCursor(x, y);

    } else if (data == 8) { // BS
        // backspace move cursor left one
        tft.getCursor(currentX, currentY);
        uint16_t x= currentX-char_width;
        if(x < 0) x= 0;
        tft.setCursor(x, currentY);

    } else if (data == 27) { // ESC
        //If it is an escape character then get the following characters to interpret
        //them as an ANSI escape sequence
        uint8_t escParam1 = 0;
        uint8_t escParam2 = 0;
        char serInChar = getcharw();

        if (serInChar == '[') {
            serInChar = getcharw();
            // Process a number after the "[" character
            while (serInChar >= '0' && serInChar <= '9') {
                serInChar = serInChar - '0';
                if (escParam1 < 100) {
                    escParam1 = escParam1 * 10;
                    escParam1 = escParam1 + serInChar;
                }
                serInChar = getcharw();
            }
            // If a ";" then process the next number
            if (serInChar == ';') {
                serInChar = getcharw();
                while (serInChar >= '0' && serInChar <= '9') {
                    serInChar = serInChar - '0';
                    if (escParam2 < 100) {
                        escParam2 = escParam2 * 10;
                        escParam2 = escParam2 + serInChar;
                    }
                    serInChar = getcharw();
                }
            }

            #ifdef DEBUG
            Serial.printf("Esc[ escP1: %d, escP2: %d, %c\n", escParam1, escParam2, serInChar);
            #endif

            if(serInChar >= 'A' && serInChar <= 'G') {
                // Handle cursor incremental move commands
                tft.getCursor(currentX, currentY);
                int16_t x= currentX, y= currentY;
                if (escParam1 < 1) escParam1= 1;
                switch(serInChar) {
                    case 'A':
                        // Esc[nA moves cursor up n lines
                        y = currentY - (escParam1 * char_height);
                        if(y < 0) y= 0;
                        break;

                    case 'B':
                        // Esc[nB moves cursor down n lines
                        y = currentY + (escParam1 * char_height);
                        if(y >= screen_height) y= screen_height - char_height;
                        break;

                    case 'C':
                        // Esc[nC moves cursor right n characters
                        x = currentX + (escParam1 * char_width);
                        if(x >= screen_width) x= screen_width - char_width;
                        break;

                    case 'D':
                        // Esc[nD moves cursor left n characters
                        x = currentX - (escParam1 * char_width);
                        if(x < 0) x= 0;
                        break;

                    case 'E':
                        // Esc[nE moves cursor to start of n next lines
                        x= 0;
                        y = currentY + (escParam1 * char_height);
                        if(y >= screen_height) y= screen_height - char_height;
                        break;

                    case 'F':
                        // Esc[nF moves cursor to start of n previous lines
                        x= 0;
                        y = currentY - (escParam1 * char_height);
                        if(y < 0) y= 0;
                        break;

                    case 'G':
                        // Esc[nG moves cursor to column n
                        --escParam1;
                        x = (escParam1 * char_width);
                        if(x >= screen_width) x= screen_width - char_width;
                        break;
                }

                tft.setCursor(x, y);
            }
            // Esc[line;ColumnH or Esc[line;Columnf moves cursor to that coordinate
            else if (serInChar == 'H' || serInChar == 'f') {
                if (escParam1 > 0) {
                    escParam1--;
                }
                if (escParam2 > 0) {
                    escParam2--;
                }
                int16_t x = escParam1 * char_width;
                int16_t y = escParam2 * char_height;
                tft.setCursor(x, y);
            }
            //Esc[J=clear from cursor down, Esc[1J=clear from cursor up, Esc[2J=clear complete screen
            else if (serInChar == 'J') {
                tft.getCursor(currentX, currentY);
                if (escParam1 == 0) {
                    if(currentX != 0) {
                        // clear to end of line
                        int16_t x = currentX;
                        int16_t y = currentY;
                        int16_t w = screen_width - currentX;
                        int16_t h = char_height;
                        tft.fillRect(x, y, w, h, RA8875_BLACK);
                    }
                    // clear to end of screen
                    int16_t y = currentY + char_height;
                    tft.fillRect(0, y, screen_width, screen_height - y, RA8875_BLACK);

                } else if (escParam1 == 1) {
                    if(currentX != 0) {
                        // clear to start of line
                        int16_t y = currentY;
                        int16_t w = currentX;
                        int16_t h = char_height;
                        tft.fillRect(0, y, w, h, RA8875_BLACK);
                    }
                    // clear to start of screen
                    int16_t y = currentY - char_height;
                    tft.fillRect(0, 0, screen_width, y, RA8875_BLACK);

                } else if (escParam1 == 2) {
                    clearScreen();
                }
            }
            // Esc[K = erase to end of line, Esc[1K = erase to start of line
            else if (serInChar == 'K') {
                tft.getCursor(currentX, currentY);
                if (escParam1 == 0) {
                    // clear to end of line
                    int16_t x = currentX;
                    int16_t y = currentY;
                    int16_t w = screen_width - currentX;
                    int16_t h = char_height;
                    tft.fillRect(x, y, w, h, RA8875_BLACK);

                } else if (escParam1 == 1) {
                    // clear to start of line
                    int16_t y = currentY;
                    int16_t w = currentX;
                    int16_t h = char_height;
                    tft.fillRect(0, y, w, h, RA8875_BLACK);

                } else if (escParam1 == 2) {
                    // clear entire line
                    int16_t y = currentY;
                    int16_t w = screen_width;
                    int16_t h = char_height;
                    tft.fillRect(0, y, w, h, RA8875_BLACK);
                }

            }
            // Esc[nT = scroll down. optional n is number of lines to scroll
            else if (serInChar == 'T') {
                if (escParam1 == 0) escParam1= 1;
                for (int i = 0; i < escParam1; ++i) {
                    scroll_down();
                }
            }
            // Esc[nS = scroll up. optional n is number of lines to scroll
            else if (serInChar == 'S') {
                if (escParam1 == 0) escParam1= 1;
                for (int i = 0; i < escParam1; ++i) {
                    scroll_up();
                }
            }
        } // end of in char = '['

        // EscM scroll up
        else if (serInChar == 'M') {
            scroll_up();
        }
        // EscL scroll down
        else if (serInChar == 'L') {
            scroll_down();
        }
    } // end of in char = 27
    else if (data > 31 && data < 128) {
        // display character, scroll if hit end of screen
        tft.getCursor(currentX, currentY);
        if(currentX >= 0 && currentY >= screen_height) {
            scroll_up();
        }
        tft.print(data);
    }
}

// read data from UART and display
// process VT100 escape sequences
void loop()
{
    char data;
#ifdef DEBUG
    while (Serial.available()) {
        data = Serial.read();
        process(data);
    }
#endif

    while (Serial1.available()) {
        data = Serial1.read();
        process(data);
    }

    // get a key from the keyboard
    char c= process_key(false);
    if(c != 0) {
        Serial1.write(c);
        if(local_echo) {
            if(c < ' ' || c >= 0x80) tft.printf("\\x%02X", c);
            else if(c >= ' ') tft.printf("%c", c);
        }
    }

#ifdef TEST
    bool dir = false;

    while(true) {
        clearScreen();
        tft.setCursor(0, 0);

        tft.println("Once upon a midnight dreary, while I pondered, weak and weary,");
        tft.println("Over many a quaint and curious volume of forgotten lore,");
        tft.println("While I nodded, nearly napping, suddenly there came a tapping,");
        tft.println("As of some one gently rapping, rapping at my chamber door.");
        tft.println("'Tis some visitor,' I muttered, 'tapping at my chamber door Only this, and nothing more.'");
        tft.println("");
        tft.println("Ah, distinctly I remember it was in the bleak December,");
        tft.println("And each separate dying ember wrought its ghost upon the floor.");
        tft.println("Eagerly I wished the morrow;- vainly I had sought to borrow");
        tft.println("From my books surcease of sorrow- sorrow for the lost Lenore-");
        tft.println("For the rare and radiant maiden whom the angels name Lenore-");
        tft.println("Nameless here for evermore.");
        delay(1000);

        if(dir) {
            for (int i = 0; i < 12; ++i) {
                scroll_up();
                delay(100);
            }
        } else {
            for (int i = 0; i < 12; ++i) {
                scroll_down();
                delay(1000);
            }
        }
        dir = !dir;
    }
#endif
}
