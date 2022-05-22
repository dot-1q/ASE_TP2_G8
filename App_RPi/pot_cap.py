from flask import Flask, render_template, request
import random
import os
import time
import pigpio

# from photo_resistor import reader

global last_treshold_value
global last_photo_value
last_treshold_value = 50
last_photo_value = 5
app = Flask(__name__, static_url_path="/static")

@app.route('/', methods=['GET', 'POST'])
def index():
   global last_treshold_value
   global last_photo_value
   rasp_photos = os.listdir('static/images')
   rasp_photos = ['images/' + file for file in rasp_photos]

   # global pc
   # s, v, r = pc.read()
   # print("Value {}".format(v))

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

   return render_template('index.html', value=(random.randint(0,10000)), last_value=last_treshold_value, rasp_photos=rasp_photos, last_photo_value=last_photo_value)


def setup_phto_resistor():
   global pc

   POT_CAP_GPIO = 20
   DRAIN_MS = 1.0
   TIMEOUT_S = 1.0

   pi = pigpio.pi()  # Connect to Pi.

   # Instantiate Pot/Cap reader.
   pc = reader(pi, POT_CAP_GPIO, DRAIN_MS, TIMEOUT_S)


if __name__ == '__main__':
   # setup_phto_resistor()
   app.run(debug=True, host='0.0.0.0')

