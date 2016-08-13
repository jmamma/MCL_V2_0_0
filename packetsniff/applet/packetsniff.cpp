
 #include <MD.h>


 #include "PatternLoadPage.h"
 #include "Sniffer.h"
 #include <MidiClockPage.h>
 #include <TurboMidi.hh>
 #include <SDCard.h>
 #include <string.h>
 #include <midi-common.hh>


  #include "WProgram.h"
extern void midi_start();
void setup();
static void sendTurbomidiHeaderHack(uint8_t cmd);
static void sendTurbomidiHeader(uint8_t cmd);
bool handleEvent(gui_event_t *evt);
PatternLoadEncoder patternload_param1(0, 99);
  PatternLoadEncoder patternload_param2(0, 15);
  PatternLoadEncoder patternload_param3(0, 64);
  PatternLoadEncoder patternload_param4(0, 11);
  PatternLoadPage patternload_page(&patternload_param1,&patternload_param2,&patternload_param3,&patternload_param4);
  int count = 0;
  uint8_t buf[100][8];
extern void midi_start() {
}
 void PatternLoadEncoder::displayAt(int encoder_offset) {
   int i = 0;
   GUI.setLine(GUI.LINE1);
   for (int i = 0; i < 8; i++) {
   GUI.put_valuex_at(i * 2, buf[patternload_param1.getValue()][i]);
   }
    GUI.setLine(GUI.LINE2);
   for (int i = 0; i < 8; i++) {
   GUI.put_valuex_at(i * 2, buf[patternload_param1.getValue() + 1][i]);
   }
 }
 
SnifferSysexListenerClass::SnifferSysexListenerClass() :
    MidiSysexListenerClass() {
    ids[0] = 0x00;
    ids[1] = 0x20;
    ids[2] = 0x3c;


}

void SnifferSysexListenerClass::handleByte(uint8_t byte) {
    if (MidiSysex.len == 3) {
        if (byte == 0x00) {
            isGenericMessage = true;
        } else {
            isGenericMessage = false;
        }
    }
}
uint32_t tmSpeeds[12] = { 
    31250,
    31250,
    62500,
    //104063,
    104062,
    125000,
    156250,
    208125,
    250000,
    312500,
    415625,
    500000,
    625000
};
void SnifferSysexListenerClass::end() {
setLed();
 // if (!isGenericMessage)
  //      return;   

    if (MidiSysex.data[5] == TURBOMIDI_SPEED_ACK_SLAVE) {



             setLed2();

    }
       int i = 0;
   
   for (int i = 0; i < 8; i++) {
     if (i < MidiSysex.len) {
  buf[count][i] = MidiSysex.data[i + 3];
     }
     
     
     
   }
   count++;
   if (count > 100) {
   count = 0;
   }
clearLed();
    }
    
 
SnifferSysexListenerClass Sniffer;
 
 void setup() {

     Sniffer.setup();

     GUI.setPage(&patternload_page);
     
 }
 
static uint8_t turbomidi_sysex_header[] = {
    0xF0, 0x00, 0x20, 0x3c, 0x00, 0x00
};

static uint8_t turbomidi_sysex_header_hack[] = { 
0xF0, 0x00, 0x20, 0x3c, 0x00, 0x00
};
static void sendTurbomidiHeaderHack(uint8_t cmd) {
    MidiUart.puts(turbomidi_sysex_header_hack, sizeof(turbomidi_sysex_header));
    MidiUart.putc(cmd);
}

static void sendTurbomidiHeader(uint8_t cmd) {
    MidiUart.puts(turbomidi_sysex_header, sizeof(turbomidi_sysex_header));
    MidiUart.putc(cmd);
}


bool handleEvent(gui_event_t *evt) {
   if (EVENT_PRESSED(evt, Buttons.BUTTON1)) {   
    sendTurbomidiHeader(TURBOMIDI_SPEED_NEGOTIATION_MASTER);
    MidiUart.putc(8 );
    MidiUart.putc(8 );
    MidiUart.putc(0xF7);
   }
     
     if (EVENT_PRESSED(evt, Buttons.BUTTON2)) { 
    sendTurbomidiHeader(TURBOMIDI_SPEED_NEGOTIATION_MASTER);
    MidiUart.putc(7 );
    MidiUart.putc(6 );
    MidiUart.putc(0xF7);
     }
       if (EVENT_PRESSED(evt, Buttons.BUTTON3)) { 
sendTurbomidiHeaderHack(0x20);
    MidiUart.putc(7 );
        MidiUart.putc(0xF7);
        delay(10);

                            MidiUart.setSpeed(tmSpeeds[7 ]);
                                    MidiUart.setActiveSenseTimer(130);
       }
         if (EVENT_PRESSED(evt, Buttons.BUTTON4)) { 
                       MidiUart.setSpeed(tmSpeeds[0]);
         }
         
 }

#include <Midi.h>

#include "WProgram.h"
extern "C" {
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
}

void timer_init(void) {
  TCCR0 = _BV(CS01);
  //  TIMSK |= _BV(TOIE0);
  
  TCCR1A = _BV(WGM10); //  | _BV(COM1A1) | _BV(COM1B1); 
  TCCR1B |= _BV(CS10) | _BV(WGM12); // every cycle
#ifdef MIDIDUINO_MIDI_CLOCK
  TIMSK |= _BV(TOIE1);
#endif

  TCCR2 = _BV(WGM20) | _BV(WGM21) | _BV(CS20) | _BV(CS21); // ) | _BV(CS21); // | _BV(COM21);
  TIMSK |= _BV(TOIE2);
}

void init(void) {
  /** Disable watchdog. **/
  wdt_disable();
  //  wdt_enable(WDTO_15MS);

  /* move interrupts to bootloader section */
  DDRC = 0xFF;
  PORTC = 0x00;
  MCUCR = _BV(IVCE);
  MCUCR = _BV(SRE);

  // activate lever converter
  SET_BIT(DDRD, PD4);
  SET_BIT(PORTD, PD4);

  // activate background pwm
  TCCR3B = _BV(WGM32) | _BV(CS30);
  TCCR3A = _BV(WGM30) | _BV(COM3A1);
  OCR3A = 160;

  DDRE |= _BV(PE4) | _BV(PE5);
  //  DDRB |= _BV(PB0);
  //  DDRC |= _BV(PC3);

  timer_init();
}


void (*jump_to_boot)(void) = (void(*)(void))0xFF11;

void start_bootloader(void) {
  cli();
  eeprom_write_word(START_MAIN_APP_ADDR, 0);
	wdt_enable(WDTO_30MS); 
	while(1) {};
}

void setup();
void loop();
void handleGui();

#define PHASE_FACTOR 16
static inline uint32_t phase_mult(uint32_t val) {
  return (val * PHASE_FACTOR) >> 8;
}

ISR(TIMER1_OVF_vect) {

  clock++;
#ifdef MIDIDUINO_MIDI_CLOCK
  if (MidiClock.state == MidiClock.STARTED) {
    MidiClock.handleTimerInt();
  }
#endif

  //  clearLed2();
}

// XXX CMP to have better time

static uint16_t oldsr = 0;

void gui_poll() {
  static bool inGui = false;
  if (inGui) { 
    return;
  } else {
    inGui = true;
  }
  sei(); // reentrant interrupt

  uint16_t sr = SR165.read16();
  if (sr != oldsr) {
    Buttons.clear();
    Buttons.poll(sr >> 8);
    Encoders.poll(sr);
    oldsr = sr;
    pollEventGUI();
  }
  inGui = false;
}

uint16_t lastRunningStatusReset = 0;

#define OUTPUTPORT PORTD
#define OUTPUTDDR  DDRD
#define OUTPUTPIN PD0

ISR(TIMER2_OVF_vect) {
  slowclock++;
  if (abs(slowclock - lastRunningStatusReset) > 3000) {
    MidiUart.resetRunningStatus();
    lastRunningStatusReset = slowclock;
  }

	MidiUart.tickActiveSense();
	MidiUart2.tickActiveSense();
  
  //  SET_BIT(OUTPUTPORT, OUTPUTPIN);

#ifdef MIDIDUINO_POLL_GUI_IRQ
  gui_poll();
#endif
  //  CLEAR_BIT(OUTPUTPORT, OUTPUTPIN);
}

uint8_t sysexBuf[8192];
MidiClass Midi(&MidiUart, sysexBuf, sizeof(sysexBuf));
uint8_t sysexBuf2[512];
MidiClass Midi2(&MidiUart2, sysexBuf2, sizeof(sysexBuf2));

void handleIncomingMidi() {
  while (MidiUart.avail()) {
    Midi.handleByte(MidiUart.getc());
  }
  
  while (MidiUart2.avail()) {
    Midi2.handleByte(MidiUart2.getc());
  }
}

void __mainInnerLoop(bool callLoop) {
  //  SET_BIT(OUTPUTPORT, OUTPUTPIN);
  //  setLed2();
  if ((MidiClock.mode == MidiClock.EXTERNAL ||
       MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
    MidiClock.updateClockInterval();
  }

  //  CLEAR_BIT(OUTPUTPORT, OUTPUTPIN);
  handleIncomingMidi();
  
  if (callLoop) {
    GUI.loop();
  }
}

void setupEventHandlers();
void setupMidiCallbacks();
void setupClockCallbacks();

int main(void) {
  delay(100);
  init();
  clearLed();
  clearLed2();

  uint16_t sr = SR165.read16();
  Buttons.clear();
  Buttons.poll(sr >> 8);
  Encoders.poll(sr);
  oldsr = sr;

  MidiSysex.addSysexListener(&MididuinoSysexListener);

  OUTPUTDDR |= _BV(OUTPUTPIN);
  setup();
	setupEventHandlers();
	setupMidiCallbacks();
	setupClockCallbacks();
  sei();

  for (;;) {
    __mainInnerLoop();
  }
  return 0;
}

void handleGui() {
  pollEventGUI();
}
