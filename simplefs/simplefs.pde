
 #include <MD.h>


 #include "PatternLoadPage.h"
 #include "Sniffer.h"
 #include <FS_FileSystem.hh>
 #include <FS_File.hh>
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
  char buf[100][16];
  
  
  
extern void midi_start() {
}
 void PatternLoadEncoder::displayAt(int encoder_offset) {
   int i = 0;
   GUI.setLine(GUI.LINE1);

   GUI.put_string_at(0, &buf[patternload_param1.getValue()][0]);
   GUI.put_string_at(0, &buf[patternload_param1.getValue() + 1][0]);
   
 }
 FS_FileSystem file_system;
 void setup() {

        GUI.setPage(&patternload_page);
     clearLed();
 //    char str = "Formatting";
 //  GUI.put_string_at(0, &str);
   file_system.format();

/*     
     clearLed();
     FS_File file1;
     FS_File file2;
     FS_File file3;
     FS_File file4;
     file1.open("saturn",1000000);
     file2.open("mars",50000);
     file3.open("jupiter",600000);
     file3.open("mercury",600);

     file_system.list_entries((char**)&buf,1000);
*/
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

   }
     
     if (EVENT_PRESSED(evt, Buttons.BUTTON2)) { 

     }
       if (EVENT_PRESSED(evt, Buttons.BUTTON3)) { 

       }
         if (EVENT_PRESSED(evt, Buttons.BUTTON4)) { 

         }
         
 }
