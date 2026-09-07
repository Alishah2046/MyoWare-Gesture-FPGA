#include <Dynamixel2Arduino.h>   // Library that lets us talk to the Dynamixel servo motor

#define DXL_SERIAL Serial1        // The internal serial port used to talk to the Dynamixel motor
#define DEBUG_SERIAL Serial       // The USB serial port — this is how the FPGA talks to us
const int DXL_DIR_PIN = -1;       // Not used on this board (only needed for certain wiring setups)
const uint8_t DXL_ID = 1;         // The servo's ID number on the bus (default is 1)
const float DXL_PROTOCOL_VERSION = 2.0;   // Dynamixel communication protocol version

Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);   // create the object we use to control the motor

void setup() {
  DEBUG_SERIAL.begin(115200);              // start USB serial at 115200 baud, matching the FPGA's speed
  dxl.begin(57600);                        // start communication with the Dynamixel motor itself
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);   // tell it which protocol version we're using
  dxl.ping(DXL_ID);                        // check that the motor with this ID actually responds
  dxl.torqueOff(DXL_ID);                   // torque must be off before changing settings below

  dxl.setOperatingMode(DXL_ID, OP_POSITION);   // set the motor to "go to a specific angle" mode

  // Controls rotation speed: lower number = slower/smoother, higher = faster. 0 = no limit.
  dxl.writeControlTableItem(ControlTableItem::PROFILE_VELOCITY, DXL_ID, 100);

  dxl.torqueOn(DXL_ID);                    // now turn torque back on so the motor can actually move
  DEBUG_SERIAL.println("Ready");           // tell whoever's watching Serial Monitor that setup finished
}

void loop() {
  if (DEBUG_SERIAL.available()) {                       // check if new data has arrived over USB
    String command = DEBUG_SERIAL.readStringUntil('\n'); // read everything up to the newline character
    command.trim();                                       // remove any extra spaces/line-ending characters
    DEBUG_SERIAL.println("Received: " + command);         // echo back what we received, for debugging

    if (command == "OPEN") {                              // if the FPGA told us to open the hand
      dxl.setGoalPosition(DXL_ID, 180.0, UNIT_DEGREE);     // rotate the servo to 180 degrees
      DEBUG_SERIAL.println("Moving to OPEN");              // confirm the action in Serial Monitor
    } else if (command == "0") {                          // if the FPGA told us to close the hand
      dxl.setGoalPosition(DXL_ID, 0.0, UNIT_DEGREE);       // rotate the servo to 0 degrees
      DEBUG_SERIAL.println("Moving to CLOSE");             // confirm the action
    }
  }
}
