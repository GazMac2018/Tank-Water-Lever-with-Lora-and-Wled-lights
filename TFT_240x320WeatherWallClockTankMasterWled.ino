/*
  WeatherWall and Clock Program by G.McAlister - Started Aug 2022

  Adding TankMaster - 2 June 2025

  Adding LED Strip Light WS2812B DC5V

  MasterRecUnit-WaterLevel-LoRo-Web-V2

  Board Management : LOLIN D32 PRO  (Changed & Worked 19 Jan 2025)
  Partition Scheme : Minimal Spiffs (Large Apps with OTA)

  ***  Special Note  ***
  If any fails at compliation check "ArduinoNotes.txt"  in Arduino directory

 ****  Edit <User_Setup_Select.h> to INCLUDE <User_Setup_ESP32_IL19341.h> WHICH holds tft PINs etc

  My Limits with OpenWeatherMap.org
  60 calls/minute
  1,000,000 calls/month

  Pins Used for TFT Screen
                 GPIO  TFT     ESP32 (Location)
  #define TFT_MISO 19    9  -->  12
  #define TFT_MOSI 23    6  -->  18
  #define TFT_SCLK 18    7  -->  11
  #define TFT_CS   15    3  -->   4        Chip select control pin
  #define TFT_DC    2    5  -->   5        Data Command control pin
  #define TFT_RST   4    4  -->   7        Reset pin (could connect to RST pin)

  #define TOUCH_CS 5   11  -->   8    // CS_PIN 16 for Clocks - Pin 5 for LoRa Units - Gaz

  //  Fad the LED Screen for Clocks
  ScreenLED = 22;       8   -->   9    // For Screen pin 17 for Clock only - Pin 22 for LoRa units

  #define ILI9341_DRIVER

  #define DHTPIN 21       LOC 14   // Digital pin connected to the DHT sensor

  #define pResLDR 36 // ESP32 pin GIOP36 (ADC0)  Using 82k Resistor

  #define RGB_LED_PIN  27


 *****************************************************************

    BEWARE THIS SKETCH USES THE FOLLOWING LIBRARY FILES:

    /home/gazman/Arduino/libraries/TFT_eSPI/User_Setup.h

    /home/gazman/Arduino/libraries/TFT_eSPI/User_Setup_Select.h

    NOTE:  THESE FILES HAVE BEEN CHANGED TO READ ONLY

    and

    #include <User_Setups/Setup42_ILI9341_ESP32.h> //Setup file for ESP32 and SPI ILI9341 240x320

 ******************************************************************

  VERSION TIME STAMPS

  V-1  Started August 2022

  V-2  Added LDR for Night Time
      Main Menu for
          Location variations
          WiFi Manager
          Night Time Brightness setting
          Added Version & Time Stamp to Main Menu

       Tried Dual CPU, spent several weeks on that idea
       and it crashed repeatly.

  V-3   Added NonClockDelay - Replaced all delay(xxx) with millis() delay and
          during the NonClockDelay process, the Clock will Update..

  V-4   From Main Menu Toggle from Large Day Display in Lower Weather
          display Area or Back to Weather Forecasts...

          Added the Feature that from the BigDay Screen press the centre area
          and the Temperature and Daily Forecasts appear for 5 seconds
          During this 5 sec pediod, the 5 Day Forecast can also be selected * If you are quick *
  
  V-5     Clock brightens up a little bit at SunRise until Sunset if in a dark room

  V-6     Convert EEPROM to Preference - June 1 2025

  V-7     Add TankMaster as a Seperate Project..  Started 2 June 2025

  V-8     Add LED Strip Light WS2812B DC5V..  Special Project as Bed Side Light
*/

//  ************* START Clock Data ****************

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>

#include <TFT_eSPI.h>  // Hardware-specific library
#include <SPI.h>
#include <time.h>

#include <LoRa.h>

//#include <Arduino_JSON.h>  //  Gaz - If Arduino_JSON.h is placed BEFORE #include <TFT_eSPI.h>  ==  MAJOR ERRORS
#include <ArduinoJson.h>

//String jsonBuffer;
// JSONVar myObject;  // = JSON.parse(jsonBuffer);
// JsonDocument myObject;
DynamicJsonDocument myObject(2048);

// https://github.com/tzapu/WiFiManager
#include <WiFiManager.h>


#include <WebServer.h>  //https://github.com/bbx10/WebServer_tng  //  Web Server
WebServer server(80);   //  Web Server

#include <Preferences.h>

#include <FastLED.h>

//#include "ColorConverterLib.h"

#define RW_MODE false  //  Preferences variables
#define RO_MODE true

Preferences preferences;

//  For Touch Prog
#include <XPT2046_Touchscreen.h>

//#include <Adafruit_Sensor.h>
//#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN 21      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22  // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);

//  Fad the LED Screen
int ScreenLED = 22;  // For Screen pin 17 for Clocks PCB - Pin 22 for LoRa PCB, Testing etc

//  For Touch Prog
#define TOUCH_CS 5  //  CS_PIN 16 for Clock PCB - Pin 5 for LoRa PCB - Gaz

#include "soc/soc.h"           //  only used to avoid BrownOut issue and used with..
#include "soc/rtc_cntl_reg.h"  //  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // prevent brownouts by silencing them

//  Special Boolean Variable for SerialPrint
bool Serial_ON_Header = false;  //  False will not display any SerialPrint Data
bool Serial_ON_HeaderRec = false;
bool Serial_ON = false;

int SensorDelay = 2500;  //  Minimum Default Delay for Temperature Sensor
uint32_t LastSensorDelay = millis();
float Calibrate = -1.5;  // used for the temperature calbration

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

//const char*
String ssid;  //= "Telstra9E2B8C";    //  = "GazOppoA91"   = "iiNetA06F87"
//const char*
String password;  // = "GazGibson2017";  //  = "twinhammer"   = "NZHZ9PGZTHJ79F5"

String WebTitle = "BALVENIE WATER TANK";

String TimeDate;  //  to Show Version and Latest Compiled Time and Date

int RssiRead;
unsigned long TimeOut;
bool InternetFail = false;
int FailAttempt = 0;
bool NoSelection = true;
bool SelectionYesNo;
bool FirstBoot = true;

//  ClockData variables was EEPROM
String esid = "";
String epass = "";
String eLocalCity = "";
String eCountryCode = "";
//bool SaveEEPROM = false;
bool SaveClockData = false;

String DOT = ".";
String UpDateReason;
uint32_t UpdateMillis = millis();  //  for updating Forecast if InternetFail = true;

// Use your City and Country Code
//String LocalCity = "Bongaree";  //  Bongaree, AU

//  Multi Dimensional Array Size == 10 but 1st Array == [0] & 10th Array == [9]
String LocalCityArray[10][2] = {
  "Keperra",
  "AU",
  "Brisbane",
  "AU",
  "Caloundra",
  "AU",
  "Sydney",
  "AU",
  "Melbourne",
  "AU",
  "Adelaide",
  "AU",
  "Darwin",
  "AU",
  "Toowoomba",
  "AU",
  "London",
  "GB",
  "Los Angeles",
  "US",
};
int DispCity = 0;

String LocalCity = LocalCityArray[DispCity][0];  //  "Brisbane";
String CountryCode = LocalCityArray[0][1];       //   "AU";

String APIKEY;
//String APIKEY = "b436ff4fe79571ac2dda4c049cbaa556";  //  1st Clock - Mum's or Office Clock - Blue/Green Clock
//String APIKEY = "6e20ed5373db527ae41b44e066f4887f";  //  2nd Clock - Mum's or Office Clock - Blue/Green Clock
//String APIKEY = "c834c5045583e40972f8ae334fef0e66";  //  3rd Clock - Bedroom Clock - White/Blue Clock

//   Time Clock Variables
const char* ntpServer = "pool.ntp.org";  //  Internet Time website
const long gmtOffset_sec = 36000;        //  +10 * 60 * 60  ==  10 hours before GMT
const int daylightOffset_sec = 0;        //  0 == NO,  3600 == YES
int Hour;
int Minute;
float DecHourMin;  //  used for LDR and Sunrise and Sunset
double Second;
int Year;
int Month;
int TwHour;
int TwMin;
int TwSec;
int DispHour;
String StrDispHour;  // For displying 12 instead of 0
bool Silent = false;
bool ShowDayDate = true;
bool ChangeDisplay = false;
bool CheckTime = true;

bool BigDayDisplay = false;
bool DoBigDay = false;
String BigDayToggle;
bool TempWeatherDisp = false;
bool FirstTime;
uint32_t TempTimer;

String StrMin;
String StrSec;
String StrYear;
String StrMonth;
String StrDay;
String AmPm;

String LongTime;
String ShortTime;
//String TimeCalc;
String DateCalc;
int ScreenColour;
int LastHour;
int LastMin;
int LastDay;
int NxtHour;

int NextDay;
int wDay;    //  day of week [0,6] (Sunday = 0)
int nDay;    //  Day Date ==  1 to 28, 30 or 31
int LstDay;  //  for Time update at Midnight
int DayTxtY = 10;

String CurrentDate;

bool AfterMidnight = true;  //  should be false but want to test
String MidNightTime = "None Yet";
String AfterMidNightTime = "Not Yet";  // these 2 time should be very close. Displayed on MainMenu.

int TimeSync = false;

int i;
float LastTemp;

//  Added to convert timeinfo.tm_wday to String == Day of the Week
char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
char monthsOfYear[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
//int MonthDays[12] =          {31,   28,    31,     30,   31,    30,    31,    31,    30,    31,    30,    31};

//  Setup Graphics (Converted  at: https://www.mischianti.org/images-to-byte-array-online-converter-cpp-arduino/
extern uint8_t Graphic_WIFI[];  //  Graphics displayed when connecting to the Internet
extern uint8_t clearsky[];      //  Graphics for Clear Sky or Sunny
extern uint8_t fewclouds[];
extern uint8_t scatteredclouds[];
extern uint8_t brokenclouds[];
extern uint8_t overcastclouds[];
extern uint8_t lightrain[];
extern uint8_t moderaterain[];
extern uint8_t heavyrain[];
extern uint8_t rain[];
extern uint8_t thunderstorm[];
extern uint8_t hail[];

extern uint8_t sunrise[];
extern uint8_t sunset[];

extern uint8_t Graphic_Monday[];
extern uint8_t Graphic_Tuesday[];
extern uint8_t Graphic_Wednesday[];
extern uint8_t Graphic_Thursday[];
extern uint8_t Graphic_Friday[];
extern uint8_t Graphic_Saturday[];
extern uint8_t Graphic_Sunday[];

//extern uint8_t keysbd[];

int temperature;

int NextForecastTime;
const char* CheckForeCastMain;  //  check for Thunderstorm and Drizzle
const char* ForeCast1stSlot;    //  string
String ForeCast1stSlotStr;
const char* ForeCast2ndSlot;
String ForeCast2ndSlotStr;
const char* City;

String TestTime;

//  SunRise and SunSet Variables
long GetSunRiseUnix;
long SunRiseLocal;
long GetSunSetUnix;
long SunSetLocal;
long TimezoneOffset;
String OutputHour;
String OutputMin;
String SunRiseHour;
String SunRiseMin;
String SunSetHour;
String SunSetMin;
float DecSunRise;
float DecSunSet;

bool DailyUpdateCall = false;

int TimeSlot1st;  //  00  03  06  09  12  15  18  21
int TimeSlot2nd;

int XX;  //  for blacking out Icons
int YY;
int Xw;
int Yh;

int xTime1Slot = 162;  //  text above Weather Icons
int yTime1Slot = 160;
int xTime2Slot = 268;
int yTime2Slot = 160;

bool GoForcast = true;
bool GetForcast = true;  //  used to get the forecast after first Boot time
bool DoOnce = true;      //  for testing 7pm or 19:00
//bool Done10am = false;
//bool Done4pm = false;
bool DoneHourlyCheck = false;
bool Testing;

unsigned long FirstMillis;  //  This respresents zero second for CountSec not real time second
unsigned long CountSec;

XPT2046_Touchscreen ts(TOUCH_CS);  // was CS_PIN  Param 2 - NULL - No interrupts

//  Touch variables
int PX;
int MaxPX;
int MinPX = 10000;
int PY;
int MaxPY;
int MinPY = 10000;
bool TouchDone = false;
bool TouchAnywhere = false;

//Setting PWM properties
//  const int PWMChannel = 0;   //  Not used anymore -GAZ- Since Jan 2025
const int PWMFreq = 5000;  //  5 KHz
const int PWMResolution = 10;
int IntScnBrightness = 600;
int MaxBrightness = 750;  // (int)(pow(2, PWMResolution) - 1);  //  1023
int Brightness = 375;
int BarNumb = 9;
uint32_t BrightCntDwn = millis();  // Hold display of Brightness display bars
uint32_t BrightnessBarsStart = millis();
bool DelayTime = true;
int MinBright = 10;
bool MinBrightSw = true;

//  LDR settings - Using 82k Resistor to detect light from adjacent room light.
#define pResLDR 36  // ESP32 pin GIOP36 (ADC0)   Using 82k Resistor
int ReadLDR;
int LdrDelay = 10000;  //  10 sec, delay when lights go out - Reset Default Delay for LDR Delay
uint32_t LastLdrDelay = millis();
int LdrDimmer = 375;
int SetLdrDimmer = 0;

//  Get MAC Address to set Temp Sensor Calibration for each Clock..
String MACaddress;

//#define __FILENAME__ (strrchr("/"__FILE__, '/') + 1)

//  ************* FINISH Clock Data ****************

//  ************* START Wled Data ****************

// Information about the LED strip itself
#define RGB_LED_PIN 27  //  Wled lights
#define NUM_LEDS 21
#define CHIPSET WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
uint8_t hue = 0;
uint8_t Saturation;
#define OnOffPin 25
int PushButton;
bool WledState = true;

int WledBrightness = 10;  //  Initial WledBrightness of Wleds  0~255
uint32_t WaitStart = millis();
int WaitTime = 500;

bool tCalibrate = false;  //  Default "false" for normal program
bool DoCalibrateScreen = true;

bool MakeCandleFlicker = false;
bool DoFullRainbow = false;

bool WledON = false;

//  Variables for Wled Menu
int Rx = 60;
int Ry = 0;
int LstRy = 0;

String ColourProfile[21] = { "None", "Candle", "Tungsten40W", "Tungsten100W", "Halogen", "CarbonArc", "HighNoonSun", "DirectSunlight",
  "OvercastSky", "ClearBlueSky", "WarmFluorescent", "StandardFluorescent", "CoolWhiteFluorescent", "FullSpectrumFluorescent",
  "GrowLightFluorescent", "BlackLightFluorescent", "MercuryVapor", "SodiumVapor", "MetalHalide", "HighPressureSodium", "UncorrectedTemperature" };

// How many seconds to show each temperature before switching
#define DISPLAYTIME 20
// How many seconds to show black between switches
#define BLACKTIME 3

//  ************* FINISH Wled Data ****************

//  ************* START Tank Data ****************

long WebRSSI;

TFT_eSprite spr = TFT_eSprite(&tft);  // Declare Sprite object "spr" with pointer to "tft" object

//  EEPROM Variables using the ESP32 #include <Preferences.h>
int TrueData = 99;

bool TankTest = true;

String TankTime;

//  These and others required for more than 1 Tank Location
//bool ReqData = true;  //  This happens at Boot Time and Every 20 minutes excluding Header Tank.
//unsigned long ReqDataDelay = millis();
//int StartReqData = 0;  //  changes to 2000 for 2nd and remaining Units
//int UnitCnt = 1;       //  Tank Cnt at Boot Time and Every 20 minutes.
//int MaxNoUnit = 1;     //  Set for 1 Tank - Remove or Modify for more tanks

// Width and height of each Tank Area Sprite
#define SprWIDTH 105
#define SprHEIGHT 120

//Set delay after plotting the sprite
#define SprDELAY 20

int FontType;

int status = WL_IDLE_STATUS;  //status of wifi
bool WiFiOK = false;

//  ******  START LoRa Varibles   ******

const int LoRaCSpin = 17;  // LoRa radio chip select
const int resetPin = 16;   // LoRa radio reset
const int irqPin = 14;     // change for your board; must be a hardware interrupt pin

String TheMessage;  //SendMessageNow message

unsigned long lastSendTime = millis();  // last send time
String RecMessage;

byte localAddress = 0xFF;  // Writes address of this device
byte destination;          // Writes destination to send to Selected Tank in LOOP
// 0xAA or 0xBB or 0xCC

byte recipient;  // Reads recipient address <> 0xFF
byte sender;     // Reads sender address < 0xAA or 0xBB or 0xCC

byte incomingTankName;  // incoming msg ID
byte incomingLength;    // incoming msg length sent by Sensor or Master

unsigned long LastDelay = millis();
unsigned long RequestDelay = 1000;  // initial 1 sec then 10 sec

unsigned long WebDelay = 10000;  //  10 sec
unsigned long LastWebDelay = millis();

unsigned long WiFiCheckDelay = millis();
unsigned long WiFiCheckTime = 300000;  //  5 minutes == 5 * 60 * 1000 ==  300000 msec

int WiFiCnt = 0;

bool PrntData = false;

String LoRaRSSI;
String PrintOut;
int BkgCol;
int TankID;  //  number not letter..

//  ******  FINISH LoRa Varibles   ******

String TankNameLoc[5] = { "MT", "Header", "DBL Main", "uDeck", "MT" };  //  [0], ... [3]
//    some defaults that should be saved in EEPROM
float TankDia[5] = { 0, 1.8, 1.8, 1.8, 0 };  //  Defaults  -  Warning - Save to EEPROM at Line 500 (approx)
float TankHeight[5] = { 0, 2.0, 2.0, 2.0, 0 };
float HgtSensor[5] = { 0, 0.15, 0.15, 0.15, 0 };                         //  Height of Sensor to Max Height of Water
int NumbTanks[5] = { 0, 1, 1, 1, 0 };                                    //  Number Of Tanks at TankNameLoc
int SleepTime[5] = { 0, 120, 20, 20, 0 };                                // default sleeptime set at 20 mins
String SleepMode[5] = { "BootUp", "Sleep", "Awake", "Awake", "Awake" };  //  Awake or Sleep

bool Set20Min = false;
int SetHr;
String SetHrStr;
String LstSetHrStr;
String newSleepTime;

bool SetHeaderSleep = false;
bool LstSetHeaderSleep = false;
//bool Mode_1_Done = false;
String SndSleepMode[5] = { "BootUp", "Sleep", "Sleep", "Sleep", "Sleep" };  //  Awake or Sleep
String LstSleepMode[5] = { "BootUp", "Sleep", "Sleep", "Sleep", "Sleep" };
String TimeStamp[5] = { "", "", "", "", "" };
bool SleepCalcDone = false;  //  checks for Sleep Calc done
bool SleepAmPmDone = false;  //  Only calc AM/PM once for Hourly Sleep Times

unsigned long HeaderAwakeTime = millis();  //  Start counting in case Awake Mode sent and forgotten about 15 mins max..

//  If any of the above change then put in EEPROM
float LstTankDia[5] = { 0, 2.5, 2.5, 1.8, 0 };
float LstTankHeight[5] = { 0, 2, 2.4, 1.8, 0 };
int PercentVol[5] = { 0, 0, 0, 0, 0 };
int LstNumbTanks[5] = { 0, 1, 1, 1, 0 };  //  Number Of Tanks at TankNameLoc
//long LstSleepTime[5] = { 0, 60, 60, 60, 0 };  // default sleeptime set at 10 mins

int CurrentVol[5];
int TankSize[5];
String RSSILoRa[5] = { "down", "down", "down", "down", "down" };  //  or "Sleep"
String Dist[5];
String Temp[5];
String PowerVolts[5];
String BatteryVolts[5];
String SolarVolts[5];

float HeightPercent[5];
float CalcHt[5] = { 0, 150, 150, 150, 0 };
String DataSent[5] = { "NA", "NA", "NA", "NA", "NA" };  //  Will be set to Sent or Decd

String RecTemp;

String PowVolts;
String BatVolts;
String SolVolts;

//  Maybe use a structure type instead of an array
//  Array for History - First calc for 8 days at 24 * 3 (every 20 minutes) = 576
//  Special NOTE:  After a Board Update initial 720 (10 Days) fails compile and
//  some array variables beyond 600 were corrupted - 72 readings in 1 day
String HistoryData[505][8];  // [505][8] max 72 readings per day (7 days) = 504 plus [0] for heading..
int MaxMem = 505;
int HistoryCnt = 1;  //  cycles 0 > 576 then starts again
String HistoryOutput = "Screen";

String BatWarning = "";

String RecTimeSleeping;
String RecWake;
// float NxtAwakeTime = 60;
String NxtAwakeTimeAmPm[5] = { "NA", "NA", "NA", "NA", "NA" };

int TankLocCnt = 1;  //  Number of Tank Locations

float Pi = 3.14159;

bool NewData = false;
int BMx;
int BMy;
float FullTank;
int WaterLevelPerct;

//  Singal Indicators  --  Not used in this Upgrade
//int SignalX;
//int SignalY;
//int TFT_LED_Dia = 8;

String RecTankNameLoc;
float LstRecWaterLevel;
String RecWLdist;     //  Recieved Water Level from Sensor
float RecWaterLevel;  //  RecWLdist - 220mm
int RecWaterLevelPerct;
int Wx;
int Wy;
int Wh;
int WxL;
int SprTopY = 30;
int SprBotY = SprTopY + 90;

long DummyDataWait = millis();
bool GetDumData = true;

//Setting PWM properties
// const int PWMFreq = 5000; //  5 KHz
// const int PWMResolution = 10;
int MaxBright = 750;  //  was 750;  // (int)(pow(2, PWMResolution) - 1);  //  1023
// int Brightness = 300;  //  Max 1023
int tft_StrLine = 0;
// int MinBright = 10;
// bool MinBrightSw = true;

int LnSpc = 25;
int NxtLneY = 0;
int ColDisp = 0;
int TankNumbSpc;

//  unsigned long FirstMillis = millis();
int DoShowTime = 0;
bool ReTryTime = false;
unsigned long ReTryMillis = millis();
bool GetTimeSilent = false;

//  Setup Graphics (Converted  at: https://www.mischianti.org/images-to-byte-array-online-converter-cpp-arduino/
extern uint8_t LoftyTank[];  //  Graphics displayed when connecting to the Internet
extern uint8_t LoftyWaterLevel[];
extern uint8_t LoftyTankTitle[];
extern uint8_t WiFiIcon[];
extern uint8_t NoWiFiIcon[];

extern uint8_t BalvenieTankTitle[];

//  ************* Finish Tank Data ****************

//******************************************************************************************

//  This is for the Openweather --- jsonbuffer
/*
String httpGETRequest(const char* serverName)  //  *****************************************
{
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  //Send HTTP POST request
  int httpResponseCode = http.GET();

  //String payload = "{}";

  if (httpResponseCode > 0)
    {
      //Serial.print("HTTP Response code: ");  //  200 == OK
      //Serial.println(httpResponseCode);

      //payload = http.getString();
      jsonBuffer = http.getString();

      Serial.print("NEW Length(jsonBuffer) == ");
      Serial.println(jsonBuffer.length());
      //  Printout Data in one continual string
      Serial.println();
      Serial.println("JSON object = ");
      Serial.println(jsonBuffer);
      Serial.println();
    }
  else
    {
      //Serial.print("Error code: ");
      //Serial.println(httpResponseCode);   //  error code == -1  ==  no internet
      //Serial.println("String httpGETRequest -- Try Reconnect Later");
      CheckInternet();
    }
  // Free resources
  http.end();

  return payload;
}
*/

void setup(void)  //  ******************************************************************
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // prevent brownouts by silencing them

  Serial.begin(115200);
  delay(1000);

  //  For Screen & Touch Parts
  pinMode(TFT_CS, OUTPUT);    //  Pin 15
  pinMode(TOUCH_CS, OUTPUT);  //  Pin 5
  digitalWrite(TFT_CS, LOW);
  digitalWrite(TOUCH_CS, LOW);

  //char* GetTime = __TIME__;
  //char* GetDate = __DATE__;

  //TimeDate = "V-8 ";
  //TimeDate = TimeDate + GetTime + " : " + GetDate;
  Serial.println();
  Serial.println();
  Serial.println("*********************************");
  Serial.println("Starting Setup for");
  Serial.println("TFT_240x320WeatherWallClockTankMasterWled.ino");
  //Serial.println(__FILENAME__);  //  Printsout /Dir/Filename
  //Serial.println(TimeDate);      //  Shows Time & Date when Last Compiled
  Serial.println("Update - 19 Janruary 2026");
  Serial.println("by G.McAlister - Started 21 September 2022");
  Serial.println("**********************************");

  delay(100);

  //  SwitchOffLoRa();  //  Switch TFT ON

  //  Change here when Testing finished..
  Testing = false;

  if (Testing)  //  Currently Testing for SunRise (Key word) options
    {
      Serial.println("*** TESTING MODE - Edit or remove the following Lines  ***");
      Serial.println("*** #define TOUCH_CS 5  // CS_PIN 16 for Clocks - Pin 5 for Testing - Gaz");
      Serial.println("*** int ScreenLED = 22; // For Screen pin 17 for Clocks - Pin 22 for Testing");
      Serial.println("*** //  For Testing without a LDR Line 677 - GAZ");
      Serial.println("*** ReadLDR = 100;");
      Serial.println("**********************************");
    }

  //  Set initial Screen Brightness here
  ledcAttach(ScreenLED, PWMFreq, PWMResolution);
  ledcWrite(ScreenLED, MinBright);  //  Turn on SCREEN to Last Brightness  was IntScnBrightness
  //Serial.print("MaxBrightness = ");
  //Serial.println(MaxBrightness);

  dht.begin();
  delay(100);
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  delay(100);

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  LastTemp = event.temperature;

  if (Testing)
    {
      LastTemp = 11;
      Serial.println("Temporary Temperature input here..  line 701");
    }

  // ******  START Tank Data  ******

  HistoryData[0][0] = "  Date   ";
  HistoryData[0][1] = "  Time  ";
  HistoryData[0][2] = " Vol ";
  HistoryData[0][3] = " Dist";
  HistoryData[0][4] = " Pow  ";
  HistoryData[0][5] = "Bat  ";
  HistoryData[0][6] = "Sol  ";
  HistoryData[0][7] = "Temp ; SleepTime";

  if (Serial_ON)
    {
      Serial.println();
      Serial.println("*** WARNING ***  Serial_ON = true - All Units Serial.print WILL be Shown");
    }
  else
    {
      Serial.println("Serial_ON = false - All Units Serial.print will NOT be Shown");
    }

  if (Serial_ON_Header)
    {
      Serial.println();
      Serial.println("*** WARNING ***  Serial_ON_Header = true - Header Only Serial.print WILL be Shown");
    }
  else
    {
      Serial.println("Serial_ON_Header = false - Header Only Serial.print will NOT be Shown");
    }

  Serial.println();

  // Create a sprite of defined size
  spr.createSprite(SprWIDTH, SprHEIGHT);

  delay(100);

  // ******  START Tank Data  ******

  pinMode(pResLDR, INPUT);  // LDR input   Using 82k Resistor

  //  For Touch Prog
  ts.begin();
  delay(100);

  //Set delay between temperature sensor readings based on sensor details.
  SensorDelay = 2500;  //  Default Delay for Temperature Sensor
  LastSensorDelay = millis();

  LdrDelay = 0;  //  This ensures there is no delay to Dim the Screen if the Clock reboots at night
  GoLDR();

  //  TFT initialised etc
  tft.init();
  delay(100);

  //  Screen Colour Problem
  tft.setRotation(1);
  tft.invertDisplay(1);  //  Try 1 or 0;

  //  Set up Background colour for TFT
  byte R = 0, G = 0, B = 4;  //  2 = Roolly Dark Blue :)
  long RGB = ((long)R << 16L) | ((long)G << 8L) | (long)B;

  //  Print RGB colour in HEX

  if (Serial_ON)
    {
      Serial.println("Touchscreen started");
      Serial.print("Background colour == ");
      Serial.println(RGB, HEX);
    }

  //BkgCol = RGB;  //  Calc colour
  BkgCol = TFT_BLACK;
  tft.fillScreen(BkgCol);
  tft.setTextColor(TFT_LIGHTGREY, BkgCol);

  //  Setup LoRa
  SwitchOnLoRa();

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(LoRaCSpin, resetPin, irqPin);  //Set CS, reset, IRQ pin
  LoRa.setTxPower(17);                        //  default == 17  max 20
  LoRa.setSpreadingFactor(7);                 // was 12 defaults to 7 - values range 6 and 12

  if (!LoRa.begin(915E6))
    {  // initialize ratio at 915 MHz
      Serial.println("LoRa init failed. Check your connections.");

      SwitchOffLoRa();  //  To Write to TFT Screen

      tft.drawCentreString("LoRa failed.", 160, 90, 4);

      tft.drawCentreString("Check connections", 160, 120, 4);

      while (true)  // Indefinately LOOP - stops the program from advancing
        {
          //  Does noting but LOOP<>LOOP
        }
    }
  else
    {
      Serial.println("LoRa init succeeded.");

      SwitchOffLoRa();  //  To Write to TFT Screen

      tft_StrLine = tft_StrLine + 35;
      tft.drawCentreString("LoRa is Working.", 160, tft_StrLine, 4);

      //  LoRaRSSI = String(LoRa.packetRssi());
      //  tft_StrLine = tft_StrLine + 35;
      //  tft.drawCentreString(LoRaRSSI, 160, tft_StrLine, 4);

      delay(500);

      if (Serial_ON)
        {
          tft_StrLine = tft_StrLine + 35;
          tft.drawCentreString("Serial Print == true", 160, tft_StrLine, 4);
          delay(500);
        }
    }

  delay(3000);
  tft.fillScreen(BkgCol);

  if (!ts.begin())
    {
      Serial.println("Couldn't start touchscreen controller");
      //while (1);
    }

  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();

  //  Get MAC Address...
  MACaddress = String(WiFi.macAddress());

  delay(100);

  // for Testing only
  Serial.print("MAC = ");
  Serial.println(MACaddress);
  Serial.println("********************************");

  delay(100);

  //  TEST APIKEY
  //APIKEY = "b436ff4fe79571ac2dda4c049cbaa556";  //  First Clock - Mum's or Office Clock - Blue/Green Clock
  //APIKEY = "6e20ed5373db527ae41b44e066f4887f";  //  2nd Clock   Mum's or Office Clock - Blue/Green Clock
  APIKEY = "c834c5045583e40972f8ae334fef0e66";  //  3rd Clock - White Bedroom Clock

  GoInternet();  //  Connect to WiFi

  GetWWWTime();

  BeginServer();  //  Web Server

  //  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING
  //  For NEW ESP32 - Uncomment To save new Default Data
  //  SaveEEPROM();

  GetTankData();  //  using the ESP32 #include <Preferences.h>

  tft.fillScreen(BkgCol);
  tft.setTextSize(1);

  //  Debugging :::: To do a Time check - change Hour and Minute here..
  /*
  Hour = 2;
  Minute = 59;
  Second = 45;
  SensorDelay = millis();
  LastSensorDelay = millis();
  */

  GetClockData();

  DoBigDay = false;  //  Just for Reboots

  if (BigDayDisplay)
    {
      DoBigDay = true;
      GetForcast = true;
    }

  //  Start Wleds
  pinMode(OnOffPin, INPUT);

  FastLED.addLeds<CHIPSET, RGB_LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  //  As tested below..  This is the best Colour Correction for ME..
  FastLED.setCorrection(TypicalPixelString);

  fill_solid(leds, NUM_LEDS, CRGB::White);

  FastLED.setBrightness(WledBrightness);  //  WledBrightness

  //  Initialise Candle as First Selection
  FastLED.setTemperature(Candle);

  if (Serial_ON)
    {
      Serial.println();
      Serial.println("*******************************");
      Serial.println("Line 953  Turn On or Off Wleds");
    }

  if (WledState)  //  Wleds ON
    {
      Serial.println("Wleds will be turned OFF");
      WledState = false;
      FastLED.setBrightness(0);  // Turn OFF

      FastLED.show();
      FastLED.delay(10);

      WledON = false;  //  Reset after Wleds turned OFF for Timer ON setting
    }
  else  //  Wleds OFF
    {
      Serial.println("Wleds will be turned ON");
      WledState = true;
      FastLED.setBrightness(WledBrightness);  // Turn ON

      FastLED.show();
      FastLED.delay(10);
    }
}

void GetClockData()
{
  //  Check for "#"  if no "#" then assume Brisbane, AU
  eLocalCity = "";
  eCountryCode = "";

  preferences.begin("ClockData", RO_MODE);
  delay(10);

  eLocalCity = preferences.getString("eLocalCity", "Null");

  eCountryCode = preferences.getString("eCountryCode", "Null");

  MinBright = preferences.getUInt("MinBright", 10);

  BarNumb = preferences.getUInt("BarNumb", 5);

  BigDayDisplay = preferences.getBool("BigDayDisplay", 0);

  preferences.end();
}

void TouchPxYx()  //  ********************************************************
{
  if (ts.touched() && !TouchDone)
    {
      //serial.println("void TouchPxYx()");
      if (Serial_ON)
        {
          Serial.print("ts.touched() == ");
          Serial.println(ts.touched());
          Serial.print("TouchDone == ");
          Serial.println(TouchDone);
        }

      TS_Point p = ts.getPoint();
      //Serial.print("Pressure = ");
      //Serial.print(p.z);
      //Serial.print(", x = ");
      //Serial.print(p.x);
      PX = p.x;
      //Serial.print(", y = ");
      //Serial.print(p.y);
      PY = p.y;
      delay(250);
      //Serial.println();

      //  MaxPX MaxPY etc below ONLY used to determine the limits of the Touch Screen
      //  if( MaxPX < PX) MaxPX = PX;  //  3931
      //  if( MaxPY < PY) MaxPY = PY;  //  3783
      //  if( MinPX > PX) MinPX = PX;  //  245
      //  if( MinPY > PY) MinPY = PY;  //  285

      //Serial.print("MaxPX = ");
      //Serial.print(MaxPX);
      //Serial.print("  MaxPY = ");
      //Serial.print(MaxPY);
      //Serial.print("  MinPX = ");
      //Serial.print(MinPX);
      //Serial.print("  MinPY = ");
      //Serial.println(MinPY);

      PX = map(PX, 245, 3931, 0, 350);
      PY = map(PY, 285, 3783, 0, 240);

      if (Serial_ON)
        {
          Serial.print("PX = ");
          Serial.print(PX);
          Serial.print("   PY = ");
          Serial.println(PY);
        }

      TouchDone = true;  //  stop multiple taps on screen

      // if( PY > 80 && PY < 175)  //  draw keyboard
      //  {
      //    GetWiFiInfo();                                    //  KeyboardKeys();
      //  }
    }
}

void TouchSpot()  //  For Touch Prog *************************************************
{
  TouchDone = false;

  SwitchOffLoRa();
  digitalWrite(TFT_CS, HIGH);  //  Add DeSelect TFT     Pin 15

  delay(10);

  TouchPxYx();

  if (TouchDone)
    {
      //  #########################################################################

      // Decrease Brightness of Screen  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
      if (PX > 220 && PY > 110)  //  Tap on Lower RHS
        {
          BarNumb--;

          if (BarNumb < 1) BarNumb = 1;

          Brightness = map(BarNumb, 1, 10, 100, 750);

          ledcWrite(ScreenLED, Brightness);  //  Turn on LED Down in Brightness

          BrightnessDisplay();

          // write Brightness to ClockData - EEPROM
          preferences.begin("ClockData", RW_MODE);

          preferences.putUInt("BarNumb", BarNumb);

          preferences.end();

          delay(50);
        }

      //  #########################################################################

      // Increase Brightness of Screen  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
      if (PX > 220 && PY < 110)  //  Tap on Upper RHS
        {
          BarNumb++;

          if (BarNumb > 10) BarNumb = 10;

          Brightness = map(BarNumb, 1, 10, 100, 750);

          //Serial.print("BarNumb = ");
          //Serial.print(BarNumb);
          //Serial.print("  Brightness = ");
          //Serial.println(Brightness);


          ledcWrite(ScreenLED, Brightness);  //  Turn on LED Up in Brightness

          BrightnessDisplay();

          // write Brightness to ClockData - EEPROM
          preferences.begin("ClockData", RW_MODE);

          preferences.putUInt("BarNumb", BarNumb);

          preferences.end();
          delay(50);
        }

      //  #########################################################################

      //  Temporary Display of Temp and Weather Forcast i.e.9am & 3pm..  ONLY IF BigDayDisplay
      if (PX > 70 && PX < 220 && PY > 155)
        {
          //  show TouchSpot
          // tft.drawRoundRect(70, 155, (220 - 70),(240-155), 4, TFT_LIGHTGREY);
          // delay(2000);

          if (BigDayDisplay)
            {
              TempWeatherDisp = true;  //  being true will trigger a timer in loop()
              FirstTime = true;
            }
        }

      //  #########################################################################

      if (!BigDayDisplay)  //  Only allow Weather if BigDayDisplay == false
        {
          //  Update Today's Forecast Tap Lower Middle Forecast Box  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
          if (PX > 108 && PX < 217 && PY > 155)
            {
              //  show TouchSpot
              //tft.drawRoundRect(108, 155, (217 - 108), (240 - 155), 4, TFT_LIGHTGREY);
              //delay(2000);
              GetForcast = true;
              //Serial.println(">>>> GetForcast = true  -- Line 1060");
              UpDateReason = "Update Forecast";
              CheckTime = false;
              UpDateGraphics(UpDateReason);

              //  No need to CheckTime here
              CheckTime = false;
              GoWeather();
            }

          //  Current Outside Temp      <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
          if (PX < 108 && PY > 155)  //  Tap Bottom Left Cnr over Inside Temperture
            {
              GetOutsideTemp();
            }

          //  Get Each Daily Forcast for next 4-5 days   Tap Top Left Cnr over the Day Name  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
          if (PX < 160 && PY < 60)
            {
              //  show TouchSpot
              // tft.drawRoundRect(0, 0, 160, 60, 4, TFT_LIGHTGREY);
              // delay(2000);

              EachDayWeather();
            }
        }

      //  #########################################################################

      //  Main Menu for All Settings including Start WiFiManager      <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
      if (PX > 35 && PX < 110 && PY > 70 && PY < 150)  //  Tap over the Second Time Number from the Left
        {
          //  show TouchSpot
          //int Xwidth = 110 - 35;
          //int Yheight = 150 - 70;
          //tft.drawRoundRect(35, 70, Xwidth, Yheight, 4, TFT_LIGHTGREY);
          //delay(2000);

          MainMenu();

          if (SelectionYesNo)
            {
              WiFiSetup();  //  YES
            }

          tft.fillScreen(BkgCol);

          DrawLines();
          //ShowTime();
          SensorDelay = 0;  //  Avoid Delay for Temperature Sensor
          LastSensorDelay = millis();

          DailyUpdate();
          TemperatureSensor();
        }

      //  #########################################################################
    }

  SwitchOnLoRa();
}

void GoSaveLocation()  //  ***EEPROM****************************************************
{
  if (Serial_ON)
    {
      Serial.println(" void GoSaveLocation() ");
    }

  preferences.begin("ClockData", RW_MODE);

  preferences.putString("eLocalCity", eLocalCity);

  preferences.end();

  delay(100);
}

void BlankRdBox()  //  oooooooOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOooooooo
{
  if (LstRy != Ry)
    {
      //Serial.println("  Blank Previous Box");
      //  Blank Rectangle around Menu Item
      tft.drawRoundRect(Rx, LstRy, 200, 30, 4, TFT_BLUE);
    }
}

void WledMenu()  //  ***************************************************************
{
  TouchDone = false;
  DoOnce = true;

  tft.fillScreen(TFT_BLUE);  //  clears screen to blue
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLUE);
  tft.setTextSize(2);

  tft_StrLine = 10;
  int LnSpc = 30;

  tft.drawCentreString("Wled MENU", 160, tft_StrLine, 1);

  NxtLneY = tft_StrLine + LnSpc + 10;

  tft.drawCentreString("Candle", 160, NxtLneY, 1);

  //tft.drawRoundRect(60, 40, 200, 30, 4, TFT_LIGHTGREY);  //  Text Box

  NxtLneY = NxtLneY + LnSpc;
  tft.drawCentreString("Candle Flicker", 160, NxtLneY, 1);

  //tft.drawRoundRect(60, 70, 200, 30, 4, TFT_LIGHTGREY);  //  Text Box

  NxtLneY = NxtLneY + LnSpc;
  //FullRainBow()
  tft.drawCentreString("FullRainBow", 160, NxtLneY, 1);

  //tft.drawRoundRect(60, 100, 200, 30, 4, TFT_LIGHTGREY);  //  Text Box

  NxtLneY = NxtLneY + LnSpc;
  tft.drawCentreString("Halogen", 160, NxtLneY, 1);

  //tft.drawRoundRect(60, 130, 200, 30, 4, TFT_LIGHTGREY);  //  Text Box

  NxtLneY = NxtLneY + LnSpc + 20;

  tft.drawString("Wled Bright", 10, NxtLneY, 1);

  int BoxX = 150;
  int BoxY = NxtLneY - 6;

  bool OK = false;
  TimeOut = millis();

  while (!OK)  //  While OK == false -->  Stays in Loop
    {
      if (DoOnce)
        {
          tft.fillRoundRect(BoxX, BoxY, 150, 30, 4, TFT_BLUE);       //  Text Box
          tft.drawRoundRect(BoxX, BoxY, 150, 30, 4, TFT_LIGHTGREY);  //  Text Box
          String WledBrightStr = " -   " + String(WledBrightness) + "   +";
          tft.drawString(WledBrightStr, BoxX + 5, NxtLneY, 1);
          tft.drawRect(BoxX + 40, BoxY, 70, 30, TFT_LIGHTGREY);

          DoOnce = false;
        }

      //  Work out Touch Spots for [-] & [+]
      //tft.drawRoundRect(140, 170, 60, 60, 4, TFT_LIGHTGREY);  //  Text Box
      //tft.drawRoundRect(250, 170, 60, 60, 4, TFT_LIGHTGREY);  //  Text Box

      TouchPxYx();

      //  #########################################################################

      if (PX > 140 && PX < 200 && PY > 170 && PY < 230 && TouchDone)
        {
          WledBrightness = WledBrightness - 1;
          if (WledBrightness < 0) WledBrightness = 0;

          DoOnce = true;

          FastLED.setBrightness(WledBrightness);  //  WledBrightness

          FastLED.show();
          FastLED.delay(10);
        }

      if (PX > 250 && PX < 310 && PY > 170 && PY < 230 && TouchDone)
        {
          WledBrightness = WledBrightness + 1;
          if (WledBrightness > 200) WledBrightness = 200;

          DoOnce = true;

          FastLED.setBrightness(WledBrightness);  //  WledBrightness

          FastLED.show();
          FastLED.delay(10);
        }

      //  Special Touch Point..  Touch ON Number and the Wleds will Toggle ON/OFF
      if (PX > 200 && PX < 250 && PY > 170 && PY < 230 && TouchDone)
        {
          if (WledState)
            {
              if (Serial_ON)
                {
                  Serial.println("Wleds will be turned OFF");
                }
              WledState = false;

              FastLED.setBrightness(0);  // Turn OFF

              FastLED.show();
              FastLED.delay(10);

              WledON = false;
            }
          else
            {
              if (Serial_ON)
                {
                  Serial.println("Wleds will be turned ON");
                }
              WledState = true;

              FastLED.setBrightness(WledBrightness);  // Turn ON

              FastLED.show();
              FastLED.delay(10);

              WledON = true;
            }
        }

      //  Candle
      if (PX > 60 && PX < 260 && PY > 40 && PY < 70 && TouchDone)
        {
          Ry = 40;
          BlankRdBox();
          tft.drawRoundRect(Rx, Ry, 200, 30, 4, TFT_LIGHTGREY);  //  Text Box
          LstRy = Ry;

          fill_solid(leds, NUM_LEDS, CRGB::White);
          FastLED.setTemperature(Candle);
        }

      //  Candle Flicker
      if (PX > 60 && PX < 260 && PY > 70 && PY < 100 && TouchDone)
        {
          Ry = 70;
          BlankRdBox();
          tft.drawRoundRect(Rx, Ry, 200, 30, 4, TFT_LIGHTGREY);  //  Text Box
          LstRy = Ry;

          fill_solid(leds, NUM_LEDS, CRGB::White);
          MakeCandleFlicker = true;
          FastLED.setTemperature(Candle);
        }

      //  Full Rainbow
      if (PX > 60 && PX < 260 && PY > 100 && PY < 130 && TouchDone)
        {
          if (!DoFullRainbow)  //  First time here..
            {
              Ry = 100;
              BlankRdBox();
              tft.drawRoundRect(Rx, Ry, 200, 30, 4, TFT_LIGHTGREY);  //  Text Box
              LstRy = Ry;
            }
          FullRainBow();
          DoFullRainbow = true;
        }

      //  Halogen
      if (PX > 60 && PX < 260 && PY > 130 && PY < 160 && TouchDone)
        {
          Ry = 130;
          BlankRdBox();
          tft.drawRoundRect(Rx, Ry, 200, 30, 4, TFT_LIGHTGREY);  //  Text Box
          LstRy = Ry;

          fill_solid(leds, NUM_LEDS, CRGB::White);
          FastLED.setTemperature(Halogen);
        }

      FastLED.show();
      FastLED.delay(10);

      if (Ry != 100)  //  Turns OFF Rainbow function
        {
          DoFullRainbow = false;
        }

      if (Ry != 70)  //  Turns OFF MakeCandleFlicker function
        {
          MakeCandleFlicker = false;
        }

      if (TouchDone)
        {
          TimeOut = millis();
        }

      if (millis() > TimeOut + 5000)
        {
          OK = true;  // exit While Loop after 5 seconds
        }

      TouchDone = false;
    }
  TimeOut = millis();
}

void MainMenu()  //  ***************************************************************
{
  bool OK = false;
  TouchDone = false;

  tft.fillScreen(TFT_BLUE);  //  clears screen to blue
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLUE);
  tft.setTextSize(2);

  //  Build Date Part of Menu - Shows Version and Latest Compiled Time and Date
  //Serial.println(TimeDate);
  tft.drawCentreString(TimeDate, 160, 10, 1);  //  Build Version i.e. V# Time & Date

  int LnSpc = 40;

  //  #########################################################################

  //  City Location Part Of Menu  **********************
  int LTx = 10;
  int LTy = 10 + LnSpc;                  //  <<<<<<<
  tft.drawString("City:", LTx, LTy, 1);  //  5c (+1c) chracters == 6c x 12p = 72p

  //  if(PX > LRx && PX < (LRx + 158) && PY > LRy && PY < (LRy + 30)
  int LRx = LTx + 70;  //  == 80
  int LRy = LTy - 8;   //  8 pixels from Top of Box to Top of Text ** 35 - 8 = 27  ** 27 + 30 = 57

  tft.drawRoundRect(LRx, LRy, 158, 30, 4, TFT_LIGHTGREY);  //  Large Text Box

  //if(PX > (LRx + 160) && PY > LRy && PY < (LRy + 30)
  //if(PX > LocOKx && PY > LocOKy && PY < LocOKy + 30
  int LocOKx = LRx + 166;
  int LocOKy = LRy;

  tft.drawRoundRect(LocOKx, LocOKy, 30, 30, 4, TFT_LIGHTGREY);  //  Small OK Text Box  LRx + 160 = 290

  LTx = LRx + 6;
  tft.drawString(LocalCity, LTx, LTy, 1);
  tft.drawString("OK", LTx + 164, LTy, 1);

  //  #########################################################################

  //  WiFi part of Menu  ************************
  int WTx = 10;
  int WTy = LTy + LnSpc;                           //  <<<<<<<  +40
  tft.drawString("Start WiFi Menu", WTx, WTy, 1);  //  15c (+1c) chracters == 16c x 12p = 192p

  int WRx = WTx + 190;
  int WRy = WTy - 8;                                      //  35 - 8 = 27  >> 27 + 30 = 57
  tft.drawRoundRect(WRx, WRy, 30, 30, 4, TFT_LIGHTGREY);  // tft.drawRect(XRect, YRect, XRL, YRH, TFT_LIGHTGREY);
  tft.drawString("GO", WTx + 194, WTy, 1);

  //  #########################################################################

  //  Min Brightness part of Menu  ****************************
  int BTx = 10;
  int BTy = WTy + LnSpc;  //  <<<<<<<  +40 for eack line

  tft.drawString("Min Bright:", BTx, BTy, 1);  //  11c (+1c) chracters == 12c x 12p = 144p

  int BRx = BTx + 144;                                    //  == 154
  int BRy = BTy - 8;                                      //  35 - 8 = 27  ** 27 + 30 = 57
  tft.drawRoundRect(BRx, BRy, 90, 30, 4, TFT_LIGHTGREY);  //  Large Text Box

  int BrOKx = BRx + 98;
  int BrOKy = BTy - 8;
  tft.drawRoundRect(BrOKx, BrOKy, 30, 30, 4, TFT_LIGHTGREY);  //  Small OK Text Box  LRx + 160 = 290
  tft.drawString("OK", BrOKx + 4, BTy, 1);

  String MinBrightStr = "- " + String(MinBright) + " +";
  tft.drawString(MinBrightStr, BRx + 10, BTy, 1);

  //  #########################################################################

  //  Big Day Display part of Menu  **************************** BigDayDisplay
  int BDDx = 10;
  int BDDy = BrOKy + LnSpc + 2;  //  <<<<<<<  +40 for eack line

  tft.drawString("Day/Weather:", BDDx, BDDy, 1);  //  11c (+1c) chracters == 12c x 12p = 144p

  int BDRx = BDDx + 144;
  int BDRy = BDDy - 8;                                       //  35 - 8 = 27  ** 27 + 30 = 57
  tft.drawRoundRect(BDRx, BDRy, 150, 30, 4, TFT_LIGHTGREY);  //  Large Text Box

  if (BigDayDisplay)  //  ON
    {
      BigDayToggle = "Show Big Day";
    }
  else  //  OFF
    {
      BigDayToggle = "Show Weather";
    }

  tft.drawString(BigDayToggle, BDRx + 4, BDDy, 1);

  //  #########################################################################

  //  RETURN part of Menu  ************************
  int RnTx = 116;
  int RnTy = 200;
  int RnRx = 108;
  int RnRy = RnTy - 8;                                       //  35 - 8 = 27  >> 27 + 30 = 57
  tft.drawRoundRect(RnRx, RnRy, 120, 30, 4, TFT_LIGHTGREY);  // tft.drawRect(XRect, YRect, XRL, YRH, TFT_WHITE);
  tft.drawString(" RETURN ", RnTx + 4, RnTy, 1);

  //  #########################################################################

  //  Wled Brightness Touch Spot
  tft.drawRoundRect(10, 192, 70, 30, 4, TFT_LIGHTGREY);  // tft.drawRect(XRect, YRect, XRL, YRH, TFT_WHITE);
  tft.drawString("Wleds", 15, RnTy, 1);

  //  #########################################################################

  TimeOut = millis();

  //*******************************************************
  while (!OK)  //  While OK == false -->  Stays in Loop
    {
      TouchPxYx();

      //  #########################################################################

      if (PX < 80 && PY > 192 && TouchDone)
        {
          OK = true;

          if (Serial_ON)
            {
              Serial.println(" Wled Selected");
              Serial.print("PX = ");
              Serial.print(PX);
              Serial.print("   PY = ");
              Serial.println(PY);
            }

          WledMenu();
        }


      //  #########################################################################

      //  OK box  for  Location Part Of Menu  **********************
      if (PX > LocOKx && PY > LocOKy && PY < LocOKy + 30 && TouchDone)
        {
          OK = true;
          tft.fillRoundRect(LRx + 166 + 4, LTy - 8 + 4, 22, 22, 4, TFT_CYAN);

          //Serial.print("Selected LocalCity == ");
          //Serial.println(LocalCity);

          GoSaveLocation();  //  Save in EEPROM eLocalCity & eCountryCode

          GetForcast = true;
          GoWeather();
          TouchDone = false;
        }

      //  #########################################################################

      //  Rotate Location Names  for  Location Part Of Menu  **********************
      if (PX > LRx && PX < (LRx + 158) && PY > LRy && PY < (LRy + 30) && TouchDone)
        {
          DispCity++;
          if (DispCity > 9)
            {
              DispCity = 0;  //  Back to Top of List
            }

          LocalCity = LocalCityArray[DispCity][0];
          CountryCode = LocalCityArray[DispCity][1];

          //  clear text box
          //tft.drawRect(LRx, LRy, 158, 30, TFT_WHITE); //  Large Text Box
          tft.fillRoundRect(LRx + 1, LRy + 1, 156, 28, 4, TFT_BLUE);  //  Blank Large Text Box
          tft.drawString(LocalCity, LTx, LTy, 1);
        }

      //  #########################################################################

      //  StartWiFiMenu()  ***********************
      if (PY > WRy && PY < (WRy + 30) && TouchDone)
        {
          OK = true;
          tft.fillRoundRect(WRx + 4, WRy + 4, 22, 22, 4, TFT_CYAN);
          TouchDone = false;
          delay(500);
          StartWiFiMenu();
        }

      //  #########################################################################

      //  MinBright part of Menu  ****************************
      // MinBright UP
      if (PX > BRx + 45 && PY > BRy && PX < BRx + 90 && PY < BRy + 30 && TouchDone)
        {
          MinBright++;

          //Serial.print("MinBright ");
          //Serial.println(MinBright);

          if (MinBright > 98)
            {
              MinBright = 99;       //  Set Max MinBright at 99
              MinBrightSw = false;  // MinBrightness Switched OFF
            }
          else
            {
              MinBrightSw = true;  // MinBrightness Switched ON
            }
          tft.fillRoundRect(BRx + 1, BRy + 1, 88, 28, 4, TFT_BLUE);  //  Blank Out Text Box

          MinBrightStr = "- " + String(MinBright) + " +";
          tft.drawString(MinBrightStr, BRx + 10, BTy, 1);

          //tft.drawNumber(MinBright, BRx + 25, BTy, 1);
          if (MinBrightSw)
            {
              ledcWrite(ScreenLED, MinBright);  //  Turn on LED Up in Brightness
            }
          else
            {
              ledcWrite(ScreenLED, Brightness);  //  Turn on LED to Normal Brightness
            }
        }

      //  #########################################################################

      // MinBright DN
      if (PX > BRx && PY > BRy && PX < BRx + 45 && PY < BRy + 30 && TouchDone)
        {
          MinBright--;
          MinBrightSw = true;  // MinBrightness Switched ON

          //Serial.print("MinBright ");
          //Serial.println(MinBright);

          if (MinBright < 1)
            {
              MinBright = 1;
            }
          tft.fillRoundRect(BRx + 1, BRy + 1, 88, 28, 4, TFT_BLUE);  //  Blank Out Text Box

          MinBrightStr = "- " + String(MinBright) + " +";
          tft.drawString(MinBrightStr, BRx + 10, BTy, 1);

          //tft.drawNumber(MinBright, BRx + 25, BTy, 1);

          ledcWrite(ScreenLED, MinBright);  //  Turn on LED Dn in Brightness
        }

      //  #########################################################################

      // MinBright OK
      if (PX > BrOKx && PY > BrOKy && PX < BrOKx + 30 && PY < BrOKy + 30 && TouchDone)
        {
          OK = true;
          tft.fillRoundRect(BrOKx + 4, BrOKy + 4, 22, 22, 4, TFT_CYAN);

          // write MinBright to EEPROM(83)
          if (Serial_ON)
            {
              Serial.println("Writting MinBright to EEPROM");
            }

          preferences.begin("ClockData", RW_MODE);
          preferences.putUInt("MinBright", MinBright);
          preferences.end();

          delay(50);
        }

      //  #########################################################################

      //  Big Day Display part of Menu  **************************** BigDayDisplay
      if (PX > BDRx && PY > BDRy && PX < BDRx + 150 && PY < BDRy + 30 && TouchDone)  // CANCEL
        {
          if (BigDayDisplay)  //  Toggle OFF
            {
              BigDayDisplay = false;
              //Serial.println(">>>> BigDayDisplay == false  -- Line 712");
              DoBigDay = false;

              //  Dispaly Mode ==> Weather
              tft.drawRoundRect(BDRx, BDRy, 150, 30, 4, TFT_LIGHTGREY);  //  Large Text Box
              BigDayToggle = "Show Weather";
              tft.drawString(BigDayToggle, BDRx + 4, BDDy, 1);

              // write BigDayDisplay to EEPROM(89)
              if (Serial_ON)
                {
                  Serial.println("Writting BigDayDisplay (false) to EEPROM");
                }

              preferences.begin("ClockData", RW_MODE);
              preferences.putBool("BigDayDisplay", BigDayDisplay);
              preferences.end();

              delay(50);
            }
          else  //  Toggle ON
            {
              BigDayDisplay = true;
              ShowDayDate = true;

              //  Dispaly Mode ==> Big Day Display
              tft.drawRoundRect(BDRx, BDRy, 150, 30, 4, TFT_LIGHTGREY);  //  Large Text Box
              BigDayToggle = "Show Big Day";
              tft.drawString(BigDayToggle, BDRx + 4, BDDy, 1);

              //Serial.println(">>>> BigDayDisplay == true  -- Line 1414");
              DoBigDay = true;
              // GetForcast = true;  // Gaz - This is not needed here

              // write BigDayDisplay to EEPROM(89)
              if (Serial_ON)
                {
                  Serial.println("Writting BigDayDisplay (true) to EEPROM");
                }

              preferences.begin("ClockData", RW_MODE);
              preferences.putBool("BigDayDisplay", BigDayDisplay);
              preferences.end();

              delay(50);
            }
        }

      //  #########################################################################

      //  RETURN part of Menu  ************************
      if (PX > RnRx && PY > RnRy && PX < RnRx + 120 && PY < RnRy + 30 && TouchDone)  // CANCEL
        {
          OK = true;
          tft.fillRoundRect(RnRx + 4, RnRy + 4, 112, 22, 4, TFT_CYAN);
          tft.setTextColor(TFT_BLUE, TFT_CYAN);  //  Reverse Colours >BLUE< Text and >CYAN< Background
          tft.drawString(" RETURN ", RnTx + 4, RnTy, 1);
          delay(500);
        }

      //  #########################################################################

      if (TouchDone)
        {
          TimeOut = millis();
        }

      if (millis() > TimeOut + 5000)
        {
          OK = true;  // exit While Loop after 5 seconds
        }

      TouchDone = false;
    }
}

void StartWiFiMenu()  //  *****************************************************
{
  //Serial.println("void StartWiFiMenu()");

  int Tx = 160;  //  Middle of screen
  int Ty = 35;

  tft.fillScreen(TFT_BLUE);  //  clears screen to blue
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLUE);
  tft.setTextSize(2);

  tft.drawCentreString(">>>  YES  <<<", Tx, Ty, 1);

  Ty = 110;
  tft.drawCentreString("***  WiFiManager?  ***", Tx, Ty, 1);

  Ty = 240 - 45;
  tft.drawCentreString(">>>  NO  <<<", Tx, Ty, 1);

  NoSelection = true;
  TouchDone = false;

  int WiFiMenuDelay = 2000;  //  only display access to WiFiManager for 2 seconds
  uint32_t LastMenuDelay = millis();

  while (millis() - LastMenuDelay < WiFiMenuDelay)  //  true will LOOP  NoSelection
    {
      TouchPxYx();

      if (PY < 110 && TouchDone)
        {
          //  YES
          NoSelection = false;    //  Selection Made
          SelectionYesNo = true;  //  Yes go to WiFiManager
          //Serial.println("Pressed YES");
          TouchDone = false;
        }
      else if (PY > 130 && TouchDone)
        {
          //  NO
          NoSelection = false;     //  Selection Made
          SelectionYesNo = false;  //  NO
          //FailAttempt = 0;
          //Serial.println("Pressed NO");
          TouchDone = false;
        }
    }
}

void BrightnessDisplay()  //  *************************************************
{
  //Serial.println("void BrightnessDisplay()");

  tft.fillRect(280, 43, 40, 111, BkgCol);  //  154 - 43 = 111

  //Serial.print("BarNumb == ");
  //Serial.println(BarNumb);

  for (i = 1; i < BarNumb + 1; i++)
    {
      //  Draw lines between X = 145 ~ 50
      int Ly = map(i, 1, 10, 145, 50);
      tft.drawLine(285, Ly, 315, Ly, TFT_LIGHTGREY);
    }

  BrightCntDwn = 750;  //  Hold display bars for 0.75 Sec
  BrightnessBarsStart = millis();
  DelayTime = true;
}

void GoLDR()  //  *******************************************************
{
  if (millis() - LastLdrDelay > LdrDelay)  //  Avoid millis() overflow
    {
      LdrDelay = 10000;  //  10 sec delay - if your hand shadows the LDR the screen won't dim straight away
      LastLdrDelay = millis();

      double SumLDR;
      for (i = 0; i < 10; i++)
        {
          SumLDR = SumLDR + analogRead(pResLDR);  //  Using 82k Resistor
          delay(5);
        }

      ReadLDR = SumLDR / 10;
      //Serial.print("ReadLDR == ");
      //Serial.println(ReadLDR);

      if (Testing)
        {
          //  For Testing without LDR connected- GAZ
          //Serial.println("Testing without LDR * LdrDimmer set to 100 - Line 1548");
          ReadLDR = 100;
        }

      LdrDimmer = ReadLDR;

      //Serial.print("LdrDimmer = ");
      //Serial.println(LdrDimmer);

      // tft.drawString(String(LdrDimmer), 140, 89, 2);  //  only used for testing

      //Set NightTime Brightness
      if (LdrDimmer >= 9)  //  Turn UP screen  was 10
        {
          SetLdrDimmer = Brightness;
          LdrDelay = 10000;  // was 10000 == 10 sec - if your hand shadows the LDR the screen won't dim
        }
      else  //  Turn DOWN Screen
        {
          SetLdrDimmer = MinBright;
          LdrDelay = 500;  //  1/2 sec - Brightens quickly if Light is turned ON

          //Serial.println("Checking SunRise Dimmer");

          //  Test for SunRise plus 60mins  atoi(String); //converts string to int
          if (DecHourMin >= DecSunRise && DecHourMin < DecSunSet)  // Check for whole day is case bad weather
            {
              if (Testing)
                {
                  Serial.println("Testing SetDimmer Settings for SunRise - Line 1577");
                }

              SetLdrDimmer = SetLdrDimmer + 9;  //  Make Screen slightly brighter than min

              //  Turn OFF Wled if ON
              if (!WledState)  //  If Wled ON, turn OFF
                {
                  FastLED.setBrightness(0);  // Turn OFF Wleds

                  WledState = false;
                  WledON = false;  //  Reset after Wleds turned OFF

                  FastLED.show();
                  FastLED.delay(10);
                }
            }
        }

      ledcWrite(ScreenLED, SetLdrDimmer);  // No Lights on -- Turn down LED to LdrDimmer setting

      //Serial.print("ReadLDR == ");
      //Serial.print(ReadLDR);
      //Serial.print("   SetLdrDimmer == ");
      //Serial.println(SetLdrDimmer);
    }
}

void DisplayOutTemp(int ListNumb)  //  ********************************************
{
  //Serial.print("Outside Temperature: ");
  //Serial.println(myObject["list"][ListNumb]["main"]["temp"]);
  //Serial.println("*********************");

  tft.fillRect(0, 157, 107, 83, BkgCol);  //  black out Outside Temp when neccessary

  double TempC = myObject["list"][ListNumb]["main"]["temp"];

  int DecTemp = abs((TempC - (int)(TempC)) * 10.00);  //  Use with Deg plus Decimal Deg

  int xT = 15;
  int yT = yTime1Slot;  //  160;  to align all text in bottom area

  tft.setTextSize(1);
  tft.setTextFont(8);
  tft.setTextColor(TFT_GREEN, BkgCol);
  tft.drawString("Outside Temp", xT, yT, 2);

  int SNMx = 0;  //  SNMx == Single Number Mod x  Just for Whole number under 10

  if (TempC < 10 && TempC > -10)
    {
      SNMx = 25;
    }

  yT = yT + 20;
  tft.setTextFont(4);
  tft.drawNumber(TempC, xT + SNMx, yT, 6);  // drawNumber == int  ::  drawFloat == float number

  tft.setCursor(xT + 55, yT - 4);  //  -4 for Decimal Rereadout
  tft.print(String("`c"));

  // Displays Decimal degrees
  tft.setCursor(xT + 58, yT + 19);
  tft.print(String("."));
  tft.drawNumber(DecTemp, xT + 65, yT + 17, 4);

  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, BkgCol);
  xT = xT + 10;
  yT = yT + 40;
  tft.drawString(LocalCity, xT, yT, 2);
}

void GetOutsideTemp()  //  ***************************************************
{
  // Display Tank Readings and Current Outside Temp

  ClockPanelDisp(1);  //  Show Tank Readings for 10 secs

  DisplayOutTemp(0);  //  Goto void DisplayOutTemp() where 0 == ListNumb

  //  Show Sunset and Sunrise Icons and Times
  //  For SunRise --> Slot 1   For SunSet --> Slot 2
  ShowForecast("SunRise", "SunSet");  //  Go get Forecast Icons for each Time Slot

  //  SunRise or SunSet Icons Times as Text
  tft.setTextSize(1);
  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, BkgCol);

  String SunRiseTime = SunRiseHour + ":" + SunRiseMin + "am";

  tft.drawCentreString(SunRiseTime, xTime1Slot, yTime1Slot, 2);

  String SunSetTime = "0" + String(SunSetHour.toInt() - 12) + ":" + SunSetMin + "pm";

  tft.drawCentreString(SunSetTime, xTime2Slot, yTime2Slot, 2);

  DailyUpdateCall = true;  //  Clears Sunrise amd Sunset Icons and replaces them with Forecasts.
                           //  Is cleared in TemperatureSensor()

  SensorDelay = 5000;  //  This maintains the display of the Outside Temp and Sun Rise/Set for 5 sec

  LastSensorDelay = millis();
}

void CleanTimeSlot()
{
  //Serial.println("void CleanTimeSlot()");
  //  clear Area before new update display
  tft.fillRect(0, 42, 320, 110, BkgCol);
  ShowTime();
}

void UpDateGraphics(String TextDisplay)  //  ******************************************************************
{
  //Serial.print("void UpDateGraphics(String TextDisplay) == ");
  //Serial.println(TextDisplay);

  //  Blackout Rect    //  Time Area  0, 42, (w) 320, (h) 110
  //  Most Warnings or Info Notices are Displayed in the Bottom Section
  int Xlng = TextDisplay.length() * 14;  //190;
  int Xtc = 217 - Xlng / 2;              //217 == centre of Text to be Displayed
  int Ytc = 180;                         //  down in the Forecast Area

  if (TextDisplay == " Forecasts ")  //  This Display in Top Section Only
    {
      Xtc = 164;
      Ytc = 0;
    }

  int Ylng = 40;

  int XRect = Xtc + 5;  //  1st White Box around Text
  int YRect = Ytc + 5;
  int XRL = Xlng - 10;
  int YRH = Ylng - 10;

  int Xtxt = XRect + 8;  //  Text
  int Ytxt = YRect + 8;

  tft.fillRect(Xtc, Ytc, Xlng, Ylng, TFT_BLUE);

  //  Draw 2 White Boxes - 1 box is not thick enough
  tft.drawRect(XRect, YRect, XRL, YRH, TFT_LIGHTGREY);
  tft.drawRect(XRect + 1, YRect + 1, XRL - 2, YRH - 2, TFT_LIGHTGREY);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLUE);
  tft.setTextSize(2);
  tft.setTextFont(1);
  tft.setCursor(Xtxt, Ytxt);
  tft.print(TextDisplay);  //  Update Forcast
}

void WiFiGraphics()  //  ************UpDateGraphics("Checking Internet");***********************************************************
{
  //Serial.println("void WiFiGraphics()");

  //  Print some Info to TFT_Screen
  tft.fillScreen(TFT_BLUE);  //  clears screen to blue
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLUE);
  tft.setTextSize(2);
  tft.setTextFont(1);

  int MenuY = 10;
  //Gazza's Time and Weather
  tft.drawCentreString("Gazza's Time:Weather:Tank", 160, MenuY, 1);  //  was 2

  MenuY = MenuY + 23;

  tft.drawCentreString("Connecting to", 160, MenuY, 1);  //  + 23  was 25

  MenuY = MenuY + 25;
  drawBitmap(96, MenuY, Graphic_WIFI, 128, 89, TFT_CYAN);  //  + 27  was 50

  MenuY = MenuY + 96;
  tft.drawCentreString(ssid, 160, MenuY, 1);  //   was 148

  MenuY = MenuY + 24;
  //  Shows Version and Latest Compiled Time and Date
  tft.drawCentreString(TimeDate, 160, MenuY, 1);  //  + 24  was  172

  MenuY = MenuY + 28;
  tft.drawCentreString("Created August 2022", 160, MenuY, 1);  //  + 28  was 200
}

//  ****************************************************************************************
void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color)
{
  int16_t i, j, byteWidth = (w + 7) / 8;
  uint8_t byte;

  for (j = 0; j < h; j++)
    {
      for (i = 0; i < w; i++)
        {
          if (i & 7) byte <<= 1;
          else byte = pgm_read_byte(bitmap + j * byteWidth + i / 8);
          if (byte & 0x80) tft.drawPixel(x + i, y + j, color);
        }
    }
}

void GoWiFiManager()  //  ***********************************************
{
  //Serial.println("void GoWiFiManager()");

  int Tx = 160;  //  Middle of screen
  int Ty = 35;

  tft.fillScreen(TFT_BLUE);  //  clears screen to blue
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLUE);
  tft.setTextSize(2);

  tft.drawCentreString("*** New  WiFi? ***", Tx, Ty, 1);

  Ty = Ty + 35;
  tft.drawCentreString("Connect to", Tx, Ty, 1);

  Ty = Ty + 25;
  tft.drawCentreString(">>>  WiFiManager  <<<", Tx, Ty, 1);

  Ty = Ty + 25;
  tft.drawCentreString("Access Point Network", Tx, Ty, 1);

  Ty = Ty + 35;
  tft.drawCentreString("on nearest Device.", Tx, Ty, 1);

  Ty = Ty + 25;
  tft.drawCentreString("http://192.168.4.1", Tx, Ty, 1);  //  Built in to the Header File
}

void WiFiSetup()  //  *******************************************************
{
  //Serial.println("void WiFiSetup() for WiFiManager");

  //  Show No Internet Screen and instructions to goto WiFiManager

  GoWiFiManager();

  //Serial.println("Starting WIFI_STA and then WiFiManager");

  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  // put your setup code here, to run once:
  //Serial.begin(115200);

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  //Serial.println(">>Checking Connection<<");

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap

  /*  wm.autoConnect loops in the Header file until a conncetion is made
      so I can't tell the user via the TFT screen that the WiFi has failed
      if an incorrect WiFi name or password has been entered  */
  res = wm.autoConnect("WiFiManager", "password");  // password protected ap

  /*
  if (!res)  // For testing only
    {  //it doesn't do the next line - WiFiManager only returns here after Network found
      //Serial.println(">>Failed to connect<<");
      // ESP.restart();
    }
  else
    {
      //it doesn't do the next line - WiFiManager only returns here after Network found
      //if you get here you have connected to the WiFi
      //Serial.println(">>>>>>>>  connected...Yay!! :)  <<<<<<<<<");
    }
    */

  String Strssid = wm.getWiFiSSID();
  String Strpassword = wm.getWiFiPass();

  //Serial.print("WiFi wm.getWiFiSSID == ");
  //Serial.println(wm.getWiFiSSID());

  //Serial.print("WiFi wm.getWiFiPass == ");
  //Serial.println(wm.getWiFiPass());

  ssid = wm.getWiFiSSID();
  password = wm.getWiFiPass();

  Silent = false;

  SaveClockData = true;  //  Only Save to EEPROM if connection is validated

  SetUpWiFi();

  TryWiFi();
}

void SaveWiFiData()
{

  //Serial.println("void SaveWiFiData()");

  UpDateGraphics("Save WiFi > EEPROM");

  preferences.begin("ClockData", RW_MODE);

  preferences.putString("ssid", ssid);

  preferences.putString("password", password);

  preferences.end();

  delay(50);  //  Time for UpDateGraphics to be displayed.
  CleanTimeSlot();
}

void SetUpWiFi()  //  *************************************************************
{

  //Serial.println("void SetUpWiFi() >>> Restart WiFi");

  //Serial.println("Attempt to Connect to LAN");

  WiFi.mode(WIFI_STA);  //   create wifi station

  //Serial.println();
  //Serial.print("(1) ssid ==");
  //Serial.print(ssid);
  //Serial.print("   password ==");
  //Serial.println(password);

  WiFi.begin(ssid.c_str(), password.c_str());

  //Serial.println();
  //Serial.print("(2) ssid ==");
  //Serial.print(ssid);
  //Serial.print("   password ==");
  //Serial.println(password);*/

  delay(1000);
}

void TryWiFi()  //  *****************************************************************
{

  //Serial.print("void TryWiFi() - 1st check - InternetFail (true = 1) == ");
  //Serial.println(InternetFail);

  /*  if(InternetFail)
    {
      SetUpWiFi();
      FailAttempt++;
      Serial.print("FailAttempt == ");
      Serial.println(FailAttempt);
    }*/

  TimeOut = 0;

  InternetFail = true;

  while (InternetFail)  //  LOOP while not connected
    {
      TimeOut = 0;

      //Serial.println("while (InternetFail) ");

      while (WiFi.status() != WL_CONNECTED && TimeOut < 25)
        {
          //Serial.print(".");

          TimeOut++;

          if (!Silent)
            {
              tft.setCursor(10, 220);
              tft.print(DOT);
              DOT = DOT + ".";
            }
          delay(500);
        }

      if (TimeOut < 25)
        {
          if (Serial_ON)
            {
              Serial.println("WiFi connection OK!");
            }

          InternetFail = false;
          if (SaveClockData)
            {
              SaveWiFiData();  //  GoWriteEEPROM();
              SaveClockData = false;
            }

          if (TimeOut > 0)  //  Not zero
            {
              String WiFiName = WiFi.localIP().toString().c_str();  //.c_str();
              tft.drawCentreString(WiFiName, 160, 70, 2);

              RssiRead = WiFi.RSSI();

              tft.drawCentreString(String(RssiRead), 160, 110, 2);

              delay(2000);
              tft.fillScreen(BkgCol);
            }
        }
      else if (FirstBoot)  //  set to true at Boot Time -- Set to False at Line 1955 after successful WiFi
        {
          StartWiFiMenu();  //  Menu  >>  Yes Or NO

          if (SelectionYesNo)  //  true ==> YES
            {
              WiFiSetup();  //  WiFiManager
            }
          else  //  NO
            {
              //  try again
              DOT = ".";
              WiFiGraphics();
              SetUpWiFi();  //  Restart WiFi
            }
        }
      else  //  TimeOut > 25
        {
          InternetFail = true;
          //Serial.println("No Internet - Try Later--  Will BREAK from while (InternetFail) ");
          UpDateGraphics("Check Internet");

          break;
        }
    }
}

void CheckWiFi()  //  *******************************************************************
{
  //Serial.println("void CheckWiFi()");

  while (TimeOut > 25)
    {
      UpDateGraphics("No Internet");
      //Serial.println("No Internet");
      delay(2000);

      //  Blank Screen
      tft.fillRect(0, 0, 320, 240, BkgCol);
      WiFiSetup();  //  WiFiManager

      TryWiFi();
    }

  //Serial.print("FailAttempt == ");
  //Serial.println(FailAttempt);

  if (FailAttempt > 1)
    {
      FailAttempt = 0;
      InternetFail = false;
      //StartWiFiMenu();
    }
}

void GoInternet()  //  ******************************************************************
{
  //Serial.println("void GoInternet()");
  delay(100);

  preferences.begin("ClockData", RO_MODE);

  ssid = preferences.getString("ssid", "Null");

  password = preferences.getString("password", "Null");

  if (Serial_ON)
    {
      Serial.print("ssid :");
      Serial.println(ssid);
      Serial.print("password :");
      Serial.println(password);
    }

  preferences.end();

  //  If ssid NULL - do WiFiManager
  if (ssid == NULL)
    {
      //Serial.println("2.. ssid == NULL");
      StartWiFiMenu();  // -- Gives a choice to goto WifiManager or not

      if (NoSelection)  //  ==  true or YES
        {
          WiFiSetup();  // WiFiManager
        }
    }

  if (!Silent)
    {
      WiFiGraphics();
    }

  //need to connect to WiFi to get internet GMT time
  //Serial.println("Connecting to :");
  //Serial.println(ssid);

  SetUpWiFi();  //  Restart WiFi
  TryWiFi();    //  See if WiFi is connection or open

  //  CheckWiFi();  // may not be needed  -  rechecks timeout value - LOOP

  //  everthing should be good...
  FirstBoot = false;

  Silent = true;  //  do not show WiFiGraphics() screen
  if (Serial_ON)
    {
      Serial.println("");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
}

void GetWWWTime()  //  *****************************************************
{
  //Serial.println("void GetWWWTime()");

  if (CheckTime)
    {
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  //  This one line get GMT time from ntpServer.. :)

      struct tm timeinfo;

      int FailCnt = 0;

      while (!getLocalTime(&timeinfo))
        {
          FailCnt++;
          //Serial.print("Failed to obtain time :");
          //Serial.println(FailCnt);

          SwitchOffLoRa();  //  To Write to TFT Screen

          tft.fillScreen(TFT_RED);
          tft.setTextColor(TFT_LIGHTGREY, TFT_RED);
          tft.setTextSize(2);
          tft.drawCentreString("Failed to obtain time", 160, 100, 1);
          tft.drawCentreString("No Internet??", 160, 130, 1);
          tft.drawNumber(FailCnt, 150, 170, 1);

          delay(2000);

          SwitchOnLoRa();

          configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  //  This one line get GMT time.. :)

          tm timeinfo;
        }

      Year = timeinfo.tm_year + 1900;  //  Add 1900
      //Serial.println(Year);

      Month = timeinfo.tm_mon + 1;  //  Where Jan = timeinfo.tm_mon + 1 for RTC
      //Serial.println(Month);

      nDay = timeinfo.tm_mday;  //  Day Date Number 1 -> 28, 30 or 31
      //Serial.print("First time get nDay == ");
      //Serial.println(nDay);

      CurrentDate = String(nDay) + "/" + String(Month) + "/" + String(Year);

      wDay = timeinfo.tm_wday;  //  day of week [0,6] (Sunday = 0)

      LstDay = nDay;  //  used for Time update at Midnight.

      Hour = timeinfo.tm_hour;

      Minute = timeinfo.tm_min;


      //  GAZ TIME CHECK - Start Time anytime HERE
      /*if(DoOnce)  //  **************Testing***************
      {
       Hour = 8;     //  ***************Testing******************
       Minute = 54;  //  *************Testing******************
       DoOnce = false;

       Serial.print("Reset Time to before ");
       Serial.print(Hour + 1);
       Serial.println(" <<<<<<<<<<<<<<<");
       Serial.println();
      }*/

      //Serial.println(Minute);

      Second = timeinfo.tm_sec;

      /* if(Second + 1 < 60)
      {
      Second = Second + 1;
      }*/

      //Serial.println(Second);

      //  To Set Time in RTC from Internet Time - Put updated Time into RTC
      //  rtc.adjust(DateTime(Year, Month, nDay, Hour, Minute, Second));
      //Serial.println("******************************************************");
      //Serial.print("NTP Day & Time : ");
      //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      //Serial.println("******************************************************");

      TimeSync = true;  //  This should only be here

      FirstMillis = millis();  //  Using this method eliminates possible millis() overflow after 49+ days

      /*  //  This method will be affected by millis() overflow at 49+ days
      CountSec = 1;        //  need to reset at MidNight
      targetTime = FirstMillis + CountSec * 1000;  //  this ensure that the time is locked to internal ESP32 clock
      */


      //  Found this -->  to put Above Formatted &timeinfo Output into a single string
      /*
      char timeStringBuff[50]; //50 chars should be enough
      strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
      //print like "const char*"
      Serial.println(timeStringBuff);

      //Optional: Construct String object
      String asString(timeStringBuff);*/
    }
}

void GoWeatherMap()  //  ****************************************************************
{
  if (Serial_ON)
    {
      Serial.println("void GoWeatherMap()");
    }

  //Serial.print("Length(jsonBuffer) Before Getting New Data == ");
  //Serial.println(jsonBuffer.length());

  //String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + LocalCity + "," + CountryCode + "&units=metric&APPID=" + APIKEY;

  //SwitchOffLoRa();

  //digitalWrite(TOUCH_CS, HIGH);  //  deselect Touch
  //digitalWrite(TFT_CS, HIGH);    //  deselect Screen

  //  https://openweathermap.org/forecast   5 day / 3 hourly forecast data  ==> LocalCity
  String serverPath = "http://api.openweathermap.org/data/2.5/forecast?q=" + LocalCity + "," + CountryCode + "&units=metric&APPID=" + APIKEY;

  //  https://pro.openweathermap.org/data/2.5/forecast/hourly?lat=35&lon=139&appid={API key}
  //String APIKEYhourly = "c834c5045583e40972f8ae334fef0e66";
  //String serverPath = "https://pro.openweathermap.org/data/2.5/forecast/hourly?q=" + LocalCity + "," + CountryCode + "&units=metric&APPID=" + APIKEYhourly;

  //jsonBuffer = httpGETRequest(serverPath.c_str());

  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverPath.c_str());

  //Send HTTP POST request
  int httpResponseCode = http.GET();

  if (Serial_ON)
    {
      Serial.print("httpResponseCode == ");
      Serial.println(httpResponseCode);
    }

  String jsonBuffer;

  if (httpResponseCode > 0)
    {
      // Parse response
      //DynamicJsonDocument jsonBuffer(2048);
      jsonBuffer = http.getString();

      deserializeJson(myObject, jsonBuffer);
    }

  if (Serial_ON)
    //if (false)  //  make >true< to printout the Json File..
    {
      Serial.print("NEW Length(jsonBuffer) == ");
      int jBufferLen = jsonBuffer.length();
      Serial.println(jBufferLen);

      //  Printout Data in one continual string
      Serial.print("jsonBuffer = ");  // Will be jumbled but just copy it and paste to notepad
      Serial.println(jsonBuffer);
      Serial.println();

      Serial.println("Created NEW myObject = JSON.parse(jsonBuffer)");
      //  Printout Data in one continual string
      Serial.print("JSON myObject = ");  // Will be jumbled but just copy it and paste to notepad

      //Serial.println(myObject);
      // Print the result in Formatted Json
      //SerializeJsonPretty(myObject, Serial);
      Serial.println();
    }

  int JsonCnt = 0;

  // Free resources
  http.end();

  //SwitchOnLoRa();
}

void CheckInternet()  //  *************************************************
{
  //Serial.println("void CheckInternet()");

  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED)
    {
      //Serial.print("WiFi.status : ");
      //Serial.println(WiFi.status());   //  3 == Connected??
      InternetFail = false;
    }
  else
    {
      //Serial.println("WiFi Disconnected - Will try Reconnect");
      Silent = true;
      InternetFail = true;

      GoInternet();  //  hopefully reconnect
    }
}

void Case10am()  //  *************************************************
{
  for (i = 0; i < 10; i++)
    {
      //Serial.println(myObject["list"][i]["dt_txt"]);

      TestTime = (const char*)myObject["list"][i]["dt_txt"];  //  Added (const char*) as a new library must have changed the process

      //Serial.print("Test TimeSlot :");
      //Serial.println(TestTime);
      //Serial.println(TestTime.substring(11, 13));

      //  Time == 10am Brisbane == 00:00 UTC same day..
      if (TestTime.substring(11, 13) == "00")  //  00 == 10am
        {
          TimeSlot1st = i;

          //  look for Thunderstorm or Drizzle
          ForeCast1stSlot = myObject["list"][TimeSlot1st]["weather"][0]["main"];

          if (String(ForeCast1stSlot) != "Thunderstorm" || String(ForeCast1stSlot) != "Drizzle")
            {
              ForeCast1stSlot = myObject["list"][TimeSlot1st]["weather"][0]["description"];
            }

          ForeCast1stSlotStr = String(ForeCast1stSlot);
          //Serial.print("Found 10am time slot: ");
          //Serial.println(i);

          //  Time == 4pm Brisbane == 06:00 UTC same day..
          //  Therefore TimeSlot2nd == TimeSlot1st + 2
          TimeSlot2nd = TimeSlot1st + 2;

          //  look for Thunderstorm or Drizzle
          ForeCast2ndSlot = myObject["list"][TimeSlot2nd]["weather"][0]["main"];

          if (String(ForeCast2ndSlot) != "Thunderstorm" || String(ForeCast2ndSlot) != "Drizzle")
            {
              ForeCast2ndSlot = myObject["list"][TimeSlot2nd]["weather"][0]["description"];
            }
          ForeCast2ndSlotStr = String(ForeCast2ndSlot);
          //Serial.print("Therefore 4pm time slot == ");
          //Serial.println(TimeSlot2nd);

          break;  //  to break out of for() loop
        }
    }

  //Serial.print("Weather @ 10am: ");
  //Serial.println(myObject["list"][TimeSlot1st]["dt_txt"]);
  //Serial.println(ForeCast1stSlotStr);    //  myObject["list"][TimeSlot1st]["weather"][0]["description"]);


  //Serial.print("Weather @ 4pm: ");
  //Serial.println(myObject["list"][TimeSlot2nd]["dt_txt"]);
  //Serial.println(ForeCast2ndSlotStr);   //  myObject["list"][TimeSlot2nd]["weather"][0]["description"]);
  //Serial.println("***************************");

  //  go get variables for Forecast
  ShowForecast(ForeCast1stSlotStr, ForeCast2ndSlotStr);  //  Go get Forecast Icons for each Time Slot

  SwitchOffLoRa();

  //  Forecasts Icons Text Names
  tft.setTextSize(1);
  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, BkgCol);

  String CityForecast = "9am " + ForeCast1stSlotStr;  //   String(City) +
  tft.drawCentreString(CityForecast, xTime1Slot, yTime1Slot, 2);

  CityForecast = "3pm " + ForeCast2ndSlotStr;  //  String(City) +
  tft.drawCentreString(CityForecast, xTime2Slot, yTime2Slot, 2);

  SwitchOnLoRa();
}

void Case4pm()  //  *************************************************
{
  for (i = 0; i < 10; i++)
    {
      //Serial.println(myObject["list"][i]["dt_txt"]);

      TestTime = (const char*)myObject["list"][i]["dt_txt"];  //  Added (const char*) as a new liburary must have changed the process

      //  Time == 4pm Brisbane == 06:00 UTC same day..
      if (TestTime.substring(11, 13) == "06")  //  06 UTC == 4pm Brisbane
        {
          //Serial.print("Test TimeSlot : ");
          //Serial.println(TestTime);

          TimeSlot1st = i;

          //  look for Thunderstorm or Drizzle in Json "main"
          ForeCast1stSlot = myObject["list"][TimeSlot1st]["weather"][0]["main"];

          if (String(ForeCast1stSlot) != "Thunderstorm" || String(ForeCast1stSlot) != "Drizzle")
            {
              ForeCast1stSlot = myObject["list"][TimeSlot1st]["weather"][0]["description"];
            }

          ForeCast1stSlotStr = String(ForeCast1stSlot);
          //Serial.print("Found 4pm time slot: ");
          //Serial.println(i);

          //  Found (06 UTC) 4pm next Timeslot is 7pm so add 1
          TimeSlot2nd = TimeSlot1st + 1;

          //  look for Thunderstorm or Drizzle in Json "main"
          ForeCast2ndSlot = myObject["list"][TimeSlot2nd]["weather"][0]["main"];

          if (String(ForeCast2ndSlot) != "Thunderstorm" || String(ForeCast2ndSlot) != "Drizzle")
            {
              ForeCast2ndSlot = myObject["list"][TimeSlot2nd]["weather"][0]["description"];
            }

          ForeCast2ndSlotStr = String(ForeCast2ndSlot);
          //Serial.print("Therefore 7pm time slot: ");
          //Serial.println(TimeSlot2nd);

          break;  //  to break out of for() loop
        }
    }

  //Serial.print("Weather @ 4pm: ");
  //Serial.println(myObject["list"][TimeSlot1st]["dt_txt"]);
  //Serial.println(ForeCast1stSlotStr);    //  myObject["list"][TimeSlot1st]["weather"][0]["description"]);


  //Serial.print("Weather @ 7pm: ");
  //Serial.println(myObject["list"][TimeSlot2nd]["dt_txt"]);
  //Serial.println(ForeCast2ndSlotStr);   //  myObject["list"][TimeSlot2nd]["weather"][0]["description"]);
  //Serial.println("***************************");

  //  go get variables for Forecast
  ShowForecast(ForeCast1stSlotStr, ForeCast2ndSlotStr);  //  Go get Forecast Icons for each Time Slot

  SwitchOffLoRa();

  //  Forecasts Icons Text Names
  tft.setTextSize(1);
  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, BkgCol);

  String CityForecast = "3pm " + ForeCast1stSlotStr;  //   String(City) +
  tft.drawCentreString(CityForecast, xTime1Slot, yTime1Slot, 2);

  CityForecast = "6pm " + ForeCast2ndSlotStr;  //  String(City) +
  //Serial.println(ForeCast2ndSlotStr);  //  just a check for correct string
  tft.drawCentreString(CityForecast, xTime2Slot, yTime2Slot, 2);

  SwitchOnLoRa();
}

void ShowForecast(String Slot1, String Slot2)  //  ************************************************************
{
  //Serial.println("void ShowForecast(String Slot1, String Slot2)");

  //  Draw Weather Forecasts in lower 2 Boxes..
  //  Also Draw SunRise and SunSet Icons and Times with Outside Temperature when requested

  XX = 110;  //  for blacking out Icons
  YY = 157;
  Xw = 107;  //  217 - 108 == 109
  Yh = 83;   //  240 - 157 == 83

  tft.fillRect(XX, YY, Xw, Yh, BkgCol);  //  black out Icons and Descriptions
  tft.fillRect(XX + Xw + 2, YY, Xw, Yh, BkgCol);

  int S1x = 135;

  int BitMapColour = TFT_LIGHTGREY;

  if (Slot1 == "clear sky") drawBitmap(S1x, 180, clearsky, 60, 60, BitMapColour);

  if (Slot1 == "few clouds") drawBitmap(S1x, 180, fewclouds, 60, 60, BitMapColour);

  if (Slot1 == "scattered clouds")
    {
      drawBitmap(S1x, 180, scatteredclouds, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "scattered";
    }

  if (Slot1 == "broken clouds")
    {
      drawBitmap(S1x, 180, brokenclouds, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "partly";
    }

  if (Slot1 == "overcast clouds")
    {
      drawBitmap(S1x, 180, overcastclouds, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "overcast";
    }

  if (Slot1 == "light rain") drawBitmap(S1x, 180, lightrain, 60, 60, BitMapColour);

  if (Slot1 == "moderate rain")
    {
      drawBitmap(S1x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "moderate";  //  shorten or change string wording
    }

  if (Slot1 == "heavy intensity rain")
    {
      drawBitmap(S1x, 180, heavyrain, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "heavy";  //  shorten or change string wording
    }

  if (Slot1 == "very heavy rain")
    {
      drawBitmap(S1x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "very heavy";  //  shorten or change string wording
    }

  if (Slot1 == "extreme rain")
    {
      drawBitmap(S1x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot1 == "freezing rain")
    {
      drawBitmap(S1x, 180, hail, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "hail";  //  shorten or change string wording
    }

  if (Slot1 == "light intensity shower rain")
    {
      drawBitmap(S1x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "moderate";  //  shorten or change string wording
    }

  if (Slot1 == "shower rain") drawBitmap(S1x, 180, lightrain, 60, 60, BitMapColour);

  if (Slot1 == "heavy intensity shower rain")
    {
      drawBitmap(S1x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot1 == "ragged shower rain")
    {
      drawBitmap(S1x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot1 == "Thunderstorm") drawBitmap(S1x, 180, thunderstorm, 60, 60, BitMapColour);

  if (Slot1 == "Drizzle")
    {
      drawBitmap(S1x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "drizzle";  //  shorten or change string wording
    }

  //  Using Forecast Subroutine for Sunrise and Suset
  if (Slot1 == "SunRise")
    {
      drawBitmap(S1x, 180, sunrise, 60, 60, BitMapColour);
      //  ForeCast1stSlotStr = "drizzle";  //  shorten or change string wording
    }


  //  ************ SLOT 2 *********************

  int S2x = 245;

  if (Slot2 == "clear sky") drawBitmap(S2x, 180, clearsky, 60, 60, BitMapColour);

  if (Slot2 == "few clouds") drawBitmap(S2x, 180, fewclouds, 60, 60, BitMapColour);

  if (Slot2 == "scattered clouds")
    {
      drawBitmap(S2x, 180, scatteredclouds, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "scattered";
    }

  if (Slot2 == "broken clouds")
    {
      drawBitmap(S2x, 180, brokenclouds, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "partly";
    }

  if (Slot2 == "overcast clouds")
    {
      drawBitmap(S2x, 180, overcastclouds, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "overcast";
    }

  if (Slot2 == "light rain") drawBitmap(S2x, 180, lightrain, 60, 60, BitMapColour);

  if (Slot2 == "moderate rain")
    {
      drawBitmap(S2x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "moderate";  //  shorten or change string wording
    }

  if (Slot2 == "heavy intensity rain")
    {
      drawBitmap(S2x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "heavy";  //  shorten or change string wording
    }

  if (Slot2 == "very heavy rain")
    {
      drawBitmap(S2x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "very heavy";  //  shorten or change string wording
    }

  if (Slot2 == "extreme rain")
    {
      drawBitmap(S2x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot2 == "freezing rain")
    {
      drawBitmap(S2x, 180, hail, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "hail";  //  shorten or change string wording
    }

  if (Slot2 == "light intensity shower rain")
    {
      drawBitmap(S2x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "moderate";  //  shorten or change string wording
    }

  if (Slot2 == "shower rain") drawBitmap(S2x, 180, lightrain, 60, 60, BitMapColour);

  if (Slot2 == "heavy intensity shower rain")
    {
      drawBitmap(S2x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot2 == "ragged shower rain")
    {
      drawBitmap(S2x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot2 == "Thunderstorm") drawBitmap(S2x, 180, thunderstorm, 60, 60, BitMapColour);

  if (Slot2 == "Drizzle")
    {
      drawBitmap(S2x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "drizzle";  //  shorten or change string wording
    }

  //  Using Forecast Subroutine for Sunrise and Suset
  if (Slot2 == "SunSet")
    {
      drawBitmap(S2x, 180, sunset, 60, 60, BitMapColour);
      //  ForeCast1stSlotStr = "drizzle";  //  shorten or change string wording
    }
}

void DisplayMinTemp(int ListNumb)  //  ********************************************
{
  //  we have to test 12 UTC and 15 UTC for the lowest Mim Temperature..
  ListNumb = ListNumb - 2;                                             //  ListNumb == 00  (-2) == 18 UTC
  double TempC = myObject["list"][ListNumb]["main"]["temp_min"];       //  18 UTC or 4am Day before
  double Temp2C = myObject["list"][ListNumb + 1]["main"]["temp_min"];  //  21 UTC or 7am

  if (Temp2C < TempC)
    {
      TempC = Temp2C;
    }

  int DecTemp = (TempC - (int)TempC) * 10.00;  //  Use with Deg plus Decimal Deg

  //Serial.print("Minium Temperature: ");
  //Serial.println(TempC);
  //Serial.println("*********************");

  //  clear Deg area
  tft.fillRect(0, 170, 107, 200, BkgCol);

  int xT = 15;

  //  problem for single digital number like 9.4
  int AddX = 0;
  if (TempC < 10) AddX = 27;  //  need to move single digit >>

  int yT = yTime1Slot;  //  160;  to align all text in bottom area

  tft.setTextFont(8);
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, BkgCol);
  tft.drawString("Minimum Temp", xT, yT, 2);

  yT = yT + 20;
  tft.setTextFont(4);
  tft.setTextColor(TFT_CYAN, BkgCol);
  tft.drawNumber(TempC, xT + AddX, yT, 6);  // drawNumber == int  ::  drawFloat == float number
  tft.setCursor(xT + 55, yT - 4);           //  -4 for Decimal Readout
  tft.print(String("`c"));

  // Displays Decimal degrees
  tft.setCursor(xT + 58, yT + 19);
  tft.print(String("."));
  tft.drawNumber(DecTemp, xT + 65, yT + 17, 4);

  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, BkgCol);
  xT = xT + 10;
  yT = yT + 40;
  tft.drawString(LocalCity, xT, yT, 2);
}

void DisplayMaxTemp(int ListNumb)  //  ********************************************
{
  //  we have to test 00 UTC and 03 UTC for the highest Max Temperature..
  double TempC = myObject["list"][ListNumb]["main"]["temp_max"];       //  00 UTC or 10am Brisbane
  double Temp2C = myObject["list"][ListNumb + 1]["main"]["temp_max"];  //  03 UTC or 1pm

  if (Temp2C > TempC)
    {
      TempC = Temp2C;
    }

  int DecTemp = (TempC - (int)TempC) * 10.00;  //  Use with Deg plus Decimal Deg

  //Serial.print("Maximum Temperature: ");
  //Serial.println(TempC);
  //Serial.println("*********************");

  //  clear Deg area
  tft.fillRect(0, 170, 107, 200, BkgCol);

  int xT = 15;

  //  problem for single digital number like 9.4
  int AddX = 0;
  if (TempC < 10) AddX = 27;  //  need to move single digit >>

  int yT = yTime1Slot;  //  160;  to align all text in bottom area

  tft.setTextFont(8);
  tft.setTextSize(1);
  tft.setTextColor(TFT_ORANGE, BkgCol);
  tft.drawString("Maximum Temp", xT, yT, 2);

  yT = yT + 20;
  tft.setTextFont(4);
  tft.setTextColor(TFT_RED, BkgCol);
  tft.drawNumber(TempC, xT + AddX, yT, 6);  // drawNumber == int  ::  drawFloat == float number
  tft.setCursor(xT + 55, yT - 4);           //  -4 for Decimal Readout
  tft.print(String("`c"));

  // Displays Decimal degrees
  tft.setCursor(xT + 58, yT + 19);
  tft.print(String("."));
  tft.drawNumber(DecTemp, xT + 65, yT + 17, 4);

  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, BkgCol);
  xT = xT + 10;
  yT = yT + 40;
  tft.drawString(LocalCity, xT, yT, 2);
}

void EachDayWeather()  //  Tap on Day Name in Top Left Corner i.e. Tuesday  *************************
{
  //Serial.println("void EachDayWeather()");

  //  GetForcast = true;  //  Not needed 7 June 2025
  //Serial.println(" >>> > GetForcast = true  -- Line 2779");

  UpDateReason = " Forecasts ";


  UpDateGraphics(UpDateReason);

  if (TimeOut < 25)  //  Got Internet
    {
      //  Show Outside Temperature, 10am and 4pm weather forecast for next 4 days
      //String TestTime;  //  global var of Day & Time i.e. 2022-09-07 12:00:00

      int NextwDay = wDay;  //Start with Today --  Day of Week i.e. Monday ==
      // int DayCheck = 0;

      if (Hour > 12)  //  i.e. 13 or after 1pm - Start : Reset to Tomorrow
        {
          NextwDay = wDay + 1;

          if (NextwDay > 6) NextwDay = 0;  //  where 0 == Sunday, 1 == Monday etc

          //DayCheck = 4;  //  need to add at least 4 to get next day's forecast
        }

      //Serial.print("Today's Day == ");
      //Serial.println(daysOfTheWeek[wDay]);    //  Example  Saturday
      //Serial.println("***********************");

      for (i = 0; i < 39; i++)  //0 + DayCheck  go thru list[]  - Start at i = 1 not 0 to prevert 12:00 being at i = 0
        {
          TestTime = (const char*)myObject["list"][i]["dt_txt"];  //  Added (const char*) as a new liburary must have changed the process

          //Serial.print("Test TimeSlot :");
          //Serial.println(TestTime.substring(11, 13));  // 12 == 12pm or noon

          //  Time == 10am Brisbane == 00:00 UTC same day..
          if (TestTime.substring(11, 13) == "00")  // 00 UTC == 12am or Midnight UTC or 10am Brisbane
            {
              //Serial.print("TestTime == ");  //  was 12
              //Serial.println(TestTime);              //  Example  2022-09-11 12:00:00

              //Serial.print("Next Day == ");
              //Serial.println(daysOfTheWeek[NextwDay]);  //  Can start today if before 12 noon

              //  Show Each Day in Top Section*********
              //  New Day and Date for Forecasting
              tft.setTextSize(1);
              tft.setTextFont(4);
              tft.setTextColor(TFT_ORANGE, BkgCol);  //  was GREEN YELLOW
              tft.setCursor(12, DayTxtY);
              tft.fillRect(0, 0, 150, 39, BkgCol);
              tft.print(daysOfTheWeek[NextwDay]);

              //  Forecasts Icons Text Names
              //  Get Forecast Icon 10am Slot 1
              TimeSlot1st = i;  //  i == 00 UTC or 10am
              ForeCast1stSlot = myObject["list"][TimeSlot1st]["weather"][0]["description"];
              ForeCast1stSlotStr = String(ForeCast1stSlot);

              //  Get Forecast Icon 4pm or 1600 Slot 2  10am +2 > 4pm
              TimeSlot2nd = i + 2;
              ForeCast2ndSlot = myObject["list"][TimeSlot2nd]["weather"][0]["description"];
              ForeCast2ndSlotStr = String(ForeCast2ndSlot);

              ShowForecast(ForeCast1stSlotStr, ForeCast2ndSlotStr);  //  Shows Weather Icon

              tft.setTextSize(1);
              tft.setTextFont(8);
              tft.setTextColor(TFT_GOLD, BkgCol);

              String CityForecast = "9am " + ForeCast1stSlotStr;  //   String(City) +
              tft.drawCentreString(CityForecast, xTime1Slot, yTime1Slot, 2);

              CityForecast = "3pm " + ForeCast2ndSlotStr;  //  String(City) +
              tft.drawCentreString(CityForecast, xTime2Slot, yTime2Slot, 2);

              NextwDay++;
              if (NextwDay > 6) NextwDay = 0;

              //  Show Forcast Min Temp at 00 UTC - 00 -2 (7am) and -3 (4am)
              if (i - 2 > -1)  // looking for morning but previous day UTC
                {
                  DisplayMinTemp(i);

                  NonClockDelay(2500);
                  //delay(2500);
                }
              //  Show Max Outside Temperature for each day - Recorded at 00 or 03 UTC,
              DisplayMaxTemp(i);  //  00 UTC == 10am Brisbane

              NonClockDelay(2500);
              //delay(2500);
            }
        }
      //Serial.println("*Should have 4 more Days Worth*");
      //Serial.println("*****************************************");
      // tft.fillRect(0, 0, 150, 39, BkgCol);  //  clear Days Section
      // tft.fillRect(0, 157, 107, 240, BkgCol);  //  clear Outside Temp Section

      SensorDelay = 0;  //  avoid delay to Refresh Temperature Display
      LastSensorDelay = millis();

      //  clear Top Day, Date Area
      tft.fillRect(0, 0, 320, 40, BkgCol);

      //Set ChangeDisplay = true; to update time instantly
      ChangeDisplay = true;

      ShowTime();
      DailyUpdate();
      TemperatureSensor();

      //  Refresh Today's 10am or 4pm Forecasts after doing the 4 day forecast
      //  DailyUpdate();
      /*
      GetForcast = true;
      UpDateReason = "Doing Weather Forcast Update from Display EachDayWeather";
      UpDateGraphics("Forecast Today");
      GoWeather();*/
    }
}

void ConvertUnix2RealTime(long timestamp)
{
  struct tm timeinfo;
  time_t tstamp = (time_t)timestamp;  // Cast timestamp to time_t
  gmtime_r(&tstamp, &timeinfo);
  timeinfo.tm_isdst = 0;          //Set daylight saving time to zero
  timestamp = mktime(&timeinfo);  // Convert back to timestamp
  char OutputTimeStr[20];
  strftime(OutputTimeStr, sizeof(OutputTimeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

  // Print local Output time
  //Serial.print("Output Local Time: ");
  //Serial.println(OutputTimeStr);

  String RealOutputTimeStr = OutputTimeStr;

  // Print local Output time converted to a String
  //Serial.print("Real Output Local Time: ");
  //Serial.println(RealOutputTimeStr);

  OutputHour = RealOutputTimeStr.substring(11, 13);  // == "05"  (If was 05:30
  OutputMin = RealOutputTimeStr.substring(14, 16);   // == "30"

  // Print local Output time : Hour and Minute
  //Serial.print("Output Local Time Hour: ");
  //Serial.println(OutputHour);
  //Serial.print("Output Local Time Minute: ");
  //Serial.println(OutputMin);
}

void DailyUpdate()  //  **********************************************************
{
  if (Serial_ON)
    {
      Serial.println("void DailyUpdate()");
    }

  if (!DailyUpdateCall)  //  Prevent clearing Sunrise and Sunset icons and times
    {
      if (!BigDayDisplay)
        {
          //  Get SunRise  -  Added March 2025
          // Extract timezone offset from JSON
          TimezoneOffset = myObject["city"]["timezone"];

          // Extract SunRise time from JSON
          GetSunRiseUnix = myObject["city"]["sunrise"];

          // Convert SunRise time to local time
          SunRiseLocal = GetSunRiseUnix + TimezoneOffset;

          //Serial.print("SunRise Local Time Unix: ");
          //Serial.println(SunRiseLocal);

          ConvertUnix2RealTime(SunRiseLocal);  //  Convert Unix Time to Real Time

          SunRiseHour = OutputHour;
          SunRiseMin = OutputMin;

          DecSunRise = SunRiseHour.toFloat() + SunRiseMin.toFloat() / 60;
          if (DecSunRise <= 0) DecSunRise = 6.0;  //  Default to 6am
          /*
          Serial.print("Line 2987 - SunRise Local Time Hour: ");
          Serial.println(SunRiseHour);
          Serial.print("SunRise Local Time Minute: ");
          Serial.println(SunRiseMin);
          Serial.print("DecSunRise: ");
          Serial.println(DecSunRise);
*/
          // Extract SunSet time from JSON
          GetSunSetUnix = myObject["city"]["sunset"];

          // Convert SunSet time to local time
          SunSetLocal = GetSunSetUnix + TimezoneOffset;

          //Serial.print("SunSet Local Time Unix: ");
          //Serial.println(SunSetLocal);

          ConvertUnix2RealTime(SunSetLocal);  //  Convert Unix Time to Real Time

          SunSetHour = OutputHour;
          SunSetMin = OutputMin;

          DecSunSet = SunSetHour.toFloat() + SunSetMin.toFloat() / 60;
          if (DecSunSet <= 0) DecSunSet = 18.0;  //  Default to 6pm

          //Serial.print("SunSet Local Time Hour: ");
          //Serial.println(SunSetHour);
          //Serial.print("SunSet Local Time Minute: ");
          //Serial.println(SunSetMin);


          //  Do Forecast for 10am and 4pm
          if (Hour > 17 || Hour < 10)  //  From (18) 6pm to before 10am
            {
              //Serial.println("Doing Weather Update!! 5pm to before 10am");

              //Serial.print("Hour == ");
              //Serial.print(Hour);
              //Serial.println(" >>>>  Doing Case10am  <<<<");

              Case10am();  //  for Next Day
            }

          //  Do Forecast for 4pm & 7pm
          if (Hour > 9 && Hour < 18)  //  From 10am to before 6pm
            {
              //Serial.println("Doing Weather Update!! 10am to 7pm");

              //Serial.print("Hour == ");
              //Serial.print(Hour);
              //Serial.println(" >>>>  Doing Case4pm   <<<<");

              Case4pm();  //  for Next Day
            }
        }
    }
}

void GoWeather()  //  ******************************************************
{
  //Serial.println("void GoWeather()");

  //  Check for Weather Update evey hour at XHr 01min
  if ((!BigDayDisplay && Minute == 01 && !DoneHourlyCheck) || GetForcast)  //  Gaz Added !BigDayDisplay
    {
      //Serial.println("Hourly Update");

      DoneHourlyCheck = true;
      GetForcast = false;

      //  Check Internet Connection first.. Get TimeOut Value, > 25 == No Internet
      TryWiFi();

      if (TimeOut > 25)
        {
          // UpDateGraphics("No Internet - Try Later");
          // delay(2000);

          //Serial.print("Internet Failed at :");
          //Serial.println(LongTime);

          //CleanTimeSlot();
          InternetFail = true;
          UpdateMillis = millis();
        }
      else
        {
          GoWeatherMap();

          ShowTime();

          DailyUpdate();

          GetWWWTime();
          CheckTime = true;
        }

      DrawLines();
    }
  else if (Minute != 01)
    {
      DoneHourlyCheck = false;  //  Gets it ready for the next Hour + 1 minute
    }
}

void TemperatureSensor()  //  ******************************************************************
{
  if (millis() - LastSensorDelay > SensorDelay)  //  Avoid millis() overflow
    {
      //Serial.println("void TemperatureSensor()");

      SensorDelay = 60000;  // == 1 minute  Reset Default Delay for Temperature Sensor
      LastSensorDelay = millis();

      sensors_event_t event;  // must be here as well..
      dht.temperature().getEvent(&event);

      if (Testing)
        {
          event.temperature = 10;
          Serial.println("Temporary Temperature input here..  line 3083");
        }

      if (isnan(event.temperature))
        {
          //Serial.println(">>>>>>>>>>Error reading temperature!");
          SensorDelay = 2500;  //  Reset Default Delay for Temperature Sensor
          LastSensorDelay = millis();
        }
      else
        {
          float TempC = event.temperature;

          //Serial.print("Time: ");
          //Serial.println(LongTime);
          //Serial.print("   event.temperature == ");
          //Serial.println(TempC);

          if (abs(LastTemp - TempC) < 10)  //  Sometimes there is an unsual Temp reading
            {
              LastTemp = TempC;

              TempC = TempC + Calibrate;

              //Serial.print("Calibration == ");
              //Serial.println(Calibrate);

              //Serial.print("TempC == ");
              //Serial.println(TempC);

              int DecTemp = abs((TempC - (int)(TempC)) * 10.00);  //  Use with Deg plus Decimal Deg

              //Serial.print("   Calibrate: ");
              //Serial.print(Calibrate);
              //Serial.print("   Decimal part: ");
              //Serial.println(DecTemp);

              SwitchOffLoRa();

              tft.fillRect(0, 157, 107, 83, BkgCol);  //  black out Outside Temp when neccessary

              tft.setTextSize(1);
              tft.setTextFont(8);
              tft.setTextColor(TFT_GOLD, BkgCol);

              int xT = 15;
              int yT = yTime1Slot;  //  160;  to align all text in bottom area
              tft.drawString("Inside Temp", xT, yT, 2);

              tft.setTextFont(4);
              tft.setTextColor(TFT_ORANGE, BkgCol);

              //  problem for single digital number like 9.4
              int AddX = 0;
              if (TempC < 10) AddX = 27;  //  need to move single digit >>

              //xT = xT;
              yT = yT + 20;
              tft.drawNumber(TempC, xT + AddX, yT, 6);  // drawNumber == int  ::  drawFloat == float number
              tft.setCursor(xT + 55, yT - 4);           //  -4 for Decimal Rereadout
              tft.print(String("`c"));

              // Displays Decimal degrees
              tft.setCursor(xT + 58, yT + 19);
              tft.print(String("."));
              tft.drawNumber(DecTemp, xT + 65, yT + 17, 4);

              tft.setTextFont(8);
              tft.setTextColor(TFT_GOLD, BkgCol);
              xT = xT + 10;
              yT = yT + 40;
              tft.drawString(LocalCity, xT, yT, 2);

              SwitchOnLoRa();
            }
          else
            {
              SensorDelay = 2500;  //  Set here for any errors check again in 2.5 sec
              LastSensorDelay = millis();
            }
        }

      /*
        // Get humidity event and print its value.
        dht.humidity().getEvent(&event);
        if(isnan(event.relative_humidity))
        {
          Serial.println(F("Error reading humidity!"));
          SensorDelay =  2500;  //  check again in 2.5 sec
        }
        else
        {
          float Humid = event.relative_humidity;

          tft.setTextColor(TFT_CYAN, BkgCol);

          xH = xH - 10;
          yH = yH + 25;
          tft.drawNumber(Humid, xH, yH, 6);             // Draw humidity
          tft.setCursor(xH + 55, yH);
          tft.print(String(" % "));

          SensorDelay = 60000;  //  check again in 60 sec
        }
    */

      //  Clear Sunrise and Sunset Icons and Times
      if (DailyUpdateCall)
        {
          DailyUpdateCall = false;

          DailyUpdate();
        }
    }
}

void DrawLines()  //  ***************************************************
{
  //Serial.println("void DrawLines()");

  //  Top Horiz Line
  tft.drawLine(0, 40, 320, 40, TFT_LIGHTGREY);  //  2 lines thick
  tft.drawLine(0, 41, 320, 41, TFT_LIGHTGREY);

  // Lower Horiz Line
  tft.drawLine(0, 155, 320, 155, TFT_LIGHTGREY);
  tft.drawLine(0, 156, 320, 156, TFT_LIGHTGREY);

  if (!BigDayDisplay)
    {
      //  Left Vert Line
      tft.drawLine(108, 156, 108, 240, TFT_LIGHTGREY);
      tft.drawLine(109, 156, 109, 240, TFT_LIGHTGREY);

      //  Right Vert Line
      tft.drawLine(217, 156, 217, 240, TFT_LIGHTGREY);
      tft.drawLine(218, 156, 218, 240, TFT_LIGHTGREY);
    }
}

void MidNightUpdate()  //  ********************************************
{
  //Serial.println("void MidNightUpdate()");

  TimeSync = true;

  //Serial.print("Time before Update - ");
  //Serial.println(LongTime);  //  LongTime == __0:00:00am   __9:04:56pm
  MidNightTime = LongTime;

  AfterMidnight = true;

  CheckInternet();

  GetWWWTime();
}

void NonClockDelay(int DelayTime)  //  *****************************************
{
  unsigned long StartDelay = millis();

  ShowDayDate = false;

  while (millis() < StartDelay + DelayTime)
    {
      ShowTime();
    }

  ShowDayDate = true;
}

void ShowTime()  //  *************************************************
{
  SwitchOffLoRa();

  int Interval = 0;

  //if(targetTime < millis())  //  This method will be affected by millis() overflow
  //        3020           999
  if ((millis() - FirstMillis) > 900)  //  Using this method, eliminates possible millis() overflow after 49+ days
    {
      Interval = millis() - FirstMillis;  // 3020 - 1000 == 2020 or 2.020 seconds or 00000202 - x9999999 == 203

      FirstMillis = millis();  //  was using 999 because getting to this line takes about 1 ms..  apparently?

      //Serial.print("FirstMillis == ");
      //Serial.println(FirstMillis);

      //  Rather than adding 1 Second >>> Add the Inverval, keeping the decimal - Second is a Double type Varible
      Second = Second + (float(Interval) / 1000);

      if (Second > 59.9)  //  60 is not greater than 60 & 60 >> 0
        {                 // Check for roll-over
          //Second = 0;                   // Reset seconds to zero

          //  Rather than Resetting Second to 0 >> Substract 60 to keep decimal seconds
          Second = Second - 60;

          LastMin = Minute;

          Minute++;           //   mm++;   // Advance minute
          if (Minute > 59)    //  was mm
            {                 // Check for roll-over
              Minute = 0;     //  mm = 0;
              Hour++;         //  hh++;     // Advance hour
              if (Hour > 23)  //  was hh
                {             // Check for 24hr roll-over (could roll-over on 13)
                  Hour = 0;   //   hh = 0;  // 0 for 24 hour clock, set to 1 for 12 hour clock
                }
            }

          DecHourMin = float(Hour) + float(Minute) / 60;  //  Decimal Hour+Min : Used for LDR with Sunrise and Sunset
        }
      //Serial.print("Second == ");
      //Serial.println(Second);

      ChangeDisplay = true;
    }

  if (ChangeDisplay)  //  The Time display is only updated every second
    {
      ChangeDisplay = false;
      //  Check for MidNight to sync Internet Time
      if (Hour == 0 && !TimeSync)  // at MidNight update Time with ntpServer
        {
          MidNightUpdate();
          if (BigDayDisplay) DoBigDay = true;
        }

      if (Hour != 0)
        {
          TimeSync = false;  //  set to false for next MidNight
        }

      AmPm = "am";
      TwHour = Hour;  //  24 hour time
      TwMin = Minute;
      TwSec = Second;
      DispHour = TwHour;

      if (TwHour == 0)
        DispHour = 12;

      if (DispHour > 12)
        {
          DispHour = DispHour - 12;
        }

      if (TwHour > 11)
        {
          AmPm = "pm";
        }

      int Tx = -10;  //Set x for Time Dispaly Double Hour Number

      StrDispHour = String(DispHour);

      if (StrDispHour.length() < 2)
        {
          StrDispHour = "__" + StrDispHour;  //  was "0"   "__"
          Tx = 5;                            //Set x for Time Dispaly Single Hour Number
        }

      if (LastMin != Minute)
        {
          if (Serial_ON)
            {
              Serial.println("Line 3373  - if (LastMin != Minute)");
              Serial.print("LastMin :");
              Serial.print(LastMin);
              Serial.print("   Minute :");
              Serial.println(Minute);
            }
          LastMin = Minute;
          CalcSleep();
        }


      StrMin = String(Minute);
      if (StrMin.length() < 2)
        {
          StrMin = "0" + StrMin;
        }

      StrSec = String(int(Second));
      if (StrSec.length() < 2)
        {
          StrSec = "0" + StrSec;
        }

      LongTime = String(DispHour) + ":" + StrMin + ":" + StrSec + AmPm;
      ShortTime = StrDispHour + ":" + StrMin;  //  + AmPm;

      if (AfterMidnight)
        {
          //Serial.print("Time After Midnight Update == ");
          //Serial.println(LongTime);
          AfterMidNightTime = LongTime;
          AfterMidnight = false;
        }

      if (LastHour != Hour)
        {
          tft.fillRect(0, 42, 320, 110, BkgCol);  // Clear Large Time area
          LastHour = Hour;
        }

      // DrawLines();

      tft.setTextColor(TFT_LIGHTGREY, BkgCol);

      if (SetLdrDimmer == MinBright)
        {
          tft.setTextColor(TFT_DARKGREY, BkgCol);  //  TFT_DARKGREY does NOT Flicker
          //Serial.print("Changing SetLdrDimmer = ");
          //Serial.println(SetLdrDimmer);
        }

      tft.setTextFont(7);  //  7 is an Old style Digital Font
      tft.setTextSize(2);
      tft.setCursor(Tx, 50);  //  was 2, 50
      tft.print(ShortTime);

      if (DelayTime)  //  Brightness has been changed - display NumBars for 0.75sec
        {
          if (millis() - BrightnessBarsStart > BrightCntDwn)
            {
              tft.fillRect(280, 43, 40, 111, BkgCol);  //  154 - 43 = 111
              DelayTime = false;
            }
        }

      if (!DelayTime)
        {
          tft.setTextFont(2);
          tft.setTextColor(TFT_CYAN, BkgCol);
          tft.setCursor(285, 50);
          tft.print(AmPm);

          //  Second Display
          tft.setTextFont(2);
          tft.setTextColor(TFT_DARKGREY, BkgCol);
          tft.setCursor(280, 105);
          tft.print(":" + StrSec);
        }

      if (ShowDayDate)
        {
          if (BigDayDisplay && DoBigDay)  // BigDayDisplay true
            {
              String BigDay = daysOfTheWeek[wDay];

              //Serial.print(BigDay);

              tft.fillRect(0, 156, 320, 84, BkgCol);  //  Lower Temp and Forecast area

              if (BigDay == "Monday") drawBitmap(0, 156, Graphic_Monday, 320, 84, TFT_GREEN);
              if (BigDay == "Tuesday") drawBitmap(0, 156, Graphic_Tuesday, 320, 84, TFT_GREEN);
              if (BigDay == "Wednesday") drawBitmap(0, 156, Graphic_Wednesday, 320, 84, TFT_GREEN);
              if (BigDay == "Thursday") drawBitmap(0, 156, Graphic_Thursday, 320, 84, TFT_GREEN);
              if (BigDay == "Friday") drawBitmap(0, 156, Graphic_Friday, 320, 84, TFT_GREEN);
              if (BigDay == "Saturday") drawBitmap(0, 156, Graphic_Saturday, 320, 84, TFT_GREEN);
              if (BigDay == "Sunday") drawBitmap(0, 156, Graphic_Sunday, 320, 84, TFT_GREEN);

              DoBigDay = false;
            }

          if (wDay != LastDay)
            {
              //  clear Top Day, Date Area
              tft.fillRect(0, 0, 320, 38, BkgCol);

              LastDay = wDay;  //  allows refresh of Day/Date field once a day - avoids flicker
            }

          tft.setTextSize(1);
          tft.setTextFont(4);
          tft.setTextColor(TFT_GREEN, BkgCol);
          tft.setCursor(12, DayTxtY);
          tft.print(daysOfTheWeek[wDay]);

          StrYear = String(Year);

          StrMonth = String(Month);
          StrMonth = monthsOfYear[Month - 1];
          //  if(StrMonth.length() < 2)
          //  {
          //    StrMonth = "0" + StrMonth;
          //  }

          StrDay = String(nDay);

          if (StrDay.length() < 2)
            {
              StrDay = "0" + StrDay;
            }

          tft.setCursor(165, DayTxtY);

          tft.print(StrDay + "  " + StrMonth + "  " + StrYear);

          //tft.drawString(String(LdrDimmer), 140, 89, 2);  //  only used for testing
        }
      if (!WledON && TwHour == 20 && TwMin == 0)  //  Turn Wleds ON at 8 pm
        {                                                       //  Turn Wleds ON
          FastLED.setBrightness(WledBrightness);                //  WledBrightness

          FastLED.show();
          FastLED.delay(10);

          WledState = true;  //  Wleds will be turned ON
          WledON = true;
        }
    }
    
  SwitchOnLoRa();
}

//  *****  START *** Tank  *** Tank *** Tank *** Tank *** Tank *** Tank *** Tank *** Tank *** Tank

void BeginServer()  //  ****************************   Web Server
{                   //Should be NO Spaces between "/"  and check below..
  server.on("/", handleRoot);
  server.begin();
  if (Serial_ON)
    {
      Serial.println("HTTP server started");
    }
}

void handleRoot()  //  ****************************   Web Server
{
  //PrntData = false;  //  change this to  true  to show Data History

  if (!PrntData)
    {
      server.send(200, "text/html", getPage());

       // Give the web browser time to receive the data
    delay(5);
    
    // CLOSE THE CONNECTION FORCEFULLY
    server.stop(); 

    BeginServer();  //  Web Server
    }
  else
    {
      server.send(200, "text/html", dataPage());

       // Give the web browser time to receive the data
    delay(5);
    
    // CLOSE THE CONNECTION FORCEFULLY
    //server.stop(); 

    BeginServer();  //  Web Server
    }
}

void GetTankData()  //  using the ESP32 #include <Preferences.h>
{
  //Serial.println("void GetTankData()");

  preferences.begin("TankData", RO_MODE);
  delay(10);

  TrueData = preferences.getUInt("TrueData", 0);  // preference.getUInt("Var", default value)

  if (TrueData == 99)
    {
      TankHeight[1] = preferences.getFloat("TankHgt1", 0);
      LstTankHeight[1] = TankHeight[1];

      TankDia[1] = preferences.getFloat("TankDia1", 0);
      LstTankDia[1] = TankDia[1];

      NumbTanks[1] = preferences.getFloat("NumbTanks1", 0);
      LstNumbTanks[1] = NumbTanks[1];

      TankHeight[2] = preferences.getFloat("TankHgt2", 0);
      LstTankHeight[2] = TankHeight[2];

      TankDia[2] = preferences.getFloat("TankDia2", 0);
      LstTankDia[2] = TankDia[2];

      NumbTanks[2] = preferences.getFloat("NumbTanks2", 0);
      LstNumbTanks[2] = NumbTanks[2];

      TankHeight[3] = preferences.getFloat("TankHgt3", 0);
      LstTankHeight[3] = TankHeight[3];

      TankDia[3] = preferences.getFloat("TankDia3", 0);
      LstTankDia[3] = TankDia[3];

      NumbTanks[3] = preferences.getFloat("NumbTanks3", 0);
      LstNumbTanks[3] = NumbTanks[3];

      Set20Min = preferences.getBool("Set20Min", 1);

      SetHr = preferences.getUInt("SetHr", 1);
    }
  preferences.end();

  if (Set20Min)
    {
      newSleepTime = "20 mins";
    }
  else
    {
      //  SetHr = 1;
      SetHrStr = String(SetHr) + " Hr";
      newSleepTime = SetHrStr;
    }

  if (TankHeight[1] < 1)
    {
      for (int i = 0; i < 5; i++)
        {
          TankDia[i] = 1.8;
          TankHeight[i] = 1.8;
          NumbTanks[i] = 1;  //  Number Of Tanks at TankNameLoc
        }
    }

  if (Serial_ON)
    {
      Serial.println("Getting Data from EEPROM");

      Serial.print("TrueData = ");
      Serial.println(TrueData);

      Serial.print("TankDia1 = ");
      Serial.println(TankDia[1]);

      Serial.print("TankHgt1 = ");
      Serial.println(TankHeight[1]);

      Serial.print("NumbTanks1 = ");
      Serial.println(NumbTanks[1]);

      Serial.print("TankDia2 = ");
      Serial.println(TankDia[2]);

      Serial.print("TankHgt2 = ");
      Serial.println(TankHeight[2]);

      Serial.print("NumbTanks2 = ");
      Serial.println(NumbTanks[2]);

      Serial.print("TankDia3 = ");
      Serial.println(TankDia[3]);

      Serial.print("TankHgt3 = ");
      Serial.println(TankHeight[3]);

      Serial.print("NumbTanks3 = ");
      Serial.println(NumbTanks[3]);

      Serial.print("Set20Min > 0 false, 1 true = ");
      Serial.println(Set20Min);
      Serial.print("SetHr = ");
      Serial.println(SetHr);
    }
}

void SaveTankData()  //  using the ESP32 #include <Preferences.h>
{
  if (Serial_ON)
    {
      Serial.println("***************Saving EEPROM*******************");

      Serial.print("TrueData = ");
      Serial.println(TrueData);

      Serial.print("TankDia1 = ");
      Serial.println(TankDia[1]);

      Serial.print("TankHgt1 = ");
      Serial.println(TankHeight[1]);

      Serial.print("NumbTanks1 = ");
      Serial.println(NumbTanks[1]);

      Serial.print("TankDia2 = ");
      Serial.println(TankDia[2]);

      Serial.print("TankHgt2 = ");
      Serial.println(TankHeight[2]);

      Serial.print("NumbTanks2 = ");
      Serial.println(NumbTanks[2]);

      Serial.print("TankDia3 = ");
      Serial.println(TankDia[3]);

      Serial.print("TankHgt3 = ");
      Serial.println(TankHeight[3]);

      Serial.print("NumbTanks3 = ");
      Serial.println(NumbTanks[3]);

      Serial.print("Set20Min (0 false, 1 true) = ");
      Serial.println(Set20Min);

      Serial.print("SetHr = ");
      Serial.println(SetHr);

      Serial.println("2 Sec Delay");

      delay(2000);
    }
  //  Save All EEPROM

  preferences.begin("TankData", RW_MODE);

  preferences.putUInt("TrueData", 99);

  preferences.putFloat("TankHgt1", TankHeight[1]);
  preferences.putFloat("TankDia1", TankDia[1]);
  preferences.putFloat("NumbTanks1", NumbTanks[1]);

  preferences.putFloat("TankHgt2", TankHeight[2]);
  preferences.putFloat("TankDia2", TankDia[2]);
  preferences.putFloat("NumbTanks2", NumbTanks[2]);

  preferences.putFloat("TankHgt3", TankHeight[3]);
  preferences.putFloat("TankDia3", TankDia[3]);
  preferences.putFloat("NumbTanks3", NumbTanks[3]);

  preferences.putBool("Set20Min", Set20Min);
  preferences.putUInt("SetHr", SetHr);

  // Close the Preferences
  preferences.end();

  delay(1000);
}

void SwitchOnLoRa()  //  **************************************************
{
  delay(20);

  digitalWrite(TFT_CS, HIGH);    //  deSelect TFT
  digitalWrite(TOUCH_CS, HIGH);  //  deSlect Touch
  digitalWrite(LoRaCSpin, LOW);  //  Select LoRa  <<<<<<<<<<<<<<<<<<<<<<<<<<<

  delay(20);
}

void SwitchOffLoRa()  //  **************************************************
{

  delay(20);
  digitalWrite(LoRaCSpin, HIGH);  //  deSelect LoRa  Pin 17 >>>>>>>>>>>>>>>>>>>>>>>>>>>>
  digitalWrite(TFT_CS, LOW);      //  Select TFT     Pin 15
  digitalWrite(TOUCH_CS, LOW);    //  Select Touch   Pin 16
  delay(20);
}

void onReceive(int packetSize)  //  *******************************************
{
  SwitchOffLoRa();

  if (packetSize != 0)
    {
      // read packet header bytes:  First 2 bytes are Recipient and Sender
      recipient = LoRa.read();  // recipient address

      sender = LoRa.read();  //Sender address

      if (Serial_ON)
        {
          Serial.print("   recipient =");
          Serial.print(recipient);
          Serial.print("  sender    =");
          Serial.println(sender);
        }

      // read packet header bytes: 3rd byte is length of text NOTE: Must be smaller that 256 chars..
      incomingLength = LoRa.read();  // incoming msg length sent to check Data

      //  Next collect the RSSI
      LoRaRSSI = String(LoRa.packetRssi());  //  This is not sent or received - it is part of the LoRa send/recieve routine

      //  clear Data collecting string
      RecMessage = "";

      while (LoRa.available())  //  Read all chars of recieved string
        {
          RecMessage += (char)LoRa.read();
        }

      if (recipient != 0xFF)  //  0xFF == Master
        {
          if (Serial_ON)
            {
              Serial.println("####vvvv####vvvv####vvvv####vvvv####vvvv####vvvv####");
              Serial.print("New Message receiver at :");
              Serial.println(LongTime);
              Serial.print("This RecMessage is NOT for Master Unit, ");
              Serial.print("packetSize =");
              Serial.print(packetSize);
              Serial.print("   recipient =");
              Serial.print(recipient, HEX);
              Serial.print("   from > ");
              Serial.println(sender, HEX);
              Serial.print("RecMessage >");
              Serial.println(RecMessage);
              Serial.print("LoRaRSSI >");
              Serial.println(LoRaRSSI);
            }
          return;
        }

      if (Serial_ON)
        {
          Serial.println("####========== Data Received ==========####");
          Serial.print("Rec packetSize > ");
          Serial.print(packetSize);
          Serial.print(" - recipient > ");
          Serial.print(recipient, HEX);
          Serial.print(" -  From Sender > ");
          Serial.println(sender, HEX);
          Serial.print("Message >");
          Serial.println(RecMessage);
          Serial.print("LoRaRSSI > ");
          Serial.println(LoRaRSSI);
        }

      //  Check incomingLength with RecMessage.length();
      if (incomingLength != RecMessage.length())
        {
          if (Serial_ON)
            {
              ExtractData(RecMessage);

              Serial.println("*****  ERROR: RecMessage length does not match incoming Data length");
              Serial.println("There may be a problem with the Data");

              Serial.print("Received Message >> ");
              Serial.println(RecMessage);
              Serial.print("LoRaRSSI > ");
              Serial.println(LoRaRSSI);
            }

          return;  //Skip rest of function
        }

      if (Serial_ON)
        {
          Serial.println("Line 3827 - Go ExtractData(RecMessage)");
        }

      ExtractData(RecMessage);

      //  Check first 2 bytes for correct ID of Recipient and Sender
      if (recipient == localAddress && RecTankNameLoc == "Header")  // 0xFF ==  Master Unit
        {
          if (RecWake == "BootUp")
            {
              if (Serial_ON_HeaderRec)
                {
                  Serial.println(">>>>>>>>>>>>>  Header BootUp Message  <<<<<<<<<<<<<<<<<<");
                  Serial.print("Line 3840 - Received BootUp - Sending ");
                  Serial.println(RecWake);
                  Serial.print("Received Message >> ");
                  Serial.println(RecMessage);
                }

              // RecWake = SndSleepMode[1];

              RequestData(1);  //  Send Sleep or Awake back to Header TankStoreSortedHistory();

              HeaderAwakeTime = millis();  //  Start counting in case Awake Mode sent and forgotten about 15 mins max..
              if (Serial_ON)
                {
                  Serial.println();
                  Serial.println("Changing SleepCalcDone = false");
                  Serial.println();
                }
              SleepCalcDone = false;  //  Data recieved, Allow the Next Awake Time to be calculated
              SleepAmPmDone = false;
            }
        }
      else  // recipent IS Master and "Awake" from other Send Units
        {
          if (Serial_ON)
            {
              Serial.println("####vvvv####vvvv####vvvv####vvvv####vvvv####vvvv####");
              Serial.print("Recieved Info from >");
              Serial.print(RecTankNameLoc);
              Serial.print("<    <<<<>>>>>>  LoRaRSSI >");
              Serial.println(LoRaRSSI);
              Serial.print("RecMessage >");
              Serial.println(RecMessage);
            }
        }

      //  Store Time to Display in Tank Level Dispaly
      TankTime = String(DispHour) + ":" + StrMin + AmPm;

      TimeStamp[1] = LongTime;  //  These variable for WebServer

      ShowTFTdata(2);  // 1 == to Normal TankMaster TFT readout  2 == Clock Lower Panel Display

      if (RecTankNameLoc == "Header")
        {
          StoreSortedHistory();
        }
    }
  packetSize = 0;

  SwitchOnLoRa();
}

void ExtractData(String RecNewMessage)  //  ************************************
{

  //  Example of message received
  //TankName  WatLv Temp  Pow   Bat   Sol  STm  Message
  //"Header: 0.5: 21.2: 3.23: 4.05: 4.99: 3000: Sleep"

  RecNewMessage = " " + RecNewMessage;  //  Gotta fix this

  int incomingLength = RecNewMessage.length();

  int StPos = 0;
  int FinPos;
  int VarCnt = 1;  //  Part of message to seperate into Variables

  for (FinPos = 0; FinPos < incomingLength; FinPos++)
    {
      //Serial.print("Char > ");
      //Serial.println(RecNewMessage.substring(FinPos, FinPos + 1));

      if (RecNewMessage.substring(FinPos, FinPos + 1) == ":")
        {

          switch (VarCnt)
            {
              case 1:  // TankNameLoc  re Tank Name
                RecTankNameLoc = RecNewMessage.substring(StPos + 1, FinPos);

                //Serial.print("RecTankNameLoc > ");
                //Serial.println(RecTankNameLoc);

                if (RecTankNameLoc == "Header")
                  {
                    if (Serial_ON_HeaderRec)
                      {
                        Serial.println("               TankName  WatLv Temp  Pow   Bat   Sol  STm  Message");
                        Serial.print("Line 3925 - RecNewMessage > ");
                        Serial.println(RecNewMessage);
                      }
                  }

                StPos = FinPos + 1;
                VarCnt++;
                break;

              case 2:  // Water Level Distance
                RecWLdist = RecNewMessage.substring(StPos, FinPos);

                //Serial.print("RecWLdist > ");
                //Serial.println(RecWLdist);

                StPos = FinPos + 1;
                VarCnt++;
                break;

              case 3:  // Temperature
                RecTemp = RecNewMessage.substring(StPos, FinPos);
                StPos = FinPos + 1;
                VarCnt++;
                break;

              case 4:  // Power Volts
                PowVolts = RecNewMessage.substring(StPos, FinPos);
                StPos = FinPos + 1;
                VarCnt++;
                break;

              case 5:  // Battery Volts
                BatVolts = RecNewMessage.substring(StPos, FinPos);
                StPos = FinPos + 1;
                VarCnt++;
                break;

              case 6:  //Solar Volts
                SolVolts = RecNewMessage.substring(StPos, FinPos);
                StPos = FinPos + 1;
                VarCnt++;
                break;

              case 7:  //Sleep Time
                RecTimeSleeping = RecNewMessage.substring(StPos, FinPos);
                StPos = FinPos + 1;
                VarCnt++;
                break;

              case 8:  // BootUp - Awake or Sleep
                RecWake = RecNewMessage.substring(StPos, FinPos);
                StPos = FinPos + 1;
                VarCnt++;
                break;
            }
        }
    }
}

void ShowTFTdata(int DisplayType)  //  *******************************************
{
  FontType = 2;
  LnSpc = 16;

  if (RecTankNameLoc == "Header")
    {
      if (Serial_ON_HeaderRec)
        {
          Serial.println("####================####");
          Serial.print("Line 3994 - TankNameLoc >");
          Serial.println(RecTankNameLoc);

          Serial.print("Recieved Message at >");
          Serial.println(LongTime);

          Serial.print("Sensor Water (Actual) Distance >");
          Serial.println(RecWLdist);  //  from sensor before substraction

          Serial.print("Temp > ");
          Serial.println(RecTemp);

          Serial.print("Power Volts Reads >");
          Serial.println(PowVolts);

          Serial.print("Battery Volts Reads >");
          Serial.println(BatVolts);

          Serial.print("Solar Volts Reads >");
          Serial.println(SolVolts);

          Serial.print("Rec Sleep Time >");
          Serial.println(RecTimeSleeping);

          Serial.print("RecWake (Awake or Sleep) >");
          Serial.print(RecWake);
          Serial.println("< ");
        }

      DataSent[1] = "Recd";  //  Data Sent & Received

      RecWaterLevel = RecWLdist.toFloat() - HgtSensor[1];  //  Substract Height of Sensor to Get Height of Water

      if (RecWaterLevel < 0) RecWaterLevel = 0;  //  FULL TANK

      //  Closest Sensor can read accurately is 220mm - anything less is Zero.

      if (RecWaterLevel != LstRecWaterLevel)
        {
          HeaderAwakeTime = millis();  //  Start counting in case Awake Mode sent and forgotten about 15 mins max..

          //  Is reset if Distance Changes
          LstRecWaterLevel = RecWaterLevel;
        }

      if (DisplayType == 1)
        {
          SprHeaderLandS();  // 1 == to Normal TankMaster TFT readout  2 == Clock Panel Display
        }
      else  //  DisplayType == 2   for Clock Lower Panel Display
        {
          ClockPanelDisp(1);  //  1 == 1st Panel  2 == 2nd Panel  3 == 3rd Panel
        }
    }

  if (RecTankNameLoc == "DBL Main")
    {
      NxtLneY = 135;

      DataSent[2] = "Recd";  //  Data Sent & Received

      RecWaterLevel = RecWLdist.toFloat() - HgtSensor[2];  //  Substract Height of Sensor to Get Height of Water

      if (RecWaterLevel < 0) RecWaterLevel = 0;  //  FULL TANK

      if (Serial_ON)
        {
          Serial.println("####================####");
          Serial.print("TankNameLoc >");
          Serial.println(RecTankNameLoc);

          Serial.print("Water Distance >");
          Serial.println(RecWaterLevel);

          //Serial.print("Temp >");
          //Serial.println(RecTemp);

          //Serial.print("Bat Volts >");
          //Serial.println(BatVolts);

          Serial.print("Sleep Time >");
          Serial.println(RecTimeSleeping);

          Serial.print("RecWake (Awake or Sleep) >>");
          Serial.println(RecWake);
        }
      if (DisplayType == 1)
        {
          SprDBL_MainLandS();  // 1 == to Normal TankMaster TFT readout  2 == Clock Panel Display
        }
      else
        {
          ClockPanelDisp(2);  //  1 == 1st Panel  2 == 2nd Panel  3 == 3rd Panel
        }
    }

  if (RecTankNameLoc == "uDeck")
    {
      NxtLneY = 220;

      DataSent[3] = "Recd";  //  Data Sent & Received

      RecWaterLevel = RecWLdist.toFloat() - HgtSensor[3];  //  Substract Height of Sensor to Get Height of Water

      if (RecWaterLevel < 0) RecWaterLevel = 0;  //  FULL TANK


      if (Serial_ON)
        {
          Serial.println("####================####");
          Serial.print("TankNameLoc >");
          Serial.println(RecTankNameLoc);

          Serial.print("Water Level Distance >");
          Serial.println(RecWLdist);

          //Serial.print("Temp >");
          //Serial.println(RecTemp);

          //Serial.print("Bat Volts >");
          //Serial.println(BatVolts);

          Serial.print("Sleep Time >");
          Serial.println(RecTimeSleeping);

          Serial.print("RecWake (Awake or Sleep) >>>");
          Serial.println(RecWake);
        }
      if (DisplayType == 1)
        {
          SpruDeckLandS();  // 1 == to Normal TankMaster TFT readout  2 == Clock Panel Display
        }
      else
        {
          ClockPanelDisp(3);  //  1 == 1st Panel  2 == 2nd Panel  3 == 3rd Panel
        }
    }

  delay(10);  //  was 100
}

void SprHeaderLandS()  //  **********************************************************************
{
  // Width and height of each Tank Area Sprite
  //#define SprWIDTH = 105;
  //#define SprHEIGHT 96

  if (Serial_ON_Header)
    {
      Serial.println("Drawing Sprite Header Tank");
    }

  spr.createSprite(SprWIDTH, SprHEIGHT);

  // Fill the whole sprite with BkgCol (Sprite is in memory so not visible yet)
  spr.fillSprite(BkgCol);

  spr.drawCentreString(RecTankNameLoc, 55, 0, FontType);  //  Put Text in Centre of Sprite

  //  Draw Tank Graphics under Tank Name x == 25 & y == 49 - 25 == 24
  BMx = 20;
  BMy = 24;
  spr.drawBitmap(BMx, BMy, LoftyTank, 65, 47, TFT_GREEN);  //  Draw Tank Graphics

  TimeStamp[1] = LongTime;

  Dist[1] = String(RecWaterLevel);

  //  Test to avoid Divide by Zero (0)
  if (TankHeight[1] == 0) TankHeight[1] = 0.1;

  HeightPercent[1] = (TankHeight[1] - RecWaterLevel) / TankHeight[1];

  RSSILoRa[1] = LoRaRSSI;

  //  Setup Voltage Warning
  BatWarning = "";

  if (BatVolts.toFloat() < 3.50 && BatVolts.toFloat() > 3.40)
    {
      BatWarning = " Low";
    }

  if (BatVolts.toFloat() < 3.40 && BatVolts.toFloat() > 3.30)
    {
      BatWarning = " Critical";
    }

  if (BatVolts.toFloat() < 3.30)
    {
      BatWarning = " Shut Down - 60m";
      // Change to Sleep if set to Awake
      SndSleepMode[1] = "Sleep";

      if (Serial_ON_Header)
        {
          Serial.print("BatVolts == ");
          Serial.println(BatVolts);
        }
    }

  //  WebPage Graphics Tank Percentage
  CalcHt[1] = 150 * (1 - HeightPercent[1]);
  PowerVolts[1] = PowVolts;
  BatteryVolts[1] = BatVolts;
  SolarVolts[1] = SolVolts;

  if (Serial_ON_Header || Serial_ON)
    {
      Serial.print("NumbTanks[1] > ");
      Serial.print(NumbTanks[1]);
      Serial.print("   TankHeight[1] > ");
      Serial.print(TankHeight[1]);
      Serial.print("   RecWaterLevel > ");
      Serial.print(RecWaterLevel);
      Serial.print("   HeightPercent[1]  > ");
      Serial.println(HeightPercent[1]);

      Serial.print("Power Volts[1]  > ");
      Serial.print(PowerVolts[1]);

      Serial.print("  BatteryVolts [1]  > ");
      Serial.print(BatteryVolts[1]);

      Serial.print("   SolarVolts[1]  > ");
      Serial.print(SolarVolts[1]);

      Serial.print("  RecWake Awake or Sleep >");
      Serial.print(RecWake);
      Serial.println("<");
    }

  Wx = BMx + 3;
  Wy = BMy + 44;  //  47 == height of BitMap

  Wh = 40 * HeightPercent[1];  //  40 = Height Inside Dimension of Tank
  WxL = BMx + 61;

  int WL;

  //  Draw Water in Tank - Series of Lines from Bottom of Tank to Top of Water Level

  for (WL = 0; WL < Wh - 1; WL++)
    {
      spr.drawLine(Wx, Wy - WL, WxL, Wy - WL, TFT_CYAN);
    }

  spr.drawBitmap(Wx, Wy - WL, LoftyWaterLevel, 62, 2, TFT_CYAN);  //  Draw Wavey Top Water Line

  CurrentVol[1] = Pi * (TankHeight[1] * HeightPercent[1]) * sq(TankDia[1] / 2) * NumbTanks[1] * 1000;  //  Current Volume

  TankSize[1] = Pi * TankHeight[1] * sq(TankDia[1] / 2) * NumbTanks[1] * 1000;

  PercentVol[1] = int(float(CurrentVol[1]) / float(TankSize[1]) * 100);  //  Calc Current Vol in Percentage

  if (CurrentVol[1] < 0) CurrentVol[1] = 0;

  PrintOut = String(CurrentVol[1]) + " Lts";
  spr.drawCentreString(PrintOut, BMx + 32, 76, FontType);

  SwitchOffLoRa();  //  To Write to TFT Screen

  delay(SprDELAY);             // Gives time to Update the Sprite - I think..  20ms
  spr.pushSprite(0, SprTopY);  //  Tank Sprite

  //  ***************************
  spr.fillSprite(BkgCol);  //  Create New/Next Text Sprite

  int x = 54;
  NxtLneY = 5;

  PrintOut = "RSSI: " + LoRaRSSI;  // default Printout
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;

  PrintOut = "Dist: " + Dist[1];
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;
  PrintOut = "Time: " + ShortTime;
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;
  Temp[1] = RecTemp;
  PrintOut = "Temp: " + Temp[1];
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;
  BatteryVolts[1] = BatVolts;
  PrintOut = "Battery: " + BatteryVolts[1] + "v";
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;

  PrintOut = "Power: " + PowerVolts[1] + "v";
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;  //  For SleepTime Countdown

  if (RecWake == "Awake" || SndSleepMode[1] == "Awake")
    {
      SleepMode[1] = "Awake";
      spr.drawCentreString("Mode - Awake", x, NxtLneY, FontType);
    }
  else if (RecWake == "Sleep" || SndSleepMode[1] == "Sleep")
    {
      SleepMode[1] = "Sleep";
      spr.drawCentreString("Mode - Sleep", x, NxtLneY, FontType);
    }
  else
    {
      PrintOut = RecWake + " - " + SleepMode[1];
      spr.drawCentreString(PrintOut, x, NxtLneY, FontType);
      //  SleepMode[1] = "Sleep";
    }

  SwitchOffLoRa();  //  To Write to TFT Screen

  delay(SprDELAY);             // Gives time to Update the Sprite - I think..  20ms
  spr.pushSprite(0, SprBotY);  //  Tank Sprite

  //  Clear Sprite Data
  spr.createSprite(1, 1);
  spr.fillSprite(BkgCol);
  spr.pushSprite(0, 239);  //  Tank Sprite

  // delay(10);  //  may be needed

  SwitchOnLoRa();
}

void SprDBL_MainLandS()  //  **********************************************************************
{
  // Width and height of each Tank Area Sprite
  //#define SprWIDTH  105
  //#define SprHEIGHT 96

  if (Serial_ON_Header)
    {
      Serial.println("Drawing Sprite DBL_Main");
    }

  spr.createSprite(SprWIDTH, SprHEIGHT);

  // Fill the whole sprite with BkgCol (Sprite is in memory so not visible yet)
  spr.fillSprite(BkgCol);

  spr.drawCentreString(RecTankNameLoc, 55, 0, FontType);  //  Put Text in Centre of Sprite

  //  Draw Tank Graphics under Tank Name x == 25 & y == 49 - 25 == 24
  BMx = 20;
  BMy = 24;
  spr.drawBitmap(BMx, BMy, LoftyTank, 65, 47, TFT_GREEN);  //  Draw Tank Graphics

  TimeStamp[2] = LongTime;

  Dist[2] = String(RecWaterLevel);

  //  Test to avoid Divide by Zero (0)
  if (TankHeight[2] == 0) TankHeight[2] = 0.1;

  HeightPercent[2] = (TankHeight[2] - RecWaterLevel) / TankHeight[2];

  if (Serial_ON_Header || Serial_ON)
    {
      Serial.print("HeightPercent[2] > ");
      Serial.println(HeightPercent[2]);
    }

  RSSILoRa[2] = LoRaRSSI;

  //  WebPage Graphis Tank Percentage
  CalcHt[2] = 150 * (1 - HeightPercent[2]);

  if (Serial_ON)
    {
      Serial.print("NumbTanks[2] > ");
      Serial.print(NumbTanks[2]);
      Serial.print("   TankHeight[2] > ");
      Serial.print(TankHeight[2]);
      Serial.print("   RecWaterLevel > ");
      Serial.print(RecWaterLevel);
      Serial.print("   HeightPercent[2]  > ");
      Serial.print(HeightPercent[2]);

      Serial.print("  RecWake Awake or Sleep >");
      Serial.println(RecWake);
    }

  Wx = BMx + 3;
  Wy = BMy + 44;  //  47 == height of BitMap

  Wh = 40 * HeightPercent[2];  //  40 = Height Inside Dimension of Tank
  WxL = BMx + 61;

  int WL;

  //  Draw Water in Tank - Series of Lines from Bottom of Tank to Top of Water Level

  for (WL = 0; WL < Wh - 1; WL++)
    {
      spr.drawLine(Wx, Wy - WL, WxL, Wy - WL, TFT_CYAN);
    }

  spr.drawBitmap(Wx, Wy - WL, LoftyWaterLevel, 62, 2, TFT_CYAN);  //  Draw Wavey Top Water Line

  CurrentVol[2] = Pi * (TankHeight[2] * HeightPercent[2]) * sq(TankDia[2] / 2) * NumbTanks[2] * 1000;

  TankSize[2] = Pi * TankHeight[2] * sq(TankDia[2] / 2) * NumbTanks[2] * 1000;

  PercentVol[2] = int(float(CurrentVol[2]) / float(TankSize[2]) * 100);  //  Calc Current Vol in Percentage

  if (CurrentVol[2] < 0) CurrentVol[2] = 0;

  PrintOut = String(CurrentVol[2]) + " Lts";
  spr.drawCentreString(PrintOut, BMx + 32, 76, FontType);

  SwitchOffLoRa();  //  To Write to TFT Screen

  delay(SprDELAY);               // Gives time to Update the Sprite - I think..  10ms
  spr.pushSprite(108, SprTopY);  //  Tank Sprite

  SwitchOnLoRa();

  //  ***************************
  spr.fillSprite(BkgCol);

  int x = 54;
  NxtLneY = 5;

  // PrintOut = "RSSI: " + LoRaRSSI;
  //Spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;

  PrintOut = "Dist: " + Dist[2];
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;
  PrintOut = "Time: " + ShortTime;
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  // Temp[2] = RecTemp;
  // PrintOut = "Temp: " + Temp[2];
  //Spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;
  // PowerVolts[2] = RecVolts;
  // PrintOut = "Volts: " + PowerVolts[2];
  //Spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;  //  For SleepTime Countdown
  if (RecWake == "Awake")
    {
      SleepMode[2] = "Awake";
      spr.drawCentreString("Mode - Awake", x, NxtLneY, FontType);
    }

  if (RecWake == "Sleep")
    {
      SleepMode[2] = "Sleep";
      spr.drawCentreString("Mode - Sleep", x, NxtLneY, FontType);
    }

  SwitchOffLoRa();  //  To Write to TFT Screen

  delay(SprDELAY);               // Gives time to Update the Sprite - I think.. 10ms
  spr.pushSprite(108, SprBotY);  //  Tank Sprite

  //  Clear Sprite Data
  spr.createSprite(10, 10);
  spr.fillSprite(BkgCol);
  spr.pushSprite(0, 239);  //  Tank Sprite

  SwitchOnLoRa();
}

void SpruDeckLandS()  //  **********************************************************************
{
  // Width and height of each Tank Area Sprite
  //#define SprWIDTH  118
  //#define SprHEIGHT 96

  if (Serial_ON_Header)
    {
      Serial.println("Drawing Sprite uDeck");
    }

  spr.createSprite(SprWIDTH, SprHEIGHT);

  // Fill the whole sprite with BkgCol (Sprite is in memory so not visible yet)
  spr.fillSprite(BkgCol);

  spr.drawCentreString(RecTankNameLoc, 55, 0, FontType);  //  Put Text in Centre of Sprite

  //  Draw Tank Graphics under Tank Name x == 25 & y == 49 - 25 == 24
  BMx = 20;
  BMy = 24;
  spr.drawBitmap(BMx, BMy, LoftyTank, 65, 47, TFT_GREEN);  //  Draw Tank Graphics

  TimeStamp[3] = LongTime;

  Dist[3] = String(RecWaterLevel);

  //  Test to avoid Divide by Zero (0)
  if (TankHeight[3] == 0) TankHeight[3] = 0.1;

  HeightPercent[3] = (TankHeight[3] - RecWaterLevel) / TankHeight[3];

  RSSILoRa[3] = LoRaRSSI;

  //  WebPage Graphis Tank Percentage  150 == Height of Tank Graphics
  CalcHt[3] = 150 * (1 - HeightPercent[3]);

  if (Serial_ON)
    {
      Serial.print("NumbTanks[3] > ");
      Serial.print(NumbTanks[3]);
      Serial.print("   TankHeight[3] > ");
      Serial.print(TankHeight[3]);
      Serial.print("   RecWaterLevel : ");
      Serial.print(RecWaterLevel);
      Serial.print("   HeightPercent[3]  > ");
      Serial.print(HeightPercent[3]);

      Serial.print("  RecWake Awake or Sleep >");
      Serial.println(RecWake);
    }

  Wx = BMx + 3;
  Wy = BMy + 44;  //  47 == height of BitMap

  Wh = 40 * HeightPercent[3];  //  40 = Height Inside Dimension of Tank
  WxL = BMx + 61;

  int WL;

  //  Draw Water in Tank - Series of Lines from Bottom of Tank to Top of Water Level

  for (WL = 0; WL < Wh - 1; WL++)
    {
      spr.drawLine(Wx, Wy - WL, WxL, Wy - WL, TFT_CYAN);
    }

  spr.drawBitmap(Wx, Wy - WL, LoftyWaterLevel, 62, 2, TFT_CYAN);  //  Draw Wavey Top Water Line

  CurrentVol[3] = Pi * (TankHeight[3] * HeightPercent[3]) * sq(TankDia[3] / 2) * NumbTanks[3] * 1000;

  TankSize[3] = Pi * TankHeight[3] * sq(TankDia[3] / 2) * NumbTanks[3] * 1000;

  PercentVol[3] = int(float(CurrentVol[3]) / float(TankSize[3]) * 100);  //  Calc Current Vol in Percentage

  if (CurrentVol[3] < 0) CurrentVol[3] = 0;

  PrintOut = String(CurrentVol[3]) + " Lts";
  spr.drawCentreString(PrintOut, BMx + 32, 76, FontType);

  SwitchOffLoRa();  //  To Write to TFT Screen

  delay(SprDELAY);               // Gives time to Update the Sprite - I think.. 10ms
  spr.pushSprite(215, SprTopY);  //  Tank Sprite

  SwitchOnLoRa();

  //  ***************************
  spr.fillSprite(BkgCol);

  int x = 54;
  NxtLneY = 5;

  PrintOut = "RSSI: " + LoRaRSSI;
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;

  PrintOut = "Dist: " + Dist[3];
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;
  PrintOut = "Time: " + ShortTime;
  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  //  Temp[3] = RecTemp;
  //  PrintOut = "Temp: " + Temp[3];
  //  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;
  //  PowerVolts[3]  = PowVolts;
  //  PrintOut = "Volts: " + PowerVolts[3];
  //  spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  NxtLneY = NxtLneY + LnSpc;  //  For SleepTime Countdown
  if (RecWake == "Awake")
    {
      SleepMode[3] = "Awake";
      spr.drawCentreString("Mode - Awake", x, NxtLneY, FontType);
    }

  if (RecWake == "Sleep")
    {
      SleepMode[3] = "Sleep";
      spr.drawCentreString("Mode - Sleep", x, NxtLneY, FontType);
    }

  SwitchOffLoRa();  //  To Write to TFT Screen

  delay(SprDELAY);               // Gives time to Update the Sprite - I think..  10ms
  spr.pushSprite(215, SprBotY);  //  Tank Sprite

  //  Clear Sprite Data
  spr.createSprite(10, 10);
  spr.fillSprite(BkgCol);
  spr.pushSprite(0, 239);  //  Tank Sprite

  SwitchOnLoRa();
}

void ClockPanelDisp(int Panel)  //  ********************************************************
{
  // Width and height of each Tank Area Sprite
  //#define SprWIDTH = 105;
  //#define SprHEIGHT 96

  SensorDelay = 10000;  //  Remove later maybe - Only show Tank Info for 1 sec..  Refresh Temperture Display

  if (Serial_ON_Header)
    {
      Serial.println("Drawing Sprite in Bottom Panels");
    }

  SwitchOffLoRa();

  spr.createSprite(SprWIDTH, SprHEIGHT);

  FontType = 2;
  spr.setTextSize(1);
  spr.setTextFont(8);
  spr.setTextColor(TFT_GOLD, BkgCol);

  // Fill the whole sprite with BkgCol (Sprite is in memory so not visible yet)
  spr.fillSprite(BkgCol);

  //spr.drawCentreString(RecTankNameLoc, 55, 2, FontType);  //  Put Text in Centre of Sprite
  spr.drawCentreString("Balvenie St", 55, 2, FontType);  //  Put Text in Centre of Sprite

  //  Draw Tank Graphics in Temperture Panel
  BMx = 20;  //  Will have to add X value for each Panel for additional Tanks
  BMy = 24;  //  Location of Tank Graphics in Sprite

  spr.drawBitmap(BMx, BMy, LoftyTank, 65, 47, TFT_GREEN);  //  Draw Tank Graphics

  Dist[1] = String(RecWaterLevel);

  //  Test to avoid Divide by Zero (0)
  if (TankHeight[1] == 0) TankHeight[1] = 0.1;

  HeightPercent[1] = (TankHeight[1] - RecWaterLevel) / TankHeight[1];

  RSSILoRa[1] = LoRaRSSI;

  //  Setup Voltage Warning
  BatWarning = "";

  if (BatVolts.toFloat() < 3.50 && BatVolts.toFloat() > 3.40)
    {
      BatWarning = " Low";
    }

  if (BatVolts.toFloat() < 3.40 && BatVolts.toFloat() > 3.30)
    {
      BatWarning = " Critical";
    }

  if (BatVolts.toFloat() < 3.30)
    {
      BatWarning = " Shut Down - 60m";
      // Change to Sleep if set to Awake
      SndSleepMode[1] = "Sleep";

      if (Serial_ON_Header)
        {
          Serial.print("BatVolts == ");
          Serial.println(BatVolts);
        }
    }

  //  WebPage Graphics Tank Percentage
  CalcHt[1] = 150 * (1 - HeightPercent[1]);
  PowerVolts[1] = PowVolts;
  BatteryVolts[1] = BatVolts;
  SolarVolts[1] = SolVolts;

  if (Serial_ON_Header || Serial_ON)
    {
      Serial.print("NumbTanks[1] > ");
      Serial.print(NumbTanks[1]);
      Serial.print("   TankHeight[1] > ");
      Serial.print(TankHeight[1]);
      Serial.print("   RecWaterLevel > ");
      Serial.print(RecWaterLevel);
      Serial.print("   HeightPercent[1]  > ");
      Serial.println(HeightPercent[1]);

      Serial.print("Power Volts[1]  > ");
      Serial.print(PowerVolts[1]);

      Serial.print("  BatteryVolts [1]  > ");
      Serial.print(BatteryVolts[1]);

      Serial.print("   SolarVolts[1]  > ");
      Serial.print(SolarVolts[1]);

      Serial.print("  RecWake Awake or Sleep >");
      Serial.print(RecWake);
      Serial.println("<");
    }

  Wx = BMx + 3;
  Wy = BMy + 44;  //  47 == height of BitMap

  Wh = 40 * HeightPercent[1];  //  40 = Height Inside Dimension of Tank
  WxL = BMx + 61;

  int WL;

  //  Draw Water in Tank - Series of Lines from Bottom of Tank to Top of Water Level
  for (WL = 0; WL < Wh - 1; WL++)
    {
      spr.drawLine(Wx, Wy - WL, WxL, Wy - WL, TFT_CYAN);
    }

  spr.drawBitmap(Wx, Wy - WL, LoftyWaterLevel, 62, 2, TFT_CYAN);  //  Draw Wavey Top Water Line

  CurrentVol[1] = Pi * (TankHeight[1] * HeightPercent[1]) * sq(TankDia[1] / 2) * NumbTanks[1] * 1000;  //  Current Volume

  TankSize[1] = Pi * TankHeight[1] * sq(TankDia[1] / 2) * NumbTanks[1] * 1000;

  PercentVol[1] = int(float(CurrentVol[1]) / float(TankSize[1]) * 100);  //  Calc Current Vol in Percentage

  if (CurrentVol[1] < 0) CurrentVol[1] = 0;

  PrintOut = String(CurrentVol[1]) + " Lts";
  //spr.drawCentreString(PrintOut, BMx + 32, 76, FontType);

  BMx = 55;       //  Will have to add X value for each Panel for additional Tanks
  BMy = 24 + 20;  //  74  Tank Height = 47   Sprite Height = 96

  spr.drawCentreString(TankTime, BMx, BMy, FontType);

  //  Clear Panel for Tank Graphics
  XX = 0;  //  for blacking out Icons
  YY = 157;
  Xw = 107;  //  217 - 108 == 109
  Yh = 83;   //  240 - 157 == 83

  //tft.fillRect(XX, YY, Xw, Yh, BkgCol);  //  black out Icons and Descriptions

  delay(SprDELAY);         // Gives time to Update the Sprite - I think..  20ms
  spr.pushSprite(0, 158);  //  Tank Sprite

  SwitchOnLoRa();

  //  ***************************
  //spr.fillSprite(BkgCol);  //  Create New/Next Text Sprite  -- NOT Needed Here

  //int x = 54;
  //NxtLneY = 5;

  //PrintOut = "RSSI: " + LoRaRSSI;  // default Printout
  //spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  //NxtLneY = NxtLneY + LnSpc;

  //PrintOut = "Dist: " + Dist[1];
  //spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  //NxtLneY = NxtLneY + LnSpc;
  //PrintOut = "Time: " + ShortTime;
  //spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  //NxtLneY = NxtLneY + LnSpc;
  Temp[1] = RecTemp;
  //PrintOut = "Temp: " + Temp[1];
  //spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  //NxtLneY = NxtLneY + LnSpc;
  BatteryVolts[1] = BatVolts;
  //PrintOut = "Battery: " + BatteryVolts[1] + "v";
  //spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  //NxtLneY = NxtLneY + LnSpc;

  //PrintOut = "Power: " + PowerVolts[1] + "v";
  //spr.drawCentreString(PrintOut, x, NxtLneY, FontType);

  //NxtLneY = NxtLneY + LnSpc;  //  For SleepTime Countdown

  if (RecWake == "Awake" || SndSleepMode[1] == "Awake")
    {
      SleepMode[1] = "Awake";
      //spr.drawCentreString("Mode - Awake", x, NxtLneY, FontType);
    }
  else if (RecWake == "Sleep" || SndSleepMode[1] == "Sleep")
    {
      SleepMode[1] = "Sleep";
      //spr.drawCentreString("Mode - Sleep", x, NxtLneY, FontType);
    }
  else
    {
      PrintOut = RecWake + " - " + SleepMode[1];
      //spr.drawCentreString(PrintOut, x, NxtLneY, FontType);
      //  SleepMode[1] = "Sleep";
    }

  //SwitchOffLoRa();  //  To Write to TFT Screen

  //delay(SprDELAY);             // Gives time to Update the Sprite - I think..  20ms
  //spr.pushSprite(0, SprBotY);  //  Tank Sprite

  //  Clear Sprite Data
  //spr.createSprite(1, 1);
  //spr.fillSprite(BkgCol);
  //spr.pushSprite(0, 239);  //  Tank Sprite

  // delay(10);  //  may be needed

  //SwitchOnLoRa();

  delay(3000);
}

void sendMessage(String SendMessageNow)  //  **************************************
{
  //Serial.println("void sendMessage(String SendMessageNow)");

  //digitalWrite(SendLED, HIGH);  //  Green

  //Serial.println("Sending Tank Name " + TankNameLoc);

  if (Serial_ON)
    {
      Serial.print("Line 4836 -  >> sender ");
      Serial.print(localAddress, HEX);
      Serial.print(" > destination ");
      Serial.print(destination, HEX);

      Serial.print(" - Sending Message == ");
      Serial.println(SendMessageNow);
    }

  LoRa.beginPacket();  //Start packet

  LoRa.write(destination);   // add destination address
  LoRa.write(localAddress);  // add sender address

  //LoRa.write(msgCount);                 // add SendMessageNow ID

  LoRa.write(SendMessageNow.length());  // add payload  length
  LoRa.print(SendMessageNow);           // add payload

  delay(10);  //  was 20

  LoRa.endPacket();  // finish packet and send it

  //msgCount++;                           // increment SendMessageNow ID

  //delay(250);  //  delay only for Green LED
  // digitalWrite(SendLED, LOW);
}

void RequestData(int TankNumb)  //  *******************************************
{
  SwitchOnLoRa();

  if (TankNumb == 1)  //  Header Tank
    {

      //  Display request from WebPage for Sleep or Awake
      //Serial.print("**************Mode_1_Done 0=false 1=true >");
      //Serial.println(Mode_1_Done);
      //Serial.print("SndSleepMode[1] >");
      //Serial.println(SndSleepMode[1]);

      destination = 0xAA;  //  Header Tank

      CalcSleep();  //  always calc Sleep/Awake time so it's ready to send..

      if (Serial_ON_Header || Serial_ON)
        {
          Serial.print("Line 4884 - Current Time: ");
          Serial.print(LongTime);
          Serial.print("   CalcSleep Done");
          Serial.print("   SleepTime[1]");
          Serial.print(String(SleepTime[1]));
          Serial.print("  SleepMode[1] set to Sleep? >");
          Serial.println(SleepMode[1]);

          if (SleepMode[1] == "Sleep")
            {
              //Serial.println("Send Header to Sleep  zzzzzZZZZZZzzzzz......");
              Serial.print("Line 4895 - Sent Header to Sleep @ ");
              Serial.print(LongTime);
              Serial.print("   NxtAwakeTimeAmPm[1] ==");
              Serial.println(NxtAwakeTimeAmPm[1]);
            }
        }

      //  Dummy or "Header"      Awake or Sleep            20 minutes or less
      TheMessage = "Header";
      TheMessage = TheMessage + ":" + SleepMode[1] + ": " + String(SleepTime[1]) + ":";

      if (Serial_ON_Header || Serial_ON)
        {
          Serial.print("Line 4908 - Sending Message == ");
          Serial.println(TheMessage);
        }

      sendMessage(TheMessage);

      DataSent[TankNumb] = "Sent";  //  Set this to see changes..

      if (Serial_ON)
        {
          Serial.print("Sending Request to > ");
          Serial.println(destination, HEX);
        }
      //  End Header **********************
    }

  if (TankNumb == 2)
    {

      destination = 0xBB;  //  DBL Main
      TheMessage = "DBL Main";

      sendMessage(TheMessage);

      DataSent[TankNumb] = "Sent";

      if (Serial_ON)
        {
          Serial.print("Sending Request to DBL Main > ");
          Serial.println(destination, HEX);
        }
    }

  if (TankNumb == 3)
    {
      //  Send Request to Tank 3

      destination = 0xCC;  //  uDeck
      TheMessage = "uDeck";

      sendMessage(TheMessage);

      DataSent[TankNumb] = "Sent";

      if (Serial_ON)
        {
          Serial.print("Sending Request to uDeck > ");
          Serial.println(destination, HEX);
        }
    }
}

void CalcSleep()  //  ************************************************
{
  //SetHr = 1;
  //Set20Min = false;

  //  For 20 min intervals - calcs which 20 min segment it's in
  if (Set20Min)  //  calc for every 20 minutes
    {
      //  Next line calcs number of minutes to next hour
      SleepTime[1] = 60 - TwMin;  //  Calc Number of minutes to next Hour..  if TwMin = 8 => 52

      int Cnt = 1;  //  if less than 20mins then TwMin > 40

      while (SleepTime[1] > 20)  //  Keeps cycling until SleepTime[1] is less than 20
        {
          SleepTime[1] = SleepTime[1] - 20;

          Cnt = Cnt + 1;
        }
      //  If Header wakes up before 60 minutes
      if (SleepTime[1] <= 2) SleepTime[1] = SleepTime[1] + 20;  //  Seems to be needed..

      if (Cnt == 1)  //  Therefore TwMin > 40 next 20 mins will take it over the hour mark
        {
          if (TwHour + 1 <= 11)  //  work out AM or PM  - if TwHour = 12 ( + 1) == 13..  Therefore 12pm
            {                    //  likewise if TwHour = 11 (+ 1) == 12..  Therefore 11am
              NxtAwakeTimeAmPm[1] = String(TwHour + 1) + ":00am";
            }
          else if (TwHour + 1 == 12)
            {
              NxtAwakeTimeAmPm[1] = String(TwHour + 1) + ":00pm";
            }
          else
            {
              NxtAwakeTimeAmPm[1] = String(TwHour + 1 - 12) + ":00pm";
            }
        }

      if (Cnt == 2)  //  Therefore TwMin > 20 mins  >>>  next min is 40
        {
          if (TwHour < 12)
            {
              NxtAwakeTimeAmPm[1] = String(TwHour) + ":40" + "am";
            }
          else if (TwHour == 12)
            {
              NxtAwakeTimeAmPm[1] = String(TwHour) + ":40pm";
            }
          else
            {
              NxtAwakeTimeAmPm[1] = String(TwHour - 12) + ":40pm";
            }
        }

      if (Cnt == 3)  //  Therefore TwMin > 00 mins  >>>  next min is 20
        {
          if (TwHour < 12)
            {
              NxtAwakeTimeAmPm[1] = String(TwHour) + ":20" + "am";
            }
          else if (TwHour == 12)
            {
              NxtAwakeTimeAmPm[1] = String(TwHour) + ":20pm";
            }
          else
            {
              NxtAwakeTimeAmPm[1] = String(TwHour - 12) + ":20pm";
            }
        }
    }
  else  //  For Hourly plus Readings, calc Time and AM/PM
    {
      //Serial.println();
      //Serial.print("Line 5034 - SleepCalcDone = ");
      //Serial.println(SleepCalcDone);

      if (!SleepAmPmDone)  //  TwHour == TwentyHour Time
        {                  // if TwHour = 5 (+2) == 7.. >> AM
          if (TwHour + SetHr <= 11)
            {
              NxtAwakeTimeAmPm[1] = String(TwHour + SetHr) + ":00am";
            }

          if (TwHour + SetHr == 12)  //  work out AM or PM  - if TwHour = 10 (+2) == 12.. >> PM
            {
              NxtAwakeTimeAmPm[1] = String(TwHour + SetHr) + ":00pm";  //  12:00pm
            }
          //  between 13 >> 24+
          if (TwHour + SetHr > 12 && TwHour + SetHr < 24)
            {  // if TwHour = 20 (+2) -12 == 10.. >> PM
              NxtAwakeTimeAmPm[1] = String(TwHour + SetHr - 12) + ":00pm";
            }

          if (TwHour + SetHr == 24)
            {
              NxtAwakeTimeAmPm[1] = "12:00am";
            }

          if (TwHour + SetHr > 24)  //  TwHour = 23 (+3) == 26.. >> AM  -  26 - 24 == 2am
            {
              NxtAwakeTimeAmPm[1] = String(TwHour + SetHr - 24) + ":00am";
            }
        }
      SleepAmPmDone = true;
    }
  //  Minutes to next Wake up needs to be calculated regularly
  //  Trying to get the Header Unit to wake up 1 or 2 minutes past the hour..
  if (TwMin > 45)
    {
      SleepTime[1] = SetHr * 60 + 60 - TwMin + SetHr;  //  If the Unit wakes just before the Hour - Add extra min for each Hour
    }
  else
    {
      SleepTime[1] = SetHr * 60 - TwMin + SetHr;  //  If the Unit wakes just after the Hour - Add extra min for each Hour
    }


  if (Serial_ON_Header)
    {
      Serial.print("Line 5065 - TwHour == ");
      Serial.print(TwHour);

      Serial.print("  TwMin == ");
      Serial.print(TwMin);

      Serial.print("  Sleep Time == ");
      Serial.println(SleepTime[1]);
    }
}

void CheckWebChanges()  //  *******************************************************
{

  //  Check for changes in Numb of Tanks  ****************************
  if (server.hasArg("NumbTanks_1"))
    {
      // Extract the updated value from the form data
      float newNumbTanks = server.arg("NumbTanks_1").toFloat();

      // Update the NumbTanks[1] variable
      NumbTanks[1] = newNumbTanks;

      if (NumbTanks[1] != LstNumbTanks[1])
        {
          if (Serial_ON_Header || Serial_ON)
            {
              Serial.print("*********** Update EEPROMs for NumbTanks[1] :");
              Serial.println(NumbTanks[1]);
            }
          SaveTankData();
          LstNumbTanks[1] = NumbTanks[1];
        }
    }

  if (server.hasArg("NumbTanks_2"))
    {
      // Extract the updated value from the form data
      float newNumbTanks = server.arg("NumbTanks_2").toFloat();

      // Update the NumbTanks[2] variable
      NumbTanks[2] = newNumbTanks;

      if (NumbTanks[2] != LstNumbTanks[2])
        {
          if (Serial_ON_Header || Serial_ON)
            {
              Serial.print("*********** Update EEPROMs for NumbTanks[2] :");
              Serial.println(NumbTanks[2]);
            }
          SaveTankData();
          LstNumbTanks[2] = NumbTanks[2];
        }
    }

  if (server.hasArg("NumbTanks_3"))
    {
      // Extract the updated value from the form data
      float newNumbTanks = server.arg("NumbTanks_3").toFloat();

      // Update the NumbTanks[3] variable
      NumbTanks[3] = newNumbTanks;

      if (NumbTanks[3] != LstNumbTanks[3])
        {
          if (Serial_ON_Header || Serial_ON)
            {
              Serial.print("*********** Update EEPROMs for NumbTanks[3] :");
              Serial.println(NumbTanks[3]);
            }
          SaveTankData();
          LstNumbTanks[3] = NumbTanks[2];
        }
    }

  //  Check for changes in Tank Heights  ****************************
  if (server.hasArg("TankHeight_1"))
    {
      // Extract the updated value from the form data
      float newTankHeight = server.arg("TankHeight_1").toFloat();

      // Update the TankHeight[1] variable
      TankHeight[1] = newTankHeight;

      if (TankHeight[1] != LstTankHeight[1])
        {
          if (Serial_ON_Header || Serial_ON)
            {
              Serial.println("*********** Update EEPROMs for TankHeight[1]");
            }
          SaveTankData();
          LstTankHeight[1] = TankHeight[1];
        }
    }

  if (server.hasArg("TankHeight_2"))
    {
      // Extract the updated value from the form data
      float newTankHeight = server.arg("TankHeight_2").toFloat();

      // Update the TankHeight[2] variable
      TankHeight[2] = newTankHeight;

      if (TankHeight[2] != LstTankHeight[2])
        {
          if (Serial_ON)
            {
              Serial.println("*********** Update EEPROMs for TankHeight[2]");
            }
          SaveTankData();
          LstTankHeight[2] = TankHeight[2];
        }
    }

  if (server.hasArg("TankHeight_3"))
    {
      // Extract the updated value from the form data
      float newTankHeight = server.arg("TankHeight_3").toFloat();

      // Update the TankHeight[3] variable
      TankHeight[3] = newTankHeight;

      if (TankHeight[3] != LstTankHeight[3])
        {
          if (Serial_ON)
            {
              Serial.println("*********** Update EEPROMs for TankHeight[3]");
            }
          SaveTankData();
          LstTankHeight[3] = TankHeight[3];
        }
    }

  //  Check for changes in Tank Diameters  ****************************
  if (server.hasArg("TankDia_1"))
    {
      // Extract the updated value from the form data
      float newTankDia = server.arg("TankDia_1").toFloat();

      // Update the TankHeight[1] variable
      TankDia[1] = newTankDia;

      if (TankDia[1] != LstTankDia[1])
        {
          if (Serial_ON_Header || Serial_ON)
            {
              Serial.println("*********** Update EEPROMs for TankDia[1]");
            }
          SaveTankData();
          LstTankDia[1] = TankDia[1];
        }
    }

  if (server.hasArg("TankDia_2"))
    {
      // Extract the updated value from the form data
      float newTankDia = server.arg("TankDia_2").toFloat();

      // Update the TankHeight[2] variable
      TankDia[2] = newTankDia;

      if (TankDia[2] != LstTankDia[2])
        {
          if (Serial_ON)
            {
              Serial.println("*********** Update EEPROMs for TankDia[2]");
            }
          SaveTankData();
          LstTankDia[2] = TankDia[2];
        }
    }

  if (server.hasArg("TankDia_3"))
    {
      // Extract the updated value from the form data
      float newTankDia = server.arg("TankDia_3").toFloat();

      // Update the TankHeight[3] variable
      TankDia[3] = newTankDia;

      if (TankDia[3] != LstTankDia[3])
        {
          if (Serial_ON)
            {
              Serial.println("*********** Update EEPROMs for TankDia[3]");
            }
          SaveTankData();
          LstTankDia[3] = TankDia[3];
        }
    }

  //  Check for changes in SleepMode[1]  ****************************
  if (server.hasArg("Mode_1"))  // && !Mode_1_Done)
    {
      LstSleepMode[1] = SndSleepMode[1];

      // Extract the updated value from the form data
      String newMode = server.arg("Mode_1");

      // Update the Mode_1 variable
      if (newMode == "Sleep")
        {
          SleepMode[1] = "Sleep";
          SndSleepMode[1] = "Sleep";

          SetHeaderSleep = true;

          if (LstSleepMode[1] != SleepMode[1])
            {
              if (Serial_ON_Header)
                {
                  Serial.println("******** NewMode changed to Sleep **********");
                }
            }
        }

      if (newMode == "Awake")
        {
          SleepMode[1] = "Awake";
          SndSleepMode[1] = "Awake";
          SetHeaderSleep = false;

          HeaderAwakeTime = millis();  //  Start counting in case Awake Mode sent and forgotten about 15 mins max..
          //  Is reset if Distance Changes

          if (LstSleepMode[1] != SleepMode[1])
            {
              if (Serial_ON_Header)
                {
                  Serial.println("******** NewMode changed to Awake **********");
                }
            }
        }

      if (LstSleepMode[1] != SndSleepMode[1])  //  SleepMode has Changed - Send to Header..
        {
          RequestData(1);  //  Send new Sleep Mode to Sender Unit

          HeaderAwakeTime = millis();  //  Start counting in case Awake Mode sent and forgotten about 15 mins max..

          LstSleepMode[1] = SleepMode[1];

          ShowTFTdata(1);  //  Update TFT Screen..
        }
    }

  //  Check for changes in Sleep Time  ****************************
  if (server.hasArg("ChgeSleepTime"))
    {
      // Extract the updated value from the form data
      newSleepTime = server.arg("ChgeSleepTime");

      LstSetHrStr = SetHrStr;

      Set20Min = false;

      if (newSleepTime == "20 mins")
        {
          Set20Min = true;

          SetHrStr = "20 mins";  //  Set here to 20 mins for checking only for change of value
          if (Serial_ON)
            {
              Serial.print("SetHrStr :");
              Serial.println(SetHrStr);
            }
        }
      else
        {
          SetHrStr = newSleepTime;  // 1 hr  or 2 hr  or 3 hr  or 4 hr
          if (Serial_ON)
            {
              Serial.print("SetHrStr :");
              Serial.println(SetHrStr);
            }

          String PrtSetHrStr = SetHrStr.substring(0, 2);
          if (Serial_ON)
            {
              Serial.print("PrtSetHrStr :");
              Serial.println(PrtSetHrStr);
            }

          SetHr = PrtSetHrStr.toInt();
          if (Serial_ON)
            {
              Serial.print("SetHr :");
              Serial.println(SetHr);
            }
        }

      if (LstSetHrStr != SetHrStr)
        {
          if (Serial_ON_Header || Serial_ON)
            {
              Serial.println();
              Serial.print("*********** Update EEPROMs for SetHr :");
              Serial.println(SetHr);
              Serial.print("Set20Min > 0 false, 1 true :");
              Serial.println(Set20Min);
              Serial.println();
            }
          //  Just save SetHr and Set20Min
          preferences.begin("TankData", RW_MODE);
          preferences.putBool("Set20Min", Set20Min);
          preferences.putUInt("SetHr", SetHr);
          preferences.end();
          //SaveTankData();

          CalcSleep();
        }
    }

  //  Check for changes in Screen Output  ****************************
  if (server.hasArg("Mode_4"))
    {
      String ScreenMode = "Screen";
      // Extract the updated value from the form data
      ScreenMode = server.arg("Mode_4");

      // Update the Mode_1 variable
      if (ScreenMode == "Screen")
        {
          //Serial.print("Goto Screen -->  ");
          //Serial.println(ScreenMode);
          PrntData = false;
          HistoryOutput = "Screen";
        }
      else
        {
          //Serial.print("Goto Screen -->  ");
          //Serial.println(ScreenMode);
          PrntData = true;
          HistoryOutput = "History";
        }
    }
}

void CheckSleepHeader()  //  *****************************************
{
  if (SndSleepMode[1] == "Sleep" && !SetHeaderSleep)
    {
      SetHeaderSleep = true;
    }

  if (SetHeaderSleep)  //  && DataSent[1] == "Recd"
    {
      //Serial.println("if (SetHeaderSleep && !CalcSleepDone)");

      if (LstSetHeaderSleep != SetHeaderSleep)
        {
          if (Serial_ON)
            {
              Serial.print("SetHeaderSleep 1 == true: ");
              Serial.println(SetHeaderSleep);
            }

          LstSetHeaderSleep = SetHeaderSleep;
        }

      SleepMode[1] = "Sleep";  //  This when sent, puts the Header Tank Sensor to Sleep

      SetHeaderSleep = false;
      //Mode_1_Done = false;
      SndSleepMode[1] = "Sleep";  //  Set to Sleep - Send to Header Tank
    }

  if (DataSent[1] == "Recd" && SndSleepMode[1] != "Sleep")
    {
      NxtAwakeTimeAmPm[1] = "NA";
    }
}

void StoreSortedHistory()
{
  if (HistoryCnt == 1)  //  Store first data in first spot
    {
      HistoryData[1][0] = CurrentDate;
      HistoryData[1][1] = TimeStamp[1];
      HistoryData[1][2] = String(CurrentVol[1]);
      HistoryData[1][3] = Dist[1];
      HistoryData[1][4] = PowerVolts[1];
      HistoryData[1][5] = BatteryVolts[1];
      HistoryData[1][6] = SolarVolts[1];
      HistoryData[1][7] = Temp[1] + ";" + RecTimeSleeping;

      if (Serial_ON_HeaderRec)
        {
          Serial.println("Line 5459 - >>>><<<<  History Count 1 ==  >>>><<<<");
        }

      HistoryCnt++;
    }
  else  //  Move data up 1 slot up and store new data in spot [1]
    {
      //  move data up before storing new data
      for (int i = HistoryCnt - 1; i >= 1; i--)  //  count down from max HistoryCnt down to 1
        {
          HistoryData[i + 1][0] = HistoryData[i][0];
          HistoryData[i + 1][1] = HistoryData[i][1];
          HistoryData[i + 1][2] = HistoryData[i][2];
          HistoryData[i + 1][3] = HistoryData[i][3];
          HistoryData[i + 1][4] = HistoryData[i][4];
          HistoryData[i + 1][5] = HistoryData[i][5];
          HistoryData[i + 1][6] = HistoryData[i][6];
          HistoryData[i + 1][7] = HistoryData[i][7];
        }

      //  Store Latest Data in top spot
      HistoryData[1][0] = CurrentDate;
      HistoryData[1][1] = TimeStamp[1];
      HistoryData[1][2] = String(CurrentVol[1]);
      HistoryData[1][3] = Dist[1];
      HistoryData[1][4] = PowerVolts[1];
      HistoryData[1][5] = BatteryVolts[1];
      HistoryData[1][6] = SolarVolts[1];
      HistoryData[1][7] = Temp[1] + ";" + RecTimeSleeping;

      HistoryCnt++;

      if (Serial_ON_HeaderRec)
        {
          Serial.print("Line 5480 - >>>> HistoryCnt++ <<<<  History Count 2 = >");
          Serial.println(HistoryCnt);
        }

      if (HistoryCnt > MaxMem)
        {
          HistoryCnt = MaxMem;  //  Max Memory allaction is 720
        }
    }

  if (Serial_ON_HeaderRec)
    {
      Serial.println("Line 5492 - =========== History Data ==================================");

      //  Printout all History Data
      for (int i = 0; i < HistoryCnt; i++)  //  HistoryData[0][j]) contains Column Names
        {
          for (int j = 0; j <= 7; j++)
            {
              Serial.print(HistoryData[i][j]);
              Serial.print(" ");
            }
          Serial.println();
        }
      Serial.println("============ History Data =================================");
    }
}

void CheckHeaderAwake()  //  **********************************************************
{
  //  Cheak Header is not in Awake Mode and forgotten about..  15mins after last diff in Distance..

  if (SleepMode[1] == "Awake" && DataSent[1] == "Recd")
    {                                           //  If Awake Mode sent and forgotten about 15 mins max..
      if (millis() - HeaderAwakeTime > 120000)  //  15min * 60 * 1000 == 900000 msec  Testing >> 2min x 60 x 1000 = 120000
        {
          SleepMode[1] = "Sleep";

          SndSleepMode[1] = "Sleep";

          if (Serial_ON_Header)
            {
              Serial.println("*******  You Forgot about Header - Header is going to Sleep  *******");
            }

          //  Send Header to Sleep - DO NOT ASK FOR A REPLY.. Header should wake up in 5 mins and proceed as normal.
          RequestData(1);  //  send Header to sleep
        }
    }
}

//  *****  FINISH *** Tank  *** Tank *** Tank *** Tank *** Tank *** Tank *** Tank *** Tank *** Tank

void RandomFlicker()  //  oooooooOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOooooooo
{
  int FlickerBrightness = WledBrightness;

  if (FlickerBrightness < 40)
    {
      FlickerBrightness = 40;
    }
  for (int i = 0; i < NUM_LEDS; i++)
    {
      int RandBright = random8(FlickerBrightness + 30, FlickerBrightness + 100);

      if (RandBright > 255) RandBright = 255;

      leds[i] = CHSV(51, 98, RandBright);  //  51, 98 is for Candle Light setting

      //  leds[i].fadeToBlackBy(200); // Just sends indivual LEDs to slowly to black

      FastLED.show();
    }
}

void FullRainBow()  //  oooooooOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOooooooo
{
  static uint8_t starthue = 0;
  fill_rainbow(leds, NUM_LEDS, --starthue, 255 / NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++)
    {  //  hue + (i * 10)  allows the rainbow to move along the strip
      leds[i] = CHSV(hue + (i * 10), 255, 200);
    }

  //EVERY_N_MILLISECONDS(1)
  //{
  hue++;
  //}
  FastLED.show();
  FastLED.delay(10);
}

void loop()  //  ******************************************************************
{
  //Serial.println("onReceive 01");
  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());

  //Serial.println("TouchSpot");
  TouchSpot();

  //Serial.println("ShowTime");
  ShowTime();

  //  Wled Functions  ##########################################

  if (MakeCandleFlicker)
    {
      RandomFlicker();
    }

  if (DoFullRainbow)
    {
      FullRainBow();
    }

  PushButton = digitalRead(OnOffPin);

  if (PushButton == HIGH)  // Button has been pressed
    {
      if (WledState)
        {
          if (Serial_ON)
            {
              Serial.println("Wleds will be turned OFF");
            }

          FastLED.setBrightness(0);  // Turn OFF Wleds

          WledState = false;
          WledON = false;  //  Reset after Wleds turned OFF
        }
      else
        {
          if (Serial_ON)
            {
              Serial.println("Wleds will be turned ON");
            }
          WledState = true;
          FastLED.setBrightness(WledBrightness);  // Turn ON Wleds
        }

      FastLED.show();
      FastLED.delay(10);

      while (PushButton == HIGH)
        {
          PushButton = digitalRead(OnOffPin);  //  Loop here until Button Released
        }
    }

  //Serial.println("onReceive 02");
  // parse for a packet, and call onReceive with the result:
  //onReceive(LoRa.parsePacket());

  server.handleClient();  //  Web Server

  //  Show Inside Temp and Daily Forecast for 5 secs when BigDayDisplay is toggled ON
  if (TempWeatherDisp)
    {
      if (FirstTime)
        {
          FirstTime = false;
          TempTimer = millis();
          BigDayDisplay = false;
          DoBigDay = false;
          ShowDayDate = false;
          // Blank Lower Area
          tft.fillRect(0, 156, 320, 84, BkgCol);
          DrawLines();
          DailyUpdate();
          SensorDelay = 0;
          TemperatureSensor();
        }

      if ((millis() - TempTimer > 5000))  //  Using this method, eliminates possible millis() overflow after 49+ days
        {
          TempWeatherDisp = false;
          BigDayDisplay = true;
          DoBigDay = true;
          ShowDayDate = true;
        }
    }
  //Serial.println("onReceive 03");
  // parse for a packet, and call onReceive with the result:
  //onReceive(LoRa.parsePacket());

  if (MinBrightSw)  //  MinBright Switch ON (<99) or OFF (=99)
    {
      GoLDR();
    }

  CheckWebChanges();

  CheckSleepHeader();
  //Serial.println("onReceive 04");
  // parse for a packet, and call onReceive with the result:
  //onReceive(LoRa.parsePacket());

  CheckHeaderAwake();  //  Cheak Header is not in Awake Mode and forgotten about..

  if (InternetFail)  //  Check internet every 1 minute 60 * 1000 = 60,000 until internet connected
    {
      if ((millis() - UpdateMillis) > 60000)  //  Using this method, eliminates possible millis() overflow after 49+ days
        {
          //Serial.print("Checking for Internet on ");
          //Serial.print(ssid);
          //Serial.print(" every 1 Minute at :");
          //Serial.println(LongTime);

          /*  tft.setTextSize(1);
        tft.setTextFont(8);
        tft.setTextColor(TFT_PINK, BkgCol);
        tft.drawString("     WiFi??    ", xTime1Slot, yTime1Slot, 2);
      */

          TryWiFi();

          if (!InternetFail)
            {
              //Serial.println("Internet now UP");
              GetForcast = true;

              GoWeather();  //  Update Weather Icons ????
            }
          else
            {
              //Serial.println("Internet still DOWN");
              UpdateMillis = millis();
            }
        }
    }
  else
    {
      GoWeather();
    }
  //Serial.println("onReceive 05");
  // parse for a packet, and call onReceive with the result:
  //onReceive(LoRa.parsePacket());

  if (!BigDayDisplay) TemperatureSensor();

  TankTest = false;
  if (TankTest)
    {
      delay(2000);

      RecTankNameLoc = "Header";
      RecWaterLevel = 1.05;
      RecWake = "Sleep";
      TankTest = false;
      RSSILoRa[1] = 101;
      BatVolts = 4.05;
      PowVolts = 3.0;
      BatVolts = 4.1;
      SolVolts = 5.1;
      RecTemp = 19.9;
      TankTime = String(DispHour) + ":" + StrMin + AmPm;

      ClockPanelDisp(1);  //  Panel == 1
    }
}

String getPage()
{  //                                                     Pages Refreshes every 10 secs
  String page = "<!DOCTYPE html><html><head> <meta http-equiv=\"Refresh\" content=\'10\'>";

  //page += "<style>.tab1 { tab-size: 2; }</style>";  //  Set up Tabbing uses <pre class='tab1'> -  Gaz not working as wxpected

  page += "<style> table, th, td {border: 1px solid; border-collapse: collapse; border-color:#0000ff;}";  //  solid black;  1px solid
  page += "body{background-color:powderblue;}";

  page += "h1{color:blue; font-family:verdana; font-size:200%; text-align: center;}";
  page += "h2{color:black; font-family:verdana; font-size:100%; text-align: centre;}</style>";

  page += "<style>.svg-container {position: relative;}";

  page += ".svg-image {position: absolute; top: 0; left: 0; z-index: 1; }";

  page += "</style></head><body>";

  page += "<h1>" + WebTitle + "</h1>";  //  LOFTY WATER INC

  page += "<table style='width:100%'><tr><th>Header Tank</th><th>DBL Main Tanks</th><th>uDeck Tanks</th></tr>";  //  svg-align=center

  page += "<tr>";

  //  =========================  Graphics of Tanks  ==================================================

  // ==========  Tank Graphics Fill with Water colour - 1st Column  Header Tank  ==============================

  page += "<td><div class='svg-container'><svg width=100% height=180><!-- Sets up Space for Graphics -->";                   //
  page += "<rect x=50 y=20 rx=20 ry=10 width=150 height=150 style=fill:blue;stroke:black;stroke-width:5;opacity:1/></svg>";  // Tank

  page += "<svg class='svg-image' width=100% height=180><!-- Show TopCap -->";
  page += "<rect x=70 y=10 rx=3 ry=3 width=40 height=8 style=fill:grey;stroke:black;stroke-width:2;opacity:1/></svg>";  // Top Cap

  //  This next line is Graphics that blanks out top part to show Water Level -->";
  //  If Tank Height is 150 then height 75 (Below) shows Water Level under Halfway -->";
  page += "<svg class='svg-image' width=100% height=180><!-- Show Water Level -->";
  page += "<rect x=50 y=20 rx=10 ry=10 width=150 height=" + String(CalcHt[1]) + " style=fill:lightsteelblue;stroke:grey;stroke-width:5;opacity:1/></svg>";

  page += "<svg class='svg-image' width=100% height=180><!-- Show Text -->";                                                                                                  //  add Volume as Percentage to SVG area
  page += "<text x=80 y=100 font-family='Arial Black' fill='black' stroke-width='2px' stroke='white' font-size='40' fill=black>" + String(PercentVol[1]) + "%</text></svg>";  //  add text to SVG area

  //page += "<text x=80 y=100 font-family='Tahoma' weight='bold' font-size='40' fill=black>" + String(PercentVol[1]) + "%</text></svg>"; //  add text to SVG area

  page += "</div></td>";

  //  ========== Tank Graphics Fill with Water colour - 2nd Column  DBL Main Tank  ==============================

  page += "<td><div class ='svg-container'><svg width=100% height=180>";

  page += "<rect x=50 y=20 rx=20 ry=10 width=150 height=150 style=fill:blue;stroke:black;stroke-width:5;opacity:1/></svg>";

  page += "<svg class='svg-image' width=100% height=180><!-- Show TopCap -->";
  page += "<rect x=70 y=10 rx=3 ry=3 width=40 height=8 style=fill:grey;stroke:black;stroke-width:2;opacity:1/></svg>";  // Top Cap

  //  This next line is Graphics that blanks out top part to show Water Level -->";
  page += "<svg class='svg-image' width=100% height=180><!-- Show Water Level -->";
  page += "<rect x=50 y=20 rx=10 ry=10 width=148 height=" + String(CalcHt[2]) + " style=fill:lightsteelblue;stroke:grey;stroke-width:5;opacity:1/></svg>";

  page += "<svg class='svg-image' width=100% height=180><!-- Show Text -->";                                                                                                  //  add Volume as Percentage to SVG area
  page += "<text x=80 y=100 font-family='Arial Black' fill='black' stroke-width='2px' stroke='white' font-size='40' fill=black>" + String(PercentVol[2]) + "%</text></svg>";  //  add text to SVG area

  page += "</div></td>";  //

  //  ========== Tank Graphics Fill with Water colour - 3rd Column  uDeck Tank  ===================================

  page += "<td><div class='svg-container'><svg width=100% height=180>";  //      text-align: center  svg-align=center

  page += "<rect x=50 y=20 rx=20 ry=10 width=150 height=150 style=fill:blue;stroke:black;stroke-width:5;opacity:1/></svg>";

  page += "<svg class='svg-image' width=100% height=180><!-- Show TopCap -->";
  page += "<rect x=70 y=10 rx=3 ry=3 width=40 height=8 style=fill:grey;stroke:black;stroke-width:2;opacity:1/></svg>";  // Top Cap

  //  This next line is Graphics foe top of Water
  page += "<svg class='svg-image' width=100% height=180><!-- Show Water Level -->";
  page += "<rect x=50 y=20 rx=10 ry=10 width=148 height=" + String(CalcHt[3]) + " style=fill:lightsteelblue;stroke:grey;stroke-width:5;opacity:1/></svg>";

  page += "<svg class='svg-image' width=100% height=180><!-- Show Text -->";                                                                                                  //  add Volume as Percentage to SVG area
  page += "<text x=80 y=100 font-family='Arial Black' fill='black' stroke-width='2px' stroke='white' font-size='40' fill=black>" + String(PercentVol[3]) + "%</text></svg>";  //  add text to SVG area

  page += "</div></td></tr>";

  //  ===========================  End of Graphics of Tanks  =======================================================

  page += "<tr><td>Current Volume = " + String(CurrentVol[1]) + " lts or " + String(CurrentVol[1] / 4.54601, 0) + " gals</td>";
  page += "<td>Current Volume = " + String(CurrentVol[2]) + " lts or " + String(CurrentVol[2] / 4.54601, 0) + " gals</td> ";
  page += "<td>Current Volume = " + String(CurrentVol[3]) + " lts or " + String(CurrentVol[3] / 4.54601, 0) + " gals</td></tr>";

  page += "<tr><td>Date/Time : " + CurrentDate + " : " + TimeStamp[1] + "</td>";
  page += "<td>Date/Time : " + CurrentDate + " : " + TimeStamp[2] + "</td>";
  page += "<td>Date/Time : " + CurrentDate + " : " + TimeStamp[3] + "</td></tr>";

  page += "<tr><td>Full Capacity = " + String(TankSize[1]) + " lts or " + String(TankSize[1] / 4.54601, 0) + " gals</td>";
  page += "<td>Full Capacity = " + String(TankSize[2]) + " lts or " + String(TankSize[2] / 4.54601, 0) + " gals</td>";
  page += "<td>Full Capacity = " + String(TankSize[3]) + " lts or " + String(TankSize[3] / 4.54601, 0) + " gals</td></tr>";


  //  ==========================================================================================

  //  No. of Tanks  1 Input
  page += "<tr><td>";
  page += "<form action='/' method='POST'>";
  page += "<label for='NumbTanks_1'>No.of Tanks : </label>";  // <pre class='tab1'> using Tab Style - Gaz wnot working
  page += "<input size=5 type='number' step='1' id='NumbTanks_1' name='NumbTanks_1' value=" + String(NumbTanks[1]) + ">";
  page += "<input type='submit' value='Send'></form></td>";  // </pre>  using Tab Style - Gaz

  //  No. of Tanks  2  Input
  page += "<td>";
  page += "<form action='/' method='POST'>";
  page += "<label for='NumbTanks_2'>No.of Tanks : </label>";
  page += "<input size=5 type='number' step='1' id='NumbTanks_2' name='NumbTanks_2' value=" + String(NumbTanks[2]) + ">";
  page += "<input type='submit' value='Send'></td>";

  //  No. of Tanks  3  Input
  page += "<td>";
  page += "<form action='/' method='POST'>";
  page += "<label for='NumbTanks_3'>No.of Tanks : </label>";
  page += "<input size=5 type='number' step='1' id='NumbTanks_3' name='NumbTanks_3' value=" + String(NumbTanks[3]) + ">";
  page += "<input type='submit' value='Send'></td></tr>";

  //  ==========================================================================================

  //  Tank Diameter Input
  page += "<tr><td>";
  page += "<form action='/' method='POST'>";                                                                            //  action_page.php
  page += "<label for='TankDia_1'>Tank Diameter : </label>";                                                            //  <br>
  page += "<input size=5 type='number' step='0.01' id='TankDia_1' name='TankDia_1' value=" + String(TankDia[1]) + ">";  //  <br>
  page += "<input type='submit' value='Send'></td>";

  page += "<td>";
  page += "<form action='/' method='POST'>";                                                                             //  action_page.php
  page += "<label for='TankDia_2'>Tank Diameter : </label>";                                                             //  <br>
  page += "<input size=5 type='number' step='0.01' id='TankDiat_2' name='TankDia_2' value=" + String(TankDia[2]) + ">";  //  <br>
  page += "<input type='submit' value='Send'></td>";

  page += "<td>";
  page += "<form action='/' method='POST'>";                                                                            //  action_page.php
  page += "<label for='TankDia_3'>Tank Diameter : </label>";                                                            //  <br>
  page += "<input size=5 type='number' step='0.01' id='TankDia_3' name='TankDia_3' value=" + String(TankDia[3]) + ">";  //  <br>
  page += "<input type='submit' value='Send'></td></tr>";

  //  ==========================================================================================

  //  Tank Height Input
  page += "<tr><td>";
  page += "<form action='/' method='POST'>";
  page += "<label for='TankHeight_1'>Tank Height : </label>";
  page += "<input size=5 type='number' step='0.01' id='TankHeight_1' name='TankHeight_1' value=" + String(TankHeight[1]) + ">";  //  <br>
  page += "<input type='submit' value='Send'></td>";

  page += "<td>";
  page += "<form action='/' method='POST'>";
  page += "<label for='TankHeight_2'>Tank Height : </label>";
  page += "<input size=5 type='number' step='0.01' id='TankHeight_2' name='TankHeight_2' value=" + String(TankHeight[2]) + ">";  //  <br>
  page += "<input type='submit' value='Send'></td>";

  page += "<td>";
  page += "<form action='/' method='POST'>";
  page += "<label for='TankHeight_3'>Tank Height : </label>";
  page += "<input size=5 type='number' step='0.01' id='TankHeight_3' name='TankHeight_3' value=" + String(TankHeight[3]) + ">";  //  <br>
  page += "<input type='submit' value='Send'></td></tr>";

  //  ==========================================================================================

  //  Distance Reading - Time Stamp - Power Volts - Battery Volts - Solar Volts - Temperature
  page += "<tr><td>Dist = " + Dist[1] + "m</td>";
  page += "<td>Dist = " + Dist[2] + "m</td>";
  page += "<td>Dist = " + Dist[3] + "m</td></tr>";
  page += "<tr><td>Power     = " + PowerVolts[1] + "v</td><td></td><td></td></tr>";
  page += "<tr><td>Battery   = " + BatteryVolts[1] + "v" + BatWarning + "</td><td></td><td></td></tr>";
  page += "<tr><td>Solar     = " + SolarVolts[1] + "v</td><td></td><td></td></tr>";
  page += "<tr><td>Temp      = " + Temp[1] + "c</td><td></td><td></td></tr>";

  // ==========================================================================================
  page += "<td>";
  page += "<form action='/action_page.php'>";
  page += "<input type='radio' id='sleep' name='Mode_1' value='Sleep'>";
  page += "<label for='sleep'>Sleep</label>";
  page += "<input type='radio' id='awake' name='Mode_1' value='Awake'>";
  page += "<label for='awake'>Awake</label>";
  page += "<input type='submit' value='Submit'>";
  page += "</form> Mode:" + SndSleepMode[1] + "</td>";

  //  This shows "Mode: Awake" for the other Sensors
  page += "<td>Mode:Awake</td><td>Mode:Awake</td>";

  /*   //  Next Lines are if you want to add Sleep mode to other Tank Sensors
    page += "<td>";
    page += "<form action='/action_page.php'>";
    page += "<input type='radio' id='sleep' name='Mode_2' value='Sleep'>";
    page += "<label for='sleep'>Sleep</label>";
    page += "<input type='radio' id='awake' name='Mode_2' value='Awake'>";
    page += "<label for='awake'>Awake</label>";
    page += "<input type='submit' value='Submit'>";
    page += "</form> Mode:" + SndSleepMode[2] + "</td>";

    page += "<td>";
    page += "<form action='/action_page.php'>";
    page += "<input type='radio' id='sleep' name='Mode_3' value='Sleep'>";
    page += "<label for='sleep'>Sleep</label>";
    page += "<input type='radio' id='awake' name='Mode_3' value='Awake'>";
    page += "<label for='awake'>Awake</label>";
    page += "<input type='submit' value='Submit'>";
    page += "</form> Mode:" + SndSleepMode[3] + "</td>";*/

  //  Change Sleep Time : 20 mins or 1 to 4 hrs
  page += "<tr><td>";
  page += "<form action='/' method='POST'>";
  page += "<label for='SleepTime'>Sleep Time : </label>";

  page += "<select name='ChgeSleepTime' id='SleepTime'>";  //  value='SleepTime'>";
  page += "  <option value='20 mins'>20 mins</option>";
  page += "  <option value='1 Hr'>1 Hr</option>";
  page += "  <option value='2 Hr'>2 Hr</option>";
  page += "  <option value='3 Hr'>3 Hr</option>";
  page += "  <option value='4 Hr'>4 Hr</option>";
  page += "</select>";
  page += "<input type='submit' value='Send' >";  //  </td>";

  page += "</form> Set Sleep Time : " + newSleepTime + "</td><td></td><td></td></tr>";

  //  page += "<tr><td>Battery   = " + BatteryVolts[1] + "v" + BatWarning + "</td><td></td><td></td></tr>";

  //page += "<input size=5 type='number' step='0.01' id='ChgeSleepTime' name='ChgeSleepTime' value=" + SetHrStr + ">";  //  <br>


  page += "</tr>";

  page += "<tr><td>Wake Up Time = " + NxtAwakeTimeAmPm[1] + "</td><td></td><td></td>";

  page += "</tr>";

  //  =========================================================================================

  page += "<tr><td>RSSI = " + String(RSSILoRa[1]) + "</td>";
  page += "<td>RSSI = " + String(RSSILoRa[2]) + "</td>";
  page += "<td>RSSI = " + String(RSSILoRa[3]) + "</td></tr>";

  page += "</table>";  //  </body></html>";

  page += "<form action='/' method='POST'>";
  page += "<ul><li>Screen : ";  //  <ul>  This is for New Line
  page += "<input type='submit' value='Refresh'>";

  //  ======================= History Paging  ===============================

  page += "<form>";
  page += "<li><input type='radio' id='screen' name='Mode_4' value='Screen'>";
  page += "<label for='screen'>Screen</label>";
  page += "<input type='radio' id='history' name='Mode_4' value='History'>";
  page += "<label for='history'>History</label>";
  page += "<input type='submit' value='Submit'></li>";
  page += "</form> Mode:" + HistoryOutput;

  page += "</body></html>";

  return page;
}

String dataPage()
{
  String AllData = "<!DOCTYPE html><html><head><meta http-equiv=\"Refresh\" content=\'60\'>";

  AllData += "<style> table, th, td {border: 1px solid; border-collapse: collapse; border-color:#0000ff;}";  //  solid black;
  AllData += "body{background-color:powderblue;}";

  AllData += "h1{color:blue; font-family:verdana; font-size:200%; text-align: center;}";
  AllData += "h2{color:black; font-family:verdana; font-size:100%; text-align: centre;}</style>";

  AllData += "<style>.svg-container {position: relative;}";

  AllData += ".svg-image {position: absolute; top: 0; left: 0; z-index: 1; }";

  AllData += "</style></head><body>";

  AllData += "<h1>" + WebTitle + " - History Data</h1>";  //  LOFTY WATER INC

  AllData += "<table style='width:100%'><tr><th>Header Tank</th></tr>";  //  <th>DBL Main Tanks</th><th>uDeck Tanks</th></tr>";  //  svg-align=center

  AllData += "<tr>";

  //  ======================= Go Back to Normal Screen Data  ===============================

  AllData += "<form>";
  AllData += "<li><input type='radio' id='screen' name='Mode_4' value='Screen'>";
  AllData += "<label for='screen'>Screen</label>";
  AllData += "<input type='radio' id='history' name='Mode_4' value='History'>";
  AllData += "<label for='history'>History</label>";
  AllData += "<input type='submit' value='Submit'></li>";
  AllData += "</form> Mode:" + HistoryOutput + "</td>";

  //  Show Data
  //Serial.println("=========== History Data on History Web Page =============================");

  //  Printout all History Data
  for (int i = 0; i < HistoryCnt; i++)  //  HistoryData[0][j]) contains Column Names
    {
      AllData += "<tr><td>";
      for (int j = 0; j <= 7; j++)
        {
          //Serial.print(HistoryData[i][j]);
          //Serial.print(" ");

          AllData += String(HistoryData[i][j]);  //  "<tr><td>" +

          if (j != 7) AllData += " ; ";  //  Doesn't add ";" to end of Line
        }
      //Serial.println();
    }

  AllData += "</body></html>";

  return AllData;

  //  =========================================================================================
}
