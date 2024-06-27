#include "Arduino.h"
#include <ArduinoJson.h>

#define ON true
#define OFF false
#define OUT_1 D1
#define OUT_2 D2

#define TRIGGER A0
#define AUX_1 D3
#define AUX_2 D8

class timeBlock
{
  uint8_t uniqueID;
  uint8_t assignedRow;
  uint8_t assignedType;
  uint8_t assignedPin;
  int startTime;
  int endTime;
  int pwmStart;
  int pwmEnd;
  uint8_t r_value;
  uint8_t g_value;
  uint8_t b_value;
  String saveCode;
  bool enabled;
  bool available;
  bool instantiated;

  JsonDocument doc;

  public:
    timeBlock();

    void instantiate(String sCode, uint8_t mappedPin, uint8_t type);

    void destantiate();

    void loadProfile(uint8_t keyframe, bool state);

    void updateState(bool state);

    void processShowtime(double showtime);

    void changeInstruction(String sCode, uint8_t mappedPin, uint8_t type);

    void flipInstruction(uint8_t keyframe);

    void enable();

    void disable();

    void updateRow(uint8_t rID, uint8_t type, uint8_t mappedPin);

    String addLeadingZeros(int i);

    String getSaveCode();

    uint8_t getID();

    uint8_t getRow();

    bool isEnabled();

    bool isAvailable();

    bool isInstantiated();

};