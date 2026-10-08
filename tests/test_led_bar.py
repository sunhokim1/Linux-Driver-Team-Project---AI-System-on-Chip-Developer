import Jetson.GPIO as GPIO, time
pins = [7, 11, 13, 16, 18, 22, 29, 31]
GPIO.setmode(GPIO.BOARD)
GPIO.setup(pins, GPIO.OUT, initial=GPIO.LOW)
for n in (2, 5, 8):
    for i, p in enumerate(pins):
        GPIO.output(p, i < n)
    time.sleep(1.5)

GPIO