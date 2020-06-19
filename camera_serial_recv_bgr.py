import serial
import PIL.Image
import PIL.ImageDraw
import struct
import binascii
import io
import numpy as np
from scipy import ndimage, misc
import time

ser = serial.Serial('/dev/ttyUSB0', baudrate = 115200,bytesize = 8,parity = 'N',stopbits = 1, timeout=3)

while True:
    try:
        #ser.write('C')
        print("wait for data")
        x = ser.read(32775)
        ser.flush()
        if (len(x) == 32775):
            rawfile = np.frombuffer(x[8:], "uint16")
            rawfile.shape = (128,128)
            #misc.imsave("test.png", rawfile)
            im = PIL.Image.frombuffer("RGB", (128,128), x, "raw", "BGR;16")
            misc.imsave("testbgr.png", im)
            print("len(rawfile): {0}".format(len(rawfile)))
        print("got {0} bytes".format(len(x)))
        time.sleep(1)

    except Exception as e:
        print(e)
