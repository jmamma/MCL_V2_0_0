
class SnifferSysexListenerClass : public MidiSysexListenerClass {
    /** 
     * \addtogroup midi_turbomidi
     *
     * @{
     **/
public:
    SnifferSysexListenerClass();

    bool isGenericMessage;
    
    virtual void handleByte(uint8_t byte);
    virtual void start() { isGenericMessage = false; }
    virtual void end();

    void    setup() {
        MidiSysex.addSysexListener(this);
    }
 };
