
class TrackInfoEncoder : public Encoder {
	/**
	 * \addtogroup gui_rangeencoder_class
	 * @{
	 **/
	
public:
	/** Minimum value of the encoder. **/
	int min;
	/** Maximum value of the encoder. **/
	int max;
	
	/**
	 * Create a new range-limited encoder with max and min value, short
	 * name, initial value, and handling function. The initRangeEncoder
	 * will be called with the constructor arguments.
	 **/
	TrackInfoEncoder(int _max = 127, int _min = 0, const char *_name = NULL, int init = 0, encoder_handle_t _handler = NULL) : Encoder(_name, _handler) {
		initTrackInfoEncoder(_max, _min, _name, init, _handler);
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
	void initTrackInfoEncoder(int _max = 128, int _min = 0, const char *_name = NULL, int init = 0,
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



class TrackInfoPage : public Page {
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
  TrackInfoPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) {
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
  // uint32_t getSlotModel(int i);
  /* @} */

};


void TrackInfoPage::update() {
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

void TrackInfoPage ::clear() {
     for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] != NULL)
      encoders[i]->clear();
  }
}



void TrackInfoPage::finalize() {
}

void TrackInfoPage::displayNames() {
}

void TrackInfoPage::display() {
  

             encoders[0]->displayAt(encoders[0]->getValue());                    
        
       

}


int TrackInfoEncoder::update(encoder_t *enc) {
	//int inc = 8;

	//cur = limit_value(cur, inc, min, max);

        
	int inc = enc->normal;
        //int inc = 4 + (pressmode ? 0 : (fastmode ? 5 * enc->button : enc->button));
	cur = limit_value(cur, inc, min, max);

	return cur;
}


