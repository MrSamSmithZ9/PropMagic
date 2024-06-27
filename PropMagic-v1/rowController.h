#include "Arduino.h"
#include <ArduinoJson.h>

#define ON true
#define OFF false
#define OUT_1 D1
#define OUT_2 D2

#define TRIGGER A0
#define AUX_1 D3
#define AUX_2 D8

class rowController
{
  uint8_t uniqueID;
  uint8_t assignedType;
  uint8_t assignedPin;

  uint8_t color_r;
  uint8_t color_g;
  uint8_t color_b;

  String description;
  String saveCode;

  bool enabled;
  bool instantiated;

  public:
    rowController();

    void instantiate(String inc);

    void destantiate();

    void update(String inc);

    void enable();

    void disable();

    void toggle();

    void moveRow(uint8_t new_rID);

    String getDescription();

    String getSaveCode();

    uint8_t getID();

    uint8_t getPin();

    uint8_t getType();

    bool isEnabled();

    bool isInstantiated();

};