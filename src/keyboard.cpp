// Handles the micro keyboard
#ifdef KEYBOARD
#include <Arduino.h>
#include <map>

#define rxPin  15
#define dcdPin 17
#define rtsPin 16

#define CHARMAPSIZE 63
PROGMEM static const uint8_t charmap[CHARMAPSIZE][3] = {
{0x01, ']', '}'},
{0x07, 'u', 'U'},
{0x0B, 0x87, 0},
{0x0D, '2', '@'},
{0x13, 0x8A, 0},
{0x17, 'q', 'Q'},
{0x19, 'y', 'Y'},
{0x1D, '6', '^'},
{0x27, 's', 'S'},
{0x29, 'w', 'W'},
{0x2B, 0x83, 0},
{0x2D, '4', '$'},
{0x33, 0x89, 0},
{0x37, 'o', 'O'},
{0x39, '-', '_'},
{0x3D, '8', '*'},
{0x41, 0x81, 0},
{0x47, 'e', 'E'},
{0x4B, ',', '<'},
{0x4D, 'h', 'H'},
{0x53, '\x09', 0},
{0x57, 'a', 'A'},
{0x59, ' ', ' '},
{0x5D, 'l', 'L'},
{0x67, 'c', 'C'},
{0x6B, ';', ':'},
{0x6D, 'j', 'J'},
{0x73, '/', '?'},
{0x77, '9', '('},
{0x79, 0x85, 0},
{0x7D, 'n', 'N'},
{0x81, '`', '~'},
{0x87, 'm', 'M'},
{0x8B, ' ', ' '},
{0x8D, '0', ')'},
{0x93, 0x82, 0},
{0x97, 'i', 'I'},
{0x99, '\'', '"'},
{0x9D, 'd', 'D'},
{0xA7, 'k', 'K'},
{0xA9, '\\', '|'},
{0xAB, '\x0D', 0},
{0xAD, 'b', 'B'},
{0xB3, '\x7F', 0},
{0xB7, 'g', 'G'},
{0xB9, '.', '>'},
{0xBD, 'f', 'F'},
{0xC1, '\x08', 0},
{0xC7, '7', '&'},
{0xCB, 'z', 'Z'},
{0xCD, 'p', 'P'},
{0xD3, '[', '{'},
{0xD7, '3', '#'},
{0xD9, 0x86, 0},
{0xDD, 't', 'T'},
{0xE7, '5', '%'},
{0xE9, 0x84, 0},
{0xEB, 'x', 'X'},
{0xED, 'r', 'R'},
{0xF3, '=', '+'},
{0xF7, '1', '!'},
{0xF9, 0x88, 0},
{0xFD, 'v', 'V'},
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

static int binary_search(uint8_t target) {
    int left = 0;
    int right = CHARMAPSIZE - 1;

    while (left <= right) {
        int mid = left + ((right - left) / 2);

        // Check if target is present at mid
        if (charmap[mid][0] == target)
            return mid;

        // If target is greater, ignore left half
        else if (charmap[mid][0] < target)
            left = mid + 1;

        // If target is smaller, ignore right half
        else
            right = mid - 1;
    }

    // If target is not present in the array
    return -1;
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
        int i = binary_search(c);
        if(i != -1) {
            if(shift) {
                a = charmap[i][2];
                if(a == 0) a= charmap[i][1]; // if no shift just use regular char

            } else {
                a = charmap[i][1];
                if(ctrl) {
                    // convert to control character
                    if(a >= 'a' && a <= '}') a = a - 96;
                }
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
