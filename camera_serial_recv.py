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

count = 0
while True:
    try:
        print("wait for data: {0}".format(count))
        count = count + 1

        #ser.write('C')
        x = ser.read(8)
        if(str(x) == "camera_0"):
            ser.flush()
            x = ser.read(4096)
            if (len(x) == 4096):
                rawfile = np.frombuffer(x, "uint8")
                rawfile.shape = (64,64)
                #misc.imsave("test.png", rawfile)
                im = PIL.Image.frombuffer("L", (64,64), x, "raw", "L", 0, 1)
                misc.imsave("test{0}.png".format(count), im)
                count = count + 1
                print("len(rawfile): {0}".format(len(rawfile)))
            print("camera_0 got {0} bytes".format(len(x)))
            ser.flush()
        elif(str(x) == "camera_1"):
            ser.flush()
            x = ser.read(8192)
            if (len(x) == 8192):
                rawfile = np.frombuffer(x, "uint16")
                rawfile.shape = (64,64)
                #misc.imsave("test.png", rawfile)
                im = PIL.Image.frombuffer("RGB", (64,64), x, "raw", "BGR;16")
                misc.imsave("test{0}.png".format(count), im)
                count = count + 1
                print("len(rawfile): {0}".format(len(rawfile)))
            print("camera_1 got {0} bytes".format(len(x)))
            ser.flush()
        time.sleep(1)

    except Exception as e:
        print(e)
