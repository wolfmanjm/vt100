// Handles the micro keyboard
#ifdef KEYBOARD
#include <Arduino.h>
#include <map>

#define rxPin  15
#define dcdPin 17
#define rtsPin 16

static const std::map<uint8_t, char> nslut {
    {0x8D, '0'},
    {0xF7, '1'},
    {0x0D, '2'},
    {0xD7, '3'},
    {0x2D, '4'},
    {0xE7, '5'},
    {0x1D, '6'},
    {0xC7, '7'},
    {0x3D, '8'},
    {0x77, '9'},
    {0x57, 'a'},
    {0xAD, 'b'},
    {0x67, 'c'},
    {0x9D, 'd'},
    {0x47, 'e'},
    {0xBD, 'f'},
    {0xB7, 'g'},
    {0x4D, 'h'},
    {0x97, 'i'},
    {0x6D, 'j'},
    {0xA7, 'k'},
    {0x5D, 'l'},
    {0x87, 'm'},
    {0x7D, 'n'},
    {0x37, 'o'},
    {0xCD, 'p'},
    {0x17, 'q'},
    {0xED, 'r'},
    {0x27, 's'},
    {0xDD, 't'},
    {0x07, 'u'},
    {0xFD, 'v'},
    {0x29, 'w'},
    {0xEB, 'x'},
    {0x19, 'y'},
    {0xCB, 'z'},
    {0x39, '-'},
    {0xF3, '='},
    {0xD3, '['},
    {0x01, ']'},
    {0x6B, ';'},
    {0x99, '\''},
    {0x4B, ','},
    {0xB9, '.'},
    {0x73, '/'},
    {0x81, '`'},
    {0xA9, '\\'},
    {0xB3, '\x7F'},

    // special keys
    {0x41, 0x81}, // up
    {0x93, 0x82}, // down
    {0x2B, 0x83}, // left
    {0xE9, 0x84}, // right

    {0x79, 0x85}, // today
    {0xD9, 0x86}, // inbox
    {0x0B, 0x87}, // contacts
    {0xF9, 0x88}, // calendar
    {0x33, 0x89}, // tasks
    {0x13, 0x8A}, // winkey

    {0xAB, '\r'}, // Enter
    {0x59, ' '}, // space
    {0x8B, ' '}, // space
    {0x53, 9}, // TAB
    {0xC1, 8}, // BS

};

static const std::map<uint8_t, char> shiftlut {
    {0x8D, ')'},
    {0xF7, '!'},
    {0x0D, '@'},
    {0xD7, '#'},
    {0x2D, '$'},
    {0xE7, '%'},
    {0x1D, '^'},
    {0xC7, '&'},
    {0x3D, '*'},
    {0x77, '('},
    {0X57, 'A'},
    {0XAD, 'B'},
    {0X67, 'C'},
    {0X9D, 'D'},
    {0X47, 'E'},
    {0XBD, 'F'},
    {0XB7, 'G'},
    {0X4D, 'H'},
    {0X97, 'I'},
    {0X6D, 'J'},
    {0XA7, 'K'},
    {0X5D, 'L'},
    {0X87, 'M'},
    {0X7D, 'N'},
    {0X37, 'O'},
    {0XCD, 'P'},
    {0X17, 'Q'},
    {0XED, 'R'},
    {0X27, 'S'},
    {0XDD, 'T'},
    {0X07, 'U'},
    {0XFD, 'V'},
    {0X29, 'W'},
    {0XEB, 'X'},
    {0X19, 'Y'},
    {0XCB, 'Z'},

    {0x39, '_'},
    {0xF3, '+'},
    {0xD3, '{'},
    {0x01, '}'},
    {0x6B, ':'},
    {0x99, '\"'},
    {0x4B, '<'},
    {0xB9, '>'},
    {0x73, '?'},
    {0x81, '~'},
    {0xA9, '|'},

};

void kbd_setup()
{
    pinMode(rxPin, INPUT);
    pinMode(dcdPin, INPUT);
    pinMode(rtsPin, OUTPUT);
    digitalWrite(rtsPin, LOW);
}


static int cnt = 0;
static uint8_t buf[10];

static bool get_key(uint8_t &rk)
{
    if(digitalRead(dcdPin) == 1) { // wait for it to go high

        // acknowledge ready to read
        digitalWrite(rtsPin, HIGH);

        while(digitalRead(dcdPin) == 1) ; // wait for it to go low

        // deassert CTS
        digitalWrite(rtsPin, LOW);

        bool flg = true;

        // wait for first start bit
        while(digitalRead(rxPin) == 0) ;

        do {
            // wait for half bit time
            delayNanoseconds(104166 >> 1);

            // now read 8 bits
            uint16_t c = 0;
            for (int i = 0; i < 8; ++i) {
                delayNanoseconds(104166);
                c = (c << 1) | digitalRead(rxPin); // read data pin @ 9600 baud
            }

            // wait for stop bit
            while(digitalRead(rxPin) == 1) ; // wait for it to go low

            buf[cnt++] = c;

            // look for next start bit but give up after 2 millis
            uint32_t last = millis();
            while(digitalRead(rxPin) == 0) {
                if((millis() - last) >= 2) {
                    flg = false;
                    break;
                }
            }
        } while(flg);
    }

    if(cnt > 0) {
        if(cnt >= 3 ) {
            if(buf[0] == 0x15 && buf[1] == 0x35) {
                rk = buf[2];
            } else {
                rk = 0;
            }
        } else {
            rk = 0;
        }

        cnt = 0;
        return true;
    }

    return false;
}

static bool shift = false;
static bool ctrl = false;
static bool alt = false;
static bool fn = false;
static bool caps = false;

// returns processed char in low 8bits, and modifiers in upper 8 bits.
uint16_t process_key(bool wait)
{
    uint8_t c;
    if(wait) {
        while(!get_key(c)) ; // block wait for key

    }else{
        if(!get_key(c)) return 0;
    }

    // process modifiers keys
    if(c == 0x3F) shift = true;
    else if(c == 0x7F) ctrl = true;
    else if(c == 0xDF) alt = true;
    else if(c == 0xBF) fn = true;
    else if(c == 0xC6) shift = false;
    else if(c == 0x86) ctrl = false;
    else if(c == 0x26) alt = false;
    else if(c == 0x46) fn = false;
    else if(c == 0x69) caps = !caps;

    else if(c == 0x5E) {
        // released normal key

    } else {
        // Serial.printf("keycode= %02X", c);
        // Serial.printf(" - shift:%d, ctrl:%d, alt:%d, fn:%d, caps:%d\n",
        //               shift, ctrl, alt, fn, caps);

        uint8_t a = 0;
        if(shift) {
            auto x = shiftlut.find(c);
            if(x != shiftlut.end()) {
                a = x->second;
            } else {
                auto x = nslut.find(c);
                if(x != shiftlut.end()) {
                    a = x->second;
                }
            }

        } else {
            auto x = nslut.find(c);
            if(x != nslut.end()) {
                a = x->second;
            }

            if(ctrl) {
                // convert to control character
                if(a >= 'a' && a <= '}') a = a - 96;
            }
        }

        uint8_t mods = 0;
        if(shift) mods |= 1;
        if(ctrl) mods |= 2;
        if(alt) mods |= 4;
        if(fn) mods |= 8;
        return (mods<<8) | a;
    }

    return 0;
}

#endif
