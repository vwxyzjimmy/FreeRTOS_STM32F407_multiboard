from __future__ import print_function
import array
from pydub import AudioSegment
from pydub.playback import play
from pydub.utils import get_array_type
import matplotlib.pyplot as plt
import numpy as np
import struct
import serial
import time
from ctypes import c_uint8
def read_wave_data(file_path):
    sound = AudioSegment.from_file(file=file_path)
    print("sound.sample_width: {0}, sound.frame_rate: {1}".format(sound.sample_width, sound.frame_rate))
    left = sound.split_to_mono()[1]
    channel_count = sound.channels
    print("channel_count: {0}".format(channel_count))
    bit_depth = left.sample_width * 8
    array_type = get_array_type(bit_depth)
    left_numeric_array = array.array(array_type, left._data)
    array_min = min(left_numeric_array)
    array_max = max(left_numeric_array)
    plt.figure(figsize=(16,12))
    x = np.linspace(0, len(left_numeric_array), len(left_numeric_array))
    plt.subplot(2,1,1)
    #total_range = array_max - array_min + 1
    total_range = 65536
    normalize_array = []
    for i in range(len(left_numeric_array)):
        son = (float(left_numeric_array[i])-float(array_min))
        normalize = son/float(total_range)
        map_normalize = (normalize*255)
        integer_map_normalize = map_normalize
        integer_map_normalize = int(map_normalize)
        normalize_array.append(integer_map_normalize)
    sound.split_to_mono()[1] = left_numeric_array
    sound.split_to_mono()[0] = left_numeric_array
    return sound.frame_rate, normalize_array

def main():
    path = "test5.mp3"
    mp3_frame_rate, mp3_data = read_wave_data(path)
    COM_PORT = '/dev/ttyUSB0'
    BAUD_RATES = 921600
    ser = serial.Serial(COM_PORT, BAUD_RATES)
    play_array = []
    if (44100/mp3_frame_rate) > 1:
        for i in range(len(mp3_data)):
            for j in range((44100//mp3_frame_rate)):
                play_array.append(mp3_data[i])
    else:
        play_array = mp3_data
    value = play_array
    play_delay = round(0.0000107399*(44100/mp3_frame_rate), 8)*2*200 #0.0000107399
    send_start = 0
    start_time = time.time()
    print("Start to play")
    while send_start <= len(value):
        if ((send_start+200)>=len(value)):
            ser.write(value[send_start:])
        else:
            ser.write(value[send_start:(send_start+200)])
        time.sleep(play_delay)
        send_start = send_start + 200
    ser.close()
    duration = time.time() - start_time
    print("ser.write over\r\nduration: {0}".format(duration))


if __name__ == "__main__":
    main()
