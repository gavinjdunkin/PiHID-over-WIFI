import time
import usb_hid
from adafruit_hid.mouse import Mouse
from adafruit_hid.keyboard import Keyboard,Keycode
mouse = Mouse(usb_hid.devices)
keyboard = Keyboard(usb_hid.devices)

while True:
    mouse.move(1, 0, 0)  # move mouse a little to the right
    time.sleep(0.1)
    mouse.move(-1, 0, 0)  # move mouse a little to the left
    time.sleep(0.1)
    keyboard.press(Keycode.SPACEBAR)  # press spacebar
    time.sleep(0.1)
    keyboard.release(Keycode.SPACEBAR)  # release spacebar
    time.sleep(0.1)