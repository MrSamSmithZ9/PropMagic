#include "rowController.h"

    rowController::rowController()
    {
      instantiated = false;
     
    }

    void rowController::instantiate(String inc)
    {
      JsonDocument incomingJSON;
      deserializeJson(incomingJSON, inc);

      uniqueID = incomingJSON["uniqueID"];
      assignedPin = incomingJSON["assignedPin"];
      assignedType = incomingJSON["assignedType"];
      String sd = incomingJSON["name"];
      description = sd;
      saveCode = inc;
      instantiated = true;
      //Serial.print(assignedPin);
      pinMode(assignedPin, OUTPUT);
      digitalWrite(assignedPin, LOW);

      Serial.println("RC Instantiate: " + String(uniqueID) + " Mapped Pin:" + String(assignedPin));
      Serial.println(saveCode);
    }

    void rowController::update(String inc)
    {
      JsonDocument incomingJSON;
      deserializeJson(incomingJSON, inc);

      uniqueID = incomingJSON["uniqueID"];
      assignedPin = incomingJSON["assignedPin"];
      assignedType = incomingJSON["assignedType"];
      
      String sd = incomingJSON["name"];
      description = sd;

      digitalWrite(assignedPin, LOW);
      saveCode = inc;
      
      Serial.println("RC Update: " + String(uniqueID) + " Mapped Pin:" + String(assignedPin));
      Serial.println(saveCode);

      pinMode(assignedPin, OUTPUT);
      digitalWrite(assignedPin, LOW);
    }

    void rowController::destantiate()
    {
        instantiated = false;
    }

    void rowController::enable()
    {
      enabled = true;
    }

    void rowController::disable()
    {
      enabled = false;
    }

    void rowController::toggle()
    {
      enabled = !enabled;
    }

    void rowController::moveRow(uint8_t new_rID)
    {
      Serial.println("Row moved from:" + String(uniqueID) + " to:"+String(new_rID));
        uniqueID = new_rID;
    }

    String rowController::getDescription()
    {
      return description;
    }

    String rowController::getSaveCode()
    {
      return saveCode;
    }

    uint8_t rowController::getID()
    {
      return uniqueID;
    }

    uint8_t rowController::getPin()
    {
      return assignedPin;
    }

    uint8_t rowController::getType()
    {
      return assignedType;
    }

    bool rowController::isEnabled()
    {
      return enabled;
    }

    bool rowController::isInstantiated()
    {
      return instantiated;
    }