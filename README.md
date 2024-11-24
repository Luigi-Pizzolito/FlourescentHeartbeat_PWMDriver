# Flourescent Heartbeat
 Heartbeat PWM driver for UV leds driving fluorecent material.

Can be debugged on Arduino, final upload for ATtiny8X or Digispark.
PWM output for LED MOSFET driver is on PB1.
Voltage divider for 2S Li-Po or direct 1S Li-Po ADC is on PB2.

Offers 4 different brightness presets, which cycle each time power is applied.
On initialisation, blinks battery level from 1-4 and the preset from 1-4.
