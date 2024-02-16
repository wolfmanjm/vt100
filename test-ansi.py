#!/usr/bin/python3

# Tests ansi sequences
import serial
import time

port = '/dev/ttyUSB0'
try:
    ser = serial.Serial(port, baudrate=9600)
except Exception as e:
    print("Failed to open uart: {}".format(e))
    exit(0)

ser.write(b"\x1B[2J")
ser.write(b"\x1B[H")

ser.write(b"Once upon a midnight dreary, while I pondered, weak and weary,\r\n")
ser.write(b"Over many a quaint and curious volume of forgotten lore,\r\n")
ser.write(b"While I nodded, nearly napping, suddenly there came a tapping,\r\n")
ser.write(b"As of some one gently rapping, rapping at my chamber door.\r\n")
ser.write(b"'Tis some visitor,' I muttered, 'tapping at my chamber door Only this, and nothing more.'\r\n")
ser.write(b"\r\n")
ser.write(b"Ah, distinctly I remember it was in the bleak December,\r\n")
ser.write(b"And each separate dying ember wrought its ghost upon the floor.\r\n")
ser.write(b"Eagerly I wished the morrow;- vainly I had sought to borrow\r\n")
ser.write(b"From my books surcease of sorrow- sorrow for the lost Lenore-\r\n")
ser.write(b"For the rare and radiant maiden whom the angels name Lenore-\r\n")
ser.write(b"Nameless here for evermore.\r\n")

ser.write(b"\x1B[0;14H")
ser.write(b"this is line 14\r\n")

ser.write(b"\x1B[20;15H")
ser.write(b"col 20\r\n")

time.sleep(2)

# scroll down then up 4 lines
ser.write(b"\x1B[4T")
time.sleep(2)
ser.write(b"\x1B[4S")

time.sleep(2)

# clear line to right
ser.write(b"\x1B[20;5H")
ser.write(b"\x1B[K")

# clear line to left
ser.write(b"\x1B[20;3H")
ser.write(b"\x1B[1K")

time.sleep(2)

# clear to end of screen
ser.write(b"\x1B[5;8H")
ser.write(b"\x1B[J")

time.sleep(2)
# clear to top of screen
ser.write(b"\x1B[6;7H")
ser.write(b"\x1B[1J")

# clear screen and set to top
ser.write(b"\x1B[2J")
ser.write(b"\x1B[0;0H")

ser.write(b"\x1B[25;0H")
for x in range(49, 59):
    b = bytearray([x, 10, 8])
    ser.write(b)

ser.write(b"\x1B[0;0H")

# move cursor down 3 lines
ser.write(b"\x1B[3B")
ser.write(b"4th line\r\n")

# move cursor up 4 lines
ser.write(b"\x1B[4A")
ser.write(b"1st line")

# move cursor down 5 lines
ser.write(b"\x1B[5B\r")
ser.write(b"12345678901234567890\r\n")
# move cursor right 10 characters
ser.write(b"\x1B[10C")
ser.write(b"11th column")

# move cursor left 21 characters
ser.write(b"\x1B[21D")
ser.write(b"1")


