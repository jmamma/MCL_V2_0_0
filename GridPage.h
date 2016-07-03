  /* 
 ================
 file: GridEncoders.h
 ================
  This file handles the GridGUI.
  GridEncoder refers to a Grid/Track in the Grid. 
  The Classes/Methods in here are responsible for drawing the Grid on screen and handling encoder movement to navigate and animate the grid.
  
  The GridEncoderPage class extends the Page class in the MIDICTRL Framework;
  It overwrites its display methods so that we can draw a customized GUI to the Minicommand screen.
  
*/


/** Store the name of a machinedrum machine. **/
bool scroll_fastmode = FALSE;
uint8_t dispeffect = 0;

typedef struct md_machine_name_s_short {
  char name1[3];
  char name2[3];
  uint8_t id;
} md_machine_name_t_short;


//Data structure for defining shorter names for the MD Machines

md_machine_name_t_short const machine_names_short[134] PROGMEM = {
  { "GN","", 0},
  { "GN", "SN", 1},
  { "GN", "NS", 2},
  { "GN", "IM", 3},
  { "TR", "BD", 16},
  { "TR", "SD", 17},
  { "TR", "XT", 18},
  { "TR", "CP", 19},
  { "TR", "RS", 20},
  { "TR", "RS", 21},
  { "TR", "CH", 22},
  { "TR", "OH", 23},
  { "TR", "CY", 24},
  { "TR", "MA", 25},
  { "TR", "CL", 26},
  { "TR", "XC", 27},
  { "TR", "B2", 28},
  { "FM", "BD", 32},
  { "FM", "SD", 33},
  { "FM", "XT", 34},
  { "FM", "CP", 35},
  { "FM", "RS", 36},
  { "FM", "CB", 37},
  { "FM", "HH", 38},
  { "FM", "CY", 39},
  { "E2", "BD", 48},
  { "E2", "SD", 49},
  { "E2", "HT", 50},
  { "E2", "LT", 51},
  { "E2", "CP", 52},
  { "E2", "RS", 53},
  { "E2", "CB", 54},
  { "E2", "CH", 55},
  { "E2", "OH", 56},
  { "E2", "RC", 57},
  { "E2", "CC", 58},
  { "E2", "BR", 59},
  { "E2", "TA", 60},
  { "E2", "TR", 61},
  { "E2", "SH", 62},
  { "E2", "BC", 63},
  { "PI", "BD", 64},
  { "PI", "SD", 65},
  { "PI", "MT", 66},
  { "PI", "ML", 67},
  { "PI", "MA", 68},
  { "PI", "RS", 69},
  { "PI", "RC", 70},
  { "PI", "CC", 71},
  { "PI", "HH", 72},
  { "IN", "GA", 80},
  { "IN", "GB", 81},
  { "IN", "FA", 82},
  { "IN", "FB", 83},
  { "IN", "EA", 84},
  { "IN", "EB", 85},
  { "MI", "01", 96},
  { "MI", "02", 97},
  { "MI", "03", 98},
  { "MI", "04", 99},
  { "MI", "05", 100},
  { "MI", "06", 101},
  { "MI", "07", 102},
  { "MI", "08", 103},
  { "MI", "09", 104},
  { "MI", "10", 105},
  { "MI", "11", 106},
  { "MI", "12", 107},
  { "MI", "13", 108},
  { "MI", "14", 109},
  { "MI", "15", 110},
  { "MI", "16", 111},
  { "CT", "AL", 112},
  { "CT", "8P", 113},
  { "CT", "RE", 120},
  { "CT", "GB", 121},
  { "CT", "EQ", 122},
  { "CT", "DX", 123},
  { "RO", "01", 128},
  { "RO", "02", 129},
  { "RO", "03", 130},
  { "RO", "04", 131},
  { "RO", "05", 132},
  { "RO", "06", 133},
  { "RO", "07", 134},
  { "RO", "08", 135},
  { "RO", "09", 136},
  { "RO", "10", 137},
  { "RO", "11", 138},
  { "RO", "12", 139},
  { "RO", "13", 140},
  { "RO", "14", 141},
  { "RO", "15", 142},
  { "RO", "16", 143},
  { "RO", "17", 144},
  { "RO", "18", 145},
  { "RO", "19", 146},
  { "RO", "20", 147},
  { "RO", "21", 148},
  { "RO", "22", 149},
  { "RO", "23", 150},
  { "RO", "24", 151},
  { "RO", "25", 152},
  { "RO", "26", 153},
  { "RO", "27", 154},
  { "RO", "28", 155},
  { "RO", "29", 156},
  { "RO", "30", 157},
  { "RO", "31", 158},
  { "RO", "32", 159},
  { "RA", "R1", 160},
  { "RA", "R2", 161},
  { "RA", "P1", 162},
  { "RA", "P2", 163},
  { "RA", "R3", 165},
  { "RA", "R4", 166},
  { "RA", "P3", 167},
  { "RA", "P4", 168},
  { "RO", "33", 176},
  { "RO", "34", 177},
  { "RO", "35", 178},
  { "RO", "36", 179},
  { "RO", "37", 180},
  { "RO", "38", 181},
  { "RO", "39", 182},
  { "RO", "40", 183},
  { "RO", "41", 184},
  { "RO", "42", 185},
  { "RO", "43", 186},
  { "RO", "44", 187},
  { "RO", "45", 188},
  { "RO", "46", 189},
  { "RO", "47", 190},
  { "RO", "48", 191}
};
uint32_t getGridModel(int i);

PGM_P getMachineNameShort(uint8_t machine,uint8_t type) {

  if (machine == NULL) { return NULL; } 

  else {
  for (uint8_t i = 0; i < countof(machine_names_short); i++) {
    if (pgm_read_byte(&machine_names_short[i].id) == machine) {
      if (type == 1) {
      return machine_names_short[i].name1;
      }
      else {
              return machine_names_short[i].name2;
      }
      
   }
  }
  return NULL;
  }
}


class GridEncoder : public Encoder {
	/**
	 * \addtogroup gui_rangeencoder_class
	 * @{
	 **/
	
public:
	/** Minimum value of the encoder. **/
	int min;
	/** Maximum value of the encoder. **/
	int max;

        uint8_t fxparam;
        uint8_t effect;
        uint8_t num;
	
	/**
	 * Create a new range-limited encoder with max and min value, short
	 * name, initial value, and handling function. The initRangeEncoder
	 * will be called with the constructor arguments.
	 **/
	GridEncoder(int _max = 127, int _min = 0, const char *_name = NULL, int init = 0, encoder_handle_t _handler = NULL) : Encoder(_name, _handler) {
		initGridEncoder(_max, _min, _name, init, _handler);
	}
	
	/**
	 * Initialize the encoder with the same argument as the constructor.
	 *
	 * The initRangeEncoder functions automatically determines which of
	 * min and max is the minimum value. As of now this can't be used to
	 * have an "inverted" encoder.
	 *
	 * The initial value is called without calling the handling function.
	 **/
	void initGridEncoder(int _max = 128, int _min = 0, const char *_name = NULL, int init = 0,
						  encoder_handle_t _handler = NULL) {
		setName(_name);
		handler = _handler;
		if (_min > _max) {
			min = _max;
			max = _min;
		} else {
			min = _min;
			max = _max;
		}
		setValue(init);
	}
	
	/**
	 * Update the value of the encoder according to pressmode and
	 * fastmode, and limit the resulting value using limit_value().
	 **/
	virtual int update(encoder_t *enc);
        virtual void displayAt(int i);

	/* @} */
};

/* 
   ==============
   class: GridEncoderPage
   ==============

    The GridEncoderPage class extends the Page class in the MIDICTRL Framework;
    It overwrites its display methods so that we can draw a customized GUI to the Minicommand screen.
  
  
*/

class GridEncoderPage : public Page {
  /**
   * \addtogroup gui_encoder_page
   * @{
   **/
	
public:
  Encoder *encoders[GUI_NUM_ENCODERS];
  /**
   * Create an EncoderPage with one or more encoders. The argument
   * order determines which encoder will be used. Unused encoders can
   * be passed as NULL pointers.
   **/
  GridEncoderPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) {
    setEncoders(e1, e2, e3, e4);
  }

  /**
   * Set the encoders used by the page later on.
   **/
  void setEncoders(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) {
    encoders[0] = e1;
    encoders[1] = e2;
    encoders[2] = e3;
    encoders[3] = e4;
  }
  /** This method will update the encoders according to the hardware moves. **/
  virtual void update();
  /** This will clear the encoder movements. **/
  virtual void clear();
  /** Display the encoders using their short name and their value (as base 10). **/
  virtual void display();
  /** Executes the encoder actions by calling checkHandle() on each encoder. **/
  virtual void finalize();

  /** Call this to lock all encoders in the page. **/
  void lockEncoders();
  /** Call this to unlock all encoders in the page. If their value
      changed while locked, they will send out their new value.
  **/
  void unlockEncoders();

  /**
   * Used to display the names of the encoders on its own (useful if
   * the encoders can update their name, for example when
   * autolearning.
   **/


  void displayNames();
  // uint32_t getGridModel(int i);
  /* @} */
  void displayScroll(uint8_t i);
};


void GridEncoderPage::update() {
  encoder_t _encoders[GUI_NUM_ENCODERS];

  USE_LOCK();
  SET_LOCK();
  m_memcpy(_encoders, Encoders.encoders, sizeof(_encoders));
  Encoders.clearEncoders();
  CLEAR_LOCK();
  
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] != NULL) 
      encoders[i]->update(_encoders + i);
  }

}

void GridEncoderPage::clear() {
   for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] != NULL)
      encoders[i]->clear();
  }
  }



/* 
   ==============
   method: displayScroll(uint8_t i)
   ==============

   The displayScroll method draws the GRID on the Minicommand Screen.
   
*/


void GridEncoderPage::displayScroll(uint8_t i) {
    if (encoders[i] != NULL) {

       
          if   (((encoders[0]->getValue() +i +1) % 4) == 0) {
                         char strn[2] = "I";
                         strn[0] = (char) 000; 
                          //           strn[0] = (char) 219; 
                         GUI.setLine(GUI.LINE1);

                         GUI.put_string_at_noterminator((2 + (i * 3)),strn); 
          
                          GUI.setLine(GUI.LINE2);
                          GUI.put_string_at_noterminator((2 + (i * 3)),strn);
          }
              
          else {
              char strn_scroll[2] = "|";
             GUI.setLine(GUI.LINE1);
  // if (((encoders[1]->getValue() + 2) % 3) == 0) {

                   GUI.put_string_at_noterminator((2 + (i * 3)),strn_scroll);
                   
  // }
    // else { strn_scroll[0] = (char) 001; GUI.put_string_at((2 + (i * 3)),strn_scroll); }
          GUI.setLine(GUI.LINE2);
   //   if (((encoders[1]->getValue() + 3) % 3) == 0) {
  // if (encoders[i]->getValue() & 1) {
       GUI.setLine(GUI.LINE2);
                                GUI.put_string_at_noterminator((2 + (i * 3)),strn_scroll);
  // }
      }
             
          }
 }
void GridEncoderPage::finalize() {
    for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] != NULL) 
      encoders[i]->checkHandle();
  }
}

void GridEncoderPage::displayNames() {
}



/*
  ===================
  method:  GridEncoderPage::display()
  ===================
 
  This method is implemented in mcl.pde due to compilation errors.
  
  */
  
 // void GridEncoderPage::display() 
 
 /*
  ===================
  method:  GridEncoder::displayAt(int i)
  ===================
 
  This method is implemented in mcl.pde due to compilation errors.
  
  */
  
//void GridEncoder::displayAt(int i) {
