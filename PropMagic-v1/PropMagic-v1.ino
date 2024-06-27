#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>
#include "FS.h"
#include "SPIFFS.h"
#include "timeBlock.h"
#include "rowController.h"
#include "ESP32_NOW.h"
#include "WiFi.h"

#include <ArduinoJson.h>

#include <esp_mac.h>  // For the MAC2STR and MACSTR macros


using namespace std;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define FORMAT_SPIFFS_IF_FAILED true
#define OUT_1 D1
#define OUT_2 D2

#define TRIGGER A0
#define AUX_1 D3
#define AUX_2 D8

//Adafruit_NeoPixel pixels(7, D3, NEO_GRB + NEO_KHZ800);

uint8_t *pixelBuffer = NULL;
uint8_t width = 0;
uint8_t height = 0;
uint8_t stride;
uint8_t componentsValue;
bool is400Hz;
uint8_t components = 3;     // only 3 and 4 are valid values

BLECharacteristic *pCharacteristic;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;

String txValue;

bool readyForTransmit = false;

bool flash_bit = false;
unsigned long watchdog_timeout = 0;

bool dataReceived = false;
String incomingData = "";

vector<timeBlock*>  blockList;
vector<rowController*>  outputList;

bool playbackInProgress = false;
double currentShowtime = 0;
double startMillis = 0;
double endTime = 600;

String deviceName = "PropMagic x2";
bool configurationChanged = false;

long triggerMillis = 0;

String outgoingString;

//----------------------------------------------------------------
//---- Bluetooth Callback Classes --------------------------------
//----------------------------------------------------------------
class MyServerCallbacks: public BLEServerCallbacks 
{
    void onConnect(BLEServer* pServer) 
    {
      deviceConnected = true;
      //digitalWrite(LED_BUILTIN, LOW);
    };

    void onDisconnect(BLEServer* pServer) 
    {
      deviceConnected = false;
      //digitalWrite(LED_BUILTIN, HIGH);
      //ESP.restart();
    }
};

class MyCallbacks: public BLECharacteristicCallbacks 
{
    void onWrite(BLECharacteristic *pCharacteristic) 
    {
      String rxValue = pCharacteristic->getValue();
      
      if (rxValue.length() > 0) 
      {
        readyForTransmit = false;
        if(dataReceived)
        {
          Serial.println("Buffer Full");
        }
        dataReceived = true;
        incomingData = rxValue;       
      }
    }
};
//------------------------------------------------------------------
//------------------------------------------------------------------
//------------------------------------------------------------------


void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("System Booting...");

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) 
  {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  listDir(SPIFFS, "/", 0);
  Serial.println("Configuration Load...");


  if(!loadConfigurationData())
  {
    saveConfigurationData();
  }
  Serial.println("Timeline Load...");
  loadSavedOutputList();
  loadSavedBlockList();

  pinMode(OUT_1, OUTPUT);
  pinMode(OUT_2, OUTPUT);
  pinMode(TRIGGER, INPUT_PULLDOWN);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(AUX_1, OUTPUT);
  pinMode(AUX_2, OUTPUT);

  digitalWrite(AUX_1, LOW);
  digitalWrite(AUX_2, LOW);

  digitalWrite(LED_BUILTIN, LOW);

  Serial.println("System Online");
  Serial.println("ID:" + deviceName);
  
  // Create the BLE Device
  BLEDevice::init(deviceName);
  delay(500);
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
										CHARACTERISTIC_UUID_TX,
										BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
									);   
                  
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX,
											BLECharacteristic::PROPERTY_WRITE
										);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();
  delay(500);
  // Start advertising
  pServer->getAdvertising()->start();

}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) 
{
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void BLE_loop() 
{
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        //Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
        digitalWrite(LED_BUILTIN, LOW);
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
		// do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

//--------------------------------------------------------------
//--------------------------------------------------------------

void updateSavedBlocks()
{
    File file = SPIFFS.open("/blockList.txt", FILE_WRITE);
    if (!file) 
    {
      Serial.println("- failed to open file for writing");
      return;
    }

    for (timeBlock * tb : blockList) 
    {
      file.println(tb->getSaveCode());     
    }

    file.close();
}

void updateSavedOutputs()
{
    File file = SPIFFS.open("/outputList.txt", FILE_WRITE);
    if (!file) 
    {
      Serial.println("- failed to open file for writing");
      return;
    }

    for (rowController * rc : outputList) 
    {
      file.println(rc->getSaveCode());     
    }

    file.close();
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------


void loadSavedBlockList()
{
  File file = SPIFFS.open("/blockList.txt");
  if (!file || file.isDirectory()) 
  {
    Serial.println("- failed to open file for reading");
    return;
  }

  vector<String> v;
  while (file.available()) 
  {
    v.push_back(file.readStringUntil('\n'));
  }
  file.close();

  for (String s : v) 
  {
    processBlockRequest(s, false);
  }
}

void loadSavedOutputList()
{
  File file = SPIFFS.open("/outputList.txt");
  if (!file || file.isDirectory()) 
  {
    Serial.println("- failed to open file for reading");
    return;
  }

  vector<String> v;
  while (file.available()) 
  {
    v.push_back(file.readStringUntil('\n'));
  }
  file.close();

  for (String s : v) 
  {
    processOutputRequest(s, false);
  }
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------


bool loadConfigurationData()
{
  File file = SPIFFS.open("/config.txt");
  if (!file || file.isDirectory()) 
  {
    Serial.println("Unable to load config, defaulting...");
    return false;
  }

  vector<String> v;
  while (file.available()) 
  {
    v.push_back(file.readStringUntil('\n'));
  }
  file.close();
  Serial.println("reading config");
  for (String s : v) 
  {

    JsonDocument incomingJSON;
    deserializeJson(incomingJSON, s);
    
    dataReceived = false;
    const char* request = incomingJSON["request"];
    //int uID = incomingJSON["uniqueID"];
    //int sTime = incomingJSON["time"][0];
    //int eTime = incomingJSON["time"][1];

    //-- Add or Edit Time Block --
    if(strcmp(request,"device option") == 0)
    {
      processDeviceOption(s, false);
    }

    if(strcmp(request,"end marker") == 0)
    {
      double newEnd = incomingJSON["frameID"];
      processTimelineInfo(newEnd, false);
    }
  }
  return true;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void saveConfigurationData()
{
    File file = SPIFFS.open("/config.txt", FILE_WRITE);
    if (!file) 
    {
      Serial.println("- failed to open file for writing");
      return;
    }

    JsonDocument nameJSON;
    nameJSON["request"] = "device option";
    nameJSON["name"] = deviceName;

    String outgoingString = "";
    serializeJson(nameJSON, outgoingString);
    file.println(outgoingString);

    JsonDocument markerJSON;
    markerJSON["request"] = "end marker";
    markerJSON["frameID"] = endTime;

    outgoingString = "";
    serializeJson(markerJSON, outgoingString);
    file.println(outgoingString);

    file.close();
    Serial.println("Configuration Data Updated");
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

void startPlayback(String inc)
{
  JsonDocument incomingJSON;
  deserializeJson(incomingJSON, inc);
  //Serial.println("Start Request @:"+inc.substring(2));
  int playTime = incomingJSON["frameID"];
  playbackInProgress = true;
  startMillis = millis() - playTime * 10;
  currentShowtime = 0;

}

void startPlayback()
{

  playbackInProgress = true;
  startMillis = millis();
  currentShowtime = 0;

}

void stopPlayback()
{
  Serial.println("Stop Request");
  playbackInProgress = false;
  currentShowtime = 0;

  for (timeBlock * tb : blockList) 
  {
    tb->updateState(OFF);
  }
}

void freezeFrame(String inc)
{
    double showtime = inc.substring(2).toDouble();
    Serial.println("Freeze frame received " +String(showtime));

    for (timeBlock * tb : blockList) 
    {
      tb->processShowtime(showtime);
    }
}

void processCurrentFrame(double showtime)
{
    for (timeBlock * tb : blockList) 
    {
      tb->processShowtime(showtime);
    }
}

void processTimelineInfo(double newTime, bool save)
{
  endTime = newTime;
  if(save)
  {
    saveConfigurationData();
  }
}

void processDeviceOption(String inc, bool save)
{
  JsonDocument incomingJSON;
  deserializeJson(incomingJSON, inc);

  //int uID = incomingJSON["uniqueID"];
  //int sTime = incomingJSON["time"][0];
  //int eTime = incomingJSON["time"][1];

  //-- Add or Edit Time Block --
  if(incomingJSON.containsKey("name"))
  {
    changeDeviceName(incomingJSON["name"]);
  }

  if(save)
  {
    saveConfigurationData();
    ESP.restart();
  }
}

void factoryReset()
{
    Serial.println("Factory Reset in Progress");
    SPIFFS.format();
    Serial.println("Format Complete, Rebooting in 3...2...1..");
    delay(1000);
    ESP.restart();
}

void changeDeviceName(String inc)
{
  if(deviceName != inc)
  {
    deviceName = inc;
    Serial.println("Device Name Changed: " + deviceName);
  }
}

void processOutputRequest(String inc, bool save)
{
  JsonDocument incomingJSON;
  deserializeJson(incomingJSON, inc);

  int uID = incomingJSON["uniqueID"];

  bool outputExists = false;

  rowController * rc = findOutput(uID);

  if(!outputExists)
  {
      rowController * newOutput = new rowController();
      newOutput->instantiate(inc);
      outputList.push_back(newOutput);
  }
  else
  {
    rc->update(inc);
  }

  if(save)
  {
    updateSavedOutputs();
  }
}

void clearRow(uint8_t rID)
{
  int iterator = 0;
  for(timeBlock * tb : blockList)
  {
    Serial.println("tbR:"+String(tb->getRow()) + " rID:"+String(rID));
    if(tb->getRow() == rID)
    {
      blockList.erase(blockList.begin()+iterator);
      Serial.println("Block Erased");
      iterator += -1;
    }
    iterator++;
  }
}

void processRowUpdate(uint8_t rID, uint8_t newPin)
{
  rowController * rc = findOutput(rID);

  for (timeBlock * tb : blockList) 
  {
    if(tb->getRow() == rID)
    {
      tb->updateRow(rID, rc->getType(), newPin);
    }
  }
}

rowController * findOutput(uint8_t uID)
{
  //rowController * existingOutput;

  for (rowController * rc : outputList) 
  {
    if(rc->getID() == uID)
    {
      return rc;
    }
  }

  return NULL;
}

// -- Block Types --
// - b - normal block (on/off)

void processBlockRequest(String inc, bool save)
{
  JsonDocument incomingJSON;
  deserializeJson(incomingJSON, inc);

  int uID = incomingJSON["uniqueID"];
  int rID = incomingJSON["assignedRow"];

  rowController * rc = findOutput(rID);

  uint8_t assignedPin = rc->getPin();
  uint8_t assignedType = rc->getType();

  bool blockExists = false;

  timeBlock * existingBlock;

  for (timeBlock * tb : blockList) 
  {
    if(tb->getID() == uID)
    {
      blockExists = true;
      existingBlock = tb;
      break;
    }
  }

  if(!blockExists)
  {
      timeBlock * newBlock = new timeBlock();
      newBlock->instantiate(inc, assignedPin, assignedType);
      blockList.push_back(newBlock);
  }
  else
  {
    existingBlock->changeInstruction(inc, assignedPin, assignedType);
  }

  if(save)
  {
    updateSavedBlocks();
  }

}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
//---------------------------------------------------------------------

void loop() 
{
  /*
  if(!playbackInProgress)
  {
    if(digitalRead(TRIGGER))
    {

      if(triggerMillis == 0)
      {
        triggerMillis = millis()+500;
      }
      else
      {
        if(triggerMillis < millis())
        {
          //startPlayback();
        }
      }
    }
    else
    {
      if(triggerMillis != 0)
      {
        triggerMillis = 0;
      }
    }
  }
  */
  if (deviceConnected) 
  {
    if(watchdog_timeout < millis())
    {
      flash_bit = !flash_bit;
      watchdog_timeout = millis() + 250;
    }

    digitalWrite(LED_BUILTIN, flash_bit);

    if(dataReceived)
    {
        JsonDocument incomingJSON;
        deserializeJson(incomingJSON, incomingData);

        dataReceived = false;
        const char* command;

        bool edit = false;
        bool remove = false;
        bool request = false;

        if(incomingJSON.containsKey("request"))
        {
          command = incomingJSON["request"];
          request = true;
        }

        if(incomingJSON.containsKey("edit"))
        {
          command = incomingJSON["edit"];
          edit = true;
        }

        if(incomingJSON.containsKey("remove"))
        {
          command = incomingJSON["remove"];
          remove = true;
        }

      if(edit)
      {   
        //-- Add or Edit Time Block --
        if(strcmp(command,"block") == 0)
        {
          Serial.println(command);
          //-- add block and save --
          processBlockRequest(incomingData, true);         
        }

        if(strcmp(command,"output") == 0)
        {
          Serial.println(command);
          processOutputRequest(incomingData, true);    
        }

        if(strcmp(command,"option") == 0)
        {
          Serial.println(command);
          processDeviceOption(incomingData, true);    
        }
      }

      if(request)
      {
        //-- Play @ frame # --
        if(strcmp(command,"play") == 0)
        {
          Serial.println(command);
          startPlayback(incomingData);         
        }

        //-- Stop Playback --
        if(strcmp(command,"stop") == 0)
        {
          Serial.println(command);
          //stopPlayback();        
        }

        //-- Freeze Frame for Realtime Scrubbing --
        if(strcmp(command,"freeze") == 0)
        {
          Serial.println(command);
          //freezeFrame(incomingData);        
        }

        //-- Change Timeline End Marker + Version #
        if(strcmp(command,"end marker") == 0)
        {
          Serial.println(command);
          double newTime = incomingJSON["time"];
          processTimelineInfo(newTime, true);    
        }

        //-- Factory Reset --
        if(strcmp(command,"factory reset") == 0)
        {
          Serial.println(command);
          factoryReset();    
        }
      }

      if(remove)
      {
        //-- Remove Block --
        if(strcmp(command,"block") == 0)
        {
          Serial.println(command);
          
            int uID = incomingJSON["uniqueID"];
            int iterator = 0;
            for (timeBlock * tb : blockList) 
            {
              if(tb->getID() == uID)
              {
                blockList.erase(blockList.begin()+iterator);
                break;;
              }
              iterator++;
            }
          
            updateSavedBlocks();           
        }

        //-- Remove Block --
        if(strcmp(command,"output") == 0)
        {
          Serial.println(command);
          
            int uID = incomingJSON["uniqueID"];
            int iterator = 0;
            clearRow(uID);

            for (rowController * rc : outputList) 
            {
              if(rc->getID() == uID)
              {
                outputList.erase(outputList.begin()+iterator);
                break;;
              }
              iterator++;
            }

            updateSavedBlocks(); 

        }

      }
      //-- Device Info Request --
      if(strcmp(command,"device information") == 0)
      {
        Serial.println(command);
        JsonDocument outgoingJSON;
        outgoingJSON["response"] = "transfer size";
        outgoingJSON["size"] = blockList.size();

        outgoingString = "";
        serializeJson(outgoingJSON,outgoingString);

        pTxCharacteristic->setValue(outgoingString);
        pTxCharacteristic->notify();
        Serial.println("Block Transfer Length: " + String(blockList.size()));

        
        for (rowController * rc : outputList) 
        {
          pTxCharacteristic->setValue(rc->getSaveCode());
          pTxCharacteristic->notify();
          delay(10);
        }
              
        for (timeBlock * tb : blockList) 
        {
          pTxCharacteristic->setValue(tb->getSaveCode());
          pTxCharacteristic->notify();

          delay(10); 
        }

        JsonDocument markerJSON;
        markerJSON["response"] = "end marker";
        markerJSON["time"] = endTime;

        outgoingString = "";
        serializeJson(markerJSON,outgoingString);

        pTxCharacteristic->setValue(outgoingString);
        pTxCharacteristic->notify();

        Serial.println(outgoingString);

        dataReceived = false;
        return;
        
      }
        dataReceived = false;
    }
    
  }

  if(playbackInProgress)
  {
    currentShowtime = (millis() - startMillis)/10;
    if(currentShowtime < endTime)
    {
      processCurrentFrame(currentShowtime);
    }
    else
    {
      //reset all blocks
      //stop playback
      Serial.println("Playback Stopping @ " + String(currentShowtime));
      stopPlayback();
      playbackInProgress = false;
    }
  }

  if(!playbackInProgress)
  {
    delay(50);
  }  

  BLE_loop();
    
  
}