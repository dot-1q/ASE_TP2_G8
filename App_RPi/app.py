from flask import Flask, render_template, request
import random
import os
import time
import pigpio

import sys

import photo_resistor

import threading

from picamera2 import Picamera2, Preview
import time

from gpiozero import Servo, Device

picam2 = None

def setup_camera():
    global picam2
    
    picam2 = Picamera2()
#     picam2.start_preview(Preview.QTGL)

    preview_config = picam2.preview_configuration(main={"size": (800, 600)})
    picam2.configure(preview_config)
    picam2.start()
    time.sleep(2)

def capture(file_name):
    global picam2

    picam2.capture_file(file_name)
    
servo_pos = [-1, -0.5, 0, 0.5, 1]

myCorrection=0.2
maxPW=(1.6)/1000
minPW=(0.3)/1000

from gpiozero.pins.pigpio import PiGPIOFactory
Device.pin_factory = PiGPIOFactory()
 
servo = Servo(17,min_pulse_width=minPW,max_pulse_width=maxPW)
    
def capture_images(file_name_prefix):
    i = 0
    for pos in servo_pos:
        print("Going to: " + str(pos))
        servo.value = pos
        time.sleep(0.5)
        file_name = file_name_prefix + "_" + str(i) + ".png"
        i += 1
        capture(file_name)
        time.sleep(0.5)
    
def camera_thread(tresh, period):
    i = 0
    while 1:
        if get_brightness() > tresh():
            print("Capture")
            file_name_prefix = "static/images/img_" + str(i)
            i += 1
            
            capture_images(file_name_prefix)
            time.sleep(period())

# from photo_resistor import reader

global last_treshold_value
global last_photo_value
last_treshold_value = 50
last_photo_value = 5
app = Flask(__name__, static_url_path="/static")

MAX_BRIGHTNESS = 100000

i = 0

@app.route('/', methods=['GET', 'POST'])
def index():
   global last_treshold_value
   global last_photo_value
   global picam2
   rasp_photos = os.listdir('static/images')
   rasp_photos = ['images/' + file for file in rasp_photos]
   
   # global pc
   # s, v, r = pc.read()
   # print("Value {}".format(v))
   
   if not picam2:
       setup_photo_resistor()
       setup_camera()
       x = threading.Thread(target=camera_thread, \
            args=(lambda: int(last_treshold_value),lambda: int(last_photo_value)))
       x.start()

   if request.method == 'POST':
      ###
      ### PODES LER AQUI O VALOR QUE ESTÁ NO SLIDER DANI
      ###
      if 'slider_value' in request.form:
         last_treshold_value = request.form['slider_value']
         print(last_treshold_value)
      if 'photo_value' in request.form: 
         last_photo_value = request.form['photo_value']
         print(last_photo_value)
         
   print("Thresh: " + str(last_treshold_value))
   print("Photo: " + str(last_photo_value))

   return render_template('index.html', value=(get_brightness()), last_value=last_treshold_value, rasp_photos=rasp_photos, last_photo_value=last_photo_value)


def setup_photo_resistor():
   global pc
   
   import pigpio

   POT_CAP_GPIO = 20
   DRAIN_MS = 1.0
   TIMEOUT_S = 2.0

   pi = pigpio.pi()  # Connect to Pi.

   # Instantiate Pot/Cap reader.
   pc = photo_resistor.reader(pi, POT_CAP_GPIO, DRAIN_MS, TIMEOUT_S)
   
def get_brightness():
   global pc
   
   s, v, r = pc.read()
   
   #print("{} {} {}".format(s, v, r))
   
   if s:
       if v > MAX_BRIGHTNESS:
           v = 0
       else:
           v = v / MAX_BRIGHTNESS * 100
           v = 100 - v
       return v
   else:
       return 0



if __name__ == '__main__':

   
   print("Running app")
   app.run(debug=True, host='0.0.0.0')
   
   

