an audrino sketch for a power button functionally equivelent to a power button on a PC or laptop.

# BOM:
* audrino nano (AVR) or Rasberry Pi pico (rp2040)
* a button
* a relay (or two)
* some wire

# wiring notes

by default, both the button pin (D3/GPIO3) and the immedite shutoff (D5/GPIO5) are set to be pulled to GND. set the "TO_GND" consts to false for 5V/3.3V pulls. This needs a pull down resistor for the nano. 

Don't set D5/GPIO5's TO_GND to false if you're using the host SBC's GPIO pins to toggle it.

Don't use 5V on the RP2040, it only supports 3.3V

By default D7/GPIO7 is the output pin, and D5/GPIO5 is meant to be hooked to the thing being powered, such as an ARM based SBC, or something controlled by it.

D2/GPIO2 will output high if D3/GPIO3 is pushed for a short time & D7/GPIO7 is on. use the systemd service or something like klipper to detect it.
