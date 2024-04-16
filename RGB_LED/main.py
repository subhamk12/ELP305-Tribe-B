from machine import Pin
from time import sleep

RED = Pin(14, Pin.OUT)
GREEN = Pin(27, Pin.OUT)
BLUE = Pin(26, Pin.OUT)

LED = [RED, GREEN, BLUE]

def Red_color():
    RED.value(0)
    GREEN.value(1);
    BLUE.value(1);
    sleep(1);

def Red_blinking():
    RED.value(0)
    GREEN.value(1);
    BLUE.value(1);
    sleep(0.5);
    for l in LED:
        l.value(1)
    sleep(0.5)

def Green_color():
    RED.value(1)
    GREEN.value(0);
    BLUE.value(1);
    sleep(1);

def White_color():
    RED.value(0)
    GREEN.value(0);
    BLUE.value(0);
    sleep(1);

def Green_blinking():
    RED.value(1)
    GREEN.value(0);
    BLUE.value(1);
    sleep(0.5);
    for l in LED:
        l.value(1)
    sleep(0.5)



while True:
    Red_blinking()
    Green_blinking()






