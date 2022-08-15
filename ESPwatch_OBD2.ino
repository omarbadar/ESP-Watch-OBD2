#include "config.h"
#include "BluetoothSerial.h"
#include "ELMduino.h"

TTGOClass *watch;
TFT_eSPI *tft;
BMA *sensor;

//#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
//#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
//#endif

BluetoothSerial SerialBT;
#define ELM_PORT   SerialBT
#define DEBUG_PORT Serial


ELM327 myELM327;
int displayNum=0;

uint32_t rpm = 0;
uint32_t kph = 0;
uint32_t ECT = -1;
uint32_t oil = -1;
float fuelLevel = 0; // in %
  //float tempBATTERY = myELM327.queryPID(SERVICE_01,CONTROL_MODULE_VOLTAGE);
  //float tempCOOLANT = myELM327.queryPID(SERVICE_01,ENGINE_COOLANT_TEMP);
  //float tempINTAKE = myELM327.queryPID(SERVICE_01,INTAKE_AIR_TEMP);
  //float tempOIL = myELM327.queryPID(SERVICE_01,ENGINE_OIL_TEMP);
  //float tempLOAD= myELM327.queryPID(SERVICE_01,ENGINE_LOAD);

void(* resetFunc) (void) = 0;//declare reset function at address 0

void setup()
{
    Serial.begin(115200);
//    SerialBT.begin("ESP32OBD2"); //Bluetooth device name
    Serial.println("Connecting OBDII");

    // Get TTGOClass instance
    watch = TTGOClass::getWatch();

    // Initialize the hardware, the BMA423 sensor has been initialized internally
    watch->begin();

    // Turn on the backlight
    watch->openBL();

    //Receive objects for easy writing
    tft = watch->tft;


    // Some display settings
    tft->setTextColor(random(0xFFFF));
    tft->drawString("Connecting to OBD-II",  10, 10, 4);
    tft->setTextFont(4);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);

    ELM_PORT.begin("ArduHUD", true);
  
  if (!ELM_PORT.connect("OBDII"))
  {
    Serial.println("Couldn't connect to OBD scanner - Phase 1");
    tft->drawString("Error OBD-II",  10, 10, 4);
    delay(5000);
    resetFunc(); //call reset 
    while(1);
  }

  if (!myELM327.begin(ELM_PORT))
  {
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
    tft->drawString("Error ELM327",  10, 10, 4);
    delay(5000);
    resetFunc(); //call reset 
    while(1);
  }

  Serial.println("Connected to ELM327");
  //tft->setTextColor(random(0xFFFF));
  tft->drawString("ELM327 connected",  10, 10, 4);
  delay(10);
}

int getDisplay()
{
  int displayNum = 0;
  int analog_value = analogRead(36);  //ADC1 CH0
  if      (analog_value <= 999)                         displayNum = 0;
  else if (analog_value > 1000 && analog_value <= 1999) displayNum = 1;
  else if (analog_value > 2000 && analog_value <= 2999) displayNum = 2;
  else if (analog_value > 3000 && analog_value <= 3999) displayNum = 3;
  else if (analog_value > 4000 )                        displayNum = 4;
 
  return displayNum;
}

void getECT()
{
      /////////////////////////////////////////////////////// ECoolant Temp
      if (myELM327.queryPID(SERVICE_01,ENGINE_COOLANT_TEMP))
      {
        int32_t tempECT = myELM327.findResponse();
    
        Serial.print("Payload received for ECT: ");
        for (byte i = 0; i < PAYLOAD_LEN; i++)
          Serial.write(myELM327.payload[i]);
        Serial.println();
        
        if (myELM327.status == ELM_SUCCESS)
        {
          ECT = (uint32_t)tempECT;
          Serial.print("ECT: "); Serial.println(ECT-40);
          //display.drawString(64, 30, String(ECT-40));
        }
        else
          printError();
        }
}

void getOIL()
{
      /////////////////////////////////////////////////////// oil Temp
      if (myELM327.queryPID(SERVICE_01,INTAKE_AIR_TEMP))
      {
        int32_t tempOIL = myELM327.findResponse();
    
        Serial.print("Payload received for OIL: ");
        for (byte i = 0; i < PAYLOAD_LEN; i++)
          Serial.write(myELM327.payload[i]);
        Serial.println();
        
        if (myELM327.status == ELM_SUCCESS)
        {
          oil = (uint32_t)tempOIL;
          Serial.print("OIL: "); Serial.println(oil);
          //display.drawString(64, 30, String(ECT-40));
        }
        else
          printError();
        }
}

void getKPH()
{
      //////////////////////////////////////////////////// speed
      float tempSPEED= myELM327.kph();  
      if (myELM327.status == ELM_SUCCESS)
      {
        kph = (uint32_t)tempSPEED;
        Serial.print("Speed: "); Serial.println(kph);  
        //display.drawString(64, 1, String(kph));
      }
      else
        printError();
}

void getRPM()
{
      //////////////////////////////////////////////////// RPM
      float tempRPM = myELM327.rpm();
      if (myELM327.status == ELM_SUCCESS)
        {
          rpm = (uint32_t)tempRPM;
          Serial.print("RPM: "); Serial.println(rpm);  
          //display.drawString(64, 1, String(rpm));
        }
      else
        printError();
}


void getFuel()
{
      //////////////////////////////////////////////////// RPM
      
      if (myELM327.queryPID(SERVICE_01, FUEL_TANK_LEVEL_INPUT))
        {
          fuelLevel = myELM327.findResponse() * 100.0 / 255.0;
          Serial.print("Fuel : "); Serial.println(fuelLevel);  
          //display.drawString(64, 1, String(rpm));
        }
      else
        printError();
}

/***************************************************
* Display Data
****************************************************/
//void displayData(int displayNum) 
//{
//  //String formattedTime = timeClient.getFormattedTime();
//  //display.clear();   // clear the display
//  switch (displayNum) 
//  {
//    case 0:
//      getECT();
//      display.setFont(ArialMT_Plain_24);
//      display.drawString(20, 31,  "T:");
//      display.drawString(70, 31,  String(ECT-40));
//      display.drawString(110, 31,  "oC");
//      break;
//    case 1:
//      getKPH();    
//      display.setFont(ArialMT_Plain_24);
//      //display.drawString(20, 31, "Sp: ");
//      display.drawString(70, 31,  String(kph));
//      display.drawString(110, 31,  "kmh");
//      break;
//    case 2:
//      getFuel();
//      display.setFont(ArialMT_Plain_24);
//      display.drawString(20, 31,  "Fuel:");
//      display.drawString(70, 31,  String(fuelLevel));
//      display.drawString(110, 31,  "%");
//      break;
//    case 3:
//      getRPM();
//      display.setFont(ArialMT_Plain_24);
//      display.drawString(20, 31,  "RPM:");
//      display.drawString(70, 31,  String(rpm));
//      //display.drawString(100, 31,  " ");
//      break;
//    case 4:
//
//      getRPM();
//      getKPH();
//      getECT();
//      getFuel();
//   
//      display.setFont(ArialMT_Plain_10);
//      display.drawString(20, 0,  "ECT:");
//      display.drawString(60, 0,  String(ECT-40));
//      display.drawString(100,0,  "C");
//      display.drawString(20, 15, "Speed:");
//      display.drawString(60, 15,  String(kph));
//      display.drawString(100,15 ,  "kmh ");
//      display.drawString(20, 30, "RPM:");
//      display.drawString(60, 30,  String(rpm));
//      display.drawString(20, 45, "Fuel:");
//      display.drawString(60, 45,  String(fuelLevel));
//      display.drawString(100,45,  "%");
//      break;
//    default: 
//      display.clear();
//      break;
//  }
//  display.display();   // write the buffer to the display
//  delay(10);
//}


void loop()
{
      getRPM();
      getKPH();
      getECT();
      getFuel();
 
        // Show the data
        //tft->fillRect(98, 100, 70, 85, TFT_BLACK);
        tft->fillRect(240, 240, 0, 0, TFT_BLACK);
        tft->drawString("<< OBD-II GAUGE >>",  0, 10, 4);
        tft->setCursor(40, 70);
        tft->print("Speed: "); tft->print(kph); tft->println(" km/h  ");
        tft->setCursor(40, 100);
        tft->print("RPM: "); tft->print(rpm);tft->println(" rpm  ");
        tft->setCursor(40, 130);
        tft->print("ECT: "); tft->print(ECT-40); tft->println(" *C  ");
        tft->setCursor(40, 160);
        tft->print("FUEL: "); tft->print(fuelLevel); tft->println(" %  ");

    delay(500);
}

void printError()
{
  Serial.print("Received: ");
  for (byte i = 0; i < PAYLOAD_LEN; i++)
    Serial.write(myELM327.payload[i]);
  Serial.println();
  tft->fillRect(98, 100, 70, 85, TFT_BLACK);
  
  
  if (myELM327.status == ELM_SUCCESS)
    {Serial.println(F("\tELM_SUCCESS"));
    tft->drawString("ELM SUCCESS",  10, 10, 4);}
    //display.drawString(60, 20, String("\tELM_SUCCESS"));}
  else if (myELM327.status == ELM_NO_RESPONSE)
    {Serial.println(F("\tERROR: ELM_NO_RESPONSE"));
    tft->drawString("ERROR: ELM_NO_RESPONSE",  10, 10, 4);}
    //display.drawString(60, 20, String("\tERROR: ELM_NO_RESPONSE"));}
  else if (myELM327.status == ELM_BUFFER_OVERFLOW)
    {Serial.println(F("\tERROR: ELM_BUFFER_OVERFLOW"));
    tft->drawString("ERROR: ELM_BUFFER_OVERFLOW",  10, 10, 4);}
    //display.drawString(60, 20, String("\tERROR: ELM_BUFFER_OVERFLOW"));}
  else if (myELM327.status == ELM_UNABLE_TO_CONNECT)
    {Serial.println(F("\tERROR: ELM_UNABLE_TO_CONNECT"));
    tft->drawString("ERROR: ELM_UNABLE_TO_CONNECT",  10, 10, 4);}
    //display.drawString(60, 20, String("\tERROR: ELM_UNABLE_TO_CONNECT"));}
  else if (myELM327.status == ELM_NO_DATA)
    {Serial.println(F("\tERROR: ELM_NO_DATA"));
    tft->drawString("ERROR: ELM_NO_DATA",  10, 10, 4);}
    //display.drawString(60, 20, String("\tERROR: ELM_NO_DATA"));}
  else if (myELM327.status == ELM_STOPPED)
    {Serial.println(F("\tERROR: ELM_STOPPED"));
    tft->drawString("ERROR: ELM_STOPPED",  10, 10, 4);}
    //display.drawString(60, 20, String("\tERROR: ELM_STOPPED"));}
  else if (myELM327.status == ELM_TIMEOUT)
    {Serial.println(F("\tERROR: ELM_TIMEOUT"));
    tft->drawString("ERROR: ELM_TIMEOUT",  10, 10, 4);}
    //display.drawString(60, 20, String("\tERROR: ELM_TIMEOUT"));}
  else if (myELM327.status == ELM_TIMEOUT)
    {Serial.println(F("\tERROR: ELM_GENERAL_ERROR"));
    tft->drawString("ERROR: ELM_GENERAL_ERROR",  10, 10, 4);}
    //display.drawString(60, 20, String("\tERROR: ELM_GENERAL_ERROR"));}

  
  delay(2000);
  resetFunc(); //call reset 
}
