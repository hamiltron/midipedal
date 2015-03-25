# Arduino One-Octave Foot Pedal MIDI Controller

![pic of pedal](https://raw.githubusercontent.com/hamiltron/midipedal/master/pic.jpg)

## License

MIT

If you really need to rip this off for a commercial application, consider hiring me instead. :wink:

## Hardware 

I've written this for the [Arduino Micro](http://arduino.cc/en/Main/ArduinoBoardMicro). 
(Thus, the use of Serial1 for the MIDI output.) 

It has a one-octave foot pedal set 
with reed switches form a 70's era electronic organ. There are also an 'octave up', 'octave down', 
and 'configure' stomp switches, bringing the total switch count to 16 which makes for a tidy 4 x 4 
switch matrix. This uses the very nice [keypad library](http://playground.arduino.cc/Code/Keypad)
to manage the scanning and hold detection. In contrast to what is mentioned in the keypad 
documentation, diodes are a necessity for this type of application (one per switch) or you may get ghosting. 

There's a seven-segment display to show the octave and channel number. 
The decimal point is used to indicate polyphonic mode. 
The driver is the [TLC5916](http://www.ti.com/product/tlc5916) which is a snap to use over the SPI interface 
plus two extra Arduino outputs for the ~OE and LE pins. The current set resistor is 1K Ohm for 
approximately 20mA. 
The display I used is the Kingbright SA23-12SRWA which is about 6 cm high. It's nice and big and RED. 
**Rant**: Yes, blue LED's are new and therefore prejudicially cool, and red LED's are dusty old hat. 
But the reality 
is that human eyes disperse blue light more that red (the reason a lot of sunglasses have yellow tint) 
and these blue LED displays often have impressive 
luminosity which when combined, turn what would otherwise be a number into a fuzzy blue blob, 
leaving my microwave clock unreadable unless you're standing right in front of it. 

This seven segment display requires around 7.4 V to drive the segment diode strings, 
so we need a boost which is 
done with the old [MC34063](http://www.ti.com/product/mc34063a) (aka MC33063) 
which has very low frequency compared to modern switching regulators but there's a 
very nifty [part value calculator](http://www.nomad.ee/micros/mc34063a/). 
I doubled checked the calculated values with a spreadsheet based 
on the datasheet design equations and they all matched close enough for confidence. The intended output
voltage is about 9 V as this should give the needed 7.4 V for the LED's plus the TLC5916 output drop of 
about 1 V plus a small margin. This can be adjusted a bit if the display forward voltage deviates from 
the expected value. 

For the 5 V needed to drive the Atmel CPU, I decided to use the MCP1702 (5 V version) which supplies
a max current of 250 mA (completely fine for this application) with a typical dropout of 0.33 V. This is a 
much safer bet that using the onboard NCP1117 which has a spec'd dropout of 1.2 V 
(granted, at much higher currrent) and even though the CPU will probably run fine with a little less than 
5 V, having a roughly 0.4 V dropout gives much better margin of safety plus better mileage 
from the batteries. 

## UI

### Normal Mode

In any mode, the pedals trigger MIDI notes on the set octave and channel. In monophonic mode, the latest
pedal pressed triggers a note and the previous note is automatically silenced. In polyphonic mode, any 
number of notes may be triggered at once. (Technically this is not true as the keypad library keeps an
internal list of modified keys and it's default max size is ten, though this could easily be changed.) 
When in normal mode, the octave up and down switches move the octave and display is briefly on the 
display which also indicated whether or not polyphonic mode is enabled with the decimal point. 
Pressing the center 'configure' button briefly will also flash the display with the current channel.

**NOTE**: The octave and channel display (see below) are in zero-indexed hex, 
which departs from the convention of numbering MIDI channels 1 - 16, but this doesn't bother me
for personal uses so I'll leave it as such. 

### Configure Mode

If the configure button is held down for a few seconds, the controller will enter the configure mode, and the 
display will flash the current MIDI channel. Pressing the octave up and down buttons will change the 
MIDI channel and pressing the configure button will switch between polyphonic and monophonic mode
when the button is released. 
Pressing and holding the configure button will return to normal mode. 

### Non-Volatile Memory 

The current octave, channel, and poly/monophonic settings are stored in EEPROM each time they are
changed so the configuration is retained when the controller is switched off. 
