import serial
import PIL.Image
ser = serial.Serial('/dev/ttyUSB0', baudrate = 115200,bytesize = 8,parity = 'N',stopbits = 1, timeout=1)

while True:
    try:
        print("wait for data")
        x = ser.read(8192)
        print("got 8192 bytes")
        ser.flush()
        imgSize = (64,64)# the image size
        img = PIL.Image.frombytes('RGB', imgSize, x)
        img.save("./foo.jpg")# can give any format you like .png
    except Exception as e:
        print(e)
        ser.write('C')
