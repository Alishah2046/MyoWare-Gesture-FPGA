#include <BLEDevice.h>        // Core library: turns the ESP32 into a Bluetooth Low Energy device
#include <BLEServer.h>        // Lets the ESP32 act as a BLE "server" that other devices connect to
#include <BLEUtils.h>         // Helper utilities used internally by the BLE libraries
#include <BLE2902.h>          // Required descriptor that enables "notify" (push updates) over BLE

// Unique ID for this sensor's BLE "service" — like a channel number. Ends in "b" for Shield 1.
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// Unique ID for the specific data stream inside that service. Same on both shields, always.
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;  // Will hold the object we use to send data over BLE
bool deviceConnected = false;        // Tracks whether the FPGA is currently connected to this board

// This class defines what happens when a device connects/disconnects
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; }   // mark as connected
  void onDisconnect(BLEServer* pServer) { deviceConnected = false; BLEDevice::startAdvertising(); }
  // when disconnected: mark as disconnected, then start broadcasting again so it can reconnect
};

void setup() {
  Serial.begin(115200);        // start USB serial communication at 115200 baud (for debugging)
  delay(2000);                 // wait 2 seconds so Serial Monitor has time to connect before we print
  pinMode(A3, INPUT);          // configure pin A3 (where the muscle sensor is wired) as an input

  BLEDevice::init("MyoWareSensor1");        // start BLE and give this device the name "MyoWareSensor1"
  BLEServer *pServer = BLEDevice::createServer();   // create the BLE server object
  pServer->setCallbacks(new MyServerCallbacks());   // attach our connect/disconnect behavior above

  BLEService *pService = pServer->createService(SERVICE_UUID);  // create the BLE service with our UUID
  pCharacteristic = pService->createCharacteristic(              // create the data channel
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |          // allow other devices to read it
                      BLECharacteristic::PROPERTY_NOTIFY          // allow it to push updates automatically
                    );
  pCharacteristic->addDescriptor(new BLE2902());   // required for notifications to actually work
  pService->start();                                // activate the service

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();  // get the advertising (broadcasting) object
  pAdvertising->addServiceUUID(SERVICE_UUID);                  // include our service ID in the broadcast
  pAdvertising->start();                                       // start broadcasting so the FPGA can find us

  Serial.println("Setup complete. BLE advertising started.");  // print confirmation to Serial Monitor
}

void loop() {
  if (deviceConnected) {              // only do this if the FPGA is actively connected
    String buffer = "";               // start with an empty text string to build up our data packet
    for (int i = 0; i < 10; i++) {    // collect 10 readings before sending, to reduce BLE traffic
      int value = analogRead(A3);     // read the current muscle sensor voltage (0-4095 range)
      buffer += String(value);        // add this number to our text string
      if (i < 9) buffer += ",";       // add a comma between numbers (but not after the last one)
      delay(5);                        // wait 5 milliseconds, giving ~200 readings per second overall
    }
    pCharacteristic->setValue(buffer.c_str());  // load our 10-number string into the BLE characteristic
    pCharacteristic->notify();                   // push it out to the connected FPGA immediately
  } else {
    delay(500);   // if nobody's connected, just wait half a second before checking again (saves power)
  }
}
