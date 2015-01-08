#include <Keypad.h>
#include <EEPROM.h>
#include <SPI.h>

#define NOTE_OFF (0)
#define NOTE_ON  (1)

#define MONO (0)
#define POLY (1)

#define EE_ADDR_OCTAVE  (0)
#define EE_ADDR_POLY    (1)
#define EE_ADDR_CHANNEL (2)

#define VELOCITY (127)

#define DOWN_BUTTON   (0xD)
#define CONFIG_BUTTON (0xE)
#define UP_BUTTON     (0xF)

#define DEBOUNCE_MS     (10)
#define HOLD_MS         (2000)
#define FLASH_MS        (2000)
#define BLINK_PERIOD_MS (500)

#define TLC5916_OE (10)
#define TLC5916_LE (11)

enum states {
  NORMAL, 
  OCT_DOWN_PRESSED, 
  OCT_UP_PRESSED,
  CONFIG_PRESSED,
  CONFIG_WAIT_RELEASE,
  CONFIG,
  CHAN_DOWN_PRESSED,
  CHAN_UP_PRESSED,
  POLY_PRESSED,
  NORMAL_WAIT_RELEASE
};

enum flash_states {
  OFF,
  FLASH,
  BLINK_ON,
  BLINK_OFF
};

const char hex_character[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
  0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
};

const char keys[4][4] = {
  {0x0, 0x1, 0x2, 0x3},
  {0x4, 0x5, 0x3, 0x7},
  {0x8, 0x9, 0xA, 0xB},
  {0xC, 0xD, 0xE, 0xF}
};
byte row_pins[4] = {6, 7, 8, 9}; 
byte col_pins[4] = {2, 3, 4, 5}; 
Keypad switches = Keypad(makeKeymap(keys), row_pins, col_pins, 4, 4);

char note_status[128];
char octave;
char poly;
char channel;
char state;
char seven_seg_state;
char flash_state;
unsigned long flash_timeout;

void setup() {
  char i;
  
  //  Set MIDI baud rate:
  Serial1.begin(31250);
  
  // initialize the note status array and status vars
  octave = EEPROM.read(EE_ADDR_OCTAVE) & 0x0F;
  poly = EEPROM.read(EE_ADDR_POLY) & 0x01;
  channel = EEPROM.read(EE_ADDR_CHANNEL) & 0x0F;
  for (i = 0; i < 128; i++) {
    note_status[i] = NOTE_OFF;
  }
  state = NORMAL;
  
  // setup keypad class timings
  switches.setDebounceTime(DEBOUNCE_MS);
  switches.setHoldTime(HOLD_MS);
  
  // init TLC5916 interface and state
  pinMode(TLC5916_OE, OUTPUT);
  digitalWrite(TLC5916_OE, HIGH);
  pinMode(TLC5916_LE, OUTPUT);
  digitalWrite(TLC5916_LE, LOW);
  write_digit(octave);
  write_dec_point(poly);
  
  // init seven seg flash timer and state
}

void loop() {
  char i;
  char kchar;
  char kstate;

  // check flash timeout
  if (flash_timeout != 0 && flash_timeout < millis()) {
    switch (flash_state) {
      case OFF:
        flash_timeout = 0;
      break;
      case FLASH:
        flash_timeout = 0;
        digitalWrite(TLC5916_OE, HIGH);
      break;
      case BLINK_ON:
        flash_timeout = millis() + BLINK_PERIOD_MS / 2;
        digitalWrite(TLC5916_OE, HIGH);
      break;
      case BLINK_OFF:
        flash_timeout = millis() + BLINK_PERIOD_MS / 2;
        digitalWrite(TLC5916_OE, LOW);
      break;
    }
  }
  
  // handle switches and buttons
  if (switches.getKeys()) {
    for (i = 0; i < LIST_MAX; i++) {
      if (switches.key[i].stateChanged) {
        kchar = switches.key[i].kchar;
        kstate = switches.key[i].kstate;
        
        // pedal note switches
        if (kchar <= 0xC) {  
          if (kstate == PRESSED) {
            if (poly == POLY) {
              all_notes_off();
            }
            note_on(channel, kchar + octave, VELOCITY);
            note_status[kchar + octave] = NOTE_ON;  
          } else if (kstate == RELEASED) {
            if (note_status[kchar + octave] == NOTE_ON) {
              note_off(channel, kchar + octave);
              note_status[kchar + octave] = NOTE_OFF;
            }
          }
          
        // octave up, octave down, config switches
        } else { 
          // state machine for stomp switch controls
          switch (state) {
            case NORMAL: 
              if (kstate == PRESSED) {
                switch (kchar) {
                  case DOWN_BUTTON:
                    octave_down();
                    write_digit(hex_character[octave]);
                    flash_seven_seg();
                    state = OCT_DOWN_PRESSED;
                  break;
                  case UP_BUTTON:
                    octave_up();
                    write_digit(hex_character[octave]);
                    flash_seven_seg();
                    state = OCT_UP_PRESSED;
                  break;
                  case CONFIG_BUTTON:
                    flash_seven_seg();
                    state = CONFIG_PRESSED;
                  break;
                }
              }
            break;
            case OCT_DOWN_PRESSED:
              if (kchar == DOWN_BUTTON && kstate == RELEASED) {
                state = NORMAL;
              }
            break;
            case OCT_UP_PRESSED:
              if (kchar == UP_BUTTON && kstate == RELEASED) {
                state = NORMAL;
              }
            break;
            case CONFIG_PRESSED:
              if (kchar == CONFIG_BUTTON && kstate == HOLD) {
                write_digit(hex_character[channel]);
                blink_seven_seg();
                state = CONFIG_WAIT_RELEASE;
              }
            break;
            case CONFIG_WAIT_RELEASE:
              if (kchar == CONFIG_BUTTON && kstate == RELEASED) {
                state = CONFIG;
              }
            break;
            case CONFIG:
              if (kstate == PRESSED) {
                switch (kchar) {
                  case DOWN_BUTTON:
                    channel_down();
                    write_digit(hex_character[channel]);
                    state = CHAN_DOWN_PRESSED;
                  break;
                  case UP_BUTTON:
                    channel_up();
                    write_digit(hex_character[octave]);
                    state = CHAN_UP_PRESSED;
                  break;
                  case CONFIG_BUTTON:
                    state = POLY_PRESSED;
                  break;
                }
              }
            break;
            case CHAN_DOWN_PRESSED:
              if (kchar == DOWN_BUTTON && kstate == RELEASED) {
                state = CONFIG;
              }
            break;
            case CHAN_UP_PRESSED:
              if (kchar == UP_BUTTON && kstate == RELEASED) {
                state = CONFIG;
              }
            break;
            case POLY_PRESSED:
              if (kchar == CONFIG_BUTTON) {
                if (kstate == RELEASED) {
                  toggle_poly();
                  write_dec_point(poly);
                  state = CONFIG;
                } else if (kstate == HOLD) {
                  write_digit(hex_character[octave]);
                  flash_seven_seg();
                  state = NORMAL_WAIT_RELEASE;
                }
              }
            break;
            case NORMAL_WAIT_RELEASE:
              if (kchar == CONFIG_BUTTON && kstate == RELEASED) {
                state = NORMAL;
              }
            break;
          }
        }
      }
    }
  }
}

// continuous rapid blinking
void blink_seven_seg() {
  flash_timeout = millis() + BLINK_PERIOD_MS / 2;
  digitalWrite(TLC5916_OE, LOW);
  flash_state = BLINK_ON;
}

// one-shot flash of longer duration
void flash_seven_seg() {
  flash_timeout = millis() + FLASH_MS;
  digitalWrite(TLC5916_OE, LOW);
  flash_state = FLASH;
}

// write the seven segment digit preserving the decimal point
void write_digit(char digit) {
  seven_seg_state = (seven_seg_state & 0x80) | (digit & 0x7F);
  SPI.transfer(seven_seg_state);
  digitalWrite(TLC5916_LE, HIGH);
  digitalWrite(TLC5916_LE, LOW);
}

// write the seven segment decimal preserving the digit
void write_dec_point(char dec_point) {
  seven_seg_state = (seven_seg_state & 0x7F) | (dec_point & 0x80);
  SPI.transfer(seven_seg_state);
  digitalWrite(TLC5916_LE, HIGH);
  digitalWrite(TLC5916_LE, LOW);
}

void octave_down() {
  all_notes_off();
  if (octave > 0) {
    octave--;
    EEPROM.write(EE_ADDR_OCTAVE, octave);
  }
}

void octave_up() {
  all_notes_off();
  if (octave < 7) {
    octave++;
    EEPROM.write(EE_ADDR_OCTAVE, octave);
  }
}

void channel_down() {
  all_notes_off();
  if (channel > 0) {
    channel--;
    EEPROM.write(EE_ADDR_CHANNEL, channel);
  }
}

void channel_up() {
  all_notes_off();
  if (channel < 0x0F) {
    channel++;
    EEPROM.write(EE_ADDR_CHANNEL, channel);
  }
}

void toggle_poly() {
  all_notes_off();
  poly = !poly;
  EEPROM.write(EE_ADDR_POLY, poly);
}

// turn off all notes flagged as having been turned on
void all_notes_off() {
  char j;
  for (j = 0; j < 128; j++) {
    if (note_status[j] == NOTE_ON) {
      note_off(channel, j);
      note_status[j] = NOTE_OFF;                
    }
  }
}

// send MIDI note-on message
void note_on(int channel, int pitch, int velocity) {
  Serial.write(0x90 | (channel & 0x0F));
  Serial.write(pitch & 0x7F);
  Serial.write(velocity & 0x7F);
}

// send MIDI note-off message
void note_off(int channel, int pitch) {
  Serial.write(0x80 | (channel & 0x0F));
  Serial.write(pitch & 0x7F);
  Serial.write(127); // velocity
}
