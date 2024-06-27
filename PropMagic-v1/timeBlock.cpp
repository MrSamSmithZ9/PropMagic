#include "timeBlock.h"

    timeBlock::timeBlock()
    {
      instantiated = false;
    }

    void timeBlock::instantiate(String sCode, uint8_t mappedPin, uint8_t type)
    {

      DeserializationError error = deserializeJson(doc, sCode);

      uniqueID = doc["uniqueID"];
      assignedRow = doc["assignedRow"];
      
      assignedType = type;

      startTime = doc["sTime"];
      endTime = doc["eTime"];

      assignedPin = mappedPin;
      //Serial.println(mappedPin);
      saveCode = sCode;

      enabled = false;
      available = true;
      instantiated = true;

      Serial.println("Assigned Row:" + String(assignedRow) + " Pin:"+String(assignedPin));
    }

    void timeBlock::destantiate()
    {
        instantiated = false;
    }

    void timeBlock::loadProfile(uint8_t keyframe, bool state)
    {

    }

    void timeBlock::processShowtime(double showtime)
    {
      if(showtime >= startTime && showtime <= endTime)
      {
        updateState(ON);
        
      }
      else
      {
        updateState(OFF);
        
      }
    }

    void timeBlock::updateState(bool state)
    {
      if(available)
      {
        if(state == true)
        {
          if(!enabled)
          {
            //Serial.println("Row On: "+ String(id) + ":" + String(assignedPin));
            digitalWrite(assignedPin, HIGH);
            enabled = true;
          }
        }
        else
        {
          if(enabled)
          {
            //Serial.println("Row Off: "+ String(id) + ":" + String(assignedPin));
            digitalWrite(assignedPin, LOW);
            enabled = false;
          }
        }
      }
    }

    void timeBlock::updateRow(uint8_t rID, uint8_t type, uint8_t mappedPin)
    {
        assignedRow = rID;
        assignedType = type;
        assignedPin = mappedPin;

        String newCode = "b:";
        newCode += addLeadingZeros(uniqueID);
        newCode += ":";
        newCode += assignedRow;
        newCode += ":";
        newCode += addLeadingZeros(startTime);
        newCode += ":";
        newCode += addLeadingZeros(endTime);
        newCode += ":";
        
        saveCode = newCode;

        Serial.println("Block Updated: " + saveCode);
    }

    String timeBlock::addLeadingZeros(int i)
    {
        String str = "";

        if(i < 1000)
        {
          str += "0";
        }

        if(i < 100)
        {
          str += "0";
        }

        if(i < 10)
        {
          str += "0";
        }

        return str + String(i);
    }

    void timeBlock::changeInstruction(String sCode, uint8_t mappedPin, uint8_t type)
    {
      assignedRow = sCode.substring(7,8).toInt();
      assignedType = type;

      startTime = sCode.substring(9,13).toInt();
      endTime = sCode.substring(14,18).toInt();

      assignedPin = mappedPin;

      saveCode = sCode;

      Serial.println("Block Change:" + saveCode);
    }

    void timeBlock::flipInstruction(uint8_t keyframe)
    {
    
    }

    void timeBlock::enable()
    {
      available = true;
    }

    void timeBlock::disable()
    {
      available = false;
    }

    String timeBlock::getSaveCode()
    {
      return saveCode;
    }

    uint8_t timeBlock::getID()
    {
      return uniqueID;
    }

    uint8_t timeBlock::getRow()
    {
      return assignedRow;
    }


    bool timeBlock::isEnabled()
    {
      return enabled;
    }

    bool timeBlock::isAvailable()
    {
      return available;
    }

    bool timeBlock::isInstantiated()
    {
      return instantiated;
    }