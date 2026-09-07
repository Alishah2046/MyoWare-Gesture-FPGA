#include <BLEDevice.h>        // Core library: turns the ESP32 into a Bluetooth Low Energy device
#include <BLEServer.h>        // Lets the ESP32 act as a BLE "server" that other devices connect to
#include <BLEUtils.h>         // Helper utilities used internally by the BLE libraries
#include <BLE2902.h>          // Required descriptor that enables "notify" (push updates) over BLE

// Unique ID for this sensor's BLE "service". Ends in "c" — this is what makes it Shield 2, not Shield 1.
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914c"
// Same characteristic ID as Shield 1 — this part never changes between shields.
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;  // Object used to send data over BLE
bool deviceConnected = false;        // Tracks connection state

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; }
  void onDisconnect(BLEServer* pServer) { deviceConnected = false; BLEDevice::startAdvertising(); }
};

void setup() {
  Serial.begin(115200);        // start USB serial for debugging
  delay(2000);                 // give Serial Monitor time to open
  pinMode(A3, INPUT);          // A3 is where this sensor's signal wire connects

  BLEDevice::init("MyoWareSensor2");        // this board's BLE name — "Sensor2", not "Sensor1"
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("Setup complete. BLE advertising started.");
}

void loop() {
  if (deviceConnected) {
    String buffer = "";
    for (int i = 0; i < 10; i++) {     // same 10-samples-per-packet approach as Shield 1
      int value = analogRead(A3);
      buffer += String(value);
      if (i < 9) buffer += ",";
      delay(5);
    }
    pCharacteristic->setValue(buffer.c_str());
    pCharacteristic->notify();
  } else {
    delay(500);
  }
}
