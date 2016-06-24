
/*
  ==============================
  MiniCommand Live Firmware: 
  ==============================
  Version 1.0
  Author: Justin Valerp
  Date: 2/04/2014
*/


 #include <MD.h>
 #include "GridPage.h"
 #include "TrackInfoPage.h"
 #include "PatternLoadPage.h"
 #include "OptionPage.h"
 #include <MidiClockPage.h>
 #include <TurboMidi.hh>
 #include <SDCard.h>
 #include <string.h>
 #include <midi-common.hh>
 //#include <TrigCapture.h>


/*MDKit and Pattern objects must be defined outside of Classes and Methods otherwise the Minicommand freezes.*/
  MDPattern pattern_rec;
  MDKit kit_new;
  MDGlobal global_new;

/*Current Loaded project file, set to test.mcl as initial name*/
  SDCardFile file("/test.mcl");
/*Configuration file used to store settings when Minicommand is turned off*/
  SDCardFile configfile("/config.mcls");

  int numProjects = 0;
  int curProject = 0;
  int global_page = 0;

  uint8_t notes_off_counter = 0;

  uint8_t currentkit_temp = 0;
  uint32_t cue1 = 0;
  uint8_t notes[16];
  uint8_t notecount=0;
  uint8_t firstnote = 0;
  uint8_t notecheck = 0;
  uint8_t noteproceed = 0;
  uint32_t div16th_last;
//Effects encoder values.
  uint8_t fx_dc = 0;
  uint8_t fx_fb = 0;
  uint8_t fx_lv = 0;
  uint8_t fx_tm = 0;

//GUI switch, used to identify what level of the GUI we are currently in.  
  uint8_t curpage = 0;
  uint8_t turbo_state = 0;
//ProjectName string

  char newprj[18];



/*A toggle for sending tracks in their original pattern position or not*/
  bool trackposition = false;

/*Counter for number of tracks to store*/
  uint8_t store_n_tracks;
  
/*500ms callback timeout for getcurrent track, kit etc... */
  const int callback_timeout = 500;

/* encodervalue */
/*Temporary register for storeing the value of the encoder position or grid/Grid position for a callback*/

  int encodervalue = NULL;
  int cur_col = 0;
  int cur_row = 0;
  int countx = 0;
  bool collect_notes = false;
//Instantiating GUI Objects.
  uint8_t dont_interrupt = 0;
  GridEncoder param1(0, 12);
  GridEncoder param2(0, 127);
  GridEncoder param3(0, 127);
  GridEncoder param4(0, 127);

  GridEncoderPage page(&param1, &param2, &param3, &param4);

  TrackInfoEncoder trackinfo_param1(0, 3);
  TrackInfoEncoder trackinfo_param2(0, 3);
  TrackInfoPage trackinfo_page(&trackinfo_param1,&trackinfo_param2);

  PatternLoadEncoder patternload_param1(0, 7);
  PatternLoadEncoder patternload_param2(0, 15);
  PatternLoadEncoder patternload_param3(1, 64);
  PatternLoadPage patternload_page(&patternload_param1,&patternload_param2, &patternload_param3);

  OptionsEncoder options_param1(0, 3);
  OptionsEncoder  options_param2(0, 1);
  OptionsPage options_page(&options_param1,&options_param2);

  TrackInfoEncoder proj_param1(1, 10);
  TrackInfoEncoder proj_param2(0, 36);
 // TrackInfoEncoder proj_param4(0, 127);
  TrackInfoPage proj_page(&proj_param1,&proj_param2);

  TrackInfoEncoder loadproj_param1(1, 64);
  TrackInfoPage loadproj_page(&loadproj_param1);
  
//  TrigCaptureClass test



/*A toggle for handling incoming pattern and kit data.*/

  uint8_t patternswitch = 254;

/* Gui CallBack flag for writing a complete pattern */
  int writepattern;

/* Allowed characters for project naming*/

  const char allowedchar[38] = "0123456789abcdefghijklmonpqrstuvwxyz_";
  

/* 
   ==============
   class: MDTrack
   ==============

  Class for defining Track Objects. The main data structure for MCL
  The GRID data structure that resides on the SDCard is is made up of a linear array of Track Objects.
  
  Each Track object holds the complete information set of a Track including it's Machine, 
  Machine Settings, Step Sequencer Triggers (trig, accent, slide, swing), Parameter Locks,
   
*/



class MDTrack {
  public:
  bool active;
  char kitName[8];
  uint64_t trigPattern;
  uint64_t accentPattern;
  uint64_t slidePattern;
  uint64_t swingPattern;
  //Machine object for Track Machine Type
  MDMachine machine;
  //int8_t paramLocks[24]; 
  //Array to hold parameter locks.
  int arraysize;
  uint8_t param_number[256];
  uint8_t value[256];
  uint8_t step[256];
    
  /* 
  ================
  method: storeTrack (int tracknumber, uint8_t column);
  ================**
  
  Store track in memory by reading it from the Pattern Data
  Both a current Pattern and current Kit must be received from the MD first
  for this to work.
  */

  bool storeTrack (int tracknumber, uint8_t column) {
    
     
   active = TRUE;
    trigPattern = pattern_rec.trigPatterns[tracknumber];
  accentPattern = pattern_rec.accentPatterns[tracknumber];
  slidePattern = pattern_rec.slidePatterns[tracknumber];
  swingPattern = pattern_rec.swingPatterns[tracknumber];
 

   //Extract parameter lock data and store it in a useable data structure
   int n = 0;
   for (int i = 0; i < 24; i++) {
                     if (IS_BIT_SET32(pattern_rec.lockPatterns[tracknumber], i)) {
                                 int8_t idx = pattern_rec.paramLocks[tracknumber][i];
                                 
                                 for (int s = 0; s < 64; s++) {

                                   if ((pattern_rec.locks[idx][s] != 254) && (pattern_rec.locks[idx][s] >= 0)) {
                                      step[n] = s;
                                      param_number[n] = i;
                                      value[n] = pattern_rec.locks[idx][s];
                                      n++;
                                   }
                                   
                                 }
                                
           }
    }

 //  itoa(n,&str[2],10);
  
   arraysize = n;

   
 
  /*Don't forget to copy the Machine data as well
  Which is obtained from the received Kit object kit_new*/
  //  m_strncpy(kitName, kit_new.name, 17);
  for (uint8_t c; c < 7; c++) {
  kitName[c] = kit_new.name[c]; 
  }
  kitName[7] = '\0';
  m_memcpy(machine.params, kit_new.params[tracknumber], 24);
  
  machine.track = tracknumber;
  machine.level = kit_new.levels[tracknumber];
  machine.model = kit_new.models[tracknumber];
   
   
  /*Check to see if LFO is modulating host track*/
  /*IF it is then we need to make sure that the LFO destination is updated to the new row posiiton*/
  
  if (kit_new.lfos[tracknumber].destinationTrack == tracknumber) {
    kit_new.lfos[tracknumber].destinationTrack = column;
  }
   /*Copies Lfo data from the kit object into the machine object*/
  m_memcpy(&machine.lfo, &kit_new.lfos[tracknumber],sizeof(machine.lfo));
  
  machine.trigGroup = kit_new.trigGroups[tracknumber];
  machine.muteGroup = kit_new.muteGroups[tracknumber];
 
 }
 
   /* 
  ================
  method: placeTrack(int tracknumber, uint8_t column);
  ================
  
  Place a Track inside a pattern and a kit
  Both a current Pattern and current Kit must be received from the MD first
  for this to work.
  */
  
  void placeTrack(int tracknumber, uint8_t column) {
//Check that the track is active, we don't want to write empty/corrupt data to the MD
    if (active == true) {
    for (int x = 0; x < 64; x++) {
   clear_step_locks(tracknumber, x);
    }
    
   // pattern_rec.lockPatterns[tracknumber] = 0;
    //Write pattern lock data to pattern
    pattern_rec.trigPatterns[tracknumber] = trigPattern;
    pattern_rec.accentPatterns[tracknumber] = accentPattern;
    pattern_rec.slidePatterns[tracknumber] = slidePattern;
    pattern_rec.swingPatterns[tracknumber] = swingPattern;
     
    for (int n = 0; n < arraysize; n ++) {
    //  if (arraysize > 5) {     GUI.flash_string_fill("greater than 5"); }
             pattern_rec.addLock(tracknumber, step[n], param_number[n], value[n]);
    }   
     
  //Possible alternative for writing machinedata to the MD without sending the entire Kit >> MD.setMachine(tracknumber, &machine)
  
  /*Don't forget to store the Kit data as well which is taken from the Track's
  /associated MDMachine object.*/
  
  /*if kit_sendmode == 1 then we're going to send an entire kit to the machinedrum inorder to load up the machine on the desired track
  In this case, we'll need to copy machine model to the kit kit_new object which will be converted into a sysex message and sent to the MD*/
  /*if kit_sendmode == 0 then we'll load up the machine via sysex and Midi CC messages without sending the kit*/

  m_memcpy(kit_new.params[tracknumber], machine.params, 24);

  kit_new.levels[tracknumber] = machine.level;
  kit_new.models[tracknumber] = machine.model;
  
  
   if (machine.lfo.destinationTrack == column) {

    machine.lfo.destinationTrack = tracknumber;
  
  }
  
  m_memcpy(&kit_new.lfos[tracknumber],&machine.lfo,sizeof(machine.lfo));
 
        
  kit_new.trigGroups[tracknumber] = machine.trigGroup;
  kit_new.muteGroups[tracknumber] = machine.muteGroup;
                   
  }
  }
};


/*Temporary track objects for manipulation*/
MDTrack temptrack;
MDTrack temptrack2;

  /* 
 ================
 class: ParamterLock
 ================
  Data structure defining one parameter lock. 
  Note: Unused.
 */
 
class ParameterLock {
  public:
 uint8_t param_number;
 uint8_t value;
 uint8_t step;
};





   /* 
  ================
  class: Config
  ================

  Data structure for storing variables when the Minicommand is powered off.
  
  */
  

class Config {
 public:
 char project[15];
 uint8_t turbomidi;
 uint8_t merge;
 uint8_t cue_output; 
  
};
Config config;


/* 

   /* 
  ================
  function: sd_load_init()
  ================

  This function is called when from Setup() when the Minicommand is powered on.
  It's responsible for handling the SD Card.
  
  First it checks to see if the card can be read with the .init method.
  
  If the card can be read it then attempts to read the config file.
  If the config file does not exist, a new project is created and the config file is written.
  If the config file does exist then the last used project is loaded.
  
  */
void sd_load_init() {

    if (SDCard.init() != 0) {
    GUI.flash_strings_fill("SD CARD ERROR", "CHECKCARD");
    }

    else {    
           if (configfile.open(true)) {
             if (configfile.read(( uint8_t*)&config, sizeof(Config))) {
             configfile.close();
                 if (config.project != NULL) {
                      sd_load_project(config.project);
                  }
                   else {
                    load_project_page();
                   }
             }
              else {
              new_project_page();
           }
           }
           else {
              new_project_page();
           }
    }

}
  
  
    /* 
  ================
  function: load_project_page()
  ================
  
  Displays the root file system of the SD Card.
  
  List 64 entries from the SD Card, including files and directories.
  
  */
  
  SDCardEntry entries[64];
void load_project_page() {

   int numEntries = SDCard.listDirectory("/", entries, countof(entries));
   if (numEntries <= 0) {
        numEntries = 0;
       loadproj_param1.max = 0;
     }
        loadproj_param1.max = numEntries - 1;

            curpage = 8;
          GUI.setPage(&loadproj_page);

}

 /* 
  ================
  function: new_project_page()  
  ================
  
  Loads the GUI for the new project page.
  
  */

void new_project_page() {

  m_strncpy(newprj,"/newproject.mcl",16);
    curpage = 7;


      update_prjpage_char();
  GUI.setPage(&proj_page);
}
 

   /* 
  ================
  function: sd_new_project(char *projectname) 
  ================
  
  Create a new Project on the SD Card.
  Opens a new file and fills it with empty data for all the Grids in the Grid.
  
  */
  
  bool sd_new_project(char *projectname) {

 
   numProjects++;
   file.close();
   file.setPath(projectname);

  temptrack.active = FALSE;
file.open(true);

   //Make sure the file is large enough for the entire GRID        
  uint8_t exitcode = fat_resize_file(file.fd, (uint32_t) sizeof(MDTrack) * (uint32_t) 130 *(uint32_t) 16);
  if (exitcode == 0) {
            file.close();
  return false;
  }    
   uint8_t ledstatus = 0;
   //Initialise the project file by filling the grid with blank data.
      for (int32_t i = 0; i < 130 * 16; i++) {   
        if (i % 50 == 0) {
         if (ledstatus == 0) {
             setLed2();
             ledstatus = 1;
         }
         else {
          clearLed2(); 
             ledstatus = 0;
         }
        }

//file.write(( uint8_t*)&temptrack.active, 550);
 //  int x = sizeof(MDTrack);  
 //  int size = 100;
 //  int count = 0;
//   while (x > 0) {
//      x = x - size;
//      if (x < 0) { size = 100 - x; }
//      int b = file.write(( uint8_t*)&temptrack + count, size);
//      count = count + size;
//   }

//Clear the Grid at position i
    clear_Grid(i);
      
//              LCD.goLine(0);
 //              char str[5];
 //  itoa(b, &str[0],10);
 //  GUI.flash_string(str, 2000);
  //   LCD.puts(str);
          
      
      }
                clearLed2();
         file.close();
             file.open(true);
   m_strncpy(config.project,projectname,15);
  

 write_config();
 return true;
 }

   /* 
  ================
  function:  sd_load_project(char *projectname)
  ================
  
  Loads a project with the filename: *projectname
  
  */
 
void sd_load_project(char *projectname) {

   file.close();
   file.setPath(projectname);
   file.open(true);
   
   m_strncpy(config.project,projectname,15);
   write_config();
  
}


 /* 
  ================
  function: write_config()
  ================
  
  Write the configuration file
  The configuration file is used to remember settings on power off, such as current project.
  
  */
  
void write_config() {
   configfile.open(true);
   
   configfile.write(( uint8_t*)&config, sizeof(Config));
   configfile.close();
}



 /* 
  ================
  function: load_track(int32_t column, int32_t row, int m = 0)
  ================
  
  Read a track/Grid from the SD Card and store it in a temporary track object "temptrack"

  */
  
bool load_track(int32_t column, int32_t row, int m = 0) {

   // char projectfile[5 + m_strlen(projectdir) + 10];
    //         get_trackfilename(projectfile, 0, 0);
             


  int32_t offset = (column + (row * (int32_t)16)) * (int32_t) sizeof(MDTrack);

  //sd_buf_readwrite(offset, true);
      file.seek(&offset,FAT_SEEK_SET);
  file.read(( uint8_t*)&(temptrack.active),sizeof(temptrack.active));
  file.read(( uint8_t*)&(temptrack.kitName),  sizeof(temptrack.kitName));
  file.read(( uint8_t*)&(temptrack.trigPattern),  sizeof(temptrack.trigPattern));
  file.read(( uint8_t*)&(temptrack.accentPattern),  sizeof(temptrack.accentPattern));
  file.read(( uint8_t*)&(temptrack.slidePattern),  sizeof(temptrack.slidePattern));
  file.read(( uint8_t*)&(temptrack.swingPattern),  sizeof(temptrack.swingPattern));

  file.read(( uint8_t*)&(temptrack.machine),  sizeof(temptrack.machine));

  file.read(( uint8_t*)&(temptrack.arraysize),  sizeof(temptrack.arraysize));
          if (m == 0) {
  file.read(( uint8_t*)&(temptrack.param_number),  128);
  file.read(( uint8_t*)&(temptrack.value),  128);
  file.read(( uint8_t*)&(temptrack.step),  128);
      }
      
//  if (x != 9) { return FALSE; }
  return TRUE;

  
 // uint64_t temp;
//  file.read(( uint8_t*)&(temp), sizeof(temp));
 // temptrack.trigPattern = temp;

 
  
}

/*
  ================
  function: sd_buf_readwrite(int32_t offset, bool read) 
  ================
  
  Function for reading and writing track data to/from the SD Card
  Dynamic Read and Write
  Automatically compensates for reduced buffer sizes, defined by variable: size
  pretty clever!!
  
  Note: Not used, data needs to be written/restored on a per object basis. See store_track_inGrid(int track, int32_t column, int32_t row)

  */

void sd_buf_readwrite(int32_t offset, bool read) {
    file.seek(&offset,FAT_SEEK_SET);
  int x = sizeof(MDTrack);
 int size = 200;
  int count = 0;
    while (x > 0) {
     x = x - size;
      if (x < 0) { size = size - x; }
      if (read == true) { size = file.read(( uint8_t*)&temptrack + count, size); }
      else { size = file.write(( uint8_t*)&temptrack2 + count, size); }
     count = count + size;
   }
  
}

/*
  ===================
  function: store_track_inGrid(int track, int32_t column, int32_t row)
  ===================
 
  Stores a track from the currently received pattern into GridPosition defined by column and row.
  
*/

bool store_track_inGrid(int track, int32_t column, int32_t row) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track object*/
  
  //load_track(column, row);
  
  temptrack2.storeTrack(track,column);
  //  if (!SDCard.isInit) { setLed(); }

               // SDCard.deleteFile("/valertest.mcl");
              //  MDTrack temp;
 

  int32_t offset = (column + (row * (int32_t) 16)) *  (int32_t) sizeof(MDTrack);
  
  

  int x = 0;
 // sd_buf_readwrite(offset, false);
  file.seek(&offset,FAT_SEEK_SET); 
  file.write(( uint8_t*)&(temptrack2.active), sizeof(temptrack2.active));
  file.write(( uint8_t*)&(temptrack2.kitName),  8);
  file.write(( uint8_t*)&(temptrack2.trigPattern),  sizeof(temptrack2.trigPattern));
  file.write(( uint8_t*)&(temptrack2.accentPattern),  sizeof(temptrack2.accentPattern));
  file.write(( uint8_t*)&(temptrack2.slidePattern),  sizeof(temptrack2.slidePattern));
  file.write(( uint8_t*)&(temptrack2.swingPattern),  sizeof(temptrack2.swingPattern));
  file.write(( uint8_t*)&(temptrack2.machine),  sizeof(temptrack2.machine));
  file.write(( uint8_t*)&(temptrack2.arraysize),  sizeof(temptrack2.arraysize));
  file.write(( uint8_t*)&(temptrack2.param_number),  128);
  file.write(( uint8_t*)&(temptrack2.value),  128);
  file.write(( uint8_t*)&(temptrack2.step),  128);
 //LCD.puts(temptrack2.kitName);
//delay(2000);
      //              LCD.goLine(0);
 //   LCD.putnumber32(x);
//delay(2000);
 //   file.write(( uint8_t*)(temptrack2.kitName[0]), 8 );
  //file.write(( uint8_t*)&(temptrack2.active), 8 * 4);

//    file.write(( uint8_t*)&(temptrack2), 1 + 8 + 8 * 4);
 // file.write(( uint8_t*)&(temptrack2.active), sizeof(temptrack2.active));
//    file.write(( uint8_t*)&(temptrack2.kitName), 8);
  //uint64_t trigpattern = 0;
  //CLEAR_BIT64(trigpattern,1);
//  CLEAR_BIT64(trigpattern,14);
 // uint64_t trigpattern = temptrack2.trigPattern;
//  file.write(( uint8_t*)&(trigpattern), sizeof(trigpattern));
  //int x = sizeof(MDTrack);
 // int size = 100;
 // int count = 0;
 //    while (x > 0) {
 //     x = x - size;
 //     if (x < 0) { size = 100 - x; }
 //     int b = file.write(( uint8_t*)&temptrack2 + count, size);
 //     count = count + size;
  // }
  
  
//  file.write(( uint8_t*)&temptrack2, 550);
  
   
   //char projectfile[5 + m_strlen(projectdir) + 10];
   //          get_trackfilename(projectfile, 0, 0);
  //if (!SDCard.writeFile("/0.mcl", ( uint8_t*)&temptrack, sizeof(MDTrack))) {
//GUI.flash_strings_fill("COULDNTWRITE", "ERROR");
  
 // return false;
 // }
  return true;
}
void draw_notes() {
   GUI.setLine(GUI.LINE1);

               /*Initialise the string with blank steps*/
               char str[17] = "----------------";
               
                /*Display 16 track cues on screen, 
                /*For 16 tracks check to see if there is a cue*/
                for (int i = 0; i < 16; i++) {

                                             if (notes[i] > 0) 
                                             {
                /*If the bit is set, there is a cue at this position. We'd like to display it as [] on screen*/
                /*Char 219 on the minicommand LCD is a []*/
                                              str[i] = (char) 219;
                                           }
                       
                 }
          
                 /*Display the cues*/                       
                GUI.put_string_at(0,str);

}

class TrigCaptureClass : public MidiCallback {
	/**
	 * \addtogroup midi_TrigCapture 
	 *
	 * @{
	 **/
	
public:

  void setup() {

  Midi.addOnNoteOnCallback(this, (midi_callback_ptr_t)&TrigCaptureClass::onNoteOnCallback);
  Midi.addOnNoteOffCallback(this, (midi_callback_ptr_t)&TrigCaptureClass::onNoteOffCallback); 

  }

void onNoteOffCallback(uint8_t *msg) {
  

   uint8_t note_num;
        note_num = (msg[1] - 36);
 //         if (notes[note_num] == 3) {
 //      return;
 //    }
 // MidiUart.sendMessage(msg[0], msg[1], msg[2]);
  // patternload_param2.cur = msg[2];
 // patternload_page.display();
  if ((msg[2] == 0)) {
   notes_off_counter++;
  
   // GUI.put_value(0, notes_off_counter);

   //if (notecheck == 1) {
//      if (firstnote == msg[1]) {
  //        noteproceed = 1;
   //   }   
    //}   

   if ((collect_notes) && (msg[0] == 153) && (noteproceed == 1))  {


   
     if (note_num < 16) {
      
       if (notes[note_num] == 1)  {
      //    if (curpage == 5) {
      //    notes[note_num] = 0;
      //    }
      //    else {
          notes[note_num] = 3;
       //   }
       }
     }
     //If we're on track read/write page then check to see
       //   if ((curpage == 3) || (curpage == 4) || (curpage == 5)) {
     if ((curpage == 3) || (curpage == 4)) {
                  //store_tracks_in_mem(param1.getValue(),param2.getValue(), 254);


//        for (int i = 0; i < 16; i++) {
 ////            if (IS_BIT_SET32(notes,i)) {
 //            all_notes_off = 1;
  //           } 
  //      }
          draw_notes();
          uint8_t all_notes_off = 0;
          uint8_t a = 0;
          uint8_t b = 0;
          for (int i = 0; i < 16; i++) {
               if (notes[i] == 1) { a++; }
               if (notes[i] == 3) { b++; }
          }
          
          if ((a == 0) && (b > 0)) { all_notes_off = 1; }
          
        if (all_notes_off == 1) {
          
          
        if (curpage == 3)  {

                  exploit_off();         
                  store_tracks_in_mem(param1.getValue(),param2.getValue(), 254);
        }
        if (curpage == 4) {
                  exploit_off();
                  write_tracks_to_md(patternload_param2.cur + (patternload_param1.cur * 16), param1.getValue(),param2.getValue());
        } 
   //     if (curpage == 5) {
   //               return; 
   //     }
           GUI.setPage(&page);
                 curpage = 0;

        }

       
     }
    // notes[note_num] = 0;
   }
 // removeNote(msg[1]);
  //   clearLed();
   }
   
}

void onNoteOnCallback(uint8_t *msg) {
      if (msg[2] > 0) {

  int note_num;
       
    //   if (notecheck == 0) {
    //       firstnote = msg[1];
     //      notecheck = 1; 
     //  }
         if (MidiClock.div16th_counter >= (div16th_last + 4)) {
          noteproceed = 1; 
         }
 
         // LCD.goLine(0);
            //     LCD.putnumber(msg[0]);
  //msg[0]
  //  addNote(msg[1], msg[2]);
   //GUI.flash_put_value_at(11, msg[1] ); 
   //38 - 36 = 2 / 2
 //       setLed();

    if ((collect_notes) && (msg[0] == 153) && (noteproceed == 1)) {

     note_num = (msg[1] - 36);
     if (note_num < 16) {
     if (notes[note_num] == 0) {
     notes[note_num] = 1;
   
     }
      }
      
     draw_notes();
   }
   
      }
	/* @} */
}


};

/*
  ===================
  class: MDHandler2
  ===================

  
Class for handling Callbacks
This class defines CallBack methods for various Machinedrum and Sysex 
callbacks.

*/

class MDHandler2 : public MDCallback {
  
  public:
   
       /*Tell the MIDI-CTRL framework to execute the following methods when callbacks for
       Pattern and Kit messages are received.*/
  void setup() {
 
        MDSysexListener.addOnPatternMessageCallback(this,(md_callback_ptr_t)&MDHandler2::onPatternMessage); 
        MDSysexListener.addOnKitMessageCallback(this,(md_callback_ptr_t)&MDHandler2::onKitMessage); 
        MDSysexListener.addOnGlobalMessageCallback(this,(md_callback_ptr_t)&MDHandler2::onGlobalMessage); 
    }
  
  
    void onGlobalMessage() {
              setLed2();
              if (!global_new.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) { GUI.flash_strings_fill("GLOBAL", "ERROR");  }
              clearLed2();
    }
/*A kit has been received by the Minicommand in the form of a Sysex message which is residing 
in memory*/

   void onKitMessage() {
        setLed2();
        /*If patternswitch == 0 then the Kit request is for the purpose of obtaining track data*/
        if ((patternswitch == 0) || (patternswitch == 3) || (patternswitch == 1)) {
        /*Stores kit from sysex into a Kit object: kit_rec.
        The actual kit data is located after the Sysex header. The header is 5 bytes long.*/
        
        if (!kit_new.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) { GUI.flash_strings_fill("SYSEX-KIT", "ERROR");  }
    }
    /*Patternswitch == 6, store pattern in memory*/
    /*load up tracks and kit from a pattern that is different from the one currently loaded*/
         if (patternswitch == 6) {
                   if (kit_new.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {

                        for (int i = 0; i < 16; i++) {
                               if ((i + cur_col + (cur_row * 16)) < (128*16)) {
              
                          /*Store the track at the  into Minicommand memory by moving the data from a Pattern object into a Track object*/
                                store_track_inGrid(i, i, cur_row);

                              }
                                /*Update the encoder page to show current Grids*/
                            page.display();
                          }
           /*If the pattern can't be retrieved from the sysex data then there's been a problem*/

           
              patternswitch = 254; 

                   }
         }

             if (patternswitch == 7) {
                       if (kit_new.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
                         if (param3.effect == MD_FX_ECHO) {
                          param3.setValue(MD.kit.delay[param3.fxparam]) ;
                         }
                         else { param3.setValue(MD.kit.reverb[param3.fxparam]);  }
                              if (param4.effect == MD_FX_ECHO) {
                          param4.setValue(MD.kit.delay[param4.fxparam]) ;
                         }
                         else { param4.setValue(MD.kit.reverb[param4.fxparam]);  }
                       }
                                     patternswitch = 254; 
             }  
    clearLed2();
}



/*For a specific Track located in Grid curtrack, store it in a pattern to be sent via sysex*/

void place_track_inpattern(int curtrack, int column, int row) {
          //       if (Grids[encodervaluer] != NULL) {
                 if (load_track(column,row)) {
                 temptrack.placeTrack(curtrack, column);
                 }
         //        }
}


/*
  ===================
  function: onPatternMessage()
  ===================
 
  Function for handling onPatternMessage Sysex Callbacks as specified in the MDHandler Class.
 
  A pattern has been received by the Minicommand in the form of a Sysex message which is residing 
  in memory (pattern_rec). 
  
  pattern_rec.fromSysex allows you to store that pattern into a Pattern object.

  
*/


void onPatternMessage() {
  setLed2();
  
/*Reverse track callback*/
if (patternswitch == 5) {
          /*Retrieve the pattern from the Sysex buffer and store it in the pattern_rec object. The MD header is 5 bytes long, hence the offset and length change*/
        

         if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {    
                     /*
          //Get current track number from MD
                       int curtrack = MD.getCurrentTrack(callback_timeout);
                       int i = 0;

          //Data structure for holding the parameter locks of a single step                                       
                       uint8_t templocks[24];

          //Retrevieve trig masks and store them temporarily
          
                       uint64_t temptrig = pattern_rec.trigPatterns[curtrack];
                       uint64_t tempslide = pattern_rec.slidePatterns[curtrack];
                       uint64_t tempaccent = pattern_rec.accentPatterns[curtrack];
                       uint64_t tempswing = pattern_rec.swingPatterns[curtrack];
          
          //Reversing a track requires reflecting a track about its mid point
          //We can therefore start from the beginning and end of the pattern, swapping triggers and working our way to the middle
          
                     while (i < pattern_rec.patternLength / 2) {

          //Initialize locks
                              for (uint8_t g = 0; g < (24); g++) { templocks[g] = 254; }
          
          //If there is a trigger, then we'll need to swap it                    
                             if (IS_BIT_SET64(pattern_rec.trigPatterns[curtrack],i)) { 
                    
                                //backup the locks
                                
                                       for (int m = 0; m < 24; m++) {
                                             if (IS_BIT_SET32(pattern_rec.lockPatterns[curtrack], m)) {
                                                  int8_t idx = pattern_rec.paramLocks[curtrack][m];
                                                  int8_t value = pattern_rec.locks[idx][ i];

                                                   if (value != 254) { templocks[m] = value; }
                                             }
                                 
                                        }
                                


                              }
                              
                              //Clear the existing locks for the step
                              clear_step_locks(curtrack,i);
                           
                              
                              
                              if (IS_BIT_SET64(pattern_rec.trigPatterns[curtrack],pattern_rec.patternLength - i - 1)) {                 
                                 SET_BIT64(pattern_rec.trigPatterns[curtrack],i);
                              }
                                 else {  CLEAR_BIT64(pattern_rec.trigPatterns[curtrack],i); }                     
                               //Copy the parameter locks of the loop step, to the new step
                                for (int m = 0; m < 24; m++) {
                                           if (IS_BIT_SET32(pattern_rec.lockPatterns[curtrack], m)) {
                                              int8_t idx = pattern_rec.paramLocks[curtrack][m];
	                                      int8_t param = m;
                                              int8_t value = pattern_rec.locks[idx][pattern_rec.patternLength - i - 1];

                                               if (value != 254) { pattern_rec.addLock(curtrack, i, param, value); }
                                           }
                                 
                                        }
                               
                               //Clear the existing locks for the step
                               clear_step_locks(curtrack, pattern_rec.patternLength - i - 1) ;
                              
                            // pattern_rec.clearTrig(curtrack, pattern_rec.patternLength - i - 1 );
                             if (IS_BIT_SET64(temptrig,i))  { SET_BIT64(pattern_rec.trigPatterns[curtrack],pattern_rec.patternLength - i - 1 ); }
                               else { CLEAR_BIT64(pattern_rec.trigPatterns[curtrack],pattern_rec.patternLength - i - 1 ); }      
              
                               
                             uint8_t h = 0;
                             while (h < (24)) {
                               if (templocks[h] != 254) {
                               pattern_rec.addLock(curtrack, pattern_rec.patternLength - i - 1 , h, templocks[h]);
                               }
                               h++;
                             }
                             
                        

                             
                            //If there is a accent trigger on the loop step, copy the accent trigger to the new step
                            if (IS_BIT_SET64(tempaccent,pattern_rec.patternLength - i - 1)) {  SET_BIT64(pattern_rec.accentPatterns[curtrack],i); }
                            else { CLEAR_BIT64(pattern_rec.accentPatterns[curtrack],i); }
                             if (IS_BIT_SET64(tempaccent,i)) { SET_BIT64(pattern_rec.accentPatterns[curtrack],pattern_rec.patternLength - i - 1); }
                            else { CLEAR_BIT64(pattern_rec.accentPatterns[curtrack],pattern_rec.patternLength - i - 1); }
                            
                                      //If there is a accent trigger on the loop step, copy the accent trigger to the new step
                            if (IS_BIT_SET64(tempslide,pattern_rec.patternLength - i - 1)) { SET_BIT64(pattern_rec.slidePatterns[curtrack],i); }
                            else { CLEAR_BIT64(pattern_rec.slidePatterns[curtrack],i); }
                             if (IS_BIT_SET64(tempslide,i)) { SET_BIT64(pattern_rec.slidePatterns[curtrack],pattern_rec.patternLength - i - 1); }
                            else { CLEAR_BIT64(pattern_rec.slidePatterns[curtrack],pattern_rec.patternLength - i - 1); }
                                      //If there is a accent trigger on the loop step, copy the accent trigger to the new step
                            if (IS_BIT_SET64(tempswing,pattern_rec.patternLength - i - 1)) { SET_BIT64(pattern_rec.swingPatterns[curtrack],i); }
                            else { CLEAR_BIT64(pattern_rec.swingPatterns[curtrack],i); }
                             if (IS_BIT_SET64(tempswing,i)) { SET_BIT64(pattern_rec.swingPatterns[curtrack],pattern_rec.patternLength - i - 1); }
                            else { CLEAR_BIT64(pattern_rec.swingPatterns[curtrack],pattern_rec.patternLength - i - 1); }
                                      //If there is a accent trigger on the loop step, copy the accent trigger to the new step
                             
                          i++;
                      }
                      
                             //Define sysex encoder objects for the Pattern and Kit
                  ElektronDataToSysexEncoder encoder(&MidiUart);
                
                  setLed();
                  
                  //Send the encoded pattern to the MD via sysex
                  pattern_rec.toSysex(encoder);
                  clearLed();
                  patternswitch = 254;
                  */
         }
                   //         else { GUI.flash_strings_fill("SYSEX", "ERROR");  }
      } 


      //Loop track switch
    else if (patternswitch == 4) {
          //Retrieve the pattern from the Sysex buffer and store it in the pattern_rec object. The MD header is 5 bytes long, hence the offset and length change
         if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {    
                     
/*
                       int curtrack = MD.getCurrentTrack(callback_timeout);
                       int i = encodervalue;
                      
                                                                   
                       if (pattern_rec.patternLength < encodervalue) { encodervalue = pattern_rec.patternLength; }
                       
                           while (i < pattern_rec.patternLength) {
                        //Loop for x amount of steps, where x = encodervalue. Eg loop the first 4 steps or 8 steps
                          for (int n = 0; n < encodervalue; n++) {
             
                            
                            
                     //Clear the locks     
                     
                              clear_step_locks(curtrack,i+n);


                             //Check to see if the current bit is set in the loop
                             if (IS_BIT_SET64(pattern_rec.trigPatterns[curtrack],n)) { 
                             //Set the new step to match the loop step
                             
                             
                                SET_BIT64(pattern_rec.trigPatterns[curtrack],i + n);
                                //Copy the parameter locks of the loop step, to the new step
                                for (int m = 0; m < 24; m++) {
                                           if (IS_BIT_SET32(pattern_rec.lockPatterns[curtrack], m)) {
                                    int8_t idx = pattern_rec.paramLocks[curtrack][m];
	                            int8_t param = m;
	                            int8_t value = pattern_rec.locks[idx][n];
                                   if (value != 254) { pattern_rec.addLock(curtrack, i+n, param, value); }
                                           }
                                 }
                             }  
                             else { CLEAR_BIT64(pattern_rec.trigPatterns[curtrack],i + n); }
                            //If there is a accent trigger on the loop step, copy the accent trigger to the new step
                            if (IS_BIT_SET64(pattern_rec.accentPatterns[curtrack],n)) { SET_BIT64(pattern_rec.accentPatterns[curtrack],i + n); }
                            else { CLEAR_BIT64(pattern_rec.accentPatterns[curtrack],i + n); }
                            //If there is a accent trigger on the loop step, copy the accent trigger to the new step
                             if (IS_BIT_SET64(pattern_rec.swingPatterns[curtrack],n)) { SET_BIT64(pattern_rec.swingPatterns[curtrack],i + n); }
                            else { CLEAR_BIT64(pattern_rec.swingPatterns[curtrack],i + n); }
                             //If there is a slide trigger on the loop step, copy the slide trigger to the new step
                             if (IS_BIT_SET64(pattern_rec.slidePatterns[curtrack],n)) { SET_BIT64(pattern_rec.slidePatterns[curtrack],i + n); }
                            else { CLEAR_BIT64(pattern_rec.slidePatterns[curtrack],i + n); }
                          }
                          i = i + encodervalue;
                      }
                      
                             //Define sysex encoder objects for the Pattern and Kit
                  ElektronDataToSysexEncoder encoder(&MidiUart);
                
                  setLed();
                  
                  //Send the encoded pattern to the MD via sysex
                  pattern_rec.toSysex(encoder);
                  clearLed();
                  */
                  patternswitch = 254;
         }
                         //   else { GUI.flash_strings_fill("SYSEX", "ERROR");  }
      } 



  
  else if (patternswitch == 6) {
      
      /*Retrieve the pattern from the Sysex buffer and store it in the pattern_rec object. The MD header is 5 bytes long, hence the offset and length change*/
         if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {    
           
                  MD.getBlockingKit(pattern_rec.kit);
         }
                       //     else { GUI.flash_strings_fill("SYSEX", "ERROR");  }
         
   }
   else if (patternswitch == 3) {
      
      /*Retrieve the pattern from the Sysex buffer and store it in the pattern_rec object. The MD header is 5 bytes long, hence the offset and length change*/
         if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {      
            
            /*For the 16 tracks in the received pattern, laod them up into Grids in the grid*/
            

            for (int i = 0; i < 16; i++) {
                if ((i + (cur_col + (cur_row * 16))) < (128*16)) {
              
                /*Store the track at the  into Minicommand memory by moving the data from a Pattern object into a Track object*/
                store_track_inGrid(i, i + cur_col, cur_row);

                }
                                /*Update the encoder page to show current Grids*/
                            page.display();
            }
           /*If the pattern can't be retrieved from the sysex data then there's been a problem*/

           
              patternswitch = 254;
        }  
              //     else { GUI.flash_strings_fill("SYSEX", "ERROR");  }
      
    }
/*If patternswitch == 0, the pattern receiveed is for storing track data*/
   else if (patternswitch == 0) {
    
           /*Retrieve the pattern from the Sysex buffer and store it in the pattern_rec object. The MD header is 5 bytes long, hence the offset and length change*/
         if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
            int i;
            bool n;
            /*Send a quick sysex message to get the current selected track of the MD*/       
          int curtrack = MD.getCurrentTrack(callback_timeout);
           //       int curtrack = 0;
            if (store_n_tracks == 254) {
               for (i = 0; i < 16; i++) {
                   if (notes[i] == 3) {
                         n = store_track_inGrid(curtrack + i, cur_col + i, cur_row);
                         //CLEAR_BIT32(notes, i);
                   }
               }
            } 
            else {     
            /*Store the selected track into Minicommand memory by moving the data from a Pattern object into a Track object*/
            for (i = 0; i < store_n_tracks; i++) {

                         n = store_track_inGrid(curtrack + i, cur_col + i, cur_row);
                                                               // bool n = store_track_inGrid(curtrack, encodervalue);
                            
                           
             }
            }
            /*Update the encoder page to show current Grids*/
            page.display();
       
            patternswitch = 254;
          } 
          /*If the pattern can't be retrieved from the sysex data then there's been a problem*/
      //  else { GUI.flash_strings_fill("SYSEX", "ERROR");  }
  
  }
  /*If patternswitch == 1, the pattern receiveed is for sending a track to the MD.*/
     else  if (patternswitch == 1) {
           
            /*Retrieve the pattern from the Sysex buffer and store it in the pattern_rec object. The MD header is 5 bytes long, hence the offset and length change*/
           if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {

                /*Send a quick sysex message to get the current selected track of the MD*/       
                  int curtrack = MD.getCurrentTrack(callback_timeout);

                  /*Write the selected trackinto a Pattern object by moving it from a Track object into a Pattern object
                  The destination track is the currently selected track on the machinedrum.
                  */

                    int i = 0;

                    while ((i < 16) && ((cur_col + i) < 16)) {
                                   
                           if ((notes[i] > 1)) {
                                place_track_inpattern(i, cur_col + i, cur_row);
                               MD.muteTrack(i);
                           }
                      i++;
                    
                  }

   
                  /*Set the pattern position on the MD the pattern is to be written to*/
              
                   pattern_rec.setPosition(writepattern); 
                   
                   
                  /*Define sysex encoder objects for the Pattern and Kit*/
                  ElektronDataToSysexEncoder encoder(&MidiUart);
                  ElektronDataToSysexEncoder encoder2(&MidiUart);
                
                  setLed();
                
  
                  /*Send the encoded pattern to the MD via sysex*/
                  pattern_rec.toSysex(encoder);
     
                  //int temp = MD.getCurrentKit(callback_timeout);
                  
                  /*If kit_sendmode == 1 then we'll be sending the Machine via sysex kit dump. */

                  /*Tell the MD to receive the kit sysexdump in the current kit position*/


                  /* Retrieve the position of the current kit loaded by the MD.
                  Use this position to store the modi
                  */
                  kit_new.origPosition = currentkit_temp;


                  md_setsysex_recpos();
                  /*Send the encoded kit to the MD via sysex*/

                      kit_new.toSysex(encoder2);
                  
                //  pattern_rec.setPosition(pattern_rec.origPosition);
                  
                  


                  /*Instruct the MD to reload the kit, as the kit changes won't update until the kit is reloaded*/
           //           if ((writepattern == -1) || (writepattern == pattern_new.origPosition)) { MD.loadKit(currentkit_temp); }
                   
                  MD.loadKit(pattern_rec.kit);
                  
                  /*kit_sendmode != 1 therefore we are going to send the Machine via Sysex and Midi cc without sending the kit*/
                  for (i=0; i < 16; i++) {
                     MD.muteTrack(i,false);
                  }
                  clearLed();
                  /*All the tracks have been sent so clear the write queue*/

                  patternswitch = 254;
    } 
   // else { GUI.flash_strings_fill("SYSEX", "ERROR"); }
    
       }
        clearLed2();
  } 
  
   
};


/* 
 ================
 function: md_setsysex_recpos();
 ================
 Instructs the MD via sysex to receive kits in their original position and not place them randomly in free Grids
 Note: Does not work. Machinedrum does not respond as defined in Sysex outline.
 
............... */


  void md_setsysex_recpos() {
     MD.sendRequest(0x6b,00000011);
/*
    uint8_t param = 00000011;
    
    MidiUart.putc(0xF0);
    MidiUart.sendRaw(machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr));
    MidiUart.putc(0x6b);
    MidiUart.putc(param);
    MidiUart.putc(0xF7); 
    */
  }
  
  
  
  /* 
 ================
 function: setLevel(int curtrack, int value));
 ================
  Set the Level Parameter of a track on the MD
  Midi-Ctrl framework as of 0018 was missing this method

 Note: Unused function 
 */


void setLevel(int curtrack, int value) {
    uint8_t cc;
    uint8_t channel = curtrack >> 2;
    if (curtrack < 4) { cc = 8 + curtrack; }
    else if (curtrack < 8) { cc = 4 + curtrack; }
    else if (curtrack < 12) { cc = curtrack; }
    else if (curtrack < 16) { cc = curtrack - 4; }

    MidiUart.sendCC(channel + 4, cc , value);
}




/* 
 ================
 function: splashscreen();
 ================
  Displays the fancy splash screen with the MCL Logo on power on.

*/
void splashscreen() {
 
 char str1[50] = "MINICOMMAND LIVE    ";
 char str2[50] = "V2.0       ";
        for (int i = 0; i < 50; i++) {
               char str3[17] = "                ";
               char str4[17] = "                ";
                    for (int x = 0; x < 16; x++)   {
                        if ((50 - i + x) <= 50) { 
                        str3[x] = str1[25 - i + x];
                        str4[x] = str2[25 - i + x];
                   }
                 } 
                 str3[16] = '\0';
                 str4[16] = '\0';
                 
                 LCD.goLine(0);
                 LCD.puts(str3);
                 LCD.goLine(1);
                 LCD.puts(str4);  
      
                 delay(50);

        } 
        
        for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 16; y++) {
   	MD.setStatus(0x22, y);
        }
        }
    MD.setStatus(0x22, 0);  
    GUI.setPage(&page);
}



void encoder_level_handle(Encoder *enc) {
TrackInfoEncoder *mdEnc = (TrackInfoEncoder *)enc;
   for (int i = 0; i < 16; i++) {
       if (notes[i] == 1) {
          setLevel(i,mdEnc->getValue());
       }
   }
}
void encoder_fx_handle(Encoder *enc) {
    GridEncoder *mdEnc = (GridEncoder *)enc;
   
/*Scale delay feedback for safe ranges*/

if (mdEnc->fxparam == MD_ECHO_FB) {
  if (mdEnc->getValue() > 68) { mdEnc->setValue(68); }

}
 MD.sendFXParam(mdEnc->fxparam, mdEnc->getValue(), mdEnc->effect); 

}

/* 
 ================
 function: toggle_fx1();
 ================
 Toggles FX Encoder 1 between Delay Time and Reverb Decay Settings

*/

void toggle_fx1() {
    if (param3.effect == MD_FX_REV) {
      fx_dc = param3.getValue();
      param3.setValue(fx_tm);
      
        param3.effect = MD_FX_ECHO;
  param3.fxparam = MD_ECHO_TIME;
    }
    else {
       fx_tm = param3.getValue();
      param3.setValue(fx_dc);
      param3.effect = MD_FX_REV;
  param3.fxparam = MD_REV_DEC;
    }
}

/* 
 ================
 function: toggle_fx2();
 ================
 Toggles FX Encoder 2 between Delay FB and Reverb Level Settings
*/
void toggle_fx2() {
  
  if (param4.effect == MD_FX_REV) {
    fx_lv = param4.getValue();
    param4.setValue(fx_fb);
  param4.effect = MD_FX_ECHO;
  param4.fxparam = MD_ECHO_FB;
  }

  else {
 fx_fb = param4.getValue();
    param4.setValue(fx_lv);
  param4.effect = MD_FX_REV;
  param4.fxparam = MD_REV_LEV;
  }

  // patternswitch = 7;
   
   // setLed();
     //  int curkit = MD.getCurrentKit(callback_timeout);
     //MD.getBlockingKit(MD.currentKit);
     //  clearLed();
       
}



 
/*
 ================
 function: store_tracks_in_mem(int column, int row, int num);
 ================
 Main function for storing one or more consecutive tracks in to the Grid

*/

void store_tracks_in_mem(int column, int row, int num) {

       cur_col = column;
       cur_row = row;
  
       store_n_tracks = num;
       setLed();
         int curkit = MD.getCurrentKit(callback_timeout);
                     patternswitch = 0;
    //  int curkit = MD.getBlockingStatus(MD_CURRENT_KIT_REQUEST, callback_timeout);


       
                     MD.saveCurrentKit(curkit);
                     MD.getBlockingKit(curkit);
     
       MD.getBlockingPattern(MD.currentPattern);
         
       clearLed(); 
       
}

/* 
 ================
 function: store_pattern_in_mem(int pattern, int column, int row)
 ================
 Main function for storing an entire pattern in to the Grid
*/

void store_pattern_in_mem(int pattern, int column, int row) {
        cur_row = row;
        cur_col = column;
         setLed();
         
         if (pattern != MD.currentPattern) {
                               patternswitch = 6;
                  MD.getBlockingPattern(pattern);

         }
         else {
                    patternswitch = 3;
      int curkit = MD.getCurrentKit(callback_timeout);
       
       MD.saveCurrentKit(curkit);
       MD.getBlockingKit(curkit);
       MD.getBlockingPattern(pattern);
         }
       clearLed();
         
}

/* 
 ================
 function: write_tracks_to_md(int patternx)
 ================
 Main function for writing one or more tracks to the Machinedrum.
 This function is called by the GUI handler, when the encoder buttons are released:
 signalling the MC to write the trackwrite_queue to the MD.
*/

void write_tracks_to_md(int pattern, int column, int row) {



        writepattern = pattern;
        cur_col = column;
        cur_row = row;
                 patternswitch = 1;
         
            //  MD.requestPattern(MD.currentPattern);
        
        /*Request both the Kit and Pattern from the MD for extracing the Pattern and Machine data respectively*/
        /*blocking nethods provided by Wesen kindly prevent the minicommand from doing anything else whilst the precious sysex data is received*/
        setLed();
      
       
    
     //If the user has sleceted a different destination pattern then we'll need to pull data from that pattern/kit instead.

       if (pattern != MD.currentPattern) {

         MD.getBlockingPattern(pattern);
         currentkit_temp = pattern_rec.kit;
         MD.getBlockingKit(currentkit_temp);
         }
       else {
         currentkit_temp = MD.getCurrentKit(callback_timeout);
         MD.saveCurrentKit(currentkit_temp);
         MD.getBlockingKit(currentkit_temp);             
         MD.getBlockingPattern(MD.currentPattern);

       }
        
       clearLed();
        
}



/* 
 ================
 function: toggle_cue(int i) 
 ================
 Toggles the individual bits of the Bitmask "cue1" that symbolises the tracks that have been routed to the Cue Output.
 */ 
 
void toggle_cue(int i) {

  if (IS_BIT_SET32(cue1, i)) { CLEAR_BIT32(cue1, i); MD.setTrackRouting(i,6); }
  else { SET_BIT32(cue1, i); MD.setTrackRouting(i,5); }

}



/* 
 ================
 function: modify_track(int looptrack_) 
 ================

Called by the GUI in order to perform the track modification functions.

Track Modification: 
Loop Track (patternswitch == 4)
Reverse Track (patternswitch == 5)

See onKitMessage and onPatternMessage functions.
 */ 
 

void modify_track(int looptrack_) {

       encodervalue = looptrack_;
       if (looptrack_ != -1) { patternswitch = 4; }
        else { patternswitch = 5; } 
       setLed();
       int curkit = MD.getCurrentKit(callback_timeout);
       MD.saveCurrentKit(curkit);
       MD.getBlockingPattern(MD.currentPattern);
       clearLed();
 
}

void loadtrackinfo_page(uint8_t i) {
      GUI.setPage(&trackinfo_page);
     curpage = 1;

     cur_col = param1.getValue() + i;
     cur_row = param2.getValue(); 
}
void clear_row (int row) {
    for (int x = 0; x < 16; x++) {
      clear_Grid(x + (row * 16));
    }
}
void clear_Grid(int i) {
    temptrack.active = false;
    int32_t offset = (int32_t) i * (int32_t) sizeof(MDTrack);
    file.seek(&offset,FAT_SEEK_SET);
    file.write(( uint8_t*)&(temptrack.active), sizeof(temptrack.active)); 
}

/* 
   ==============
   function: clear_step_locks(int curtrack, int i)
   ==============

   Clears the individual parameter locks of a track: curtrack at step: i 
   
*/
void clear_step_locks(int curtrack, int i) {
                             for (uint8_t p = 0; p < 24; p++) {
	                          int8_t idxn = pattern_rec.getLockIdx(curtrack, p);
	                          if (idxn != -1) { pattern_rec.locks[idxn][i] = 254;  }
	                        }
}


/*
Utility Functions
*/

  void combine_strings(char *newstring, char *string1, char *string2) {
       
    
  uint8_t strlen1 = m_strlen(string1);
  uint8_t strlen2 = m_strlen(string2);
  uint8_t newlength = strlen1 + strlen2;
  
  
  //char *newstring = (char*)malloc(newlength);
  //if (newstring == NULL) { GUI.flash_strings_fill("cannot assign mem","combine string"); return NULL; } 
    
   m_strncpy(newstring,string1,strlen1 - 1);

   
   m_strncpy(&newstring[strlen1 - 1],&string2[0],strlen2);
   
    newstring[newlength - 1] = '\0';

  };

/* 
 ================
 variable: uint8_t charmap[7]
 ================
 Character Map for the LCD Display
 Defines the pixels of one glyph and is used to create the custom || character
*/

 uint8_t charmap[7] = { 10, 10, 10, 10, 10, 10, 10 };

/*
  ================
  function: setup()
  ================
  
  Initialization function for the MCL firmware, run when the Minicommand is powered on.
*/


void setup() {


 LCD.createChar(1,charmap);

 //Enable callbacks, and disable some of the ones we don't want to use.
 
 
 MDTask.setup();
 MDTask.verbose = false;
 MDTask.autoLoadKit = false;
 MDTask.reloadGlobal = false;
   
  GUI.addTask(&MDTask);

 //Create a mdHandler object to handle callbacks. 

  MDHandler2 mdHandler;
  mdHandler.setup();
  


// int temp = MD.getCurrentKit(50);

   //Load the splashscreen
   splashscreen();
 
   //Initialise Track Routing to Default output (1 & 2)  
    for (uint8_t i = 0; i < 16; i++) {
   MD.setTrackRouting(i,6);
 }
   
   set_midinote_totrack_mapping();
 
   //Initalise the  Effects Ecnodres
  param3.handler = encoder_fx_handle;
  param3.effect = MD_FX_ECHO;
  param3.fxparam = MD_ECHO_TIME;
  param4.handler = encoder_fx_handle;
  param4.effect = MD_FX_ECHO;
  param4.fxparam = MD_ECHO_FB;
  
 //proj_param4.handler = encoder_level_handle;
  //Setup Turbo Midi
  TurboMidi.setup();
  //Start the SD Card Initialisation.
  sd_load_init();
  

  
  TrigCaptureClass trigger;

  trigger.setup();
//  
  MidiClock.mode = MidiClock.EXTERNAL;
                                                //      GUI.flash_strings_fill("MIDI CLOCK SRC", "MIDI PORT 2");
  MidiClock.start();
// patternswitch = 7;
  //     int curkit = MD.getCurrentKit(callback_timeout);
 //      MD.getBlockingKit(curkit);

}


/*
GUI Functions & Methods:
*/



/*
  ===================
  function: getPatternMask(int column, int row, uint8_t j, bool load)
  ===================
 
  Return the Sequencer Pattern bit mask from TrackGrid i 
  The Sequencer Pattern bit mask is used to represent the 64 step sequencer trigs
  A 1 represents a trig, a 0 represents an empty step
  
*/

uint64_t getPatternMask(int column, int row, uint8_t j, bool load) {
  
      if (load) {
      if (!load_track(column,row,50)) {
       return 0; 
      }
      }
      
      if (temptrack.active != TRUE) { return (uint64_t) 0; }
      else if (j == 1) { return temptrack.swingPattern; }
      else if (j == 2) { return temptrack.slidePattern; }
      else if (j == 3) { return temptrack.accentPattern; }
      else { return temptrack.trigPattern; }
}



/*
  ===================
  function: getGridModel(int column, int row, bool load)
  ===================
 
  Return the machine model object of the track object located at grid Grid i
  
*/

uint32_t getGridModel(int column, int row, bool load) {
     if ( load == true) {
      if (!load_track(column,row,50)) {
       return NULL; 
      }
     }
  if (temptrack.active != TRUE) {
   return NULL; 
  }
  else { return temptrack.machine.model; }
}

/*
  ===================
  function:  *getTrackKit(int column, int row, bool load)
  ===================
 
 Return a the name of kit associated with the track object located at grid Grid i
  
*/


char *getTrackKit(int column, int row, bool load) {
  if (load) {
  if (!load_track(column,row,50)) {
  return "    ";  
  }
  }
  if (temptrack.active != TRUE) { return "    "; }
  return temptrack.kitName;
}


/*
  ===================
  function:   update_prjpage_char()
  ===================
 
  The following function is responsible for updating the ProjectName based on user input.
  The user rotates encoder 1 to select a letter, encoder two then changes the character.
  The name is stored as a character array (newprj) in which each character can be modified separately.
  
*/

void update_prjpage_char() {
     uint8_t x = 0;
     //Check to see that the character chosen is in the list of allowed characters
                   while ((newprj[proj_param1.cur] != allowedchar[x]) && (x < 38)) {
           
                    x++;
                    }
                   
    //Ensure the encoder does not go out of bounds, by resetting it to a character within the allowed characters list
                  proj_param2.setValue(x);
                  //Update the projectname.
                  proj_param1.old = proj_param1.cur; 
}

//Options Page GUI
void OptionsPage::display() {

               GUI.setLine(GUI.LINE1);
                GUI.put_string_at_fill(0,"Name:"); 
                                GUI.put_string_at(6,&config.project[1]); 
               GUI.setLine(GUI.LINE2);
              
              
              
             if (options_param1.getValue() == 3) { 
               
              
               if (options_param1.hasChanged()) { options_param1.old = options_param1.cur; options_param2.setValue(merge);   }
               GUI.put_string_at_fill(0,"MERGE");
             
                                 if (options_param2.getValue() == 0) {
                                     GUI.put_string_at_fill(10,"OFF");
                                 }
                                 if (options_param2.getValue() == 1) {
                                     GUI.put_string_at_fill(10,"ON");
                                 }
                            if (options_param2.hasChanged()) {
                                                     merge = options_param2.getValue();
                           }
             }
             else if (options_param1.getValue() == 2) { 
                
                              if (options_param1.hasChanged()) { options_param1.old = options_param1.cur; options_param2.setValue(turbo); }
                              GUI.put_string_at_fill(0,"TURBO");
             
                                 if (options_param2.getValue() == 0) {

                                     GUI.put_string_at_fill(10,"OFF");
                                 }
                                 else if (options_param2.getValue() == 1) {
                                     GUI.put_string_at_fill(10,"ON");
                                 }
                              if (options_param2.hasChanged()) {
                                                
                                                     turbo = options_param2.getValue();
                                  }
            
              }
              
              
             else if (options_param1.getValue() == 0) { 
                              GUI.put_string_at_fill(0,"Load Project");
             }
             else if (options_param1.getValue() == 1) { 
                              GUI.put_string_at_fill(0,"New Project");
             }
             
}







void PatternLoadEncoder::displayAt(int encoder_offset) {

               GUI.setLine(GUI.LINE2);
               
              if (curpage == 3) { GUI.put_string_at(12,"Read"); }
              else if (curpage == 4) { GUI.put_string_at(12,"Write"); }


               /*Initialise the string with blank steps*/
               char str[5];
                           MD.getPatternName(encoder_offset,str);          
                 GUI.put_string_at(0,str);

                     //  patternload_param3.setValue(currentkit_temp + 1);
             if (curpage == 4) { GUI.put_string_at(4,"Kit:"); GUI.put_value_at(8,patternload_param3.getValue()); }
             
}

/* Overriding display method for the TrackInfoEncoder
This method is responsible for the display of the TrackInfo page.
The selected track information is displayed in this screen (Line 1).
This includes the kit name and a graphical representation of the Step Sequencer Locks for the track (Line 2).
*/

void TrackInfoEncoder::displayAt(int encoder_offset) {
              if (curpage == 8) {

      GUI.setLine(GUI.LINE1);
                 GUI.put_string_at(0,"Project:");
      GUI.setLine(GUI.LINE2);
      GUI.put_string_at_fill(0, entries[loadproj_param1.getValue()].name);
                }
              
  
             else if (curpage == 7) {
                if (proj_param1.hasChanged()) {
                 update_prjpage_char();

                 }
              //    if ((proj_param2.hasChanged())){
                   newprj[proj_param1.getValue()] = allowedchar[proj_param2.getValue()];
              //  }
                 
                 GUI.setLine(GUI.LINE1);
                 GUI.put_string_at(0,"New Project:");
                 GUI.setLine(GUI.LINE2);
                 GUI.put_string_at(0,&newprj[1]);
              }
              else if (curpage == 5) {
  
               GUI.setLine(GUI.LINE1);
               

               GUI.put_string_at(0,"Track Cues");

               GUI.setLine(GUI.LINE2);


               
               /*Initialise the string with blank steps*/
               char str[17] = "----------------";
               


                
                /*Display 16 track cues on screen, 
                /*For 16 tracks check to see if there is a cue*/
                for (int i = 0; i < 16; i++) {

                                             if  (IS_BIT_SET32(cue1,i)) {
                /*If the bit is set, there is a cue at this position. We'd like to display it as [] on screen*/
                /*Char 219 on the minicommand LCD is a []*/
                                              str[i] = (char) 219;
                                           }
                                        
                          

                 }
                 /*Display the cues*/                       
                 GUI.put_string_at(0,str);
              }
            else {
               GUI.setLine(GUI.LINE1);
               
               /*Display the Kit name associated with selected track on line 1*/
               GUI.put_string_at(0,getTrackKit(cur_col,cur_row,true));
               GUI.put_value_at(8,(1 + trackinfo_param1.getValue()) * 160);

               if (trackinfo_param2.getValue() == 1) { GUI.put_string_at(10," Swing"); }
               else if (trackinfo_param2.getValue() == 2) { GUI.put_string_at(10," Slide"); }
               else if (trackinfo_param2.getValue() == 3) { GUI.put_string_at(10," Accen"); }
               else { GUI.put_string_at(10," Trigs"); }
      
               GUI.setLine(GUI.LINE2);
               /*str is a string used to display 16 steps of the patternmask at a time*/
               /*blank steps sequencer trigs are denoted by a - */
               
               /*Initialise the string with blank steps*/
               char str[17] = "----------------";
               
               /*Get the Pattern bit mask for the selected track*/
                uint64_t patternmask = getPatternMask(cur_col, cur_row ,trackinfo_param2.getValue(), false);
                
                /*Display 16 steps on screen, starting at an offset set by the encoder1 value*/
                /*The encoder offset allows you to scroll through the 4 pages of the 16 step sequencer triggers that make up a 64 step pattern*/
                
                /*For 16 steps check to see if there is a trigger at pattern position i + (encoder_offset * 16) */
                for (int i = 0; i < 16; i++) {
                                           
                       if  (IS_BIT_SET64(patternmask,i + (encoder_offset * 16)) ) {
                /*If the bit is set, there is a trigger at this position. We'd like to display it as [] on screen*/
                /*Char 219 on the minicommand LCD is a []*/
                                              str[i] = (char) 219;
                                           }

                 }
                 /*Display the step sequencer pattern on screen, 16 steps at a time*/                       
                 GUI.put_string_at(0,str);
            }
}


/*
  ===================
  method:  :update(encoder_t *enc)
  ===================
 
Responsible for the display of the Grid/Grid system
This method draws the main Page. 
The minicommand has access to a 16 x 8 grid of Grids. Each Grid can store a captured track from the MD.
The grid is displayed on screen with 4 Grids at a time. When a Grid is occupied by a track it displays the name of the Machine associated with that track, ie TR-BD. 

  
*/


int GridEncoder::update(encoder_t *enc) {
  
  
  /* Set encoder value to it's minimum or maximum depending on the rotation of the encoder*/
  /* Old value is compared to new value to determine which way the encoder is turned*/
  if (BUTTON_DOWN(Buttons.BUTTON1)) {
	int inc = enc->normal + (pressmode ? 0 : (scroll_fastmode ? 4 * enc->button : enc->button));
	cur = limit_value(cur, inc, min, max);
                     if (cur > old) { cur = max; }
                     if (cur < old) { cur = min; }

                     if (cur > old) { cur = max; }
                     if (cur < old) { cur = min; }
                                
                  
           }
           
    /*If shift1 is pressed then increase the encoder value by 4 (Fast encoder movement)*/
        else if (BUTTON_DOWN(Buttons.BUTTON2)) {
	int inc = (enc->normal + (pressmode ? 0 : (scroll_fastmode ? 4 * enc->button : enc->button))) * 4;

	cur = limit_value(cur, inc, min, max);
             }
             
             /*Increase encoder value as usual*/
             else { 
	int inc = enc->normal + (pressmode ? 0 : (scroll_fastmode ? 4 * enc->button : enc->button));
        //int inc = 4 + (pressmode ? 0 : (fastmode ? 5 * enc->button : enc->button));
	cur = limit_value(cur, inc, min, max);

    //   GridEncoder *mdEnc = (GridEncoder *)enc;
    
      // if (mdEnc->effect) { dispeffect = 1; }
        }
              
           
	return cur;
}


/*
  ===================
  method:  GridEncoderPage::display()
  ===================
 
  Display the GRID by drawing each encoder 1 at a time. 
  
  Each encoder has its own display method displayAt(i). This method will draw the correct Grid at encoder position i
  
  
*/
void GridEncoderPage::display() {
   if (BUTTON_DOWN(Buttons.BUTTON3)) {
                   if (param3.hasChanged()) {
                                         toggle_fx1();
              }
                   if (param4.hasChanged()) {
                                         toggle_fx2();
              }
           }
           
  /*For each of the 4 encoder objects, ie 4 Grids to be displayed on screen*/
  for (uint8_t i = 0; i < 4; i++) {
     
     /*Display the encoder, ie the Grid i*/
     encoders[i]->displayAt(i);
     /*Display the scroll animation. (Scroll animation draws a || at every 4 Grids in a row, making it easier to separate and visualise the track Grids)*/
     displayScroll(i); 
     /*value = the grid position of the left most Grid (displayed on screen).*/
     
 
    }
  // int value = encoders[0]->getValue() + (encoders[1]->getValue() * 16);  


    /*If the effect encoders have been changed, then set a switch to indicate that the effects values should be displayed*/
         
              if (param3.hasChanged() || param4.hasChanged() ) {
               dispeffect = 1; 
               
                }
                
    /*If the grid encoders have been changed, set a switch to indicate that the row/col values should be displayed*/
              if (param1.hasChanged() || param2.hasChanged()  )  {
               dispeffect = 0; 
              }


     if (dispeffect == 1) {
    GUI.setLine(GUI.LINE1);
     /*Displays the kit name of the left most Grid on the first line at position 12*/
    if (param3.effect == MD_FX_ECHO) {
        GUI.put_string_at(12, "TM");
    }  
    else { GUI.put_string_at(12, "DC"); }  
    
       if (param4.effect == MD_FX_ECHO) {
        GUI.put_string_at(14, "FB");
    }  
    else { GUI.put_string_at(14, "LV"); } 

     GUI.setLine(GUI.LINE2);
     /*Displays the value of the current Row on the screen.*/
      GUI.put_valuex_at(12,(encoders[2]->getValue()) );
      GUI.put_valuex_at(14,(encoders[3]->getValue()) );
     // mdEnc1->dispnow = 0;
    //  mdEnc2->dispnow = 0;

     }
     else {
           GUI.setLine(GUI.LINE1);
     /*Displays the kit name of the left most Grid on the first line at position 12*/
     
     GUI.put_string_at(12, getTrackKit(encoders[0]->getValue(), encoders[1]->getValue(),true ));
                                        
     GUI.setLine(GUI.LINE2);
     
     /*Displays the value of the current Row on the screen.*/
      GUI.put_value_at(12,(encoders[0]->getValue()) * 10);
     /*Displays the value of the current Column on the screen.*/
      GUI.put_value_at(14,(encoders[1]->getValue()) * 10);
     }
}



/*
  ===================
  method:  GridEncoder::display()
  ===================
 
  Grid encoders display the Machine Name associated with track located at a specific Grid position
  The Grid position is calculated from the current selected Row and Column, as determined by encoder1 and encoder2 values respectively
  4 GridSlots are displayed on screen. Encoders 1,2,3,4 display Grid Slots a,b,c,d respectively. 
  Grid positions a,b,c,d are calculated based on the summation of the current row, column and encoder's position on screen
  
*/



void GridEncoder::displayAt(int i) {
  
   
    
    /*Calculate the position of the Grid to be displayed based on the Current Row, Column and Encoder*/
    //int value = displayx + (displayy * 16) + i;  

    GUI.setLine(GUI.LINE1);    
    
    /*Retrieve the first 2 characters of Maching Name associated with the Track at the current Grid. First obtain the Model object from the Track object, then convert the MachineType into a string*/
    const char *str = getMachineNameShort(getGridModel(param1.getValue() + i, param2.getValue(),true),1);
  
    /*If the MachineName str is Null then it means that Track Grid is empty. Empty track Grids are depicted by -- strings*/
    if (str == NULL) {
          char strn[3] = "--";
          GUI.put_string_at((0 + (i * 3)),strn);
     }
 
     else {  GUI.put_p_string_at((0 + (i * 3)),str); }

    GUI.setLine(GUI.LINE2);    
    str= NULL;
    str = getMachineNameShort(getGridModel( param1.getValue() + i, param2.getValue(),false),2);
  
    /*If the MachineName str is Null then it means that Track Grid is empty. Empty track Grids are depicted by -- strings*/
    if (str == NULL) {
      char strn[3] = "--";
       GUI.put_string_at((0 + (i * 3)),strn);
   }
 
 //  GUI.put_string_at((4 + (i * 3)),getMachineNameShort2(getGridModel(getValue())));
 else {  GUI.put_p_string_at((0 + (i * 3)),str); }
  redisplay = false;
   
}

void set_midinote_totrack_mapping() {
        uint8_t note;
      //  uint8_t track;
        for (uint8_t track_n=0; track_n < 16; track_n++) {
          note=36 + track_n;
         // uint8_t data[] = { 0x5a, (uint8_t)note & 0x7F, (uint8_t)track_n & 0x7F  };
	//  MD.sendSysex(data, countof(data));
          MD.mapMidiNote(note,track_n);
        }

}

void init_notes() {
  for (int i = 0; i < 16; i++) {
     notes[i] = 0;
    // notes_off[i] = 0;
  } 
}

void exploit_on() {
      // MD.getBlockingGlobal(0);
       init_notes();
    
       if (MidiClock.state == 2) {
       div16th_last = MidiClock.div16th_counter;
       noteproceed = 0;
       }
       else {
       noteproceed = 1; 
       }

       notecount = 0;
   //   global_new.baseChannel = 9;
     //  ElektronDataToSysexEncoder encoder(&MidiUart);
    //   global_new.toSysex(encoder);
    //    MD.setTempo(MidiClock.tempo); 
       global_page = 0;
    int flag = 0;
            	uint8_t data[] = { 0x56, (uint8_t)global_page & 0x7F };
    //     if ((MidiClock.state == 2) && (MidiClock.mode == MidiClock.EXTERNAL_UART2)) { 
          if ((MidiClock.state == 2) && (MidiClock.mode == MidiClock.EXTERNAL_UART2)) { flag = 1;}
          
      if (flag == 1) { MidiUart.putc_immediate(MIDI_STOP); }
    //   }

	MD.sendSysex(data, countof(data));
   //     if ((MidiClock.state == 2) &&  (MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
      if (flag == 1) { MidiUart.putc_immediate(MIDI_CONTINUE); }
  //    }
      //  MD.getBlockingStatus(MD_CURRENT_GLOBAL_SLOT_REQUEST,200);
        collect_notes = true;
       
}


void exploit_off() {
           collect_notes = false;
         global_page = 1;
  uint8_t data[] = { 0x56, (uint8_t)global_page & 0x7F };
   //
   global_new.tempo = MidiClock.tempo;
   //   global_new.baseChannel = 3;
   //    ElektronDataToSysexEncoder encoder(&MidiUart);
    //   global_new.toSysex(encoder);
    int flag = 0;
    if ((MidiClock.state == 2) && (MidiClock.mode == MidiClock.EXTERNAL_UART2)) { flag = 1;}
        if (flag == 1) { MidiUart.putc_immediate(MIDI_STOP); }
    //   }
	MD.sendSysex(data, countof(data));
    //    if ((MidiClock.state == 2) && (MidiClock.mode == MidiClock.EXTERNAL_UART2)) { 
        if (flag == 1) { MidiUart.putc_immediate(MIDI_CONTINUE); }
   //   }

                               
}

/*
 ================
 function: bool handleEvent(gui_event_t *evt) 
 ================
 
 GUI Handling.
 
 All button presses, command execution and menu diving is defined in this function.
 
 */
 
bool handleEvent(gui_event_t *evt) {
     // TurboMidi.startTurboMidi();
     
                // MD.sendRequest(0x55,(uint8_t)global_page);
      // MD.loadGlobal(global_page);


     
     if (curpage == 8) {


          if (EVENT_RELEASED(evt, Buttons.ENCODER1) || EVENT_RELEASED(evt, Buttons.ENCODER2) || EVENT_RELEASED(evt, Buttons.ENCODER3) || EVENT_RELEASED(evt, Buttons.ENCODER4)) {
            uint8_t size = m_strlen(entries[loadproj_param1.getValue()].name);        
            if (strcmp(&entries[loadproj_param1.getValue()].name[size - 4],"mcl") == 0)  {
              

            
            char temp[size + 1];
            temp[0] = '/';
            m_strncpy(&temp[1],entries[loadproj_param1.getValue()].name,size);
           

            sd_load_project(temp); 
            GUI.setPage(&page);
            curpage = 0; 
            }
                
         }
              return true;
     }
  else if (curpage == 7) {
     if (EVENT_RELEASED(evt, Buttons.ENCODER1) || EVENT_RELEASED(evt, Buttons.ENCODER2) || EVENT_RELEASED(evt, Buttons.ENCODER3) || EVENT_RELEASED(evt, Buttons.ENCODER4)) {
     LCD.goLine(0);
     LCD.puts("Please Wait");
     LCD.goLine(1);
     LCD.puts("Creating Project");
     
     bool exitcode = sd_new_project(newprj);
     if (exitcode == true) { 
        GUI.setPage(&page);
     curpage = 0; 
     }
     else {
       GUI.flash_strings_fill("SD FAILURE","--");
     //  LCD.goLine(0);
     //LCD.puts("SD Failure");
     }
    
     return true;
     }
  }
  else if (curpage == 1) {
    
    if (EVENT_PRESSED(evt, Buttons.BUTTON3) || EVENT_PRESSED(evt, Buttons.ENCODER1) || EVENT_PRESSED(evt, Buttons.ENCODER2) || EVENT_PRESSED(evt, Buttons.ENCODER3) || EVENT_PRESSED(evt, Buttons.ENCODER4)) {
  GUI.setPage(&page);
     curpage = 0; 
    }
    return true;
  }
  /*end if page == 1*/
  
  //PATTENRN LOAD
  else if (curpage == 3) {
  //  if (EVENT_PRESSED(evt, Buttons.BUTTON3) || EVENT_PRESSED(evt, Buttons.BUTTON4)) {
   //             GUI.setPage(&page);
   //            curpage = 0;
    //           return true;
    //          }
    


       // if (global_page != 1) {
        //       global_page = 1;
         //   }
        //else {
        //       global_page = 0; 
         //   }
      /*
       
          if (EVENT_PRESSED(evt, Buttons.ENCODER1)) {
          
               store_tracks_in_mem(param1.getValue(), param2.getValue(), 1);
                                           GUI.setPage(&page);
                                           curpage = 0;
                                           return true;
           } 
            if (EVENT_PRESSED(evt, Buttons.ENCODER2)) {
               store_tracks_in_mem(param1.getValue(), param2.getValue(), 2);
                                           GUI.setPage(&page);
                                           curpage = 0;
                                           return true;
           } 
            if (EVENT_PRESSED(evt, Buttons.ENCODER3)) {
               store_tracks_in_mem(param1.getValue(),param2.getValue(), 4);
                                           GUI.setPage(&page);
                                           curpage = 0;
                                           return true;
           } 
            if (EVENT_PRESSED(evt, Buttons.ENCODER4)) {
               store_tracks_in_mem(param1.getValue(),param2.getValue(), 8);
                                           GUI.setPage(&page);
                                           curpage = 0;
                                           return true;
           } 
           */
           if (EVENT_RELEASED(evt,Buttons.BUTTON1) || EVENT_RELEASED(evt, Buttons.BUTTON4)) {
               exploit_off();
               GUI.setPage(&page);
               curpage = 0;
           }
           
                      if (EVENT_PRESSED(evt,Buttons.BUTTON3)) {
                               MD.getBlockingKit(currentkit_temp);
              MD.loadKit(currentkit_temp);

                      }
           if (EVENT_RELEASED(evt, Buttons.BUTTON2) ) {
               exploit_off();
               store_pattern_in_mem(patternload_param2.cur + (patternload_param1.cur * 16), param1.getValue(), param2.getValue() );

               GUI.setPage(&page);
               curpage = 0;
           } 
         
         
           //EXIT PATTERN READ PAGE
          // GUI.setPage(&page);
          // curpage = 0;
           return true;
                         
  }
   else if (curpage == 4) {

            if (EVENT_RELEASED(evt, Buttons.BUTTON4) || EVENT_RELEASED(evt, Buttons.BUTTON1)) {

               exploit_off();
               GUI.setPage(&page);
                          curpage = 0;
                         return true;
           }
           if (EVENT_PRESSED(evt, Buttons.BUTTON3)) {
                for (int i = 0; i < 16; i++) { 

                          notes[i] = 3;
                      }
             trackposition = TRUE;
                       //   write_tracks_to_md(-1);
            exploit_off();
            write_tracks_to_md(patternload_param2.cur + (patternload_param1.cur * 16), 0, param2.getValue());

            GUI.setPage(&page);
           curpage = 0;
           return true;
           } 
           
       
                         
  }
   else if (curpage == 6) {
      
      if (!EVENT_RELEASED(evt, Buttons.BUTTON1) && !EVENT_RELEASED(evt, Buttons.BUTTON4) && EVENT_RELEASED(evt, Buttons.ENCODER1)) {
        if (options_param1.getValue() == 0) {
        load_project_page();
        return true;
        }    
   if (options_param1.getValue() == 1) {
        new_project_page();
        return true;
        }         
      
        if (turbo == 0) {
                                                   TurboMidi.stopTurboMidi();
                                                   }
                                                   if (turbo == 1) { 
                                                   if (turbo_state != 1) { 
                                                     bool exitcode = TurboMidi.startTurboMidi();
                                                   if (exitcode) {
                                                   turbo_state = 1;
                                                   }                                                  
                                                   }
                                                   }
                                                   if (merge == 1) {
                                                 //    Merger merger;
                                                 //    merger.setMergeMask(MERGE_CHANPRESS_MASK);
                                                       MidiClock.stop();
                                                             MidiClock.transmit = true;

                                                      MidiClock.mode = MidiClock.EXTERNAL_UART2;
                                                //      GUI.flash_strings_fill("MIDI CLOCK SRC", "MIDI PORT 2");
                                                  MidiClock.start();
                                                   }   
                                                if (merge == 0) {
                                                MidiClock.stop();
                                                }                    
                                                  
                                               GUI.setPage(&page);
                                               curpage = 0;
                                               return true;
                                              
                                               
                                              }
                                              
      
    }
    
 else  if (curpage == 5) {
    
     if (EVENT_PRESSED(evt, Buttons.ENCODER1) && BUTTON_DOWN(Buttons.BUTTON4)) {
            modify_track(8);
           return true;
          }
           if (EVENT_PRESSED(evt, Buttons.ENCODER2) && BUTTON_DOWN(Buttons.BUTTON4)) {
            modify_track(4);
          
           return true;
          }
           if (EVENT_PRESSED(evt, Buttons.ENCODER3) && BUTTON_DOWN(Buttons.BUTTON4)) {
            modify_track(-1);
 
           return true;
          }
         if (EVENT_PRESSED(evt, Buttons.ENCODER4)) {
              int curtrack = MD.getCurrentTrack(callback_timeout);
      
             toggle_cue(curtrack);
             trackinfo_page.display();
            return true;
          }
         if (EVENT_RELEASED(evt, Buttons.BUTTON4)) {
           //          exploit_off();
           GUI.setPage(&page);
           curpage = 0;
           return true;
         }
  }
  
  //HOME SCREEN -- GRID -- DEFAULT
 else if (curpage == 0) {
 
   
   
      if ((EVENT_PRESSED(evt, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) || (EVENT_PRESSED(evt, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON1) ))
       {
         
         
                 
                    GUI.setPage(&options_page);

            curpage = 6;
                   

           return true; 
        }
       
        /*Store pattern in track Grids from offset set by the current left most Grid on screen*/
        //        if ( (EVENT_PRESSED(evt, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON2)) || (EVENT_PRESSED(evt, Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON1) ))
        
        
        //TRACK READ PAGE
        if (EVENT_RELEASED(evt, Buttons.BUTTON1) )
       {
                   	uint8_t data[] = { 0x56, (uint8_t)global_page & 0x7F };


       

                   MD.getCurrentPattern(callback_timeout);
                    patternload_param1.cur = (int) MD.currentPattern / (int) 16;
          
                    patternload_param2.cur = MD.currentPattern - 16 * ((int) MD.currentPattern / (int) 16);
                    
                   exploit_on();


                    curpage = 3;
                   
                     GUI.setPage(&patternload_page);


           return true; 
        }
          
        //TRACK WRITE PAGE
        
         if (EVENT_RELEASED(evt, Buttons.BUTTON4)) { 
         
         
                    MD.getCurrentPattern(callback_timeout);
                    patternload_param1.cur = (int) MD.currentPattern / (int) 16;
          
                    patternload_param2.cur = MD.currentPattern - 16 * ((int) MD.currentPattern / (int) 16);
                    
                    exploit_on();
                    GUI.setPage(&patternload_page);

                   curpage = 4; 
           return true; 
        }
       if (BUTTON_DOWN(Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON3)) {
                          clear_row(param2.getValue());
                         return true; 
        }
                         
                       
                         
                 if (BUTTON_DOWN(Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON4)) {
               setLed();
                  int curtrack = MD.getCurrentTrack(callback_timeout);
       
            param1.cur = curtrack;


       clearLed();
            return true;
          }
        
          
   //      if (EVENT_PRESSED(evt, Buttons.BUTTON3)) { 
         // Track Cue Page commands
    //   exploit_on();
      //   GUI.setPage(&trackinfo_page);
        //           curpage = 5;
            //...
    //        .................................
  
            
        //.............................000    return true; 
         // }
        
        /*IF button1 and encoder buttons are pressed, store current track selected on MD into the corresponding Grid*/
        if (EVENT_PRESSED(evt, Buttons.ENCODER1) && BUTTON_DOWN(Buttons.BUTTON1)) {
           store_tracks_in_mem(param1.getValue(), param2.getValue(),1);
            return true;
          }
   
        if (EVENT_PRESSED(evt, Buttons.ENCODER2) && BUTTON_DOWN(Buttons.BUTTON1))  {
           store_tracks_in_mem(param1.getValue() + 1, param2.getValue(),1);
            return true;
          }
  
        if (EVENT_PRESSED(evt, Buttons.ENCODER3)  && BUTTON_DOWN(Buttons.BUTTON1)) {
            store_tracks_in_mem(param1.getValue() + 2, param2.getValue(),1);
            return true;
          }
          
         if (EVENT_PRESSED(evt, Buttons.ENCODER4) && BUTTON_DOWN(Buttons.BUTTON1)) {
            store_tracks_in_mem(param1.getValue() + 3, param2.getValue(),1);
            return true;
         }
      
      if (BUTTON_DOWN(Buttons.ENCODER1) && BUTTON_DOWN(Buttons.BUTTON3))  {

           loadtrackinfo_page(0);
           
       
    
            return true;
         }
  if (BUTTON_DOWN(Buttons.ENCODER2) && (BUTTON_DOWN(Buttons.BUTTON3)))  {


                loadtrackinfo_page(1);

     return true;
 }
  if (BUTTON_DOWN(Buttons.ENCODER3) && (BUTTON_DOWN(Buttons.BUTTON3)))  {

               loadtrackinfo_page(2);
     
     

     return true;
 }
  if (BUTTON_DOWN(Buttons.ENCODER4) && (BUTTON_DOWN(Buttons.BUTTON3)))  {

               loadtrackinfo_page(3);

     return true;
 }
        
 
}

}

