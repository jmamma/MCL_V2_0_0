/*
  ==============================
  MiniCommand Live Firmware:
  ==============================
  Author: Justin Valer
  Date: July/2016
*/

#include <MD.h>
#include <Scales.h>
#include "GridPage.h"
#include "TrackInfoPage.h"
#include "PatternLoadPage.h"
#include "OptionPage.h"
#include <MidiClockPage.h>
//#include <TurboMidi.hh>
#include <SDCard.h>
#include <string.h>
#include <midi-common.hh>
//#include <SimpleFS.hh>
//#include <TrigCapture.h>
//RELEASE 1 BYTE/STABLE-BETA 1 BYTE /REVISION 2 BYTES
#define VERSION 2003
#define LOCK_AMOUNT 256
#define GRID_LENGTH 130
#define CALLBACK_TIMEOUT 500

#define CUE_PAGE 5
#define MIXER_PAGE 10
#define S_PAGE 3
#define W_PAGE 4
#define SEQ_STEP_PAGE 1
#define SEQ_EUC_PAGE 17
#define SEQ_RLCK_PAGE 13
#define SEQ_RTRK_PAGE 11
#define SEQ_RPTC_PAGE 14
#define SEQ_PARAM_A_PAGE 12
#define SEQ_PARAM_B_PAGE 15
#define SEQ_PTC_PAGE 16

#define EXPLOIT_DELAY_TIME 300
/*MDKit and Pattern objects must be defined outside of Classes and Methods otherwise the Minicommand freezes.*/
MDPattern pattern_rec;
//MDKit MD.kit;

MDGlobal global_one;
MDGlobal global_two;
uint8_t rec_global = 0;
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
uint8_t notecount = 0;
uint8_t firstnote = 0;
uint8_t noteproceed = 0;
uint32_t div16th_last;
//Effects encoder values.
uint8_t fx_dc = 0;
uint8_t fx_fb = 0;
uint8_t fx_lv = 0;
uint8_t fx_tm = 0;

uint8_t level_pressmode = 0;
//GUI switch, used to identify what level of the GUI we are currently in.
uint8_t curpage = 0;
uint8_t turbo_state = 0;

uint8_t write_original = 0;
//ProjectName string

char newprj[18];
char row_name[5] = "    ";
float row_name_offset = 0;
uint8_t write_ready = 0;

uint8_t exploit = 0;
/*A toggle for sending tracks in their original pattern position or not*/
bool trackposition = false;

/*Counter for number of tracks to store*/
uint8_t store_behaviour;
/*500ms callback timeout for getcurrent track, kit etc... */
float frames_fps = 10;
uint16_t frames = 0;
uint16_t frames_startclock;

uint32_t pattern_start_clock32th = 0;

uint32_t note_hold = 0;

/* encodervalue */
/*Temporary register for storeing the value of the encoder position or grid/Grid position for a callback*/
uint8_t load_the_damn_kit = 255;
int encodervalue = NULL;
int cur_col = 0;
int cur_row = 0;
int countx = 0;
bool collect_notes = false;
//Instantiating GUI Objects.
uint8_t dont_interrupt = 0;

uint8_t in_sysex = 0;

uint8_t page_select = 0;

GridEncoder param1(0, 12);
GridEncoder param2(0, 127);
GridEncoder param3(0, 127);
GridEncoder param4(0, 127);

GridEncoderPage page(&param1, &param2, &param3, &param4);

//TrackInfoEncoder trackinfo_param1(0, 3);
// TrackInfoEncoder trackinfo_param2(0, 3);
// TrackInfoEncoder trackinfo_param3(0, 7);
// TrackInfoEncoder trackinfo_param4(0, 7);

TrackInfoEncoder trackinfo_param1(0, 3);
TrackInfoEncoder trackinfo_param2(0, 64);
TrackInfoEncoder trackinfo_param3(0, 10);
TrackInfoEncoder trackinfo_param4(0, 16);

TrackInfoPage trackinfo_page(&trackinfo_param1, &trackinfo_param2, &trackinfo_param3, &trackinfo_param4);

//   TrackInfoEncoder mixer_param1(0, 127);
// TrackInfoEncoder mixer_param2(0, 127);
// TrackInfoEncoder mixer_param3(0, 8);

TrackInfoEncoder mixer_param1(0, 127);
TrackInfoEncoder mixer_param2(0, 127);
TrackInfoEncoder mixer_param3(0, 8);
TrackInfoPage mixer_page(&mixer_param1, &mixer_param2, &mixer_param3);


// PatternLoadEncoder patternload_param1(0, 8);
// PatternLoadEncoder patternload_param2(0, 15);
//PatternLoadEncoder patternload_param3(0, 64);
// PatternLoadEncoder patternload_param4(0, 11);

TrackInfoEncoder patternload_param1(0, 8);
TrackInfoEncoder patternload_param2(0, 15);
TrackInfoEncoder patternload_param3(0, 64);
TrackInfoEncoder patternload_param4(0, 11);
PatternLoadPage patternload_page(&patternload_param1, &patternload_param2, &patternload_param3, &patternload_param4);

//OptionsEncoder options_param1(0, 3);
//OptionsEncoder  options_param2(0, 2);

TrackInfoEncoder options_param1(0, 3);
TrackInfoEncoder  options_param2(0, 2);
OptionsPage options_page(&options_param1, &options_param2);

// TrackInfoEncoder proj_param1(1, 10);
//  TrackInfoEncoder proj_param2(0, 36);
// TrackInfoEncoder proj_param4(0, 127);

TrackInfoEncoder proj_param1(1, 10);
TrackInfoEncoder proj_param2(0, 36);
TrackInfoEncoder proj_param4(0, 127);
TrackInfoPage proj_page(&proj_param1, &proj_param2);

//  TrackInfoEncoder loadproj_param1(1, 64);
TrackInfoEncoder loadproj_param1(1, 64);
TrackInfoPage loadproj_page(&loadproj_param1);

uint8_t PatternLengths[16];
uint8_t PatternLocks[16][4][64];
uint8_t PatternLocksParams[16][4];
uint64_t PatternMasks[16];
uint64_t LockMasks[16];
uint8_t conditional_timing[16][64];

uint8_t euclid_root[16];
uint16_t exploit_start_clock = 0;
//  TrigCaptureClass test

void clear_step_locks(int curtrack, int i) ;
/*A toggle for handling incoming pattern and kit data.*/

#define PATTERN_STORE 0
#define PATTERN_UDEF 254
#define STORE_IN_PLACE 0
#define STORE_AT_SPECIFIC 254
uint8_t patternswitch = PATTERN_UDEF;

/* Gui CallBack flag for writing a complete pattern */
int writepattern;


/* Allowed characters for project naming*/

const char allowedchar[38] = "0123456789abcdefghijklmnopqrstuvwxyz_";

uint8_t last_md_track = 0;

const scale_t *scales[36] { 
 &ionianScale,
&dorianScale,
&phrygianScale,
&lydianScale,
&mixolydianScale,
&aeolianScale,
&locrianScale,
&harmonicMinorScale,
&melodicMinorScale,
&lydianDominantScale,
&wholeToneScale,
&wholeHalfStepScale,
&halfWholeStepScale,
&majorPentatonicScale,
&minorPentatonicScale,
&suspendedPentatonicScale,
&inSenScale,
&bluesScale,
&majorBebopScale,
&dominantBebopScale,
&minorBebopScale,
&majorArp,
&minorArp,
&majorMaj7Arp,
&majorMin7Arp,
&minorMin7Arp,
&minorMaj7Arp,
&majorMaj7Arp9,
&majorMaj7ArpMin9,
&majorMin7Arp9,
&majorMin7ArpMin9,
&minorMin7Arp9,
&minorMin7ArpMin9,
&minorMaj7Arp9,
&minorMaj7ArpMin9
};

/*
   ==============
   class: MDTrack
   ==============

  Class for defining Track Objects. The main data structure for MCL
  The GRID data structure that resides on the SDCard is is made up of a linear array of Track Objects.

  Each Track object holds the complete information set of a Track including it's Machine,
  Machine Settings, Step Sequencer Triggers (trig, accent, slide, swing), Parameter Locks,

*/

class KitExtra {
  public:
    /** The settings of the reverb effect. f**/
    uint8_t reverb[8];
    /** The settings of the delay effect. **/
    uint8_t delay[8];
    /** The settings of the EQ effect. **/
    uint8_t eq[8];
    /** The settings of the compressor effect. **/
    uint8_t dynamics[8];
    uint32_t swingAmount;
    uint8_t accentAmount;
    uint8_t patternLength;
    uint8_t doubleTempo;
    uint8_t scale;
};


class MDTrack {
  public:
    bool active;
    char kitName[17];
    char trackName[17];
    uint8_t origPosition;
    uint8_t patternOrigPosition;
    uint8_t length;
    uint64_t trigPattern;
    uint64_t accentPattern;
    uint64_t slidePattern;
    uint64_t swingPattern;
    //Machine object for Track Machine Type
    MDMachine machine;
    uint8_t SeqPatternLocks[4][64];
    uint8_t SeqPatternLocksParams[4];
    uint64_t SeqPatternMask;
    uint64_t SeqLockMask;
    uint8_t SeqConditional_timing[64];
    int8_t SeqParamLocks[24];
    //Array to hold parameter locks.
    int arraysize;
    KitExtra kitextra;
    uint8_t param_number[LOCK_AMOUNT];
    int8_t value[LOCK_AMOUNT];
    uint8_t step[LOCK_AMOUNT];

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
      length = pattern_rec.patternLength;
      kitextra.swingAmount = pattern_rec.swingAmount;
      kitextra.accentAmount = pattern_rec.accentAmount;
      kitextra.patternLength = pattern_rec.patternLength;
      kitextra.doubleTempo = pattern_rec.doubleTempo;
      kitextra.scale = pattern_rec.scale;

      //Extract parameter lock data and store it in a useable data structure
      int n = 0;
      arraysize = 0;
      for (int i = 0; i < 24; i++) {
        if (IS_BIT_SET32(pattern_rec.lockPatterns[tracknumber], i)) {
          int8_t idx = pattern_rec.paramLocks[tracknumber][i];
          if (idx >= 0) {
            for (int s = 0; s < 64; s++) {

              if ((pattern_rec.locks[idx][s] <= 127) && (pattern_rec.locks[idx][s] >= 0)) {
                if (IS_BIT_SET64(trigPattern, s)) {

                  step[n] = s;
                  param_number[n] = i;
                  value[n] = pattern_rec.locks[idx][s];
                  n++;
                }
              }
            }
          }
        }
      }

      //  itoa(n,&str[2],10);

      arraysize = n;



      /*Don't forget to copy the Machine data as well
        Which is obtained from the received Kit object MD.kit*/
      //  m_strncpy(kitName, MD.kit.name, 17);
      uint8_t white_space = 0;
      for (uint8_t c; c < 17; c++) {
        if (white_space == 0) {
          kitName[c] = MD.kit.name[c];
          trackName[c] = MD.kit.name[c];
        }
        else {
          kitName[c] = ' ';
          trackName[c] = ' ';
        }
        if (MD.kit.name[c] == '\0') {
          white_space = 1;
        }

      }
      m_memcpy(SeqPatternLocks, PatternLocks[tracknumber], sizeof(SeqPatternLocks));
      m_memcpy(SeqPatternLocksParams, PatternLocksParams[tracknumber], sizeof(SeqPatternLocksParams));
      SeqPatternMask = PatternMasks[tracknumber];
      SeqLockMask = LockMasks[tracknumber];
      m_memcpy(SeqConditional_timing, conditional_timing[tracknumber], sizeof(SeqConditional_timing));

      //  trackName[0] = '\0';
      m_memcpy(machine.params, MD.kit.params[tracknumber], 24);

      machine.track = tracknumber;
      machine.level = MD.kit.levels[tracknumber];
      machine.model = MD.kit.models[tracknumber];


      /*Check to see if LFO is modulating host track*/
      /*IF it is then we need to make sure that the LFO destination is updated to the new row posiiton*/

      if (MD.kit.lfos[tracknumber].destinationTrack == tracknumber) {
        MD.kit.lfos[tracknumber].destinationTrack = column;
      }
      /*Copies Lfo data from the kit object into the machine object*/
      m_memcpy(&machine.lfo, &MD.kit.lfos[tracknumber], sizeof(machine.lfo));

      machine.trigGroup = MD.kit.trigGroups[tracknumber];
      machine.muteGroup = MD.kit.muteGroups[tracknumber];

      m_memcpy(&kitextra.reverb, &MD.kit.reverb, sizeof(kitextra.reverb));
      m_memcpy(&kitextra.delay, &MD.kit.delay, sizeof(kitextra.delay));
      m_memcpy(&kitextra.eq, &MD.kit.eq, sizeof(kitextra.eq));
      m_memcpy(&kitextra.dynamics, &MD.kit.dynamics, sizeof(kitextra.dynamics));
      origPosition = MD.kit.origPosition;
      patternOrigPosition = pattern_rec.origPosition;
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
          In this case, we'll need to copy machine model to the kit MD.kit object which will be converted into a sysex message and sent to the MD*/
        /*if kit_sendmode == 0 then we'll load up the machine via sysex and Midi CC messages without sending the kit*/

        m_memcpy(MD.kit.params[tracknumber], machine.params, 24);

        MD.kit.levels[tracknumber] = machine.level;
        MD.kit.models[tracknumber] = machine.model;


        if (machine.lfo.destinationTrack == column) {

          machine.lfo.destinationTrack = tracknumber;

        }

        m_memcpy(&MD.kit.lfos[tracknumber], &machine.lfo, sizeof(machine.lfo));


        MD.kit.trigGroups[tracknumber] = machine.trigGroup;
        MD.kit.muteGroups[tracknumber] = machine.muteGroup;

        m_memcpy(&PatternLocks[tracknumber], SeqPatternLocks , sizeof(SeqPatternLocks));
        m_memcpy(&PatternLocksParams[tracknumber], SeqPatternLocksParams, sizeof(SeqPatternLocksParams));
        PatternMasks[tracknumber] = SeqPatternMask;
        LockMasks[tracknumber] = SeqLockMask;
        m_memcpy(&conditional_timing[tracknumber], SeqConditional_timing, sizeof(SeqConditional_timing));


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
    char project[16];
    uint8_t turbomidi;
    uint8_t merge;
    uint8_t cue_output;
    uint32_t cues;
    uint8_t cur_row;
    uint8_t cur_col;
    uint8_t number_projects;
    uint32_t version;
};
Config config;


class Project {
  public:
    uint32_t version;
    Config config;
    uint8_t reserved[16];
    uint32_t hash;
};
Project project_header;



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
    GUI.flash_strings_fill("SD CARD ERROR", "");
  }

  else {
    if (configfile.open(true)) {
      if (configfile.read(( uint8_t*)&config, sizeof(Config))) {
        configfile.close();
        if (config.project != NULL) {

          if (!sd_load_project(config.project)) {
            new_project_page();
          }
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
  if (config.version != 2000) {
    config.version = 2000;
    config.number_projects = 0;
  }


  char my_string[16] = "/project___.mcl";

  my_string[8] = (config.number_projects % 1000) / 100 + '0';
  my_string[8 + 1] = (config.number_projects % 100) / 10 + '0';
  my_string[8 + 2] = (config.number_projects % 10) + '0';

  m_strncpy(newprj, my_string, 16);
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
void write_project_header() {
  project_header.version = VERSION;
  //  Config config;
  //  uint8_t reserved[16];
  project_header.hash = 0;
  file.seek(0, FAT_SEEK_SET);
  file.write(( uint8_t*)&project_header, sizeof(project_header));
  /* file.write(( uint8_t*)&project_header.version, sizeof(project_header.version));
    file.write(( uint8_t*)&config, sizeof(project_header.config));
    file.write(( uint8_t*)&project_header.reserved, sizeof(project_header.reserved));
    file.write(( uint8_t*)&project_header.hash, sizeof(project_header.hash));*/
}


bool sd_new_project(char *projectname) {


  numProjects++;
  file.close();
  file.setPath(projectname);



  temptrack.active = FALSE;

  file.open(true);

  //Make sure the file is large enough for the entire GRID
  uint8_t exitcode = fat_resize_file(file.fd, (uint32_t) sizeof(project_header) + (uint32_t) sizeof(MDTrack) * (uint32_t) GRID_LENGTH * (uint32_t) 16);
  if (exitcode == 0) {
    file.close();
    return false;
  }

  write_project_header();
  uint8_t ledstatus = 0;
  //Initialise the project file by filling the grid with blank data.
  for (int32_t i = 0; i < GRID_LENGTH * 16; i++) {
    if (i % 25 == 0) {
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
  m_strncpy(config.project, projectname, 16);

  config.number_projects++;
  write_config();
  return true;
}

/*
  ================
  function:  sd_load_project(char *projectname)
  ================

  Loads a project with the filename: *projectname

*/

bool sd_load_project(char *projectname) {

  file.close();
  file.setPath(projectname);
  file.open(true);
  if (!check_project_version()) {
    file.close();
    return false;
  }

  m_strncpy(config.project, projectname, 15);
  write_config();
  return true;

}

bool check_project_version() {
  file.seek(0, FAT_SEEK_SET);
  file.read(( uint8_t*) & (project_header), sizeof(project_header));
  //  file.read(( uint8_t*)&(project_header.config),sizeof(project_header.config));
  if (project_header.version >= VERSION) {
    return true;
  }
  else {
    return false;
  }
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


  int32_t offset = (int32_t) sizeof(Project) + (column + (row * (int32_t)16)) * (int32_t) sizeof(MDTrack);

  int32_t len;

  //sd_buf_readwrite(offset, true);
  file.seek(&offset, FAT_SEEK_SET);
  len = (sizeof(MDTrack) - sizeof(temptrack.kitextra) - (LOCK_AMOUNT * 3));
  //len = (sizeof(MDTrack)  - (LOCK_AMOUNT * 3));
  file.read(( uint8_t*) & (temptrack), len);
  if (m == 0) {


    file.read(( uint8_t*) & (temptrack.kitextra), sizeof(temptrack.kitextra));
    file.read(( uint8_t*) & (temptrack.param_number[0]), temptrack.arraysize);
    file.read(( uint8_t*) & (temptrack.value[0]), temptrack.arraysize);
    file.read(( uint8_t*) & (temptrack.step[0]), temptrack.arraysize);
  }

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
  file.seek(&offset, FAT_SEEK_SET);
  int x = sizeof(MDTrack);
  int size = 200;
  int count = 0;
  while (x > 0) {
    x = x - size;
    if (x < 0) {
      size = size - x;
    }
    if (read == true) {
      size = file.read(( uint8_t*)&temptrack + count, size);
    }
    else {
      size = file.write(( uint8_t*)&temptrack2 + count, size);
    }
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

  temptrack2.storeTrack(track, column);
  //  if (!SDCard.isInit) { setLed(); }

  // SDCard.deleteFile("/valertest.mcl");
  //  MDTrack temp;


  // int32_t offset = (column + (row * (int32_t) 16)) *  (int32_t) sizeof(MDTrack);
  int32_t len;
  int32_t offset = (int32_t) sizeof(Project) + (column + (row * (int32_t) 16)) *  (int32_t) sizeof(MDTrack);
  file.seek(&offset, FAT_SEEK_SET);
  len = sizeof(MDTrack) - (LOCK_AMOUNT * 3);

  file.write(( uint8_t*) & (temptrack2), len);

  file.write(( uint8_t*) & (temptrack2.param_number[0]), temptrack2.arraysize);
  file.write(( uint8_t*) & (temptrack2.value[0]), temptrack2.arraysize);
  file.write(( uint8_t*) & (temptrack2.step[0]), temptrack2.arraysize);
  return true;
}


void draw_levels() {
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  char str[17] = "                ";
  for (int i = 0; i < 16; i++) {
    //  if (MD.kit.levels[i] > 120) { scaled_level = 8; }
    // else if (MD.kit.levels[i] < 4) { scaled_level = 0; }
    scaled_level = (int) (((float) MD.kit.levels[i] / (float) 127) * 7);
    if (scaled_level == 7) {
      str[i] = (char) (255);
    }
    else if (scaled_level > 0) {
      str[i] = (char) (scaled_level + 2);
    }
  }
  GUI.put_string_at(0, str);
}

void draw_notes(uint8_t line_number) {
  if (line_number == 0) {
    GUI.setLine(GUI.LINE1);
  }
  else {
    GUI.setLine(GUI.LINE2);
  }
  /*Initialise the string with blank steps*/
  char str[17] = "----------------";

  /*Display 16 track cues on screen,
    /*For 16 tracks check to see if there is a cue*/
  for (int i = 0; i < 16; i++) {
    if (curpage == CUE_PAGE) {

      if  (IS_BIT_SET32(cue1, i)) {
        str[i] = 'X';
      }
    }
    if (notes[i] > 0)  {
      /*If the bit is set, there is a cue at this position. We'd like to display it as [] on screen*/
      /*Char 219 on the minicommand LCD is a []*/

      str[i] = (char) 219;
    }

  }

  /*Display the cues*/
  GUI.put_string_at(0, str);

}
extern void midi_start() {
  if ((curpage == S_PAGE) || (curpage == W_PAGE) || (curpage == CUE_PAGE) || (curpage == MIXER_PAGE)) {
    exploit_start_clock = read_slowclock();
    noteproceed = 0;
  }
  pattern_start_clock32th = 0;
}
void load_the_damnkit(uint8_t pattern) {
  if (load_the_damn_kit != 255) {
    if (writepattern == pattern) {
      MD.loadKit(load_the_damn_kit);
    }
    load_the_damn_kit = 255;

  }



}

void clear_seq_conditional(uint8_t i) {
  for (uint8_t c = 0; c < 64; c++) {
conditional_timing[i][c] = 0; 
  }
}
void clear_seq_locks(uint8_t i) {
  for (uint8_t c = 0; c < 4; c++) {
    for (uint8_t x = 0; x < 64; x++) {
      PatternLocks[i][c][x] = 0;
    }
    PatternLocksParams[i][c] = 0;
  }
}
void clear_seq_track(uint8_t i) {
  uint8_t c;

  //for (c=0; c < 4; c++) {
  //uint8_t PatternLengths[i] = 16;
  //}
  clear_seq_locks(i);
  clear_seq_conditional(i);

  LockMasks[i] = 0;
  PatternMasks[i] = 0;


}

uint8_t euclid_randomScalePitch(const scale_t *scale, uint8_t octaves, uint8_t offset) {
    uint8_t pitch = scale->pitches[random(scale->size)];
  if (octaves == 0) {
    return pitch + offset;
  } else {
    return pitch + offset;
  }
}


void setEuclid( uint8_t track, uint8_t pulses, uint8_t len, uint8_t offset, uint8_t scale, uint8_t root) {
  clear_seq_locks(track);
  uint8_t octave = 4;
  PatternMasks[track] = 0;
  if (pulses == 0)
    return;

  uint8_t cnt = len;
  for (uint8_t i = 0; i < len; i++) {
    if (i < (len - 1)) {
            PatternMasks[track] <<= 1;
        }   
    
    if (cnt >= len) {
      SET_BIT64(PatternMasks[track], 0); 
      cnt -= len;
    }   
    cnt += pulses;
  }
  PatternMasks[track] <<= offset;

  if (scale > 0) {

  PatternLocksParams[track][0] = 1;
      for (uint8_t x=0; x < PatternLengths[track]; x++) {
      if (IS_BIT_SET64(PatternMasks[track], x )) {
        
      
      SET_BIT64(LockMasks[track], x );
      PatternLocks[track][0][x] = euclid_randomScalePitch(scales[scale], octave, root); ;
      }
      }
  }
}
void random_pattern(uint8_t pulses, uint8_t len, uint8_t offset, uint8_t scale, uint8_t root) {

  for (uint8_t i=0; i< 16; i++) {
      setEuclid(i, random(0,random(0,pulses)), len, random(0,offset),scale,root);
       
      }

  
  
}
void parameter_locks(uint8_t i, uint8_t step_count) {
  uint8_t c;
  if (IS_BIT_SET64(LockMasks[i], step_count )) {
    for (c = 0; c < 4; c++) {
            if (PatternLocks[i][c][step_count] > 0) {setTrackParam(i, PatternLocksParams[i][c] - 1, PatternLocks[i][c][step_count] - 1); }

    }
  }
  else if (IS_BIT_SET64(PatternMasks[i], step_count )) {

    for (c = 0; c < 4; c++) {

          setTrackParam(i, PatternLocksParams[i][c] - 1, MD.kit.params[i][PatternLocksParams[i][c] - 1]);

    }
  
  }
}
void trig_conditional(uint8_t condition, uint8_t i) {
  if ((condition == 0)) {
    MD.triggerTrack(i, 127);
  }

  else if (condition <= 8) {
    if (((MidiClock.div16th_counter - pattern_start_clock32th / 2 + PatternLengths[i]) / PatternLengths[i]) % ( (condition)) == 0) {
      MD.triggerTrack(i, 127);
    }
  }
  else if ((condition == 9) && (random(100) <= 10)) {
    MD.triggerTrack(i, 127);
  }
  else if ((condition == 10) && (random(100) <= 25)) {
    MD.triggerTrack(i, 127);
  }
  else if ((condition == 11) && (random(100) <= 50)) {
    MD.triggerTrack(i, 127);
  }
  else if ((condition == 12) && (random(100) <= 75)) {
    MD.triggerTrack(i, 127);
  }
  else if ((condition == 13) && (random(100) <= 90)) {
    MD.triggerTrack(i, 127);
  }
}
class MDSequencer : public ClockCallback {
  public:

    void setup() {
      for (uint8_t i = 0; i < 16; i++) {
          PatternLengths[i] = 16;
       }
      MidiClock.addOn96Callback(this, (midi_clock_callback_ptr_t)&MDSequencer::MDSequencerCallback);
    };
    void MDSequencerCallback() {
      //   setTrackParam(1,0,random(127));
      if (in_sysex == 1) {
        return;
      }
      for (uint8_t i = 0; i < 16; i++) {
        //if (MidiClock.mod6_counter == 0) { MD.triggerTrack(i, 127); }
        uint8_t step_count = (MidiClock.div16th_counter - pattern_start_clock32th / 2) - (PatternLengths[i] * ((MidiClock.div16th_counter - pattern_start_clock32th / 2) / PatternLengths[i]));

        int8_t timing = conditional_timing[i][step_count] >> 4; //upper
        uint8_t condition = conditional_timing[i][step_count] & 0x0F; //lower

        int8_t timing_next = conditional_timing[i][step_count + 1] >> 4; //upper
        uint8_t condition_next = conditional_timing[i][step_count + 1] & 0x0F; //lower

        //-5 -4 -3 -2 -1  0  1 2 3 4 5
        //   0 1  2  3  4  5  6  7 8 9 10 11
        ///  0  1  2  3  4  5  0  1 2 3 4 5


        if ((timing >= 6) && (timing - 6 == (int8_t)MidiClock.mod6_counter)) {

          parameter_locks(i, step_count);

          if (IS_BIT_SET64(PatternMasks[i], step_count )) {
            trig_conditional(condition, i);
          }

        }

        if ((timing_next < 6) && ((timing_next) == (int8_t) MidiClock.mod6_counter)) {

          parameter_locks(i, step_count + 1);

          if (IS_BIT_SET64(PatternMasks[i], step_count + 1)) {
            trig_conditional(condition_next, i);
          }

        }
      }
      // for (uint8_t i = 0; i < 16; i++) {

      //  }

    }
};
uint8_t globalbasechannel_to_channel(uint8_t b) {
  // -- 0
  switch (b) {
    case 0 : return 0;
    case 1 : return 1;
    case 2 : return 5;
    case 3 : return 9;
    case 4 : return 13;
  }
}
void encoder_param2_handle(Encoder *enc) {
  // for (int i = 0; i < 16; i++) {
  //  PatternMasks[i] = getPatternMask(i, enc->getValue(), 3, true);
  //  PatternLengths[i] = temptrack.length;
  // }
}
class TrigCaptureClass : public MidiCallback {


  public:

    void setup() {

      Midi.addOnProgramChangeCallback(this, (midi_callback_ptr_t)&TrigCaptureClass::onProgramChangeCallback);
      Midi.addOnNoteOnCallback(this, (midi_callback_ptr_t)&TrigCaptureClass::onNoteOnCallback);
      Midi.addOnNoteOffCallback(this, (midi_callback_ptr_t)&TrigCaptureClass::onNoteOffCallback);
      Midi.addOnControlChangeCallback(this, (midi_callback_ptr_t)&TrigCaptureClass::onControlChangeCallback);

    };
    void onControlChangeCallback(uint8_t *msg) {
      uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
      uint8_t param = msg[1];
      uint8_t value = msg[2];
      uint8_t track;
      uint8_t track_param;
      uint8_t param_true = 0;
      if (param >= 16) {
        param_true = 1;
      }
      if (param < 63) {
        param = param - 16;
        track = (param / 24) + (channel - MD.global.baseChannel) * 4;
        track_param = param - ((param / 24) * 24);
      }
      else if (param >= 72) {
        param = param - 72;
        track = (param / 24) + 2 + (channel - MD.global.baseChannel) * 4;
        track_param = param - ((param / 24) * 24);
      }

      if (param_true) {

        MD.kit.params[track][track_param] = value;
      }

      if ((curpage == S_PAGE) || (curpage == W_PAGE) || (curpage == CUE_PAGE) || (curpage == MIXER_PAGE)) {
        exploit_off();
        GUI.setPage(&page);
        curpage = 0;

      }
      if ((curpage == SEQ_PARAM_A_PAGE) || (curpage == SEQ_PARAM_B_PAGE)) {
        exploit_off();
        GUI.setPage(&page);
        curpage = 0;
        //   if (!IS_BIT_SET64(LockMasks[last_md_track], (note_num + (0 * 16)))) {
        //           SET_BIT64(LockMasks[last_md_track], (note_num + (0 * 16)) );
        //        }
        //        else {
        //          CLEAR_BIT64(LockMasks[last_md_track], (note_num + (0 * 16)));
        //        }
        //       trackinfo_param1.cur = PatternLocksParams[last_md_track][0];
        //      trackinfo_param3.cur = PatternLocksParams[last_md_track][1];

        //     trackinfo_param2.cur = PatternLocks[last_md_track][0][(note_num + (0 * 16))];
        //     trackinfo_param4.cur = PatternLocks[last_md_track][1][(note_num + (0 * 16))];

      }
      if ((curpage == SEQ_RLCK_PAGE) && (param_true == 1)) {

        //  uint8_t timing = MidiClock.mod6_counter + 6 << 4;
        //  uint8_t condition = 0 & 0x0F;
        //  timing = 3;
        //condition = 3;

        uint8_t step_count = (MidiClock.div16th_counter - pattern_start_clock32th / 2) - (PatternLengths[track] * ((MidiClock.div16th_counter - pattern_start_clock32th / 2) / PatternLengths[track]));



        //Decoding parameter informationc
        //16 tracks split into 4 groups.
        //Each group is 32 parameters
        //First 8 parameters are reserved for Levels and mutes
        //Group 1: MIDI base channel + 0:
        //Group2: MIDI base channel + 1:
        //Group3: MIDI base channel + 2:
        //Group4: MIDI base channel + 3:
        //To convert
        // group = (param/32)
        //
        cur_col = track;
        trackinfo_param3.cur = PatternLengths[cur_col];
        uint8_t match = 255;
        uint8_t c = 0;
        //Let's try and find an existing param
        for (c = 0; c < 4 && match == 255; c++)  {
          if (PatternLocksParams[track][c] == (track_param + 1)) {
            match = c;
          }
        }
        //  PatternLocksParams[track][0] = track_param + 1;
        //PatternLocksParams[track][0] = track_param + 1;
        //match = 0;
        //We learn first 4 params then stop.
        for (c = 0; c < 4 && match == 255; c++) {
          if (PatternLocksParams[track][c] == 0) {
            PatternLocksParams[track][c] = track_param + 1;
            match = c;
          }
        }
        if (match != 254) {
          PatternLocks[track][match][step_count] = value;
        }
        if (MidiClock.state == 2) {
          SET_BIT64(LockMasks[track], step_count);
        }
      }
    }
    void onProgramChangeCallback(uint8_t *msg) {
      load_the_damnkit(msg[1]);
      pattern_start_clock32th = MidiClock.div32th_counter;
      //
      //((int) (MidiClock.div32th_counter / 32) * 32);
    }
    void onNoteOffCallback(uint8_t *msg) {
      uint16_t current_clock = read_slowclock();


      uint8_t note_num;
      for (uint8_t i = 0; i < sizeof(MD.global.drumMapping); i++) {
        if (msg[1] == MD.global.drumMapping[i])
          note_num = i;
      }



      // if (msg[2] == 0) {

      if ((collect_notes) && (msg[0] == 153) && (noteproceed == 1))  {

        if (note_num < 16) {

          if (notes[note_num] == 1)  {
            if ((curpage == CUE_PAGE) && (trackinfo_param4.getValue() == 0)) {
              notes[note_num] = 0;
            }
            else if (curpage == SEQ_STEP_PAGE) {
              notes[note_num] = 0;
              uint8_t timing = (trackinfo_param2.cur + 0) << 4;
              uint8_t condition = trackinfo_param1.cur & 0x0F;

              //  timing = 3;
              //condition = 3;
              conditional_timing[cur_col][(note_num + (page_select * 16))] = timing | condition; //upper
              //   conditional_timing[cur_col][(note_num + (trackinfo_param1.cur * 16))] = condition; //lower

              if (!IS_BIT_SET64(PatternMasks[cur_col], (note_num + (page_select * 16)))) {
                SET_BIT64(PatternMasks[cur_col], (note_num + (page_select * 16)) );
              }
              else {
                if ((current_clock - note_hold) < 300) {
                  CLEAR_BIT64(PatternMasks[cur_col], (note_num + (page_select * 16)));
                }
              }
              //Cond
              //trackinfo_param3.cur = condition;
              //Microƒ
              // trackinfo_param4.cur = timing;
              //draw_notes(1);
              return;
            }
            else if ((curpage == SEQ_PARAM_A_PAGE) || (curpage == SEQ_PARAM_B_PAGE)) {

              int8_t timing = conditional_timing[cur_col][(note_num + (page_select * 16))] >> 4; //upper
              uint8_t condition = conditional_timing[cur_col][(note_num + (page_select * 16))] & 0x0F; //lower

              //Fudge timing info if it's not there
              if ( timing == 0 ) {
                timing = 6 << 4;
                conditional_timing[last_md_track][(note_num + (page_select * 16))] = timing | condition;
              }
              if (IS_BIT_SET64(LockMasks[last_md_track], (note_num + (page_select * 16)))) {
                if ((current_clock - note_hold) < 300) {
                  CLEAR_BIT64(LockMasks[last_md_track], (note_num + (page_select * 16)));
                }
              }
              else {
                SET_BIT64(LockMasks[last_md_track], (note_num + (page_select * 16)) );
              }

              notes[note_num] = 0;
              uint8_t param_offset;
              if (curpage == SEQ_PARAM_A_PAGE) {
                param_offset = 0;
              }
              else {
                param_offset = 2;
              }

              PatternLocks[last_md_track][param_offset][(note_num + (page_select * 16))] = trackinfo_param2.cur;
              PatternLocks[last_md_track][param_offset + 1][(note_num + (page_select * 16))] = trackinfo_param4.cur;

              PatternLocksParams[last_md_track][param_offset] = trackinfo_param1.cur;
              PatternLocksParams[last_md_track][param_offset + 1] = trackinfo_param3.cur;
            }
            else if ((curpage == MIXER_PAGE) || (curpage == SEQ_RPTC_PAGE) || (curpage == SEQ_PTC_PAGE)) {
              notes[note_num] = 0;
              draw_notes(0);
              return;
            }
            else {
              notes[note_num] = 3;
            }
          }
        }
        //If we're on track read/write page then check to see

        if ((curpage == S_PAGE) || (curpage == W_PAGE) || (curpage == CUE_PAGE)) {
          int i;
          draw_notes(0);
          uint8_t all_notes_off = 0;
          uint8_t a = 0;
          uint8_t b = 0;
          for (i = 0; i < 16; i++) {
            if (notes[i] == 1) {
              a++;
            }
            if (notes[i] == 3) {
              b++;
            }
          }

          if ((a == 0) && (b > 0)) {
            all_notes_off = 1;
          }

          if (all_notes_off == 1) {
            if (curpage == S_PAGE)  {
              exploit_off();
              store_tracks_in_mem( 0, param2.getValue(), STORE_IN_PLACE);
              GUI.setPage(&page);
              curpage = 0;
            }
            if (curpage == W_PAGE) {
              exploit_off();
              write_tracks_to_md( 0, param2.getValue(), 0);
              GUI.setPage(&page);
              curpage = 0;
            }
            if ((curpage == CUE_PAGE) && (trackinfo_param4.getValue() > 0) && (b > 1)) {
              toggle_cues_batch();
              send_globals();
              exploit_off();
              GUI.setPage(&page);
              curpage = 0;
            }
          }

        }
        // notes[note_num] = 0;
      }
      // removeNote(msg[1]);
      //   clearLed();
      //  }

    }

    void onNoteOnCallback(uint8_t *msg) {
      //   if (msg[2] > 0) {
      uint16_t current_clock = read_slowclock();

      int note_num;

      if ((curpage == SEQ_RTRK_PAGE) && (msg[0] == 153)) {

        if (clock_diff(exploit_start_clock, current_clock) > EXPLOIT_DELAY_TIME) {
          noteproceed = 1;
        }

        if (noteproceed == 1) {
          for (uint8_t i = 0; i < sizeof(MD.global.drumMapping); i++) {
            if (msg[1] == MD.global.drumMapping[i])
              note_num = i;
          }
          uint8_t step_count = (MidiClock.div16th_counter - pattern_start_clock32th / 2) - (PatternLengths[note_num] * ((MidiClock.div16th_counter - pattern_start_clock32th / 2) / PatternLengths[note_num]));

          MD.triggerTrack(note_num, 127);

          uint8_t timing = MidiClock.mod6_counter + 6 << 4;
          uint8_t condition = 0 & 0x0F;
          cur_col = note_num;
          trackinfo_param3.cur = PatternLengths[cur_col];
          //  timing = 3;
          //condition = 3;
          if (MidiClock.state != 2) {
            return;
          }

          SET_BIT64(PatternMasks[note_num], step_count);
          conditional_timing[note_num][step_count] = timing | condition;
        }
      }
      else if ((curpage == SEQ_RPTC_PAGE) || (curpage == SEQ_PTC_PAGE) ) {

        if (clock_diff(exploit_start_clock, current_clock) > EXPLOIT_DELAY_TIME) {
          noteproceed = 1;
        }
        if ((noteproceed = 1) && (msg[0] == 153)) {
          for (uint8_t i = 0; i < sizeof(MD.global.drumMapping); i++) {
            if (msg[1] == MD.global.drumMapping[i])
              note_num = i;
          }
          uint8_t realPitch;
          if (notes[note_num] == 0) {
            notes[note_num] = 1;
          }

          tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);
                    
          uint8_t pitch = trackinfo_param1.getValue() * 12 + note_num + trackinfo_param2.getValue();
          
          if (trackinfo_param4.cur > 0) {
          uint8_t size = scales[trackinfo_param4.cur]->size;
         // if (note_num >= size) { 
            uint8_t m = note_num / size;
            
            note_num = note_num - (scales[trackinfo_param4.cur]->size * m); 
         //   }
          note_num = scales[trackinfo_param4.cur]->pitches[note_num];
       //   pitch = (trackinfo_param1.getValue() * 12) + trackinfo_param2.getValue() + note_num;
          pitch = (trackinfo_param1.getValue() * 12) + trackinfo_param2.getValue() + (m * 12) + note_num;

          }
          if (tuning == NULL) {
            return;
          }
          trackinfo_param3.cur = PatternLengths[cur_col];

          if (pitch >= tuning->len) {
            pitch = tuning->len - 1;
          }

          realPitch = pgm_read_byte(&tuning->tuning[pitch]);
        //            setTrackParam(last_md_track, 1, 0);

          setTrackParam(last_md_track, 0, realPitch);
//delay(25);

      //              setTrackParam(last_md_track, 1, MD.kit.params[last_md_track][1]);
          MD.triggerTrack(last_md_track, 127);


          if ((MidiClock.state != 2) || (curpage == SEQ_PTC_PAGE)) {
            return;
          }

          uint8_t step_count = (MidiClock.div16th_counter - pattern_start_clock32th / 2) - (PatternLengths[last_md_track] * ((MidiClock.div16th_counter - pattern_start_clock32th / 2) / PatternLengths[last_md_track]));


          uint8_t timing = MidiClock.mod6_counter + 6 << 4;
          uint8_t condition = 0 & 0x0F;
          //  cur_col = note_num;
          //  timing = 3;
          //condition = 3;
          SET_BIT64(PatternMasks[last_md_track], step_count);
          conditional_timing[last_md_track][step_count] = timing | condition;

          uint8_t match = 255;
          uint8_t c = 0;
          //Let's try and find an existing param
          for (c = 0; c < 4 && match == 255; c++)  {
            if (PatternLocksParams[last_md_track][c] == 1) {
              match = c;
            }
          }
          //We learn first 4 params then stop.
          for (c = 0; c < 4 && match == 255; c++) {
            if (PatternLocksParams[last_md_track][c] == 0) {
              PatternLocksParams[last_md_track][c] = 1;
              match = c;
            }
          }
          if (match != 255) {
            PatternLocks[last_md_track][match][step_count] = realPitch + 1;
            SET_BIT64(LockMasks[last_md_track], step_count);
          }


        }
      }
      else {
        if ((noteproceed == 0) && (curpage > 0)) {
          uint16_t current_clock = read_slowclock();

          //We need to wait 500ms for the exploit to take effect before collecting notes
          if (clock_diff(exploit_start_clock, current_clock) > EXPLOIT_DELAY_TIME) {
            noteproceed = 1;
          }
        }
        // if (MidiClock.div16th_counter >= (div16th_last + 4)) {
        //  noteproceed = 1;
        // }

        if (noteproceed == 1) {
          if ((collect_notes) && (msg[0] == 153)) {
            for (uint8_t i = 0; i < sizeof(MD.global.drumMapping); i++) {
              if (msg[1] == MD.global.drumMapping[i])
                note_num = i;
            }

            if (note_num < 16) {

              if (notes[note_num] == 0) {
                notes[note_num] = 1;
              }

              if ((curpage == CUE_PAGE) && (trackinfo_param4.getValue() == 0)) {
                toggle_cue(note_num); send_globals();
              }
              if (curpage == SEQ_STEP_PAGE) {
                note_hold = current_clock;
                int8_t timing = conditional_timing[cur_col][(note_num + (page_select * 16))] >> 4; //upper
                uint8_t condition = conditional_timing[cur_col][(note_num + (page_select * 16))] & 0x0F; //lower


                //Cond
                trackinfo_param1.cur = condition;
                //Micro
                if (timing == 0) {
                  timing = 6;
                }
                trackinfo_param2.cur = timing;
              }

              else if ((curpage == SEQ_PARAM_A_PAGE) || (curpage == SEQ_PARAM_B_PAGE)) {
                note_hold = current_clock;
                uint8_t param_offset;
                if (curpage == SEQ_PARAM_A_PAGE) {
                  param_offset = 0;
                }
                else {
                  param_offset = 2;
                }
                trackinfo_param1.cur = PatternLocksParams[last_md_track][param_offset];
                trackinfo_param3.cur = PatternLocksParams[last_md_track][param_offset + 1];

                trackinfo_param2.cur = PatternLocks[last_md_track][param_offset][(note_num + (page_select * 16))];
                trackinfo_param4.cur = PatternLocks[last_md_track][param_offset + 1][(note_num + (page_select * 16))];
                notes[note_num] = 1;


              }
              else {
                draw_notes(0);
              }
            }


          }

        }
      }
      //}
    }

};

/*For a specific Track located in Grid curtrack, store it in a pattern to be sent via sysex*/

void place_track_inpattern(int curtrack, int column, int row) {
  //       if (Grids[encodervaluer] != NULL) {
  if (load_track(column, row)) {
    temptrack.placeTrack(curtrack, column);
  }
  //        }
}

/*
  ===================
  class: MDHandler2
  ===================


  Class for handling Callbacks
  This class defines CallBack methods for various Machinedrum and Sysex
  callbacks.

*/
void switchGlobal (uint8_t global_page) {
  uint8_t data[] = { 0x56, (uint8_t)global_page & 0x7F };
  MD.sendSysex(data, countof(data));
}
class MDHandler2 : public MDCallback {

  public:

    /*Tell the MIDI-CTRL framework to execute the following methods when callbacks for
      Pattern and Kit messages are received.*/
    void setup() {
      MDSysexListener.setup();
      MDSysexListener.addOnStatusResponseCallback(this, (md_status_callback_ptr_t)&MDHandler2::onStatusResponseCallback);
      MDSysexListener.addOnPatternMessageCallback(this, (md_callback_ptr_t)&MDHandler2::onPatternMessage);
      MDSysexListener.addOnKitMessageCallback(this, (md_callback_ptr_t)&MDHandler2::onKitMessage);
      MDSysexListener.addOnGlobalMessageCallback(this, (md_callback_ptr_t)&MDHandler2::onGlobalMessage);

    }

    void onStatusResponseCallback(uint8_t type, uint8_t value) {
      switch (type) {
        case MD_CURRENT_KIT_REQUEST:
          MD.currentKit = value;

        case MD_CURRENT_GLOBAL_SLOT_REQUEST:
          MD.currentGlobal = value;

          break;

        case MD_CURRENT_PATTERN_REQUEST:
          MD.currentPattern = value;
          break;
         
      }
    }

    void onGlobalMessage() {

      if (!global_one.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
        GUI.flash_strings_fill("GLOBAL", "ERROR");
        return;
      }
      if (rec_global != 1) {

        rec_global = 1;
        send_globals();

        switchGlobal(7);
        uint8_t curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);
        for (uint8_t x = 0; x < 2; x++) {
          for (uint8_t y = 0; y < 16; y++) {
            MD.setStatus(0x22, y);

          }
        }          

        MD.setStatus(0x22, curtrack);
        delay(500);
        turboSetSpeed(4);

      }

    }
    /*A kit has been received by the Minicommand in the form of a Sysex message which is residing
      in memory*/

    void onKitMessage() {
      setLed2();
      /*If patternswitch == PATTERN_STORE then the Kit request is for the purpose of obtaining track data*/
      if (!MD.kit.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
        return;
      }

      if (patternswitch == PATTERN_STORE) {
        int i;
        bool n;
        /*Send a quick sysex message to get the current selected track of the MD*/

        //       int curtrack = 0;
        uint8_t first_note = 254;

        int curtrack = 0;
        if (store_behaviour == STORE_AT_SPECIFIC) {
          curtrack = last_md_track;
          //MD.getCurrentTrack(CALLBACK_TIMEOUT);
        }
        for (i = 0; i < 16; i++) {
          if (notes[i] == 3) {
            if (first_note == 254) {
              first_note = i;
            }
            if (store_behaviour == STORE_IN_PLACE) {
              n = store_track_inGrid(i, i, cur_row);
            }
            if (store_behaviour == STORE_AT_SPECIFIC) {
              n = store_track_inGrid(curtrack + (i - first_note), i, cur_row);
            }
            //CLEAR_BIT32(notes, i);
          }
        }


        /*Update the encoder page to show current Grids*/
        page.display();
      }


      /*Patternswitch == 6, store pattern in memory*/
      /*load up tracks and kit from a pattern that is different from the one currently loaded*/
      if (patternswitch == 6) {
        //   if (MD.kit.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {

        for (int i = 0; i < 16; i++) {
          if ((i + cur_col + (cur_row * 16)) < (128 * 16)) {

            /*Store the track at the  into Minicommand memory by moving the data from a Pattern object into a Track object*/
            store_track_inGrid(i, i, cur_row);

          }
          /*Update the encoder page to show current Grids*/
          page.display();
        }
        /*If the pattern can't be retrieved from the sysex data then there's been a problem*/


        patternswitch = PATTERN_UDEF;

        //  }
      }

      if (patternswitch == 7) {
        //  if (MD.kit.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
        if (param3.effect == MD_FX_ECHO) {
          param3.setValue(MD.kit.delay[param3.fxparam]) ;
        }
        else {
          param3.setValue(MD.kit.reverb[param3.fxparam]);
        }
        if (param4.effect == MD_FX_ECHO) {
          param4.setValue(MD.kit.delay[param4.fxparam]) ;
        }
        else {
          param4.setValue(MD.kit.reverb[param4.fxparam]);
        }
        //   }
        patternswitch = PATTERN_UDEF;
      }
      clearLed2();
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

        /*
                 if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {

                  //Get current track number from MD
                               int curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);
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
                          patternswitch = PATTERN_UDEF;

                 }
        */
        //         else { GUI.flash_strings_fill("SYSEX", "ERROR");  }
      }



      //Loop track switch
      else if (patternswitch == 4) {
        //Retrieve the pattern from the Sysex buffer and store it in the pattern_rec object. The MD header is 5 bytes long, hence the offset and length change
        if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {

          /*
                                 int curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);
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
          patternswitch = PATTERN_UDEF;
        }
        //   else { GUI.flash_strings_fill("SYSEX", "ERROR");  }
      }

      /*If patternswitch == PATTERN_STORE, the pattern receiveed is for storing track data*/
      else if (patternswitch == PATTERN_STORE) {

        /*Retrieve the pattern from the Sysex buffer and store it in the pattern_rec object. The MD header is 5 bytes long, hence the offset and length change*/
        if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {


          //patternswitch = PATTERN_UDEF;
        }
        /*If the pattern can't be retrieved from the sysex data then there's been a problem*/
        //  else { GUI.flash_strings_fill("SYSEX", "ERROR");  }

      }
      /*If patternswitch == 1, the pattern receiveed is for sending a track to the MD.*/
      else  if (patternswitch == 1) {

        /*Retrieve the pattern from the Sysex buffer and store it in the pattern_rec object. The MD header is 5 bytes long, hence the offset and length change*/
        if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {


          send_pattern_kit_to_md();
          patternswitch = PATTERN_UDEF;
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
// (SYSEX init)|
//$6b | Receive position ID
//%000aaaab | receive pos on type 0001=global 0010=kit
//          | 0100=pattern 1000=song
// %0ccccccc | to b=0 => pos ccccccc, b=1 original position
// %0ddddddd | for ddddddd sys

//4 for kit, 8 for pattern, pos
void md_setsysex_recpos(uint8_t rec_type, uint8_t position) {


  uint8_t data[] = { 0x6b, (uint8_t)rec_type & 0x7F, position, (uint8_t) 1 & 0x7f };
  MD.sendSysex(data, countof(data));


  //  MD.sendRequest(0x6b,00000011);
  /*
      uint8_t param = 00000011;

      MidiUart.m_putc(0xF0);
      MidiUart.sendRaw(machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr));
      MidiUart.m_putc(0x6b);
      MidiUart.m_putc(param);
      MidiUart.m_putc(0xF7);
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
  if (curtrack < 4) {
    cc = 8 + curtrack;
  }
  else if (curtrack < 8) {
    cc = 4 + curtrack;
  }
  else if (curtrack < 12) {
    cc = curtrack;
  }
  else if (curtrack < 16) {
    cc = curtrack - 4;
  }

  MidiUart.sendCC(channel + globalbasechannel_to_channel(MD.global.baseChannel), cc , value);
}

void setTrackParam(uint8_t track, uint8_t param, uint8_t value) {

  if ((track > 15) || (param > 33))
    return;

  uint8_t channel = (track >> 2);
  uint8_t b = track & 3;
  uint8_t cc = 0;
  if (param == 32) { // MUTE
    cc = 12 + b;
  } else if (param == 33) { //
    cc = 8 + b;
  } else {
    cc = param;
    if (b < 2) {
      cc += 16 + b * 24;
    } else {
      cc += 24 + b * 24;
    }
  }
  if (exploit == 1) {
    MidiUart.sendCC(channel + 3, cc, value);
  }
  else {
    MidiUart.sendCC(channel + 9, cc, value);
  }
}




/*
  ================
  function: splashscreen();
  ================
  Displays the fancy splash screen with the MCL Logo on power on.

*/
void splashscreen() {

  char str1[17] = "MINICOMMAND LIVE";
  char str2[17] = "V2.0";
  str1[16] = '\0';
  LCD.goLine(0);
  LCD.puts(str1);
  LCD.goLine(1);
  LCD.puts(str2);

  delay(100);
  // while (rec_global == 0) {


  GUI.setPage(&page);
}



void encoder_level_handle(Encoder *enc) {
  TrackInfoEncoder *mdEnc = (TrackInfoEncoder *)enc;
  uint8_t increase = 0;
  if (enc->pressmode == false) {
    increase = 1;
  }
  if (enc->pressmode == true) {
    increase = 4;
  }
  int track_newlevel;
  for (int i = 0; i < 16; i++) {
    if (notes[i] == 1) {
      //        setLevel(i,mdEnc->getValue() + MD.kit.levels[i] );
      for (int a = 0; a < increase; a++) {
        if ((mdEnc->getValue() - mdEnc->old) < 0) {
          track_newlevel = MD.kit.levels[i] - 1;
        }
        //      if ((mdEnc->getValue() - mdEnc->old) > 0) { track_newlevel = MD.kit.levels[i] + 1; }
        else {
          track_newlevel = MD.kit.levels[i] + 1;
        }
        if ((track_newlevel <= 127) && (track_newlevel >= 0)) {
          MD.kit.levels[i] += mdEnc->getValue() - mdEnc->old;
          //if ((MD.kit.levels[i] < 127) && (MD.kit.levels[i] > 0)) {
          setLevel(i, MD.kit.levels[i] );
          //}
        }
      }
    }
  }
  if (mdEnc->getValue() >= 127) {
    mdEnc->cur = 1;
    mdEnc->old = 0;
  }
  else if (mdEnc->getValue() <= 0) {
    mdEnc->cur = 126;
    mdEnc->old = 127;
  }


  //draw_levels();
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
  Toggles FX  ! 2 between Delay FB and Reverb Level Settings
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
  //  int curkit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  //MD.getBlockingKit(MD.currentKit);
  //  clearLed();

}




/*
  ================
  function: store_tracks_in_mem(int column, int row, int num);
  ================
  Main function for storing one or more consecutive tracks in to the Grid

*/

void store_tracks_in_mem( int column, int row, int store_behaviour_) {
  uint8_t readpattern = MD.currentPattern;
  if ((patternload_param1.getValue() * 16 + patternload_param2.getValue()) != MD.currentPattern) {
    readpattern = (patternload_param1.getValue() * 16 + patternload_param2.getValue());
  }

  cur_col = column;
  cur_row = row;

  store_behaviour = store_behaviour_;
  setLed();
  patternswitch = PATTERN_STORE;
  MD.getBlockingPattern(readpattern);
  int curkit;
  if (readpattern != MD.currentPattern) {
    curkit = pattern_rec.kit;
  }
  else {
    curkit = last_md_track;
    //MD.getCurrentKit(CALLBACK_TIMEOUT);
    MD.saveCurrentKit(curkit);

  }

  MD.getBlockingKit(curkit);

  //  int curkit = MD.getBlockingStatus(MD_CURRENT_KIT_REQUEST, CALLBACK_TIMEOUT);



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
void write_tracks_to_md( int column, int row, int b) {



  store_behaviour = b;
  writepattern = MD.currentPattern;
  if (((patternload_param1.getValue() * 16 + patternload_param2.getValue()) != MD.currentPattern)) {
    writepattern = (patternload_param1.getValue() * 16 + patternload_param2.getValue());
  }

  if (patternload_param3.getValue() != MD.currentKit) {
    currentkit_temp = patternload_param3.getValue();
  }

  cur_col = column;
  cur_row = row;
  patternswitch = 1;

  //  MD.requestPattern(MD.currentPattern);

  /*Request both the Kit and Pattern from the MD for extracing the Pattern and Machine data respectively*/
  /*blocking nethods provided by Wesen kindly prevent the minicommand from doing anything else whilst the precious sysex data is received*/
  //   setLed();



  //If the user has sleceted a different destination pattern then we'll need to pull data from that pattern/kit instead.

  //    if (pattern != MD.currentPattern) {

  //        MD.getBlockingPattern(pattern);
  //      currentkit_temp = pattern_rec.kit;
  //    MD.getBlockingKit(currentkit_temp);
  // }
  //else {
  // currentkit_temp = MD.getCurrentKit(CALLBACK_TIMEOUT);
  //    MD.saveCurrentKit(currentkit_temp);
  //   MD.getBlockingKit(currentkit_temp);
  MD.getBlockingPattern(MD.currentPattern);

  //  }

  //clearLed();

}

void send_pattern_kit_to_md() {
  MD.getBlockingKit(currentkit_temp);
  load_track(0, cur_row, 0);


  /*Send a quick sysex message to get the current selected track of the MD*/
  int curtrack = last_md_track;
 // MD.getCurrentTrack(CALLBACK_TIMEOUT);

  uint8_t reload = 1;
  uint16_t quantize_mute = 0;
  uint8_t q_pattern_change = 0;
  if (writepattern != MD.currentPattern) {
    reload = 0;
  }
  if (patternload_param4.getValue() == 0)  {
    quantize_mute = 0;
  }
  else if (patternload_param4.getValue() < 7) {
    quantize_mute = 1 << patternload_param4.getValue();
  }
  if (patternload_param4.getValue() == 7) {
    quantize_mute = 254;
  }
  if (patternload_param4.getValue() == 8) {
    quantize_mute = 254;
  }

  if (patternload_param4.getValue() >= 9) {
    quantize_mute = pattern_rec.patternLength; q_pattern_change = 1;
    reload = 0;
    if ((patternload_param4.getValue() == 9) && (writepattern == MD.currentPattern)) {
      reload = 1;
    }
    if (patternload_param4.getValue() == 10) {
      if (writepattern == 127) {
        writepattern = 0;
      }
      else {
        writepattern = writepattern + 1;
      }
      patternload_param4.cur = 11;
    }
    else if (patternload_param4.getValue() == 11) {
      if (writepattern == 0) {
        writepattern = 127;
      }
      else {
        writepattern = writepattern - 1;
      }
      patternload_param4.cur = 10;

    }
  }





  /*Define sysex encoder objects for the Pattern and Kit*/
  ElektronDataToSysexEncoder encoder(&MidiUart);
  ElektronDataToSysexEncoder encoder2(&MidiUart);
  /*Write the selected trackinto a Pattern object by moving it from a Track object into a Pattern object
    The destination track is the currently selected track on the machinedrum.
  */

  int i = 0;
  int track = 0;
  uint8_t note_count = 0;
  uint8_t first_note = 254;
  while ((i < 16)) {

    if ((notes[i] > 1)) {
      if (first_note == 254) {
        first_note = i;
      }
      //  if (cur_col > 0) {
      if (store_behaviour == STORE_IN_PLACE) {
        track = i;
        place_track_inpattern(track, i + cur_col, cur_row);

      }
      else if (cur_col + (i - first_note) < 16) {
        track = cur_col + (i - first_note);
        place_track_inpattern(track,  i, cur_row);

      }

      if (patternload_param4.getValue() == 8) {
        MD.kit.levels[track] = 0;
      }
      //   }

      note_count++;
      if ((quantize_mute > 0) && (patternload_param4.getValue() < 8)) {
        MD.muteTrack(track, true);
      }
    }
    i++;

  }


  /*Set the pattern position on the MD the pattern is to be written to*/


  setLed();


  /*Send the encoded pattern to the MD via sysex*/


  //int temp = MD.getCurrentKit(CALLBACK_TIMEOUT);

  /*If kit_sendmode == 1 then we'll be sending the Machine via sysex kit dump. */

  /*Tell the MD to receive the kit sysexdump in the current kit position*/


  /* Retrieve the position of the current kit loaded by the MD.
    Use this position to store the modi
  */
  //If write original, let's copy the master fx settings from the first track in row
  //Let's also set the kit receive position to be the original.


  if (write_original == 1) {

    //     MD.kit.origPosition = temptrack.origPosition;
    for (uint8_t c = 0; c < 17; c++) {
      MD.kit.name[c] = temptrack.kitName[c];
    }

    m_memcpy(&MD.kit.reverb[0], &temptrack.kitextra.reverb, sizeof(temptrack.kitextra.reverb));
    m_memcpy(&MD.kit.delay[0], &temptrack.kitextra.delay, sizeof(temptrack.kitextra.delay));
    m_memcpy(&MD.kit.eq[0], &temptrack.kitextra.eq, sizeof(temptrack.kitextra.eq));
    m_memcpy(&MD.kit.dynamics[0], &temptrack.kitextra.dynamics, sizeof(temptrack.kitextra.dynamics));
    pattern_rec.patternLength = temptrack.length;
    pattern_rec.swingAmount = temptrack.kitextra.swingAmount;
    pattern_rec.accentAmount = temptrack.kitextra.accentAmount;
    pattern_rec.patternLength = temptrack.kitextra.patternLength;
    pattern_rec.doubleTempo = temptrack.kitextra.doubleTempo;
    pattern_rec.scale = temptrack.kitextra.scale;



  }
  // MD.kit.origPosition = currentkit_temp;

  //Kit
  //If Kit is OG.
  if (patternload_param3.getValue() == 64) {
    MD.kit.origPosition = temptrack.origPosition;
    pattern_rec.kit = temptrack.origPosition;
  }
  else {


    pattern_rec.kit = currentkit_temp;
    MD.kit.origPosition = currentkit_temp;
    //       }
  }
  //If Pattern is OG
  if (patternload_param1.getValue() == 8) {
    pattern_rec.origPosition = temptrack.patternOrigPosition;
    reload = 0;
  }
  else {
    pattern_rec.setPosition(writepattern);

  }
  // MidiUart.setActiveSenseTimer(0);
  in_sysex = 1;

  md_setsysex_recpos(8, pattern_rec.origPosition);

  pattern_rec.toSysex(encoder);
  in_sysex = 1;

  //    if (turbo_state == 1) {
  // delay(50);
  //  }
  //    while (MidiClock.mod6_counter != 0);

  /*Send the encoded kit to the MD via sysex*/
  md_setsysex_recpos(4, MD.kit.origPosition);
  MD.kit.toSysex(encoder2);
  //        MidiUart.sendActiveSenseTimer = 290;
  //    if (turbo_state == 1) {
  //   delay(25);
  //   }
  //  pattern_rec.setPosition(pattern_rec.origPosition);



  /*Instruct the MD to reload the kit, as the kit changes won't update until the kit is reloaded*/
  //
  //    MidiUart.setActiveSenseTimer(290);
  if (reload == 1) {
    MD.loadKit(pattern_rec.kit);
  }
  else if ((q_pattern_change == 1) || (writepattern != MD.currentPattern)) {
    load_the_damn_kit = pattern_rec.kit;
    if (q_pattern_change == 1) {
      MD.loadPattern(writepattern);
    }

  }
  /*kit_sendmode != 1 therefore we are going to send the Machine via Sysex and Midi cc without sending the kit*/

  //I fthe sequencer is running then we will pause and wait for the next divison
  //  for (int n=0; n < 16; n++) {
  //      MD.global.baseChannel = 10;
  //      for (i=0; i < 16; i++) {
  //       MD.muteTrack(i,true);
  //      }
  // }
  //          delay(100);
  //Midiclock start hack
  if (pattern_start_clock32th > MidiClock.div32th_counter) {
    pattern_start_clock32th = 0;
  }
  if (quantize_mute > 0) {
    if (MidiClock.state == 2) {



      if ((q_pattern_change != 1) && (quantize_mute <= 64)) {
        // (MidiClock.div32th_counter - pattern_start_clock32th)
        //                   while (((MidiClock.div32th_counter + 3) % (quantize_mute * 2))  != 0) {
        while ((((MidiClock.div32th_counter - pattern_start_clock32th) + 3) % (quantize_mute * 2))  != 0) {
          GUI.display();
        }
      }



      if (q_pattern_change != 1) {
        for (i = 0; i < 16; i++) {
          //If we're in cue mode, send the track to cue before unmuting
          if ((notes[i] > 1)) {
            if (patternload_param4.getValue() == 7) {
              SET_BIT32(cue1, i);
              MD.setTrackRouting(i, 5);
            }
            MD.muteTrack(i, false);
          }
        }
      }
    }
    if (patternload_param4.getValue() == 7) {
      send_globals();
    }
  }
  in_sysex = 0;

  clearLed();
  /*All the tracks have been sent so clear the write queue*/
  write_original = 0;

}


/*
  ================
  function: toggle_cue(int i)
  ================
  Toggles the individual bits of the Bitmask "cue1" that symbolises the tracks that have been routed to the Cue Output.
*/

void toggle_cue(int i) {

  if (IS_BIT_SET32(cue1, i)) {
    CLEAR_BIT32(cue1, i);
    MD.setTrackRouting(i, 6);
  }
  else {
    SET_BIT32(cue1, i);
    MD.setTrackRouting(i, 5);
  }

}

void toggle_cues_batch() {

  uint16_t quantize_mute;
  quantize_mute = 1 << trackinfo_param4.getValue();
  int i;
  for (i = 0; i < 16; i++) {
    if (notes[i] == 3) {
      MD.muteTrack(i, true);
    }

  }
  if (trackinfo_param4.getValue() < 7) {
    while ((((MidiClock.div32th_counter - pattern_start_clock32th)  + 3) % (quantize_mute * 2))  != 0) {
      GUI.display();
    }
  }


  //send the track to master before unmuting

  for (i = 0; i < 16; i++) {
    if (notes[i] == 3) {
      if (trackinfo_param4.getValue() == 7) {
        setLevel(i, 0 );
      }
      toggle_cue(i);

      MD.muteTrack(i, false);

    }
    //  notes[i] = 0;
    // trackinfo_page.display();
  }
}

//          setLevel(i,MD.kit.levels[i] )
void ptc_root_handler(Encoder *enc) {
}
void octave_handler(Encoder *enc) {
  if (BUTTON_DOWN(Buttons.BUTTON2)) {
    euclid_root[last_md_track] = trackinfo_param4.getValue() - (trackinfo_param4.getValue() / 12) * 12;
  }
}
void pattern_len_handler(Encoder *enc) {
  
  if (BUTTON_DOWN(Buttons.BUTTON3)) {
    for (uint8_t c = 0; c < 16; c++) {
      PatternLengths[c] = trackinfo_param3.getValue();
    }

  }
  else {
    if ((curpage == SEQ_RTRK_PAGE) || (curpage == SEQ_RLCK_PAGE) || (curpage == SEQ_STEP_PAGE)) {
      PatternLengths[cur_col] = trackinfo_param3.getValue();
    }
    if ((curpage == SEQ_RPTC_PAGE) || (curpage == SEQ_RPTC_PAGE)) {
      PatternLengths[last_md_track] = trackinfo_param3.getValue();

    }
  }
}
void encoder_fx_handle(Encoder *enc) {
  GridEncoder *mdEnc = (GridEncoder *)enc;

  /*Scale delay feedback for safe ranges*/

  if (mdEnc->fxparam == MD_ECHO_FB) {
    if (mdEnc->getValue() > 68) {
      mdEnc->setValue(68);
    }

  }
  USE_LOCK();
  SET_LOCK();
  MD.sendFXParam(mdEnc->fxparam, mdEnc->getValue(), mdEnc->effect);
  CLEAR_LOCK();
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
  if (looptrack_ != -1) {
    patternswitch = 4;
  }
  else {
    patternswitch = 5;
  }
  setLed();
  int curkit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  MD.saveCurrentKit(curkit);
  MD.getBlockingPattern(MD.currentPattern);
  clearLed();

}
void loadtrackinfo_page(uint8_t i) {
  // if (MidiClock.state == 2) {
  // while (MidiClock.mod6_counter != 0);
    
  // }
  trackinfo_param2.min = 0;

 
    currentkit_temp = MD.getCurrentKit(CALLBACK_TIMEOUT);
    MD.getCurrentTrack(CALLBACK_TIMEOUT);

    if (MidiClock.state != 2) {
      MD.saveCurrentKit(currentkit_temp);
    }
    MD.getBlockingKit(currentkit_temp);
     
  if (i == 0) {
    curpage = SEQ_STEP_PAGE;

    exploit_on();

    trackinfo_param1.max = 13;
    trackinfo_param2.max = 11;
    trackinfo_param2.min = 1;
    trackinfo_param2.cur = 6;
    trackinfo_param3.max = 64;
    trackinfo_param4.max = 16;
    trackinfo_param3.cur = PatternLengths[cur_col];

  }
  if (i == 1) {

    exploit_on();
    collect_notes = false;

    trackinfo_param1.max = 4;
    trackinfo_param2.max = 64;
    trackinfo_param3.max = 64;
    trackinfo_param4.max = 11;
    trackinfo_param3.cur = PatternLengths[cur_col];


  }
  if (i == 2) {

    trackinfo_param1.max = 23;
    trackinfo_param2.max = 127;
    trackinfo_param3.max = 23;
    trackinfo_param4.max = 127;

    exploit_on();
    trackinfo_param1.cur = PatternLocksParams[last_md_track][0];
    trackinfo_param3.cur = PatternLocksParams[last_md_track][1];
    trackinfo_param2.cur = MD.kit.params[last_md_track][PatternLocksParams[last_md_track][0]];
    trackinfo_param4.cur = MD.kit.params[last_md_track][PatternLocksParams[last_md_track][1]];
  }
  if (i == 3) {

    exploit_on();
    collect_notes = false;

    trackinfo_param1.max = 8;
    trackinfo_param2.max = 12;
    trackinfo_param3.max = 64;
    trackinfo_param4.max = 35;
    trackinfo_param2.cur = 0;
    trackinfo_param1.cur = 1;

    trackinfo_param3.cur = PatternLengths[cur_col];

  }

  GUI.setPage(&trackinfo_page);
    if (i == 0) { curpage = SEQ_STEP_PAGE; }
  else if (i == 1) { curpage = SEQ_RTRK_PAGE; }
  else if (i == 2) { curpage = SEQ_PARAM_A_PAGE; }
  else if (i == 3) { curpage = SEQ_PTC_PAGE; }
  //Len
  cur_col = last_md_track;
  cur_row = param2.getValue();
}
void clear_row (int row) {
  for (int x = 0; x < 16; x++) {
    clear_Grid(x + (row * 16));
  }
}
void clear_Grid(int i) {
  temptrack.active = false;
  int32_t offset = (int32_t) sizeof(project_header) + (int32_t) i * (int32_t) sizeof(MDTrack);
  file.seek(&offset, FAT_SEEK_SET);
  file.write(( uint8_t*) & (temptrack.active), sizeof(temptrack.active));
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
    if (idxn != -1) {
      pattern_rec.locks[idxn][i] = 254;
    }
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

  m_strncpy(newstring, string1, strlen1 - 1);


  m_strncpy(&newstring[strlen1 - 1], &string2[0], strlen2);

  newstring[newlength - 1] = '\0';

};

/*
  ================
  variable: uint8_t charmap[7]
  ================
  Character Map for the LCD Display
  Defines the pixels of one glyph and is used to create the custom || character
*/

uint8_t charmap[8] = { 10, 10, 10, 10, 10, 10, 10, 00 };


/*
  ================
  function: setup()
  ================

  Initialization function for the MCL firmware, run when the Minicommand is powered on.
*/

void send_globals() {
  if (rec_global == 1) {
    ElektronDataToSysexEncoder encoder(&MidiUart);
    ElektronDataToSysexEncoder encoder2(&MidiUart);
    setup_global(0);
    global_one.toSysex(encoder);
    setup_global(1);
    global_one.toSysex(encoder2);
  }
}
void setup_global(int global_num) {
  /** Original position of the global inside the MD (0 to 7). **/
  if (global_num == 0) {
    global_one.origPosition = 6;
  }
  else {
    global_one.origPosition = 7;
  }
  /** Stores the audio output for each track. **/



  for (uint8_t track_n = 0; track_n < 16; track_n++) {
    if (IS_BIT_SET32(cue1, track_n)) {
      global_one.drumRouting[track_n] = 5;
    }
    else {
      global_one.drumRouting[track_n] = 6;
    }



  }

  //baseChannel
  // -- 0
  // 1-4 1
  // 5-8 2
  // 9-12 3
  // 13-16 4

  /** The MIDI base channel of the MachineDrum. **/
  if (global_num == 0) {
    global_one.baseChannel = 3;
  }
  else {
    global_one.baseChannel = 9;
  }

  global_one.extendedMode = true;
  if (MidiClock.mode == MidiClock.EXTERNAL_MIDI) {
    global_one.clockIn = false;
    global_one.clockOut = true;
  }
  else {
    global_one.clockIn = true;
    global_one.clockOut = false;
  }
  global_one.transportIn = true;
  //some bug
  global_one.transportOut = true;
  global_one.localOn = 1;
  global_one.programChange = 2;

  /*
      global_one.drumLeft = 0;
      global_one.drumRight = 0;
      global_one.gateLeft = 0;
      global_one.gateRight = 0;
      global_one.senseLeft = 0;
      global_one.senseRight = 0;
      global_one.minLevelLeft = 0;
      global_one.minLevelRight = 0;
      global_one.maxLevelLeft = 0;
      global_one.maxLevelRight = 0;


      global_one.trigMode = 1;
  */
}

MDSequencer md_seq;
void setup() {


  LCD.createChar(1, charmap);

  uint8_t temp_charmap[8] = { 0, 0, 0, 0, 0, 0, 0, 31  };

  for (uint8_t i = 1; i < 8; i++) {
    for (uint8_t x = 1; x < i; x++) {
      temp_charmap[(8 - x)] = 31;
      LCD.createChar(1 + i, temp_charmap);
    }

  }

  //Enable callbacks, and disable some of the ones we don't want to use.


  //MDTask.setup();
  //MDTask.verbose = false;
  //MDTask.autoLoadKit = false;
  //MDTask.reloadGlobal = false;

  //GUI.addTask(&MDTask);

  //Create a mdHandler object to handle callbacks.

  MDHandler2 mdHandler;
  mdHandler.setup();



  // int temp = MD.getCurrentKit(50);

  //Load the splashscreen
  splashscreen();

  //Initialise Track Routing to Default output (1 & 2)
  //   for (uint8_t i = 0; i < 16; i++) {
  //  MD.setTrackRouting(i,6);
  // }

  //set_midinote_totrack_mapping();

  //Initalise the  Effects Ecnodres
  
  param2.handler = encoder_param2_handle;
  param3.handler = encoder_fx_handle;
  param3.effect = MD_FX_ECHO;
  param3.fxparam = MD_ECHO_TIME;
  param4.handler = encoder_fx_handle;
  param4.effect = MD_FX_ECHO;
  param4.fxparam = MD_ECHO_FB;

trackinfo_param4.handler = octave_handler;
  mixer_param1.handler = encoder_level_handle;
  mixer_param2.handler = encoder_level_handle;
  trackinfo_param3.handler = pattern_len_handler;
trackinfo_param2.handler = ptc_root_handler;
  //mixer_param2.handler = encoder_filter_handle;
  //Setup Turbo Midi
  frames_startclock = read_slowclock();
  //TurboMidi.setup();
  //Start the SD Card Initialisation.
  sd_load_init();
  MidiClock.mode = MidiClock.EXTERNAL_MIDI;

  switchGlobal(7);
  MD.resetMidiMap();

  //For base channel 10. needed for mutes to work properly.
  MD.global.baseChannel = 9;
  TrigCaptureClass trigger;

  trigger.setup();
  //

  //      GUI.flash_strings_fill("MIDI CLOCK SRC", "MIDI PORT 2");
  MidiClock.start();
  //   MidiUart.setActiveSenseTimer(290);
  MD.requestGlobal(7);


  // patternswitch = 7;
  //     int curkit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  //      MD.getBlockingKit(curkit);

  md_seq.setup();
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
    if (!load_track(column, row, 50)) {
      return 0;
    }
  }

  if (temptrack.active != TRUE) {
    return (uint64_t) 0;
  }
  else if (j == 1) {
    return temptrack.swingPattern;
  }
  else if (j == 2) {
    return temptrack.slidePattern;
  }
  else if (j == 3) {
    return temptrack.accentPattern;
  }
  else {
    return temptrack.trigPattern;
  }
}
/*
  void my_block(uint8_t time) {
    uint16_t start_clock = read_slowclock();
    uint16_t current_clock = start_clock;
    do {
        current_clock = read_slowclock();

       //MCL Code, trying to replicate main loop

        if ((MidiClock.mode == MidiClock.EXTERNAL ||
                                 MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
              MidiClock.updateClockInterval();
         }
        handleIncomingMidi();
        GUI.display();
    } while ((clock_diff(start_clock, current_clock) < time);
  }
*/
/*
  ===================
  function: getGridModel(int column, int row, bool load)
  ===================

  Return the machine model object of the track object located at grid Grid i

*/

uint32_t getGridModel(int column, int row, bool load) {
  if ( load == true) {
    if (!load_track(column, row, 50)) {
      return NULL;
    }
  }
  if (temptrack.active != TRUE) {
    return NULL;
  }
  else {
    return temptrack.machine.model;
  }
}

/*
  ===================
  function:  *getTrackKit(int column, int row, bool lo
  ===================

  Return a the name of kit associated with the track object located at grid Grid i

*/


char *getTrackKit(int column, int row, bool load, bool scroll) {
  if (load) {
    if (!load_track(column, row, 50)) {
      return "    ";
    }

    if (temptrack.active != TRUE) {
      return "    ";
    }


    uint8_t char_position = 0;
    //if ((read_slowclock() % 50) == 0) { row_name_offset++; }
    if (row_name_offset > 15) {
      row_name_offset = 0;
    }
    if (scroll) {

      for (uint8_t c = 0; c < 4; c++) {

        if (c + (uint8_t)row_name_offset > 15) {
          char_position =  c + (uint8_t)row_name_offset - 16;
        }
        else {
          char_position =  c + (uint8_t)row_name_offset;
        }
        //  char some_string[] = "hello my baby";
        //row_name[c] = some_string[char_position];
        if (char_position < 5) {
          row_name[c] = ' ';
        }
        else {
          row_name[c] = temptrack.kitName[char_position - 5];
        }
      }
      row_name[4] = '\0';

    }
    else {
      for (uint8_t a = 0; a < 16; a++) {
        row_name[a] = temptrack.kitName[a];
      }
    }
    return row_name;
  }
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
  GUI.put_string_at_fill(0, "Name:");
  GUI.put_string_at(6, &config.project[1]);
  GUI.setLine(GUI.LINE2);



  if (options_param1.getValue() == 3) {


    if (options_param1.hasChanged()) {
      options_param1.old = options_param1.cur;
      options_param2.setValue(merge);
    }
    GUI.put_string_at_fill(0, "MERGE");

    if (options_param2.getValue() == 0) {
      GUI.put_string_at_fill(10, "OFF");
    }
    if (options_param2.getValue() >= 1) {
      GUI.put_string_at_fill(10, "ON");
    }
    if (options_param2.hasChanged()) {
      if (options_param2.getValue() > 1) {
        options_param2.cur = 1;
      }
      merge = options_param2.getValue();
    }
  }
  else if (options_param1.getValue() == 2) {

    if (options_param1.hasChanged()) {
      options_param1.old = options_param1.cur;
      options_param2.setValue(turbo);
    }
    GUI.put_string_at_fill(0, "TURBO");

    if (options_param2.getValue() == 0) {

      GUI.put_string_at_fill(10, "1x");
    }
    if (options_param2.getValue() == 1) {
      GUI.put_string_at_fill(10, "4x");
    }
    if (options_param2.getValue() == 2) {
      GUI.put_string_at_fill(10, "8x");
    }
    if (options_param2.hasChanged()) {

      turbo = options_param2.getValue();
    }

  }


  else if (options_param1.getValue() == 0) {
    GUI.put_string_at_fill(0, "Load Project");
  }
  else if (options_param1.getValue() == 1) {
    GUI.put_string_at_fill(0, "New Project");
  }

}


void PatternLoadPage::display() {
  draw_notes(0);
  GUI.setLine(GUI.LINE2);
  if (curpage == S_PAGE) {
    GUI.put_string_at(0, "S");
  }
  else if (curpage == W_PAGE) {
    GUI.put_string_at(0, "W");
  }

  char str[5];


  if (patternload_param1.getValue() < 8) {
    MD.getPatternName(patternload_param1.getValue() * 16 + patternload_param2.getValue() , str);
    GUI.put_string_at(2, str);
  }
  else {
    GUI.put_string_at(2, "OG");
  }





  /*Initialise the string with blank steps*/
  //  char str[5];
  //                MD.getPatternName(encoder_offset,str);
  //        GUI.put_string_at(0,str);

  //  patternload_param4.setValue(currentkit_temp + 1);
  uint8_t step_count = (MidiClock.div16th_counter - pattern_start_clock32th / 2) - (64 * ((MidiClock.div16th_counter - pattern_start_clock32th / 2) / 64));
  GUI.put_value_at2(14, step_count);
  if (curpage == W_PAGE) {
    uint8_t x;




    GUI.put_string_at(9, "Q:");

    //0-63 OG
    if (patternload_param3.getValue() == 64) {
      GUI.put_string_at(6, "OG");
    }
    else {
      GUI.put_value_at2(6, patternload_param3.getValue() + 1);
    }


    if (patternload_param4.getValue() == 0) {
      GUI.put_string_at(11, "--");
    }
    if (patternload_param4.getValue() == 7) {
      GUI.put_string_at(11, "CU");
    }
    if (patternload_param4.getValue() == 8) {
      GUI.put_string_at(11, "LV");
    }
    if (patternload_param4.getValue() == 9) {
      GUI.put_string_at(11, "P ");
    }
    if (patternload_param4.getValue() == 10) {
      GUI.put_string_at(11, "P+");
    }
    if (patternload_param4.getValue() == 11) {
      GUI.put_string_at(11, "P-");
    }

    if ((patternload_param4.getValue() < 7) && (patternload_param4.getValue() > 0)) {
      x = 1 << patternload_param4.getValue();
      GUI.put_value_at2(11, x);
    }


  }

}
//WR  F10 XX CU 64
//0123456789012345


//WRITE -- Q:CU 64
/* Overriding display method for the TrackInfoEncoder
  This method is responsible for the display of the TrackInfo page.
  The selected track information is displayed in this screen (Line 1).
  This includes the kit name and a graphical representation of the Step Sequencer Locks for the track (Line 2).
*/

void TrackInfoPage::display()  {
  uint8_t x;
  if (curpage == 8) {

    GUI.setLine(GUI.LINE1);
    GUI.put_string_at(0, "Project:");
    GUI.setLine(GUI.LINE2);
    GUI.put_string_at_fill(0, entries[loadproj_param1.getValue()].name);
    return;
  }

  else if (curpage == MIXER_PAGE) {
    draw_notes(0);
    draw_levels();
    //   uint8_t x;
    //   if (mixer_param3.getValue() == 0) {  GUI.put_string_at(10,"--");  }
    //    else {
    //       x = 1 << mixer_param3.getValue();
    //       GUI.put_value_at(9,x);
    ///       GUI.put_string_at(3,"Q: :");
    //      }
    //    x = 1 << mixer_param3.getValue();
    //     GUI.put_value_at(4,x);
    //    GUI.put_string_at(12,"LEN: :");

    return;

  }

  else if (curpage == 7) {
    if (proj_param1.hasChanged()) {
      update_prjpage_char();

    }
    //    if ((proj_param2.hasChanged())){
    newprj[proj_param1.getValue()] = allowedchar[proj_param2.getValue()];
    //  }

    GUI.setLine(GUI.LINE1);
    GUI.put_string_at(0, "New Project:");
    GUI.setLine(GUI.LINE2);
    GUI.put_string_at(0, &newprj[1]);
  }
  else if (curpage == CUE_PAGE) {


    GUI.setLine(GUI.LINE2);

    // GUI.put_string_at(12,"Cue");
    GUI.put_string_at(0, "CUES");

    GUI.put_string_at(9, "Q:");

    if (trackinfo_param4.getValue() == 0) {
      GUI.put_string_at(11, "--");
    }
    else if (trackinfo_param4.getValue() == 7) {
      GUI.put_string_at(11, "LV");
    }
    else {
      x = 1 << trackinfo_param4.getValue();


      GUI.put_value_at2(11, x);
    }
    uint8_t step_count = (MidiClock.div16th_counter - pattern_start_clock32th / 2) - (64 * ((MidiClock.div16th_counter - pattern_start_clock32th / 2) / 64));
    GUI.put_value_at2(14, step_count);

    draw_notes(0);
  }

  else if (curpage == MIXER_PAGE) {

    draw_notes(0);

  }

  else if ((curpage == SEQ_RTRK_PAGE) || (curpage == SEQ_RLCK_PAGE)) {
    GUI.setLine(GUI.LINE1);
    if (curpage == SEQ_RLCK_PAGE) {
      GUI.put_string_at(0, "RLCK");

    } if (curpage == SEQ_RTRK_PAGE) {
      GUI.put_string_at(0, "RTRK");

    }


    const char *str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
    const char *str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);
    GUI.put_p_string_at(11, str1);
    GUI.put_p_string_at(14, str2);


    GUI.put_value_at2(8, trackinfo_param3.getValue());
    if (curpage == SEQ_RLCK_PAGE) {

      draw_lockmask(page_select * 16);

    } if (curpage == SEQ_RTRK_PAGE) {

      draw_patternmask(page_select * 16);

    }

    //  draw_patternmask((trackinfo_param1.getValue() * 16));


  }
  else if ((curpage == SEQ_RPTC_PAGE) || (curpage == SEQ_PTC_PAGE)) {

    const char *str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
    const char *str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);
    GUI.setLine(GUI.LINE1);
if ((curpage == SEQ_RPTC_PAGE)) {
    GUI.put_string_at(0, "RPTC");
}
else {
     GUI.put_string_at(0, "PTC");

}
    GUI.put_value_at2(8, trackinfo_param3.getValue());

    GUI.put_p_string_at(11, str1);
    GUI.put_p_string_at(14, str2);

    GUI.setLine(GUI.LINE2);
    GUI.put_string_at(0, "OC:");
    GUI.put_value_at2(3, trackinfo_param1.getValue());
    GUI.put_string_at(6, "RT:");

    GUI.put_value_at2(9, trackinfo_param2.getValue());

    GUI.put_string_at(12, "S:");


    GUI.put_value_at2(14, trackinfo_param4.getValue());


    //  draw_patternmask((trackinfo_param1.getValue() * 16));

    //draw_notes(0);
  }
  else if ((curpage == SEQ_PARAM_A_PAGE) || (curpage == SEQ_PARAM_B_PAGE))  {
    GUI.setLine(GUI.LINE1);
    char myName[4];
    if (trackinfo_param1.getValue() == 0) {
      GUI.put_string_at(0, "--");
    }
    else {
      m_strncpy_p(myName, model_param_name(MD.kit.models[last_md_track], trackinfo_param1.getValue() - 1), 4);
      GUI.put_string_at(0, myName);
    }
    GUI.put_value_at2(4, trackinfo_param2.getValue());
    if (trackinfo_param3.getValue() == 0) {
      GUI.put_string_at(7, "--");
    }
    else {
      m_strncpy_p(myName, model_param_name(MD.kit.models[last_md_track], trackinfo_param3.getValue() - 1), 4);
      GUI.put_string_at(7, myName);
    }

    GUI.put_value_at2(11, trackinfo_param4.getValue());


    if (curpage == SEQ_PARAM_A_PAGE) {
      GUI.put_string_at(14, "A");
    }
    if (curpage == SEQ_PARAM_B_PAGE) {
      GUI.put_string_at(14, "B");
    }
    GUI.put_value_at2(15, (page_select + 1) * 10);
    draw_lockmask(page_select * 16);

  }
  else {
    GUI.setLine(GUI.LINE1);
      GUI.put_value_at2(14, page_select + 1);

    if (curpage == SEQ_EUC_PAGE) {
            GUI.put_string_at(0, "E     ");

      GUI.put_value_at2(2, trackinfo_param1.getValue());

            GUI.put_value_at2(5, trackinfo_param2.getValue());

          GUI.put_value_at2(8, trackinfo_param3.getValue());

          if (BUTTON_DOWN(Buttons.BUTTON2)) {
          GUI.put_value_at2(11, euclid_root[last_md_track]);
          }
          else {
          GUI.put_value_at2(11, trackinfo_param4.getValue());
          }
      draw_patternmask((page_select * 16));

    }
    else if (curpage == SEQ_STEP_PAGE) {

      /*Display the Kit name associated with selected track on line 1*/
      //GUI.put_string_at(0, getTrackKit(cur_col, cur_row, true, false));
      //Len
      GUI.put_value_at2(8, trackinfo_param3.getValue());
      char c[3] = "--";
      if (trackinfo_param1.getValue() == 0) {
        GUI.put_string_at(0, "L01");

      }
      else if (trackinfo_param1.getValue() <= 8) {
        GUI.put_string_at(0, "L");

        GUI.put_value_at2(1, trackinfo_param1.getValue());
      }
      else {
        GUI.put_string_at(0, "P");
        uint8_t prob[5] = { 10, 25, 50, 75, 90  };
        GUI.put_value_at2(1, prob[trackinfo_param1.getValue() - 9]);
      }


      //Cond
      //    GUI.put_value_at2(0, trackinfo_param1.getValue());
      //Pos
      //0  1   2  3  4  5  6  7  8  9  10  11
      //  -5  -4 -3 -2 -1 0
      if (trackinfo_param2.getValue() == 0) {
        GUI.put_string_at(3, "--");
      }
      else if ((trackinfo_param2.getValue() < 6) && (trackinfo_param2.getValue() != 0))  {
        GUI.put_string_at(4, "-");
        GUI.put_value_at2(5, 6 - trackinfo_param2.getValue());

      }
      else {
        GUI.put_string_at(4, "+");
        GUI.put_value_at2(5, trackinfo_param2.getValue() - 6);
      }
      //PatternLengths[cur_col] = trackinfo_param3.getValue();
      draw_patternmask((page_select * 16));

    }


    /*              PatternLengths[cur_col] = trackinfo_param2.getValue();

      if (trackinfo_param2.getValue() == 1) {
        GUI.put_string_at(14, "SW");
      }
      else if (trackinfo_param2.getValue() == 2) {
        GUI.put_string_at(14, "SL");
      }
      else if (trackinfo_param2.getValue() == 3) {
        GUI.put_string_at(14, "AC");
      }
      else {
        GUI.put_string_at(14, "TR");
      }
    */

    // cur_col = MD.getCurrentTrack(CALLBACK_TIMEOUT);;

  }
}
void draw_lockmask(uint8_t offset) {
  GUI.setLine(GUI.LINE2);

  char str[17] = "----------------";

  for (int i = 0; i < 16; i++) {

    if  (IS_BIT_SET64(LockMasks[cur_col], i + offset) ) {
      str[i] = 'x';

    }
    if  (IS_BIT_SET64(PatternMasks[cur_col], i + offset) && !IS_BIT_SET64(LockMasks[cur_col], i + offset) ) {

      str[i] = (char) 165;

    }
    if (IS_BIT_SET64(PatternMasks[cur_col], i + offset) && IS_BIT_SET64(LockMasks[cur_col], i + offset) )  {

      str[i] = (char) 219;

    }

    if (notes[i] > 0)  {
      /*If the bit is set, there is a cue at this position. We'd like to display it as [] on screen*/
      /*Char 219 on the minicommand LCD is a []*/

      str[i] = (char) 255;
    }

  }
  GUI.put_string_at(0, str);

}
void draw_patternmask(uint8_t offset) {
  GUI.setLine(GUI.LINE2);
  /*str is a string used to display 16 steps of the patternmask at a time*/
  /*blank steps sequencer trigs are denoted by a - */

  /*Initialise the string with blank steps*/
  char mystr[17] = "----------------";

  /*Get the Pattern bit mask for the selected track*/
  //    uint64_t patternmask = getPatternMask(cur_col, cur_row , 3, false);
  uint64_t patternmask = PatternMasks[cur_col];
  /*Display 16 steps on screen, starting at an offset set by the encoder1 value*/
  /*The encoder offset allows you to scroll through the 4 pages of the 16 step sequencer triggers that make up a 64 step pattern*/

  /*For 16 steps check to see if there is a trigger at pattern position i + (encoder_offset * 16) */
  for (int i = 0; i < 16; i++) {

    if  (IS_BIT_SET64(patternmask, i + offset) ) {
      /*If the bit is set, there is a trigger at this position. We'd like to display it as [] on screen*/
      /*Char 219 on the minicommand LCD is a []*/
      mystr[i] = (char) 219;
    }
    if (notes[i] > 0)  {
      /*If the bit is set, there is a cue at this position. We'd like to display it as [] on screen*/
      /*Char 219 on the minicommand LCD is a []*/

      mystr[i] = (char) 255;
    }

  }
  /*Display the step sequencer pattern on screen, 16 steps at a time*/
  GUI.put_string_at(0, mystr);

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
  /*if (BUTTON_DOWN(Buttons.BUTTON1)) {
    int inc = enc->normal + (pressmode ? 0 : (scroll_fastmode ? 4 * enc->button : enc->button));
    cur = limit_value(cur, inc, min, max);
                     if (cur > old) { cur = max; }
                     if (cur < old) { cur = min; }

                     if (cur > old) { cur = max; }
                     if (cur < old) { cur = min; }


           } */

  /*If shift1 is pressed then increase the encoder value by 4 (Fast encoder movement)*/
  //      else if (BUTTON_DOWN(Buttons.BUTTON2)) {
  //	int inc = (enc->normal + (pressmode ? 0 : (scroll_fastmode ? 4 * enc->button : enc->button))) * 4;

  //	cur = limit_value(cur, inc, min, max);
  //            }

  /*Increase encoder value as usual*/
  //            else {
  int inc = enc->normal + (pressmode ? 0 : (scroll_fastmode ? 4 * enc->button : enc->button));
  //int inc = 4 + (pressmode ? 0 : (fastmode ? 5 * enc->button : enc->button));
  cur = limit_value(cur, inc, min, max);

  //   GridEncoder *mdEnc = (GridEncoder *)enc;

  // if (mdEnc->effect) { dispeffect = 1; }
  //      }


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
  uint16_t current_clock = read_slowclock();

  frames += 1;
  if (clock_diff(frames_startclock, current_clock) >= 400) {
    frames_startclock = read_slowclock();
    frames = 0;
  }
  if (clock_diff(frames_startclock, current_clock) >= 250) {
    frames_fps = frames;
    // frames_fps = ((frames + frames_fps)/ 2);
    frames = 0;
    frames_startclock = read_slowclock();
  }


  row_name_offset += (float) 1 / frames_fps * 1.5;



  if (BUTTON_DOWN(Buttons.ENCODER3) && (param3.hasChanged())) {
    toggle_fx1();
  }

  if (BUTTON_DOWN(Buttons.ENCODER4) && (param4.hasChanged())) {
    toggle_fx2();
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
    else {
      GUI.put_string_at(12, "DC");
    }

    if (param4.effect == MD_FX_ECHO) {
      GUI.put_string_at(14, "FB");
    }
    else {
      GUI.put_string_at(14, "LV");
    }

    GUI.setLine(GUI.LINE2);
    /*Displays the value of the current Row on the screen.*/
    GUI.put_value_at2(12, (encoders[2]->getValue()) );
    GUI.put_value_at2(14, (encoders[3]->getValue()) );
    // mdEnc1->dispnow = 0;
    //  mdEnc2->dispnow = 0;

  }
  else {
    GUI.setLine(GUI.LINE1);
    /*Displays the kit name of the left most Grid on the first line at position 12*/

    GUI.put_string_at(12, getTrackKit(encoders[0]->getValue(), encoders[1]->getValue(), true, true ));

    GUI.setLine(GUI.LINE2);

    /*Displays the value of the current Row on the screen.*/
    GUI.put_value_at2(12, (encoders[0]->getValue()) );
    /*Displays the value of the current Column on the screen.*/
    GUI.put_value_at2(14, (encoders[1]->getValue()));
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
  const char *str = getMachineNameShort(getGridModel(param1.getValue() + i, param2.getValue(), true), 1);

  /*If the MachineName str is Null then it means that Track Grid is empty. Empty track Grids are depicted by -- strings*/
  if (str == NULL) {
    char strn[3] = "--";
    GUI.put_string_at((0 + (i * 3)), strn);
  }

  else {
    GUI.put_p_string_at((0 + (i * 3)), str);
  }

  GUI.setLine(GUI.LINE2);
  str = NULL;
  str = getMachineNameShort(getGridModel( param1.getValue() + i, param2.getValue(), false), 2);

  /*If the MachineName str is Null then it means that Track Grid is empty. Empty track Grids are depicted by -- strings*/
  if (str == NULL) {
    char strn[3] = "--";
    GUI.put_string_at((0 + (i * 3)), strn);
  }

  //  GUI.put_string_at((4 + (i * 3)),getMachineNameShort2(getGridModel(getValue())));
  else {
    GUI.put_p_string_at((0 + (i * 3)), str);
  }
  redisplay = false;

}

void set_midinote_totrack_mapping() {
  uint8_t note;
  //  uint8_t track;
  for (uint8_t track_n = 0; track_n < 16; track_n++) {
    note = 36 + track_n;
    // uint8_t data[] = { 0x5a, (uint8_t)note & 0x7F, (uint8_t)track_n & 0x7F  };
    //  MD.sendSysex(data, countof(data));
    MD.mapMidiNote(note, track_n);
  }

}

void init_notes() {
  for (uint8_t i = 0; i < 16; i++) {
    notes[i] = 0;
    // notes_off[i] = 0;
  }
}

/*
  void beat_repeat_clear () {
  MD.sendFXParam(3, 0, MD_SET_RHYTHM_ECHO_PARAM_ID);
          uint16_t quantize = 0;

  if (mixer_param3.getValue() == 0)  {  quantize = 0;  }
   else if (mixer_param3.getValue() < 7) { quantize = 1 << mixer_param3.getValue(); }
  MD.sendFXParam(0, quantize, MD_SET_RHYTHM_ECHO_PARAM_ID);
  //MD.sendFXParam(7, 0, MD_SET_RHYTHM_ECHO_PARAM_ID);
  // MD.sendFXParam(0, 127, MD_SET_RHYTHM_ECHO_PARAM_ID);

  }
  void beat_repeat_sample () {
  // for (uint8_t i = 0; i < 16; i++) {
  //    MD.muteTrack(i,false);
  //   }

  exploit_off();
  // delay(200);
    for (uint8_t i = 0; i < 16; i++) {
  setTrackParam(i,19,127);
    }
      exploit_on();
  MD.sendFXParam(3, 64, MD_SET_RHYTHM_ECHO_PARAM_ID);
  }

  void beat_repeat_stop_sample() {
    exploit_off();
       for (uint8_t i = 0; i < 16; i++) {
   setTrackParam(i,19,0);
       }
             exploit_on();
  }
  void beat_repeat_play() {
     for (int i = 0; i < 16; i++) {
     //  toggle_cue(i);

     MD.muteTrack(i,true);

    }
  MD.sendFXParam(7, 127, MD_SET_RHYTHM_ECHO_PARAM_ID);

  }
  void beat_repeat_stop_play() {
    MD.sendFXParam(7, 0, MD_SET_RHYTHM_ECHO_PARAM_ID);
         for (int i = 0; i < 16; i++) {
     //  toggle_cue(i);

     MD.muteTrack(i,false);

    }
  }
*/
void exploit_on() {
 // in_sysex = 1;
  exploit = 1;
  //last_md_track = MD.getCurrentTrack(CALLBACK_TIMEOUT);
  last_md_track = MD.currentTrack;
  MD.setStatus(0x22, 15);
  // MD.getBlockingGlobal(0);
  init_notes();
  exploit_start_clock = read_slowclock();
  noteproceed = 0;

  /*if (MidiClock.state == 2) {

    div16th_last = MidiClock.div16th_counter;
    noteproceed = 0;
    }
    else {
    noteproceed = 1;
    }
  */
  notecount = 0;
  //   global_new.baseChannel = 9;
  //  ElektronDataToSysexEncoder encoder(&MidiUart);
  //   global_new.toSysex(encoder);
  //    MD.setTempo(MidiClock.tempo);

  int flag = 0;

  //     if ((MidiClock.state == 2) && (MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
  if ((MidiClock.state == 2) && (MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
    flag = 1;
  }

  if (flag == 1) {
    MidiUart.m_putc_immediate(MIDI_STOP);
  }
  //   }
  MD.global.baseChannel = 4;
  switchGlobal(6);

  //     if ((MidiClock.state == 2) &&  (MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
  if (flag == 1) {
    MidiUart.m_putc_immediate(MIDI_CONTINUE);
  }
  //    }
  //  MD.getBlockingStatus(MD_CURRENT_GLOBAL_SLOT_REQUEST,200);
  collect_notes = true;
//in_sysex = 0;
}


void exploit_off() {
 // in_sysex = 1;
  exploit = 0;
  collect_notes = false;

  //
//  global_new.tempo = MidiClock.tempo;
  //   global_new.baseChannel = 3;
  //    ElektronDataToSysexEncoder encoder(&MidiUart);
  //   global_new.toSysex(encoder);
  int flag = 0;
  if ((MidiClock.state == 2) && (MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
    flag = 1;
  }
  if (flag == 1) {
    MidiUart.m_putc_immediate(MIDI_STOP);
  }
  //   }
  MD.global.baseChannel = 9;

  switchGlobal(7);
  //    if ((MidiClock.state == 2) && (MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
  if (flag == 1) {
    MidiUart.m_putc_immediate(MIDI_CONTINUE);
  }
  //   }

  MD.setStatus(0x22, last_md_track);
//in_sysex = 0;
}
static uint8_t turbomidi_sysex_header[] = {
  0xF0, 0x00, 0x20, 0x3c, 0x00, 0x00
};
uint32_t tmSpeeds[12] = {
  31250,
  31250,
  62500,
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
void sendTurbomidiHeader(uint8_t cmd) {
  MidiUart.puts(turbomidi_sysex_header, sizeof(turbomidi_sysex_header));
  MidiUart.m_putc(cmd);
}

void turboSetSpeed(uint8_t speed) {
  USE_LOCK();
  SET_LOCK();
  turbo_state = speed;
  sendTurbomidiHeader(0x20);
  MidiUart.m_putc(speed );
  MidiUart.m_putc(0xF7);
      CLEAR_LOCK();

  delay(10);
  MidiUart.setSpeed(tmSpeeds[speed ]);
  MidiUart.setActiveSenseTimer(290);
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
      if (strcmp(&entries[loadproj_param1.getValue()].name[size - 4], "mcl") == 0)  {



        char temp[size + 1];
        temp[0] = '/';
        m_strncpy(&temp[1], entries[loadproj_param1.getValue()].name, size);


        if (sd_load_project(temp)) {
          GUI.setPage(&page);
          curpage = 0;
        }
        else {
          GUI.flash_strings_fill("PROJECT ERROR", "NOT COMPATIBLE");
        }
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
        GUI.flash_strings_fill("SD FAILURE", "--");
        //  LCD.goLine(0);
        //LCD.puts("SD Failure");
      }

      return true;
    }
  }
  if ((curpage > 10) || (curpage == SEQ_STEP_PAGE)) {
    if ((EVENT_PRESSED(evt, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) || (EVENT_PRESSED(evt, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3) )) {
      for (uint8_t n = 0; n < 16; n++) {
        clear_seq_track(n);
      }


    }

  }

  if ((curpage == SEQ_PARAM_A_PAGE) && EVENT_RELEASED(evt, Buttons.BUTTON1)) {
    trackinfo_param1.cur = PatternLocksParams[last_md_track][2];
    trackinfo_param3.cur = PatternLocksParams[last_md_track][3];
    trackinfo_param2.cur = MD.kit.params[last_md_track][PatternLocksParams[last_md_track][2]];
    trackinfo_param4.cur = MD.kit.params[last_md_track][PatternLocksParams[last_md_track][3]];
    curpage = SEQ_PARAM_B_PAGE;
    return true;

  }
  if ((curpage == SEQ_PARAM_B_PAGE) && EVENT_RELEASED(evt, Buttons.BUTTON1)) {
    trackinfo_param1.cur = PatternLocksParams[last_md_track][0];
    trackinfo_param3.cur = PatternLocksParams[last_md_track][1];
    trackinfo_param2.cur = MD.kit.params[last_md_track][PatternLocksParams[last_md_track][0]];
    trackinfo_param4.cur = MD.kit.params[last_md_track][PatternLocksParams[last_md_track][1]];
    curpage = SEQ_PARAM_A_PAGE;
    return true;

  }

  if (((curpage == SEQ_PARAM_A_PAGE) || (curpage == SEQ_PARAM_B_PAGE) || (curpage == SEQ_RTRK_PAGE)) && EVENT_RELEASED(evt, Buttons.BUTTON4)) {
    clear_seq_locks(cur_col);
    return true;

  }
  if (((curpage == SEQ_RPTC_PAGE) || (curpage == SEQ_STEP_PAGE)) && EVENT_RELEASED(evt, Buttons.BUTTON4)) {
    clear_seq_track(last_md_track);
    return true;

  }
    if ((curpage == SEQ_STEP_PAGE) && EVENT_RELEASED(evt, Buttons.BUTTON1))  {
    trackinfo_param1.max = 64;
    trackinfo_param2.max = 64;
    trackinfo_param2.min = 0;
    trackinfo_param2.cur = 0;
    trackinfo_param3.max = 96;
    trackinfo_param4.max = 36;
    curpage = SEQ_EUC_PAGE;
    return true;
 
    }
    if ((curpage == SEQ_EUC_PAGE) && EVENT_RELEASED(evt, Buttons.BUTTON1))  {
    trackinfo_param1.max = 13;
    trackinfo_param2.max = 11;
    trackinfo_param2.min = 1;
    trackinfo_param2.cur = 6;
    trackinfo_param3.max = 64;
    trackinfo_param4.max = 16;
    curpage = SEQ_STEP_PAGE;
    return true;
 
    }
  if ((curpage == SEQ_PTC_PAGE) && EVENT_RELEASED(evt, Buttons.BUTTON1)) {
    curpage = SEQ_RPTC_PAGE;
    return true;

  }
    if ((curpage == SEQ_RPTC_PAGE) && EVENT_RELEASED(evt, Buttons.BUTTON1)) {
    curpage = SEQ_PTC_PAGE;
    return true;

  }
  if ((curpage == SEQ_RTRK_PAGE) && EVENT_RELEASED(evt, Buttons.BUTTON1)) {
    exploit_off();
    curpage = SEQ_RLCK_PAGE;
    return true;

  }
  if ((curpage == SEQ_RLCK_PAGE) && EVENT_RELEASED(evt, Buttons.BUTTON1)) {
    exploit_on();
    collect_notes = false;
    curpage = SEQ_RTRK_PAGE;
    return true;

  } 
  if ((curpage == SEQ_RTRK_PAGE) && EVENT_RELEASED(evt, Buttons.ENCODER1))  {
    exploit_off();

    GUI.setPage(&page);
    curpage = 0;

    return true;

  }
       if (( (curpage == SEQ_EUC_PAGE)) && EVENT_PRESSED(evt, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3) ) {
            random_pattern( trackinfo_param1.cur, PatternLengths[last_md_track], trackinfo_param2.cur,trackinfo_param4.cur,trackinfo_param3.cur);

  
    return true;
  }
 if (( (curpage == SEQ_EUC_PAGE)) && EVENT_PRESSED(evt, Buttons.BUTTON4) ) {
  
    setEuclid(last_md_track, trackinfo_param1.cur, PatternLengths[last_md_track], trackinfo_param2.cur,trackinfo_param4.cur,trackinfo_param3.cur);
    return true;
  }
  if (( (curpage == SEQ_STEP_PAGE)) && EVENT_PRESSED(evt, Buttons.BUTTON4) ) {
    clear_seq_track(last_md_track);
    return true;
  }
  if (((curpage == SEQ_PARAM_A_PAGE) || (curpage == SEQ_PARAM_B_PAGE)) && EVENT_PRESSED(evt, Buttons.BUTTON4) ) {
    clear_seq_track(cur_col);
    return true;
  }

  if (( (curpage == SEQ_RTRK_PAGE) || (curpage == SEQ_PARAM_A_PAGE) || (curpage == SEQ_RLCK_PAGE) || (curpage == SEQ_RPTC_PAGE) || (curpage == SEQ_STEP_PAGE) || (curpage == SEQ_PARAM_B_PAGE)) && EVENT_PRESSED(evt, Buttons.BUTTON2) ) {
    page_select = page_select + 1;
    if (page_select >= 4) {
      page_select = 0;
    }

    return true;

  }

  else if ((curpage == SEQ_STEP_PAGE) || (curpage > 10)) {

    if (EVENT_PRESSED(evt, Buttons.ENCODER1) || EVENT_PRESSED(evt, Buttons.ENCODER2) || EVENT_PRESSED(evt, Buttons.ENCODER3) || EVENT_PRESSED(evt, Buttons.ENCODER4)) {
      //  PatternMasks[i] = getPatternMask(i, enc->getValue(), 3, true);
      //  PatternLengths[i] = temptrack.length;
      exploit_off();

      GUI.setPage(&page);

      curpage = 0;
    }
    return true;
  }

  /*end if page == 1*/

  //PATTENRN LOAD
  else if ((curpage == S_PAGE) || (curpage == W_PAGE))   {

    //PRECISION READ

    if (EVENT_PRESSED(evt, Buttons.ENCODER1)) {
      notes[param1.getValue()] = 3;
      draw_notes(0);
      return true;
    }

    if (EVENT_PRESSED(evt, Buttons.ENCODER2))  {
      notes[param1.getValue() + 1] = 3;
      draw_notes(0);
      return true;
    }

    if (EVENT_PRESSED(evt, Buttons.ENCODER3)) {
      notes[param1.getValue() + 2] = 3;
      draw_notes(0);
      return true;
    }

    if (EVENT_PRESSED(evt, Buttons.ENCODER4)) {
      notes[param1.getValue() + 3] = 3;
      draw_notes(0);
      return true;
    }

    if (EVENT_RELEASED(evt, Buttons.BUTTON1) || EVENT_RELEASED(evt, Buttons.BUTTON4)) {
      exploit_off();
      GUI.setPage(&page);
      curpage = 0;
      return true;
    }

    if (curpage == S_PAGE) {




      if (EVENT_RELEASED(evt, Buttons.BUTTON2) ) {
        for (int i = 0; i < 16; i++) {

          notes[i] = 3;
        }
        exploit_off();
        store_tracks_in_mem(param1.getValue(), param2.getValue(), STORE_IN_PLACE);
        GUI.setPage(&page);
        curpage = 0;
        return true;
      }

    }



    if  ((EVENT_RELEASED(evt, Buttons.ENCODER1) || EVENT_RELEASED(evt, Buttons.ENCODER2) || EVENT_RELEASED(evt, Buttons.ENCODER3) || EVENT_RELEASED(evt, Buttons.ENCODER4))
        && ( BUTTON_UP(Buttons.ENCODER1) && BUTTON_UP(Buttons.ENCODER2) && BUTTON_UP(Buttons.ENCODER3) && BUTTON_UP(Buttons.ENCODER4) ))
    {
      if (curpage == W_PAGE) {
        // MD.getCurrentTrack(CALLBACK_TIMEOUT);
        int curtrack = last_md_track;
        //        int curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);

        exploit_off();
        write_original = 0;
        write_tracks_to_md( MD.currentTrack, param2.getValue(), 254);
        GUI.setPage(&page);
        curpage = 0;
        return true;
      }
      if (curpage == S_PAGE) {
        // MD.getCurrentPattern(CALLBACK_TIMEOUT);
        exploit_off();
        store_tracks_in_mem(param1.getValue(), param2.getValue(), STORE_AT_SPECIFIC);
        GUI.setPage(&page);
        curpage = 0;
        return true;

      }


    }
    if (curpage == W_PAGE) {

      if (EVENT_PRESSED(evt, Buttons.BUTTON3)) {
        for (int i = 0; i < 16; i++) {

          notes[i] = 3;
        }
        trackposition = TRUE;
        //   write_tracks_to_md(-1);
        exploit_off();
        write_original = 1;
        write_tracks_to_md( 0, param2.getValue(), 0);

        GUI.setPage(&page);
        curpage = 0;
        return true;
      }


    }

  }

  else if (curpage == 6) {

    if (EVENT_RELEASED(evt, Buttons.ENCODER1) || EVENT_RELEASED(evt, Buttons.ENCODER2) || EVENT_RELEASED(evt, Buttons.ENCODER3) || EVENT_RELEASED(evt, Buttons.ENCODER1)) {
      if (options_param1.getValue() == 0) {
        load_project_page();
        return true;
      }
      else if (options_param1.getValue() == 1) {
        new_project_page();
        return true;
      }


      MidiClock.stop();
      if (merge == 0) {

        MidiClock.mode = MidiClock.EXTERNAL_MIDI;
        MidiClock.transmit = false;

      }
      else {
        MidiClock.transmit = true;

        MidiClock.mode = MidiClock.EXTERNAL_UART2;
      }
      MidiClock.start();
      send_globals();
      delay(100);

      switchGlobal(7);
      if (turbo == 0) {
        //   if (turbo_state != 1) {
        turboSetSpeed(1);
        //  }
      }
      if (turbo == 1) {


        if (turbo_state != 4)  {
          //  TurboMidi.startTurboMidi();
          turboSetSpeed(4);
          //     }
          turbo_state = 4;
        }
        //turboSetSpeed(4);

      }
      if (turbo == 2) {
        //       if (turbo_state != 7) {
        turboSetSpeed(7);
        //    }
      }
      GUI.setPage(&page);
      curpage = 0;
      return true;


    }

  }

  else  if ((curpage == CUE_PAGE) || (curpage == MIXER_PAGE)) {
    /*
          if (curpage == MIXER_PAGE) {
                        if  (EVENT_PRESSED(evt, Buttons.ENCODER2)) {
              MD.sendFXParam(3, 0, MD_SET_RHYTHM_ECHO_PARAM_ID);
                        uint16_t quantize = 0;


             if (mixer_param3.getValue() > 0) { quantize = 1 << mixer_param3.getValue(); }
                 MD.sendFXParam(0, quantize, MD_SET_RHYTHM_ECHO_PARAM_ID);
                                       return true;
                        }
                          if  (EVENT_RELEASED(evt, Buttons.ENCODER2)) {
                MD.sendFXParam(3, 64, MD_SET_RHYTHM_ECHO_PARAM_ID);
                                         return true;
                          }
            if (EVENT_PRESSED(evt, Buttons.ENCODER3)) {
              beat_repeat_sample();
                         return true;
            }
            if  (EVENT_RELEASED(evt, Buttons.ENCODER3)) {
              beat_repeat_stop_sample();
                                       return true;
            }
             if  (EVENT_PRESSED(evt, Buttons.ENCODER4)) {
              beat_repeat_play();
                                       return true;
            }
           if  (EVENT_RELEASED(evt, Buttons.ENCODER4)) {
             beat_repeat_stop_play();
                                        return true;
            }

          }
    */
    if (curpage == MIXER_PAGE) {
      if (EVENT_PRESSED(evt, Buttons.ENCODER1)) {
        level_pressmode = 1;
      }
      if (EVENT_RELEASED(evt, Buttons.ENCODER1)) {
        level_pressmode = 0;
      }
    }
    if ((EVENT_PRESSED(evt, Buttons.BUTTON2)) || (EVENT_PRESSED(evt, Buttons.BUTTON3))) {
      exploit_off();
      GUI.setPage(&page);
      curpage = 0;
      return true;
    }
  }

  //HOME SCREEN -- GRID -- DEFAULT
  else if (curpage == 0) {
    //PRECISION WRITE



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
      MD.getCurrentTrack(CALLBACK_TIMEOUT);
      MD.getCurrentPattern(CALLBACK_TIMEOUT);
      patternload_param1.cur = (int) MD.currentPattern / (int) 16;
      patternload_param2.cur = MD.currentPattern - 16 * ((int) MD.currentPattern / (int) 16);
      exploit_on();


      curpage = S_PAGE;

      GUI.setPage(&patternload_page);


      return true;
    }

    //TRACK WRITE PAGE

    if (EVENT_RELEASED(evt, Buttons.BUTTON4)) {

      MD.getCurrentTrack(CALLBACK_TIMEOUT);
      MD.getCurrentPattern(CALLBACK_TIMEOUT);
      patternload_param1.cur = (int) MD.currentPattern / (int) 16;
      patternload_param2.cur = MD.currentPattern - 16 * ((int) MD.currentPattern / (int) 16);

      patternswitch = 1;
      currentkit_temp = MD.getCurrentKit(CALLBACK_TIMEOUT);
      patternload_param3.cur = currentkit_temp;
      MD.saveCurrentKit(currentkit_temp);

      //MD.requestKit(currentkit_temp);
      exploit_on();
      GUI.setPage(&patternload_page);
      // GUI.display();
      curpage = W_PAGE;
      return true;
    }
    if (BUTTON_DOWN(Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON3)) {
      clear_row(param2.getValue());
      return true;
    }

    if (BUTTON_DOWN(Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON4)) {
      setLed();
      int curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);

      param1.cur = curtrack;


      clearLed();
      return true;
    }


    if (EVENT_PRESSED(evt, Buttons.BUTTON3)) {
      exploit_on();
      curpage = CUE_PAGE;
      GUI.setPage(&trackinfo_page);
    }
    if (EVENT_PRESSED(evt, Buttons.BUTTON2)) {
      currentkit_temp = MD.getCurrentKit(CALLBACK_TIMEOUT);
      curpage = MIXER_PAGE;
      MD.saveCurrentKit(currentkit_temp);
      MD.getBlockingKit(currentkit_temp);
      level_pressmode = 0;
      mixer_param1.cur = 60;
      exploit_on();

      GUI.setPage(&mixer_page);
      //   draw_levels();
    }
    /*IF button1 and encoder buttons are pressed, store current track selected on MD into the corresponding Grid*/

    //  if (BUTTON_PRESSED(Buttons.BUTTON3)) {
    //      MD.getBlockingGlobal(1);
    //          MD.global.baseChannel = 9;
    //        //global_new.baseChannel;
    //          for (int i=0; i < 16; i++) {
    //            MD.muteTrack(i,true);
    //          }
    //           setLevel(8,100);
    //  }

    if (BUTTON_PRESSED(Buttons.ENCODER1))  {

      loadtrackinfo_page(0);



      return true;
    }
    if (BUTTON_PRESSED(Buttons.ENCODER2))  {


      loadtrackinfo_page(1);

      return true;



    }
    if (BUTTON_PRESSED(Buttons.ENCODER3))  {

      loadtrackinfo_page(2);



      return true;
    }
    if (BUTTON_PRESSED(Buttons.ENCODER4))  {

      loadtrackinfo_page(3);

      return true;
    }


  }

}
