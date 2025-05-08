/* ICM-20948 Libraries */
#include <SPI.h>
#include <ICM20948_WE.h>

/* Bluetooth Libraries */
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h> // Required for notify

/* ICM-20948 Definitions */
#define CS_PIN 10   // Chip Select Pin

/* Bluetooth Definitions */
// UUIDs for BLE
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// GPIO pin for LEDs
const int veloLed = 2;
const int angleLed = 1;
const int statusLed = 3;

// button for starting/stopping recording
const int buttonPin = 4;

// vibration motor pin
const int motorPin = 5;

/* User-Made Structs */
struct VeloAndAccel {
  xyzFloat velocity;
  xyzFloat correctedAccel;
};

/* ICM-20948 Global Variables */
bool spi = true;
ICM20948_WE myIMU = ICM20948_WE(&SPI, CS_PIN, spi);

/* Bluetooth Global Variables */

BLECharacteristic *pCharacteristic; // BLE characteristic for communication with the iOS app
bool deviceConnected = false; // Tracks if a BLE central device is connected
String incomingValue = ""; // Stores last received command string from the iOS app

/* User-Made Global Variables */
// You may want to keep this global or static if calling in a loop
unsigned long lastUpdateTime = 0;
xyzFloat velocity = {0.0, 0.0, 0.0};
xyzFloat startAngle = {0.0, 0.0, 0.0};
int recordingCounter = 0;
int veloCounter = 0;
int angleCounter = 0;
float veloThreshold = 2.0;
float angleThreshold = 25.0;
float angleX = 0, angleY = 0, angleZ = 0;  // Angles in degrees
unsigned long gyr_lastTime = 0;
unsigned long graphStart = 0;
float AangleX = 0, AangleY = 0, AangleZ = 0;
float CangleX = 0, CangleY = 0, CangleZ = 0;
float alpha = 0.98;

// flags
bool receivedData = false;
bool goOver = false;
bool veloAlert = false;
bool angleAlert = false;

/* Bluetooth Helper Functions */

// Utility: Check if string contains an integer
bool isInteger(String str) {
  str.trim();
  if (str.length() == 0) return false;
  for (uint8_t i = 0; i < str.length(); i++) {
    if (!isDigit(str[i])) return false;
  }
  return true;
}

// Reset everything
void resetValues() {
  // reset veloThreshold and angleThreshold to defaults
  recordingCounter = 0;
  veloCounter = 0;
  angleCounter = 0;
  veloThreshold = 2.0;
  angleThreshold = 25.0;
  receivedData = false;
  goOver = false;
  veloAlert = false;
  angleAlert = false;
}

// BLE Server Callback
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("Device Connected!");
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("Device Disconnected!");
    deviceConnected = false;
    pServer->startAdvertising();
    resetValues();
  }
};

// BLE Characteristic Write Callback
class ESP32Callbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    if (!receivedData)
    {
      receivedData = true;
    }
    String value = pCharacteristic->getValue().c_str();
    value.trim();
    incomingValue = value;

    Serial.print("Received: ");
    Serial.println(incomingValue);

    if (incomingValue == "0")
    {
      goOver = false;
      Serial.println("Go Under Threshold");
    }
    else if (incomingValue == "1")
    {
      goOver = true;
      Serial.println("Go Over Threshold");
    }
    else
    {
      // splice string into exercise and velocity
      int spaceIndex = incomingValue.indexOf(' ');
      String exerciseInput = incomingValue.substring(0, spaceIndex);
      String velocityInput = incomingValue.substring(spaceIndex + 1);
      exerciseInput.trim();
      velocityInput.trim();

      veloThreshold = velocityInput.toFloat();
    }
  }
};

void sendMessage(String message) {
  pCharacteristic->setValue(message.c_str());
  pCharacteristic->notify();
  Serial.println("Sent to app: " + message);
}

void sendStatusByte(uint8_t status) {
  pCharacteristic->setValue(&status, 1);
  pCharacteristic->notify();
}

/* User-Made Helper Functions */

xyzFloat removeGravity(xyzFloat accel, float x_angle, float y_angle)
{
  // assume pitch and roll are in degrees; convert to radians
  float pitchRad = y_angle * PI / 180.0;
  float rollRad = x_angle * PI / 180.0;

  // gravity in sensor frame
  float gravity = .99;

  // components of gravity in sensor frame
  xyzFloat gravityComp;
  gravityComp.x = gravity * sin(rollRad);
  gravityComp.y = gravity * sin(pitchRad) * cos(rollRad);
  gravityComp.z = gravity * cos(pitchRad) * cos(rollRad);
  if(accel.z < 0) gravityComp.z = gravityComp.z*-1;

  // subtract gravity component from measured acceleration
  xyzFloat linearAccel;
  linearAccel.x = accel.x + gravityComp.y;
  linearAccel.y = accel.y - gravityComp.x;
  linearAccel.z = accel.z - gravityComp.z;

  return linearAccel;
}

VeloAndAccel computeVelocity(xyzFloat acceleration, xyzFloat angle) {
  unsigned long currentTime = millis();
  float dt = (currentTime - lastUpdateTime) / 1000.0; // Convert ms to s
  lastUpdateTime = currentTime;

  // Deadband to reduce noise-induced drift
  const float accelDeadband = 0.06;//2* 0.03;
  // xyzFloat angle;
  // myIMU.getAngles(&angle);
  xyzFloat correctedAccel = removeGravity(acceleration, angle.x, angle.y);

  // Apply deadband
  if (abs(correctedAccel.x) < accelDeadband) correctedAccel.x = 0;
  if (abs(correctedAccel.y) < accelDeadband) correctedAccel.y = 0;
  if (abs(correctedAccel.z) < accelDeadband) correctedAccel.z = 0;

  // Integrate acceleration to get velocity
  //Serial.println(dt);
  velocity.x += correctedAccel.x * dt *9.81;
  //Serial.println(velocity.x);
  velocity.y += correctedAccel.y * dt * 9.81;
  velocity.z += correctedAccel.z * dt * 9.81;

  VeloAndAccel result;
  result.velocity = velocity;
  result.correctedAccel = correctedAccel;
  return result;
}

void setup()
{
  /* ICM-20948 setup */

  //delay(2000); // maybe needed for some MCUs, in particular for startup after power off 
  Serial.begin(115200);
  while(!Serial) {}
  
  if(!myIMU.init())
  {
    Serial.println("ICM20948 does not respond");
  }
  else
  {
    Serial.println("ICM20948 is connected");
  }

  if (!myIMU.initMagnetometer())
  {
    Serial.println("Magnetometer does not respond");
  }
  else
  {
    Serial.println("Magnetometer is connected");
  }
  
  /******************* Basic Settings ******************/

  /* You can set the SPI clock speed. The default is 7 MHz, which is the maximum. */ 
  myIMU.setSPIClockSpeed(7000000);
 
  /*  This is a method to calibrate. You have to determine the minimum and maximum 
   *  raw acceleration values of the axes determined in the range +/- 2 g. 
   *  You call the function as follows: setAccOffsets(xMin,xMax,yMin,yMax,zMin,zMax);
   *  The parameters are floats. 
   *  The calibration changes the slope / ratio of raw acceleration vs g. The zero point is 
   *  set as (min + max)/2.
   */
  //myIMU.setAccOffsets(-16330.0, 16450.0, -16600.0, 16180.0, -16520.0, 16690.0);
    
  /* The starting point, if you position the ICM20948 flat, is not necessarily 0g/0g/1g for x/y/z. 
   * The autoOffset function measures offset. It assumes your ICM20948 is positioned flat with its 
   * x,y-plane. The more you deviate from this, the less accurate will be your results.
   * It overwrites the zero points of setAccOffsets, but keeps the correction of the slope.
   * The function also measures the offset of the gyroscope data. The gyroscope offset does not   
   * depend on the positioning.
   * This function needs to be called after setAccOffsets but before other settings since it will 
   * overwrite settings!
   * You can query the offsets with the functions:
   * xyzFloat getAccOffsets() and xyzFloat getGyrOffsets()
   * You can apply the offsets using:
   * setAccOffsets(xyzFloat yourOffsets) and setGyrOffsets(xyzFloat yourOffsets)
   */
   Serial.println("Position your ICM20948 flat and don't move it - calibrating...");
   delay(1000);
   myIMU.autoOffsets();
   Serial.println("Done!"); 
  
  /*  The gyroscope data is not zero, even if you don't move the ICM20948. 
   *  To start at zero, you can apply offset values. These are the gyroscope raw values you obtain
   *  using the +/- 250 degrees/s range. 
   *  Use either autoOffset or setGyrOffsets, not both.
   */
  //myIMU.setGyrOffsets(-115.0, 130.0, 105.0);
  
  /*  ICM20948_ACC_RANGE_2G      2 g   (default)
   *  ICM20948_ACC_RANGE_4G      4 g
   *  ICM20948_ACC_RANGE_8G      8 g   
   *  ICM20948_ACC_RANGE_16G    16 g
   */
  myIMU.setAccRange(ICM20948_ACC_RANGE_2G);
  
  /*  Choose a level for the Digital Low Pass Filter or switch it off.  
   *  ICM20948_DLPF_0, ICM20948_DLPF_2, ...... ICM20948_DLPF_7, ICM20948_DLPF_OFF 
   *  
   *  IMPORTANT: This needs to be ICM20948_DLPF_7 if DLPF is used in cycle mode!
   *  
   *  DLPF       3dB Bandwidth [Hz]      Output Rate [Hz]
   *    0              246.0               1125/(1+ASRD) 
   *    1              246.0               1125/(1+ASRD)
   *    2              111.4               1125/(1+ASRD)
   *    3               50.4               1125/(1+ASRD)
   *    4               23.9               1125/(1+ASRD)
   *    5               11.5               1125/(1+ASRD)
   *    6                5.7               1125/(1+ASRD) 
   *    7              473.0               1125/(1+ASRD)
   *    OFF           1209.0               4500
   *    
   *    ASRD = Accelerometer Sample Rate Divider (0...4095)
   *    You achieve lowest noise using level 6  
   */
  myIMU.setAccDLPF(ICM20948_DLPF_6);    
  
  /*  Acceleration sample rate divider divides the output rate of the accelerometer.
   *  Sample rate = Basic sample rate / (1 + divider) 
   *  It can only be applied if the corresponding DLPF is not off!
   *  Divider is a number 0...4095 (different range compared to gyroscope)
   *  If sample rates are set for the accelerometer and the gyroscope, the gyroscope
   *  sample rate has priority.
   */
  myIMU.setAccSampleRateDivider(1);
  
  /*  ICM20948_GYRO_RANGE_250       250 degrees per second (default)
   *  ICM20948_GYRO_RANGE_500       500 degrees per second
   *  ICM20948_GYRO_RANGE_1000     1000 degrees per second
   *  ICM20948_GYRO_RANGE_2000     2000 degrees per second
   */
  //myIMU.setGyrRange(ICM20948_GYRO_RANGE_250);
  
  /*  Choose a level for the Digital Low Pass Filter or switch it off. 
   *  ICM20948_DLPF_0, ICM20948_DLPF_2, ...... ICM20948_DLPF_7, ICM20948_DLPF_OFF 
   *  
   *  DLPF       3dB Bandwidth [Hz]      Output Rate [Hz]
   *    0              196.6               1125/(1+GSRD) 
   *    1              151.8               1125/(1+GSRD)
   *    2              119.5               1125/(1+GSRD)
   *    3               51.2               1125/(1+GSRD)
   *    4               23.9               1125/(1+GSRD)
   *    5               11.6               1125/(1+GSRD)
   *    6                5.7               1125/(1+GSRD) 
   *    7              361.4               1125/(1+GSRD)
   *    OFF          12106.0               9000
   *    
   *    GSRD = Gyroscope Sample Rate Divider (0...255)
   *    You achieve lowest noise using level 6  
   */
  myIMU.setGyrDLPF(ICM20948_DLPF_6);  
  
  /*  Gyroscope sample rate divider divides the output rate of the gyroscope.
   *  Sample rate = Basic sample rate / (1 + divider) 
   *  It can only be applied if the corresponding DLPF is not OFF!
   *  Divider is a number 0...255
   *  If sample rates are set for the accelerometer and the gyroscope, the gyroscope
   *  sample rate has priority.
   */
  myIMU.setGyrSampleRateDivider(5);

  /*  Choose a level for the Digital Low Pass Filter. 
   *  ICM20948_DLPF_0, ICM20948_DLPF_2, ...... ICM20948_DLPF_7, ICM20948_DLPF_OFF 
   *  
   *  DLPF          Bandwidth [Hz]      Output Rate [Hz]
   *    0             7932.0                    9
   *    1              217.9                 1125
   *    2              123.5                 1125
   *    3               65.9                 1125
   *    4               34.1                 1125
   *    5               17.3                 1125
   *    6                8.8                 1125
   *    7             7932.0                    9
   *                 
   *    
   *    GSRD = Gyroscope Sample Rate Divider (0...255)
   *    You achieve lowest noise using level 6  
   */
  // myIMU.setTempDLPF(ICM20948_DLPF_6);
 
  /* You can set the following modes for the magnetometer:
   * AK09916_PWR_DOWN          Power down to save energy
   * AK09916_TRIGGER_MODE      Measurements on request, a measurement is triggered by 
   *                           calling setMagOpMode(AK09916_TRIGGER_MODE)
   * AK09916_CONT_MODE_10HZ    Continuous measurements, 10 Hz rate
   * AK09916_CONT_MODE_20HZ    Continuous measurements, 20 Hz rate
   * AK09916_CONT_MODE_50HZ    Continuous measurements, 50 Hz rate
   * AK09916_CONT_MODE_100HZ   Continuous measurements, 100 Hz rate (default)
   */
  myIMU.setMagOpMode(AK09916_CONT_MODE_20HZ);
  // delay(50); // add a delay of 1000/magRate to avoid first mag value being zero

  /* Bluetooth Setup */

  // Configure pins
  pinMode(veloLed, OUTPUT);
  pinMode(angleLed, OUTPUT);
  pinMode(statusLed, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(motorPin, OUTPUT);
  resetValues();

  // Initialize BLE
  BLEDevice::init("ATS Microcontroller");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902()); // Enables notifications
  pCharacteristic->setCallbacks(new ESP32Callbacks());
  pCharacteristic->setValue("ESP32 Control Service");

  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("BLE Initialized and Advertising...");
}

void loop()
{
  /* ICM-20948 Loop */

  xyzFloat gValue;
  xyzFloat gyr;
  xyzFloat gyrAngle;
  xyzFloat angle;
  recordingCounter = 0;
  veloCounter = 0;
  angleCounter = 0;
  float rep_active;
  float total_V;
  float max_V;
  digitalWrite(angleLed, LOW);
  digitalWrite(veloLed, LOW);
  digitalWrite(statusLed, LOW);
  digitalWrite(motorPin, LOW);
  resetValues();
  Serial.println("Waiting...");
  while(digitalRead(buttonPin) == HIGH || !receivedData)
  {
    recordingCounter += 1;
    if (recordingCounter >= 3000)
    {
      digitalWrite(statusLed, HIGH);
    }
    
    if (recordingCounter >= 5000)
    {
      digitalWrite(statusLed, LOW);
      recordingCounter = 0;
    }
  }
  // Serial.println("Waiting for 1 second setup...");
  while(digitalRead(buttonPin) == LOW){}
  digitalWrite(statusLed, LOW);
  Serial.println("Calibrating...");
  delay(6000);
  digitalWrite(motorPin, HIGH);
  delay(1000);
  sendMessage("Recording Started!");
  digitalWrite(motorPin, LOW);
  digitalWrite(statusLed, HIGH);

  //set up gyroscope offset
  angleX = 0;
  angleY = 0;
  angleZ = 0;
  gyr_lastTime = millis();
  graphStart = millis();

  while(digitalRead(buttonPin) == HIGH) {
    myIMU.readSensor();
    myIMU.getGValues(&gValue);
    myIMU.getGyrValues(&gyr);

    float currentTime = millis();
    float deltaTime = (currentTime - gyr_lastTime) / 1000.0;  // Seconds
    gyr_lastTime = currentTime;

    // Apply deadband to gyroscope raw data
    float gyrDeadband = 2;
    if (abs(gyr.x) < gyrDeadband) gyr.x = 0;
    if (abs(gyr.y) < gyrDeadband) gyr.y = 0;
    if (abs(gyr.z) < gyrDeadband) gyr.z = 0;

    // Integrate gyro readings to get angles
    angleX += gyr.x * deltaTime;
    angleY += gyr.y * deltaTime;
    angleZ += gyr.z * deltaTime;
    // Serial.print(deltaTime);
    // Serial.print("     ");
    //Check for dangerous angle.
        // turn on second LED based on angle
    if (abs(angleX) > angleThreshold)
    {
      if (!angleAlert)
      {
        angleCounter = 0;
        angleAlert = true;
      }
      digitalWrite(angleLed, HIGH);
      digitalWrite(motorPin, HIGH);
    }
    else
    {
      angleAlert = false;
      digitalWrite(angleLed, LOW);
      if (!veloAlert)
      {
        digitalWrite(motorPin, LOW);
      }
    }

    if (angleAlert)
    {
      angleCounter += 1;
      if (angleCounter >= 100)
      {
        digitalWrite(angleLed, LOW);
        digitalWrite(motorPin, LOW);
      }

      if (angleCounter >= 200)
      {
        digitalWrite(angleLed, HIGH);
        digitalWrite(motorPin, HIGH);
        angleCounter = 0;
      }
    }
    // else if (abs(angleX) <= angleThreshold && angleOn)
    // {
    //   digitalWrite(angleLed, LOW);
    //   digitalWrite(motorPin, LOW);
    //   angleOn = false;
    // }

    //myIMU.getMagValues(&magValue);
    // get angle using library
    myIMU.getAngles(&angle);
    // float temp = myIMU.getTemperature();
    // float resultantG = myIMU.getResultantG(&gValue);
    AangleX = angle.y;
    AangleY = -1*angle.x;
    AangleZ = angle.z;
    xyzFloat newAccAngle = {AangleX,AangleY,AangleZ};

    CangleX = alpha * angleX + (1.0 - alpha) * AangleX;
    CangleY = alpha * angleY + (1.0 - alpha) * AangleY;
    CangleZ = angleZ;

    gyrAngle.x = angleX;
    gyrAngle.y = angleY;
    gyrAngle.z = angleZ;

    // velocity vector
    VeloAndAccel result = computeVelocity(gValue, newAccAngle);
    xyzFloat vel = result.velocity;
    xyzFloat new_g = result.correctedAccel;

    //Analyze Velocity
    if(new_g.x + new_g.y + new_g.z != 0){
      if(rep_active == 0){ //if rep just started
        max_V = 0;
      }
      rep_active = 1;
    }
    else{  
      if(rep_active ==1){
        // Serial.print("Max Velocity: ");
        // Serial.println(max_V);
      }
      rep_active = 0;
    }

    if(rep_active == 0){  //if not in rep
      vel.x = 0;
      vel.y = 0;
      vel.z = 0;
      result.velocity.x = 0;
      result.velocity.y = 0;
      result.velocity.z = 0;
      velocity.x = 0;
      velocity.y = 0;
      velocity.z = 0;
    }
    else{
      if(total_V > max_V) max_V = total_V;

      // other spot for total_V
    }
    total_V = sqrt(vel.x*vel.x + vel.y*vel.y + vel.z*vel.z);

    // turn on green LED based on velocity
    if (abs(total_V) > veloThreshold)
    {
      veloCounter = 0;
      veloAlert = true;
      digitalWrite(veloLed, HIGH);
      digitalWrite(motorPin, HIGH);
    }

    if (veloAlert)
    {
      veloCounter += 1;
      if (veloCounter >= 200)
      {
        digitalWrite(veloLed, LOW);
        if (!angleAlert)
        {
          digitalWrite(motorPin, LOW);
        }
        veloAlert = false;
      }
    }

    // calculate time for graph and send velocity and angle in separate messages
    float graphTime = (millis() - graphStart) / 1000.0;
    sendMessage(String(graphTime, 3) + " " + String(total_V, 2) + " " + String(angleX, 2));

    delay(5);
  }
  while(digitalRead(buttonPin) == LOW){}
  delay(50);
  sendMessage("Recording Ending!");
  digitalWrite(statusLed, LOW);
}
