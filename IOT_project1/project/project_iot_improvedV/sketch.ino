#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>
#include <stdbool.h>

//sensor pins
#define  sensorData_p 25
#define  sensorInput_p 26

//constants
#define ss 0.034
#define limitDistance 50  //in cm
#define transmissionPower WIFI_POWER_2dBm
#define dutyCyclePeriod 75%50 + 5   //wake up time from deep sleep state (in seconds)
#define uS_TO_S_FACTOR 1000000
const uint8_t broadcastAddress[] = {0x8C, 0xAA, 0xB5, 0x84, 0xFB, 0x90};

//custom types
#define GENERATE_STRING(STRING) #STRING,
#define FOREACH_PARKING(PARK) \
        PARK(FREE)   \
        PARK(OCCUPIED)  \

enum ParkingSpotState{
  FREE = 0,
  OCCUPIED = 1
};
static const char *ParkingSpotStat_STRING[] = {
    FOREACH_PARKING(GENERATE_STRING) 
};

//global variables
esp_now_peer_info_t peerInfo;
ParkingSpotState state;
Preferences p;

// prototypes
void wifiSetup();  //net
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status); 
void onDataRecv(const uint8_t *mac_addr, const uint8_t * data, int len);
void sendBroadcastMessasge(String message);
ParkingSpotState sensorDataRead(int sensorInput, int sensorData); //sensors
void goToDeepSleep();//duty cycle management

void saveCurrentState(ParkingSpotState state); //NVS
ParkingSpotState getPreviousState();

void setup() {
  Serial.begin(115200);

  //pin setup
  pinMode(sensorData_p, INPUT);
  pinMode(sensorInput_p, OUTPUT);

  //setup NVS
  p.begin("test-1", false);
}

void loop() {
  //read data and send it through wifi

  ParkingSpotState state =  sensorDataRead(sensorInput_p, sensorData_p);
  ParkingSpotState oldState = getPreviousState();
  if(state!=oldState){
    sendBroadcastMessasge(ParkingSpotStat_STRING[state]);
    saveCurrentState(state);
  }

  //going to sleep
  goToDeepSleep();
}

ParkingSpotState sensorDataRead(int sensorInput, int sensorData){
  digitalWrite(sensorInput, LOW);   //for safety reasons
  delayMicroseconds(2);

  //start sensors reading 
  digitalWrite(sensorInput, HIGH);
  delayMicroseconds(10);
  digitalWrite(sensorInput, LOW);

  //read 
  int read = (pulseIn(sensorData, HIGH)*0.034/ 2)<limitDistance;
  state = (ParkingSpotState)read;

  return state;
}

void wifiSetup(){
  //wifi setup
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}


void sendBroadcastMessasge(String message){
  wifiSetup();

  WiFi.setTxPower(transmissionPower);
  esp_now_send(broadcastAddress, (uint8_t*)message.c_str(), message.length()+1);
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
   if(status!=0)Serial.println("network error");
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t * data, int len){
  char message[len];
  memcpy(message, data, len);
  Serial.println(String(message));
}

void goToDeepSleep(){
  delayMicroseconds(2000);  //for safety reasons
  WiFi.mode(WIFI_OFF);
  esp_sleep_enable_timer_wakeup(dutyCyclePeriod * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void saveCurrentState(ParkingSpotState state){
  bool parkingNonVolatileState = (state==OCCUPIED);
  p.putBool("parkingState", parkingNonVolatileState);
}

ParkingSpotState getPreviousState(){
  bool parkingNonVolatileState = p.getBool("parkingState", false);
  ParkingSpotState state = (ParkingSpotState) int(parkingNonVolatileState);
  return state;
}