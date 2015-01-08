# Arduino One-Octave Foot Pedal MIDI Controller

## License

MIT

If you really need to rip this off for a commercial application, consider hiring me instead. :wink:

## Hardware 

I've written this for the [Arduino Micro](http://arduino.cc/en/Main/ArduinoBoardMicro). 
(Thus, the use of Serial1 for the MIDI output.) 

It has a one-octave footpedal set 
with reed switches form a 70's era electronic organ. There are also an octave-up, octave-down, 
and configure stomp switches, bringin the total switch count to 16 which makes for a tidy 4 x 4 
switch matrix. This uses the very nice [keypad library](http://playground.arduino.cc/Code/Keypad)
to manage the scanning and hold detection. In contrast to what is mentioned in the keypad 
documentation, diodes are a necessity for this type of application (one per switch) or you may get ghosting. 

There's a seven-segment display to show the octave and channel number. 
The decimal point is used to indicate polyphonic mode. 
The driver is the [TLC5916](http://www.ti.com/product/tlc5916) which is a snap use over the SPI interface 
plus two extra outputs for the ~OE and LE pins. The current set resistor is 1K Ohm for approximately 20mA. 

