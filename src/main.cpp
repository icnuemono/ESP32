
// Remote Cooler and Freezer temperature monitoring device 
// Leslie T Rose
// ESP32 and DS18B20 temperature sensor
//git test

#include <Arduino.h>

//BLE libraries
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEClient.h>
#include <BLE2902.h>

//Dallas Onewire Libraries
#include <OneWire.h>
#include <DallasTemperature.h>

// pre-declaration of functions
void getReadings();

// Define deep sleep options
uint64_t uS_TO_S_FACTOR = 1000000;
// Sleep for 2 min = 120 seconds  ( this will eventually be 2-4 hours)
uint64_t TIME_TO_SLEEP = 300;

// GATT Service and Descriptor UUID (https://www.bluetooth.com/specifications/gatt/services/ )  
// Define the Temperature service, Characteristic and Descriptor 
#define TemperatureService BLEUUID((uint16_t)0x181A)
BLECharacteristic TemperatureCharacteristic(BLEUUID((uint16_t)0x2A6E), BLECharacteristic::PROPERTY_READ | 
BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor TemperatureDescriptor(BLEUUID((uint16_t)0x290C));

// Setup OneWire (Connected data pin 13)
#define ONE_WIRE_BUS 13
// Create instance 
OneWire oneWire(ONE_WIRE_BUS);
// Pass oneWire reference to Dallas Temperature Sensor
DallasTemperature sensors(&oneWire);
//create float for temperature
// float temperature;    

// setup callback for server
bool _BLEClientConnected = false;
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    _BLEClientConnected = true;
  };
  void onDisconnect(BLEServer* pServer){
    _BLEClientConnected = false;
  }
};

uint8_t temperature = 32;
void InitBLE(){

  BLEDevice::init("Temperature Monitor");

  // Create the server 
  BLEServer *pServer=BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLEService
  BLEService *pTemperature = pServer->createService(TemperatureService);
 
  // Link the descriptor with characteristics, and set the values
  pTemperature->addCharacteristic(&TemperatureCharacteristic);
  TemperatureDescriptor.setValue("Temperature(F)");
  TemperatureCharacteristic.addDescriptor(&TemperatureDescriptor);
  TemperatureCharacteristic.addDescriptor(new BLE2902());

  // Advertise the Service
  pServer->getAdvertising()->addServiceUUID(TemperatureService);
  pTemperature->start();

  // Start Advertising
  pServer->getAdvertising()->start();
  Serial.println("BLE initialized");
  TemperatureCharacteristic.setValue(&temperature,1);
  
}

void setup() {

  // Initialize Serial communication
  Serial.begin(115200);
  Serial.println("Starting BLE!");

  Serial.println("Getting Temperature");
  sensors.begin();
  delay(1000); //Give sensor time to initialize 
  getReadings(); //Use dallastemperature to grab readings 

  // Enable wake up TIMER
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  InitBLE(); // Initialize BLE device and service 
  
  // Ifinite loop to allow client to connect and grab data before it goes back to sleep.
  int i = 0;
  unsigned long time = millis();
  while(1) {
    time = millis()-time;
    while (i<1){
      Serial.println("Waiting for BLE Client");
      i++;
    }
    if (_BLEClientConnected == true) {
      Serial.println("Client connected");
      TemperatureCharacteristic.notify(); 
      delay(20000); // wait 20 seconds to allow time to manually read characteristic
      _BLEClientConnected = false;
      break;
    }
    if (time>60000){
      break;
    }
  }
  // Put back into deep sleep
  Serial.println("Going back to sleep.");
  esp_bt_controller_disable(); // Gracefully shutdown BT
  esp_deep_sleep_start();
}

void loop() {
  // ESP32 should go to deep sleep mode between readings
  // it should never reach the loop() function
}

// Function to get temperature reading from onewiresensor 
float tempF;
void getReadings(){
  sensors.requestTemperatures(); // Request temperature readings from all sensors
  temperature = sensors.getTempC(0); // Get temperature in Celsius
  tempF = sensors.getTempF(0);       // Get temperature in Farenheight
  Serial.print("Temperature (C): ");
  Serial.println(temperature);
  Serial.print("Temperature (F): ");
  Serial.println(tempF); 
}
