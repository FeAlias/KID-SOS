#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <RFM69.h>
#include <SPI.h>
#define NETWORKID     100  // The same on all nodes that talk to each other
#define NODEID        2    // The unique identifier of this node
#define RECEIVER      1    // The recipient of packets
#define FREQUENCY     RF69_433MHZ
//#define ENCRYPT       true // Set to "true" to use encryption
#define KEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HCW   true // set to 'true' if you are using an RFM69HCW module
#define RFM69_CS      10
#define RFM69_IRQ     2
#define RFM69_IRQN    0  // Pin 2 is IRQ 0!
#define RFM69_RST     9 
//#define LED           13  // onboard blinky
int16_t packetnum = 0;  // packet counter, we increment per xmission
RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);
#define ACK_TIME    30  // # of ms to wait for an ack
float ftemp = 0;
int TRANSMITPERIOD = 300; //transmit a packet to gateway so often (in ms)
byte sendSize = 0;
boolean requestACK = false;
typedef struct {
  int           nodeId; //store this nodeId
  unsigned long uptime; //uptime in ms
  long temp; //float         temp;   //temperature maybe?
  int AAltitude;
  float LLat;
  float LLon;
  float TTemperature;
  int SendCount;
}
Payload;
Payload theData;
int CCounter = 0;
int potPin = A3;    // select the input pin for the potentiometer
int potValue = 0;  // variable to store the value coming from the pot
byte buffer[10];
// The TinyGPS++ object
TinyGPSPlus gps;
SoftwareSerial ss(4, 3);// The serial connection to the GPS device

void setup()
{
  Serial.begin(115200);
    Serial.println("starting...");  
  ss.begin(9600);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);  delay(100);
  digitalWrite(RFM69_RST, LOW);  delay(100);  
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  radio.setHighPower(); //uncomment only for RFM69HW!
  if (IS_RFM69HCW) {
    radio.setHighPower();    // Only for RFM69HCW & HW!
  }
  radio.setPowerLevel(31); // power output ranges from 0 (5dBm) to 31 (20dBm)    
  radio.encrypt(KEY);
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY == RF69_433MHZ ? 433 : FREQUENCY == RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
}
long lastPeriod = -1;

void loop()
{
  double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;  
  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
  printDateTime(gps.date, gps.time);  
  unsigned long distanceKmToLondon =
    (unsigned long)TinyGPSPlus::distanceBetween(
      gps.location.lat(),
      gps.location.lng(),
      LONDON_LAT,
      LONDON_LON) / 1000;
  printInt(distanceKmToLondon, gps.location.isValid(), 9);
 
  double courseToLondon =
    TinyGPSPlus::courseTo(
      gps.location.lat(),
      gps.location.lng(),
      LONDON_LAT,
      LONDON_LON);
  const char *cardinalToLondon = TinyGPSPlus::cardinal(courseToLondon);
 smartDelay(1000);
  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));

  float latitude = gps.location.lat();
  float longitude = gps.location.lng();
  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input >= 48 && input <= 57) //[0,9]
    {
      TRANSMITPERIOD = 100 * (input - 48);
      if (TRANSMITPERIOD == 0) TRANSMITPERIOD = 1000;

    }

    if (input == 'r') //d=dump register values
      radio.readAllRegs();
  }

  //check for any received packets
  if (radio.receiveDone())
  {
    Serial.print('[');
//    Serial.print(radio.SENDERID, DEC);
    Serial.print("] ");
    //for (byte i = 0; i < radio.DATALEN; i++)    //These lines were removed to prevent a compile error
    //  Serial.print((char)radio.DATA[i]);       // due to LCD library incompatibility with the RF library
    Serial.print("   [RX_RSSI:");
    Serial.print(radio.readRSSI());
    Serial.print("]");

    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.print(" - ACK sent");
      delay(10);
    }
    Serial.println();
  }

Serial.println("");
  int currPeriod = millis() / TRANSMITPERIOD;
  if (currPeriod != lastPeriod)
  {
    //fill in the struct with new values
    theData.LLat = latitude;
    theData.LLon = longitude;

    CCounter = ++CCounter  ;
    theData.SendCount = CCounter;
    if (radio.sendWithRetry(RECEIVER, (const void*)(&theData), sizeof(theData)))
    lastPeriod = currPeriod;
  }
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
 //     Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i = flen; i < len; ++i)
      Serial.print(' ');
  }
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i = strlen(sz); i < len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len - 1] = ' ';
  Serial.print(sz);
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    Serial.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    Serial.print(sz);
  }

  if (!t.isValid())
  {
    Serial.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    Serial.print(sz);
  }

//  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i = 0; i < len; ++i)
//    Serial.print(i < slen ? str[i] : ' ');
  smartDelay(0);
}


 
/* PARENT
 * void gps_to_float()
 * void get_gps_parent() // get gps data from parent and child
 * void get_gps_child() // get gps data from parent and child
 * void rf_send_receive() //sending and receiving gps data
 * void distance_check() //check distance is true or false
 * void distance_alert()    
 *    if (distance_check == true)//buzzer off
 *    if {distance_check == false)//buzzer on, send text alert to phone 
 * void ble_send() //alert to phone
 *    if {distance_check == true) // send text to phone, "child located"
 *    if {distance_check == false) //send text to phone, "child missing"
 * CHILD   
 * void gps_to_float()
 * void get_gps()// get gps data from parent and child
 * void rf_send_receive()//sending and receiving gps data
 * void distance_check()//check distance is true or false
 * void distance_alert()    
 *    if (distance_check == true)//buzzer off
 *    if {distance_check == false)//buzzer on, send text alert to phone 
 * void gsm_listen()
 *    if //distance_check false and rf_send_receive false, send text gsm, else if distance_check is true
 *    and rf_send_receive is true, use rf to send text to parent unit
 * 
 */
