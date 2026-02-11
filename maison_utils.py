import serial
import time
import threading
import math
import re

ser = serial.Serial('COM4', 9600, timeout=1)
time.sleep(2)

#-- Antonin Fournier Lequin --#
temperature = None
humidity = None
luminosity = None
rain = None
door_state = None

_led_state = False
_buzzer_state = False
_manual_led_override = False
_manual_buzzer_override = False

pattern = re.compile(
    r"Rain=(\d+)\s\|\sTemp=([0-9.]+)C\s\|\sHum=([0-9.]+)%\s\|\sLumi=(\d+)\s\|\sLED=(ON|OFF)\s\|\sBUZZER=(ON|OFF)"
)

door_pattern = re.compile(r"DOOR=(\d)")

def get_temperature_and_humidity():
    if temperature is None or humidity is None:
        return None, None
    if math.isnan(temperature) or math.isnan(humidity):
        return None, None
    return temperature, humidity

#-- Mike Ducasse Dudley --#
def get_luminosity():
    return luminosity

def get_rain():
    return rain

def get_door_state():
    return door_state

def get_led_state():
    return _led_state

def get_buzzer_state():
    return _buzzer_state

def set_led(state: bool):
    global _led_state, _manual_led_override
    _manual_led_override = True
    if state != _led_state:
        ser.write(b't' if state else b'f')
        _led_state = state

#-- Antonin Fournier Lequin --#
def set_buzzer(state: bool):
    global _buzzer_state, _manual_buzzer_override
    _manual_buzzer_override = True
    if state != _buzzer_state:
        ser.write(b'b' if state else b'n')
        _buzzer_state = state

#-- AJOUT POUR LA PORTE --#
def set_door(state: bool):
    global door_state
    if state:
        ser.write(b'o')
        door_state = "ouvert"
    else:
        ser.write(b'c')
        door_state = "Ferme"

def read_from_serial():
    global temperature, humidity, luminosity, rain, door_state
    global _led_state, _buzzer_state
    global _manual_led_override, _manual_buzzer_override

    while True:
        try:
            line = ser.readline().decode(errors="ignore").strip()
            if not line:
                continue

            door_match = door_pattern.search(line)
            if door_match:
                value = int(door_match.group(1))
                door_state = "ouvert" if value == 0 else "Ferme"
                continue

            match = pattern.search(line)
            if match:
                rain = int(match.group(1))
                temperature = float(match.group(2))
                humidity = float(match.group(3))
                luminosity = float(match.group(4))

                _led_state = (match.group(5) == "ON")
                buz_state = (match.group(6) == "ON")

                if _manual_buzzer_override and not buz_state:
                    _manual_buzzer_override = False

                _buzzer_state = buz_state

        except:
            pass

        time.sleep(0.1)

thread = threading.Thread(target=read_from_serial, daemon=True)
thread.start()
