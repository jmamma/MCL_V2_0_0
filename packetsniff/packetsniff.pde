
 #include <MD.h>


 #include "PatternLoadPage.h"
 #include "Sniffer.h"
 #include <MidiClockPage.h>
 #include <TurboMidi.hh>
 #include <SDCard.h>
 #include <string.h>
 #include <midi-common.hh>


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
        delay(60);
                            MidiUart.setSpeed(tmSpeeds[7 ]);
       }
         if (EVENT_PRESSED(evt, Buttons.BUTTON4)) { 
                       MidiUart.setSpeed(tmSpeeds[0]);
         }
         
 }
