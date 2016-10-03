#include <SoftwareSerial.h> //use digital pin 18 for button
#include <TinyGPS++.h>
#include <RFM69.h>
#include <SPI.h>
#define NETWORKID     100  // The same on all nodes that talk to each other
#define NODEID        1    // The unique identifier of this node
#define FREQUENCY     RF69_433MHZ
//#define ENCRYPT       true // Set to "true" to use encryption
#define KEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HCW   true // set to 'true' if you are using an RFM69HCW module
#define RFM69_CS      10
#define RFM69_IRQ     2
#define RFM69_IRQN    0  // Pin 2 is IRQ 0!
#define RFM69_RST     9 
#define LED           SDA  // onboard blinky/speaker
#define IS_RFM69HCW   true // set to 'true' if you are using an RFM69HCW module

char* text;
char* number;
bool error; //to catch the response of sendSms

int16_t packetnum = 0;  // packet counter, we increment per xmission
RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);
bool promiscuousMode = false; //set to 'true' to sniff all packets on the same network
typedef struct {
  int           nodeId; //store this nodeId
  unsigned long uptime; //uptime in ms
  float LLat;
  float LLon;
  int SendCount;}
Payload;
Payload theData;
long AvgAltitude = 0;
int Readings = 0;
int MINhdop = 999;
TinyGPSPlus gps; // The TinyGPS++ object
SoftwareSerial ss(4, 3);// The serial connection to the GPS device
float child_latitude = 0.000000;
float child_longitude = 0.000000;
float child_data_lat = 0.000000;
float child_data_lng = 0.000000;
byte ackCount = 0;
int count_down_1 = 30;
char alert_text_1[] = "Heads up, child unit has wandered, starting countdown for alert...";
char alert_text_2[] = "Countdown complete. Most recent location \t";
char alert_reset_found[] ="Child located, resetting to default alerts ... ready to use.";
int alerts = 0;
float radius_1 = 0.0000;
float the_lat_data = theData.LLat;
float the_lon_data = theData.LLon;

void setup() {
  Serial.begin(115200);
  ss.begin(9600);  
  // Hard Reset the RFM module  
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  radio.setHighPower();    // Only for RFM69HCW & HW!  
  radio.setPowerLevel(31); // power output ranges from 0 (5dBm) to 31 (20dBm)      
  radio.encrypt(KEY);
  radio.promiscuous(promiscuousMode);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY == RF69_433MHZ ? 433 : FREQUENCY == RF69_868MHZ ? 868 : 915);
  Serial.println(buff); 
}

void loop() {
  smartDelay(1000); 
  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
  Serial.println(""),  print_distance(),  Serial.println("");
  distance_check();            
  rf_radio ():
}

static void smartDelay(unsigned long ms){
  unsigned long start = millis();
  do  {
    while (ss.available())
      gps.encode(ss.read());    
  } while (millis() - start < ms);
  }

static void printFloat(float val, bool valid, int len, int prec){
  if (!valid)  {
    while (len-- > 1)
    Serial.print(' ');  }
  else  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i = flen; i < len; ++i)
      Serial.print(' ');  }
  smartDelay(0);}

static void printInt(unsigned long val, bool valid, int len){
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i = strlen(sz); i < len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len - 1] = ' ';
  Serial.print(sz);
  smartDelay(0);}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t){
  if (!d.isValid())  {
    Serial.print(F("********** "));  }
  else  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    Serial.print(sz);  }
  if (!t.isValid())  {
    Serial.print(F("******** "));  }
  else  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    Serial.print(sz);  }
  smartDelay(0);}

static void printStr(const char *str, int len){
  int slen = strlen(str);
  for (int i = 0; i < len; ++i)
  smartDelay(0);}
  
void print_distance(){
  child_data_lat = theData.LLat;
  child_data_lng = theData.LLon;  
  Serial.print("  Incoming Lat =  "),  Serial.print(theData.LLat, 6);       
  Serial.print("  Incoming Lon =  "),  Serial.print(theData.LLon, 6);                     
  Serial.println("");   
  unsigned long distanceMToParent=(unsigned long)TinyGPSPlus::distanceBetween(
  gps.location.lat(),
  gps.location.lng(),   
  child_data_lat, //for lab testing remove .5 when field testing.
  child_data_lng); //divide by 1000 is kilometers
  printInt((distanceMToParent), gps.location.isValid(), 6); //3.28084 feet per meter
  Serial.println("");    
  radius_1 = distanceMToParent;
}   

void timer_check(){ //default count down is 30 sec after first alert sent.
  for (count_down_1; count_down_1 <= 30; count_down_1--){
    Serial.println(count_down_1);
    if (count_down_1 < 1){
      alerts = 2;
      count_down_1 = 30;
    }
     else if (count_down_1 <10){
       break;
  }
    delay(1000);     
  }   
}

void distance_check(){ 
  for (alerts=1; alerts < 4; alerts++){      
      Serial.println(alerts);
      switch_alerts(); 
      if (alerts > 3){
        alerts = 1;
        }
    delay(1000);                  
    }  
}   

void switch_alerts(){
  switch (alerts) {
    case 1:   
      Serial.println(alert_text_1);  
        if (radius_1 > 60){ //unit of measure is in feet
          timer_check();  }       
      break;
    case 2:
      digitalWrite(LED, HIGH);      
      Serial.println(alert_text_2);
      Serial.print("  Incoming Lat =\t"),  Serial.print(theData.LLat, 6);       
      Serial.print("  Incoming Lon =\t"),  Serial.print(theData.LLon, 6);
      Serial.println(radius_1);
      break;
    case 3:
      digitalWrite(LED, LOW);
      Serial.println(alert_reset_found);
      radius_1 = -33;
        if (radius_1 < 60){
          count_down_1 = 30;      
        }
      break;
//    default:     
  }   }

void use_rf_distance (){ //default use when GPS has no signal, estimates distance with rf
  if (ss.available() < 0){
    rf_distance_check();
  }
}

void gsm_backup(){ //default use when rf is out of range. sends text to phone from gsm module.
  if (Serial.available() < 0){
    send_sms_gsm ();
    Serial.println("Sending Via GSM");
  }  
}

void send_sms_gsm (){

}

void rf_distance_check(){
  
}

void rf_radio (){
    if (Serial.available() > 0)  {    
    char input = Serial.read();
    if (input == 'r') //d=dump all register values
      radio.readAllRegs();
    if (input == 'E') //E=enable encryption
      radio.encrypt(KEY);
    if (input == 'e') //e=disable encryption
      radio.encrypt(null);
    if (input == 'p')    {
      promiscuousMode = !promiscuousMode;
      radio.promiscuous(promiscuousMode);
      Serial.print("Promiscuous mode ");
      Serial.println(promiscuousMode ? "on" : "off");
    }  }
  if (radio.receiveDone())  {      
    if (promiscuousMode)  {
      Serial.print("to [");
      Serial.print("] ");  }
    if (radio.DATALEN != sizeof(Payload))
      Serial.print("Invalid payload received, not matching Payload struct!");
    else    {
      theData = *(Payload*)radio.DATA; //assume radio.DATA actually contains our struct not something else
      Readings = ++Readings;    }     
    if (radio.ACKRequested())    {
      byte theNodeID = radio.SENDERID;
      radio.sendACK();    }
    Serial.println();
    } 
}

/* PARENT


 *    
 * CHILD   
 * void gps_to_float()
 * void get_gps()// get gps data from parent and child
 * void rf_send_receive()//sending and receiving gps data
 * void distance_check()//check distance is true or false
 * void distance_alert()    
 *    if (distance_check == true)//buzzer off
 *    if {distance_check == false)//buzzer on, send text alert to phone 
 * void gsm_backup()
 *    if //distance_check false and rf_send_receive false, send text gsm, else if distance_check is true
 *    and rf_send_receive is true, use rf to send text to parent unit
 * 
 */
