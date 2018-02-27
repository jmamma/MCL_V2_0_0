/*To enable serial port debugging,
   Uncomment DEBUG_MCL line below
*/
#define DEBUG_MCL 1

#ifdef DEBUG_MCL
#define DEBUG_PRINT(x)  Serial.print(x)
#define DEBUG_PRINTLN(x)  Serial.println(x)

#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif
//

#define SD_MAX_RETRIES 5 /* Number of SD card read/write retries before return failure */

//RELEASE 1 BYTE/STABLE-BETA 1 BYTE /REVISION 2 BYTES

#define VERSION 2013
#define CONFIG_VERSION 2012

#define LOCK_AMOUNT 256

#define GRID_LENGTH 130
#define GRID_WIDTH 22
#define GRID_SLOT_BYTES 4096
#define CALLBACK_TIMEOUT 500
#define GUI_NAME_TIMEOUT 800

#define CUE_PAGE 5
#define NEW_PROJECT_PAGE 7
#define MIXER_PAGE 10
#define S_PAGE 3
#define W_PAGE 4
#define SEQ_STEP_PAGE 1
#define SEQ_EXTSTEP_PAGE 18
#define SEQ_EUC_PAGE 20
#define SEQ_EUCPTC_PAGE 21
#define SEQ_RLCK_PAGE 13
#define SEQ_RTRK_PAGE 11
#define SEQ_RPTC_PAGE 14
#define SEQ_PARAM_A_PAGE 12
#define SEQ_PARAM_B_PAGE 15
#define SEQ_PTC_PAGE 16
#define SEQ_NOTEBUF_SIZE 8
#define EXPLOIT_DELAY_TIME 350
#define TRIG_HOLD_TIME 200

#define OLED_CLK 52
#define OLED_MOSI 51

// Used for software or hardware SPI
#define OLED_CS 42
#define OLED_DC 44

// Used for I2C or SPI
#define OLED_RESET 38

#define A4_TRACK_TYPE 2
#define MD_TRACK_TYPE 1
#define EXT_TRACK_TYPE 3

#define EMPTY_TRACK_TYPE 0
#define DEVICE_NULL 0
#define DEVICE_MIDI 128
#define DEVICE_MD 2
#define DEVICE_A4 6

//Encoder rotation resolution for different pages.

#define ENCODER_RES_GRID 2
#define ENCODER_RES_SEQ 2
#define ENCODER_RES_SYS 2
#define ENCODER_RES_PAT 2

#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

#define PATTERN_STORE 0
#define PATTERN_UDEF 254
#define STORE_IN_PLACE 0
#define STORE_AT_SPECIFIC 254
