/*******************************************************************************************************
**  Name: Zetty Elica Binti Affandi                                                                   **
**  Matric No.: 63879                                                                                 **
**  FYP Title: Smart Seed Germination System Based on Internet of Things (IOT) Technology             **
**  Supervisor: Associate Professor Dr. Noor Alamshah B. Bolhassan                                    **
********************************************************************************************************/

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <EEPROM.h>

#define BLYNK_PRINT Serial

// Define digital pin connected to the DHT sensor
#define DHTPIN 5

// Define DHT sensor type DHT 22  (AM2302), AM2321
#define DHTTYPE DHT22

// Define digital pin connected to the capacitive soil moisture sensor
#define MoistureSensor 34

// Define digital pin connected to each relay
// Connected to the fans
#define RELAY_PIN_1 13

 // Connected to the light
#define RELAY_PIN_2 33

// Connected to the heating mat
#define RELAY_PIN_3 32

 // Connected to the water pump
#define RELAY_PIN_4 25

const int AirValue = 2640;   //Maximum value when capacitive soil moisture sensor is hung in the air
const int WaterValue = 1400;  //Minimum value when capacitive soil moisture sensor is placed in the water

// Define virtual pin for displaying sensors value
#define VPIN_TEMPERATURE V6
#define VPIN_HUMIDITY    V7
#define VPIN_MOISTURE    V9
#define VPIN_LIGHT       V13

// Define virtual pin for every Blynk button
#define VPIN_BUTTON_FAN    V1
#define VPIN_BUTTON_LIGHT  V2
#define VPIN_BUTTON_MAT    V3
#define VPIN_BUTTON_PUMP   V4
#define VPIN_BUTTON_MODE   V5

// Define virtual pin for adjustable sliders
#define VPIN_SLIDER_MAXTEMP  V8
#define VPIN_SLIDER_MINTEMP  V12
#define VPIN_SLIDER_MAXHUM  V11
#define VPIN_SLIDER_DRYLEVEL V10
#define VPIN_SLIDER_LIGHTLEVEL V14
#define VPIN_SLIDER_PUMPTIME V15

// Define initial state of every power button
int BUTTON_STATE_FAN = 0;
int BUTTON_STATE_LIGHT = 0;
int BUTTON_STATE_MAT = 0;
int BUTTON_STATE_PUMP = 0;

// Define initial value for sensors, control mode and notification indicator
int moisture = 0;
int moistureCumulative = 0;
int moisturePercent = 0;
float lightLevel = 0;
float lightLevelCumulative = 0;
float temperature = 0;
float humidity = 0;
int switchMode = 0;
int notif_fan_flag = 0; //To ensure user has been notified to switch on fan
int notif_mat_flag = 0; //To ensure user has been notified to switch on heating mat
int notif_pump_flag = 0; //To ensure user has been notified to switch on water pump
int notif_autoPump_flag = 0; //To ensure user has been notified that water pump is switched on for automatic mode
int notif_light_flag = 0; //To ensure user has been notified to switch on LED light

//Set values for Auto Control Mode
float maxTemp = 30.5;
float minTemp = 30.0;
float maxHumidity = 85.0;
int minLight = 500;
int dryLevel = 10;
int pumpTime = 1;
int pumpFlag = 0; //For checking if water has just been pump

// Define address to store value on EEPROM
int EEAddr = 0;
int EEAddr_next_1 = EEAddr + sizeof(float);
int EEAddr_next_2 = EEAddr_next_1 + sizeof(int);
int EEAddr_next_3 = EEAddr_next_2 + sizeof(float);
int EEAddr_next_4 = EEAddr_next_3 + sizeof(float);
int EEAddr_next_5 = EEAddr_next_4 + sizeof(int);

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
BH1750 lightMeter;

// Go to the Project Settings (nut icon).
char auth[] = "Blynk auth"; // Get auth token from Blynk application

// WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "wifi name/ssid"; // Enter your wifi ssid
char pass[] = "wifi password"; // Enter your wifi password

unsigned long previousMillis = 0;
unsigned long interval = 30000;

// For control mode set to Auto
void autoMode()
{
        // Control operation of fan
        // If current temperature is more than the set maximum temperature or current humidity is more than the set humidity
        if(temperature > maxTemp || humidity > maxHumidity){
          // If current Blynk fan button state is 0 which means fan is OFF
          if(BUTTON_STATE_FAN == 0){
            BUTTON_STATE_FAN = 1; // Set Blynk fan button state value to 1 to switch ON the fan button
            digitalWrite(RELAY_PIN_1, LOW); // Switch ON relay 1 connected to the fan (LOW = ON for relay because it is active low)
          }
          // Update fan button state
          Blynk.virtualWrite(VPIN_BUTTON_FAN, BUTTON_STATE_FAN); // Switch ON the Blynk fan button
        }
        // If current temperature is less than or equal the set maximum temperature or current humidity is less than or equal the set humidity
         else if (temperature <= maxTemp || humidity <= maxHumidity){
          if(BUTTON_STATE_FAN == 1){
             BUTTON_STATE_FAN = 0; // Set button state value to 0 to switch OFF the fan button
             digitalWrite(RELAY_PIN_1, HIGH); // Switch OFF relay 1 connected to fan (HIGH = OFF for relay because it is active low)
         }
          // Update fan button state
          Blynk.virtualWrite(VPIN_BUTTON_FAN, BUTTON_STATE_FAN); // Switch OFF the Blynk fan button
        }

        // Control operation of heating mat
        // If current temperature is less than the set minimum temperature
        if (temperature < minTemp) {
          // If current Blynk mat button state is 0 which means mat is OFF
          if(BUTTON_STATE_MAT == 0){
            BUTTON_STATE_MAT = 1; // Set Blynk mat button state value to 1 to switch ON the mat button
            digitalWrite(RELAY_PIN_3, LOW); // Switch ON relay 1 connected to the mat (LOW = ON for relay because it is active low)
          }
          // Update mat button state
          Blynk.virtualWrite(VPIN_BUTTON_MAT, BUTTON_STATE_MAT); // Switch ON the Blynk mat button
          }
         // If current temperature is more than or equal the set minimum temperature
         else{
          if(BUTTON_STATE_MAT == 1){
             BUTTON_STATE_MAT = 0; // Set button state value to 0 to switch OFF the mat button
             digitalWrite(RELAY_PIN_3, HIGH); // Switch OFF relay 1 connected to mat (HIGH = OFF for relay because it is active low)
         }
          // Update mat button state
          Blynk.virtualWrite(VPIN_BUTTON_MAT, BUTTON_STATE_MAT); // Switch OFF the Blynk mat button
        }

        // Control operation of light
        // If current light intensity is less than the set light intensity
        if (lightLevel < minLight) {
          // If current Blynk light button state is 0 which means light is OFF
          if(BUTTON_STATE_LIGHT == 0){
            BUTTON_STATE_LIGHT = 1; // Set Blynk light button state value to 1 to switch ON the light button
            digitalWrite(RELAY_PIN_2, LOW); // Switch ON relay 1 connected to the light (LOW = ON for relay because it is active low)
          }
          // Update light button state
          Blynk.virtualWrite(VPIN_BUTTON_LIGHT, BUTTON_STATE_LIGHT); // Switch ON the Blynk light button
          }
         // If current light intensity is more than or equal the set light intensity
         else{
          if(BUTTON_STATE_LIGHT == 1){
            if ((lightLevel - 165) > minLight){// Check if there is sufficient amount of light once LED light is switched off (165lx is the maximum light supplied by the LED light)
               BUTTON_STATE_LIGHT = 0; // Set button state value to 0 to switch OFF the light button
               digitalWrite(RELAY_PIN_2, HIGH); // Switch OFF relay 1 connected to light (HIGH = OFF for relay because it is active low)
            }
         }
          // Update light button state
          Blynk.virtualWrite(VPIN_BUTTON_LIGHT, BUTTON_STATE_LIGHT);
        }

        // Control operation of water pump
        // If soil moisture is less than or equal the set moisture level
        if (moisturePercent <= dryLevel) {
          // After water has just been pump, wait for soil to absorb water.
          if (pumpFlag > 0 && pumpFlag <= 10){
             BUTTON_STATE_PUMP = 0;
            digitalWrite(RELAY_PIN_4, HIGH); // Switch OFF relay 1 connected to pump (HIGH = OFF for relay because it is active low)
            Blynk.virtualWrite(VPIN_BUTTON_PUMP, BUTTON_STATE_PUMP); // Switch ON the Blynk pump button
            pumpFlag += 1;
            }
           // If water haven't been pump OR after water has been  pumped and mositure still hasn't increase
           else if (pumpFlag == 0 || pumpFlag > 10){
            // If current Blynk pump button state is 0 which means pump is OFF
              if(BUTTON_STATE_PUMP == 0){
                BUTTON_STATE_PUMP = 1; // Set Blynk light button state value to 1 to switch ON the pump button
                digitalWrite(RELAY_PIN_4, LOW); // Switch ON relay 1 connected to the pump (LOW = ON for relay because it is active low)
              }

              // Update pump button state
              Blynk.virtualWrite(VPIN_BUTTON_PUMP, BUTTON_STATE_PUMP); // Switch ON the Blynk pump button
              if(pumpFlag == 0){
                pumpFlag += 1;
              }
              else if (pumpFlag > 10){
                pumpFlag = 0;
                notif_autoPump_flag = 0;
              }
              // keep watering according duration set
              int delayDuration = pumpTime*1000;
              delay(delayDuration);

              // turn off water pump
              BUTTON_STATE_PUMP = 0;
              digitalWrite(RELAY_PIN_4, HIGH); // Switch OFF relay 1 connected to pump (HIGH = OFF for relay because it is active low)
              Blynk.virtualWrite(VPIN_BUTTON_PUMP, BUTTON_STATE_PUMP); // Switch ON the Blynk pump button
            }
          }
         // If current soil moisture is more than the set moisture level
         else{
          if(BUTTON_STATE_PUMP == 1){
             BUTTON_STATE_PUMP = 0; // Set button state value to 0 to switch OFF the pump button
             digitalWrite(RELAY_PIN_4, HIGH); // Switch OFF relay 1 connected to pump (HIGH = OFF for relay because it is active low)
         }
          // Update pump button state
          Blynk.virtualWrite(VPIN_BUTTON_PUMP, BUTTON_STATE_PUMP);
        }
}

// To get value from all sensors
void readSensors()
{
    // Get value of current Humidity for DHT22
    humidity = dht.readHumidity();

    // Get value of current Temperature for DHT22
    temperature = dht.readTemperature();

    for (int i=0; i<5; i++){
      // Get cumulative value of current Soil Mositure from the capacitive soil moisture sensor
      moistureCumulative += analogRead(MoistureSensor);

      // Get cumulative value of current Light level from BH1750
      lightLevelCumulative += lightMeter.readLightLevel();
    }

    // Calculate average value from 5 reading of current soil moisture
    moisture = moistureCumulative/5;
    moistureCumulative = 0;
    moisturePercent = map(moisture, AirValue, WaterValue, 0, 100);
    if (moisturePercent > 100){
      moisturePercent = 100;
    }
    else if (moisturePercent < 0){
      moisturePercent = 0;
    }
    else {
      moisturePercent = moisturePercent;
    }

    // Calculate average value from 5 reading of current light level
    lightLevel = lightLevelCumulative/5;
    lightLevelCumulative = 0;

    //To get values once every 5 seconds
    delay(5000);
}

// To send data to Blynk app
void sendData(){
  readSensors();

  // Send the current set dry level, maximum temperature, minimum temperature, maximum humidity and minimum light level to the respective sliders
  Blynk.virtualWrite(VPIN_SLIDER_DRYLEVEL, dryLevel);
  Blynk.virtualWrite(VPIN_SLIDER_MAXTEMP, maxTemp);
  Blynk.virtualWrite(VPIN_SLIDER_MINTEMP, minTemp);
  Blynk.virtualWrite(VPIN_SLIDER_MAXHUM, maxHumidity);
  Blynk.virtualWrite(VPIN_SLIDER_LIGHTLEVEL, minLight);

  // Send the current value of the temperature, humidity, soil moisture level and light level to be displayed
  Blynk.virtualWrite(VPIN_TEMPERATURE, temperature);
  Blynk.virtualWrite(VPIN_HUMIDITY, humidity);
  Blynk.virtualWrite(VPIN_MOISTURE, moisturePercent);
  Blynk.virtualWrite(VPIN_LIGHT, lightLevel);

  // If fan is not switched on and control mode is manual, notify user and suggest to switch the fan on
  if(temperature > maxTemp && BUTTON_STATE_FAN == 0 && switchMode == 0){
    if(notif_fan_flag == 0){
      Blynk.notify("Alert: Temperature exceeded maximum temperature. Switch on the fans");
      notif_fan_flag = 1;
    }
  }
  // If fan is not switched on and control mode is automatic, notify user that fan will be on
  else if(temperature > maxTemp && BUTTON_STATE_FAN == 0 && switchMode == 1){
    Blynk.notify("Alert: Temperature exceeded maximum temperature. Fans will be switched on.");
  }

  // If fan is not switched on and control mode is manual, notify user and suggest to switch the fan on
  if(humidity > maxHumidity && BUTTON_STATE_FAN == 0 && switchMode == 0){
    if(notif_fan_flag == 0){
      Blynk.notify("Alert: Humidity exceeded maximum humidity. Switch on the Fans");
      notif_fan_flag = 1;
    }
  }
  // If fan is not switched on and control mode is automatic, notify user that fan will be switched on
  else if(humidity > maxHumidity && BUTTON_STATE_FAN == 0 && switchMode == 1){
    Blynk.notify("Alert: Humidity exceeded maximum temperature. Fans will be switched on.");
  }

  // If heating mat is not switched on and control mode is manual, notify user and suggest to switch the mat on
  if(temperature < minTemp && BUTTON_STATE_MAT == 0 && switchMode == 0){
    if(notif_mat_flag == 0){
      Blynk.notify("Alert: Temperature is lower then minimum temperature. Switch on heating mat.");
      notif_mat_flag = 1;
    }
  }
  // If heating mat is not switched on and control mode is automatic, notify user that the mat will be switched on
  else if(temperature < minTemp && BUTTON_STATE_MAT == 0 && switchMode == 1){
    Blynk.notify("Alert: Humidity exceeded maximum temperature. Heating mat will be switched on.");
  }

  // If water pump is not switched on and control mode is manual, notify user and suggest to switch the pump on
  if(moisturePercent < dryLevel && BUTTON_STATE_PUMP == 0 && switchMode == 0){
    if(notif_pump_flag == 0){
      Blynk.notify("Alert: Soil is too dry. Switch on the water pump to water seeds.");
      notif_pump_flag = 1;
    }
  }
  // If water pump is not switched on and control mode is automatic, notify user that the pump will be switched on
  else if(moisturePercent < dryLevel && BUTTON_STATE_PUMP == 0 && switchMode == 1){
    if(notif_autoPump_flag == 0){
      Blynk.notify("Alert: Soil is too dry. Water pump will be switched on.");
      notif_autoPump_flag = 1;
    }
  }

  // If light is not switched on and control mode is manual, notify user and suggest to switch the light on
  if(lightLevel < minLight && BUTTON_STATE_LIGHT == 0 && switchMode == 0){
    if(notif_light_flag == 0){
      Blynk.notify("Alert: Lighting is too dark. Switch on the light.");
      notif_light_flag = 1;
    }
  }
  // If light is not switched on and control mode is automatic, notify user that the light will be switched on
  else if(lightLevel < minLight && BUTTON_STATE_LIGHT == 0 && switchMode == 1){
    Blynk.notify("Alert: Lighting is too dark. LED lights will be switched on.");
  }


  //if Mode is set to Auto
  if(switchMode == 1){
      autoMode();
    }

  if(notif_autoPump_flag == 1 && switchMode == 0){
    notif_autoPump_flag = 0;
  }

  // If user has switched on fan after the notification, change the notif_fan_flag value back to 0
  if(BUTTON_STATE_FAN == 1 && notif_fan_flag == 1){
    notif_fan_flag = 0;
  }

  // If user has switched on mat after the notification, change the notif_mat_flag value back to 0
  if(BUTTON_STATE_MAT == 1 && notif_mat_flag == 1){
    notif_mat_flag = 0;
  }

  // If user has switched on pump after the notification, change the notif_pump_flag value back to 0
  if(BUTTON_STATE_PUMP == 1 && notif_pump_flag == 1){
    notif_pump_flag = 0;
  }

  // If user has switched on light after the notification, change the notif_light_flag value back to 0
  if(BUTTON_STATE_LIGHT == 1&& notif_light_flag == 1){
    notif_light_flag = 0;
  }

}

// When connected to Blynk server
BLYNK_CONNECTED() {
  // Request the latest state of buttons from the server
  Blynk.syncVirtual(VPIN_BUTTON_FAN);
  Blynk.syncVirtual(VPIN_BUTTON_MAT);
  Blynk.syncVirtual(VPIN_BUTTON_LIGHT);
  Blynk.syncVirtual(VPIN_BUTTON_PUMP);
  Blynk.syncVirtual(VPIN_BUTTON_MODE);
}

// When the button in Blynk is pushed, switch the state
// Fan button
BLYNK_WRITE(VPIN_BUTTON_FAN) {
  // To get the current state of fan button from Blynk
  BUTTON_STATE_FAN = param.asInt();
  // If current Blynk fan button state is 1 which means fan is ON
  if (BUTTON_STATE_FAN == 1){
    digitalWrite(RELAY_PIN_1, LOW); // Switch ON relay 1 connected to the fan (LOW = ON for relay because it is active low)
  }
  // If current Blynk fan button state is 0 which means fan is OFF
  else if (BUTTON_STATE_FAN == 0){
    digitalWrite(RELAY_PIN_1, HIGH);
  }
}

// Light button
BLYNK_WRITE(VPIN_BUTTON_LIGHT) {
  // To read the current state of light button from Blynk
  BUTTON_STATE_LIGHT = param.asInt();
  // If Blynk fan button state is 1 which means light is ON
  if (BUTTON_STATE_LIGHT == 1){
    digitalWrite(RELAY_PIN_2, LOW); // Switch ON relay 1 connected to the light (LOW = ON for relay because it is active low)
  }
  // If current Blynk fan button state is 0 which means light is OFF
  else if (BUTTON_STATE_LIGHT == 0){
    digitalWrite(RELAY_PIN_2, HIGH);
  }

}

// Heating mat button
BLYNK_WRITE(VPIN_BUTTON_MAT) {
  // To read the current state of mat button from Blynk
  BUTTON_STATE_MAT = param.asInt();
  // If Blynk mat button state is 1 which means mat is ON
  if (BUTTON_STATE_MAT == 1){
    digitalWrite(RELAY_PIN_3, LOW); // Switch ON relay 1 connected to the mat (LOW = ON for relay because it is active low)
  }
  // If current Blynk mat button state is 0 which means mat is OFF
  else if (BUTTON_STATE_MAT == 0){
    digitalWrite(RELAY_PIN_3, HIGH);
  }

}

// Water pump button
BLYNK_WRITE(VPIN_BUTTON_PUMP) {
  // To read the current state of pump button from Blynk
  BUTTON_STATE_PUMP = param.asInt();
  // If Blynk pump button state is 1 which means pump is ON
  if (BUTTON_STATE_PUMP == 1){
    digitalWrite(RELAY_PIN_4, LOW); // Switch ON relay 1 connected to the pump (LOW = ON for relay because it is active low)
  }
  // If current Blynk pump button state is 0 which means pump is OFF
  else if (BUTTON_STATE_PUMP == 0){
    digitalWrite(RELAY_PIN_4, HIGH);
  }

}

//Control mode button
BLYNK_WRITE(VPIN_BUTTON_MODE) {
  switchMode = param.asInt();
}

// Gets called when slider attched to virtual pin 8 in the Blynk app changes, sets the value of the slider to maxTemp.
// This is the setpoint level for the activation of the fans.
BLYNK_WRITE(VPIN_SLIDER_MAXTEMP)
{
  float tmpMaxTemp = param.asFloat();         // assigning incoming value from pin V8 as an float to the tmpMaxTemp variable
  //Check if set max temperature is less than or equal to set minimum temperature
  if (tmpMaxTemp <= minTemp){
    Blynk.notify("Max temperature cannot be less than or equal to min temperature.");
  }
  else{
    notif_fan_flag = 0;
    maxTemp = tmpMaxTemp;
    EEPROM.put(EEAddr,maxTemp);        // Stores the value of maxTemp to EEPROM
    EEPROM.commit();                   // To save changes to flash/EEPROM.
  }
}

BLYNK_WRITE(VPIN_SLIDER_MINTEMP)
{
  float tmpMinTemp = param.asFloat();          // Assigning incoming value from pin V12 as float to the tmpMinTemp variable
  //Check if set min temperature is more than or equal to set maximum temperature
  if (tmpMinTemp >= maxTemp){
    Blynk.notify("Min temperature cannot be more than or equal to max temperature.");
  }
  else{
    notif_mat_flag = 0;
    minTemp = tmpMinTemp;
    EEPROM.put(EEAddr_next_2,minTemp);  // Stores the value of minTemp to EEPROM
    EEPROM.commit();                     // To save changes to flash/EEPROM
  }
}

// Gets called when slider attched to virtual pin 10 in the Blynk app changes, sets the value of the slider to drylevel.
// This is the setpoint level for the activation of the water pump.
BLYNK_WRITE(VPIN_SLIDER_DRYLEVEL)
{
  notif_pump_flag = 0;
  dryLevel = param.asInt();          // Assigning incoming value from pin V10 as an int to the dryLevel variable
  EEPROM.put(EEAddr_next_1,dryLevel);  // Stores the value of drylevel to EEPROM
  EEPROM.commit();                     // To save changes to flash/EEPROM
}

// Gets called when slider attched to virtual pin 11 in the Blynk app changes, sets the value of the slider to maxHumidity.
// This is the setpoint level for the activation of the fans.
BLYNK_WRITE(VPIN_SLIDER_MAXHUM)
{
  notif_fan_flag = 0;
  maxHumidity = param.asFloat();          // Assigning incoming value from pin V11 as an float to the maxHumidity variable
  EEPROM.put(EEAddr_next_3,maxHumidity);  // Stores the value of maxHumidity to EEPROM
  EEPROM.commit();                     // To save changes to flash/EEPROM
}

// Gets called when slider attched to virtual pin 14 in the Blynk app changes, sets the value of the slider to minLight.
// This is the setpoint level for the activation of the LED light.
BLYNK_WRITE(VPIN_SLIDER_LIGHTLEVEL)
{
  notif_light_flag = 0;
  minLight = param.asInt();          // Assigning incoming value from pin V14 as an int to the minLIght variable
  EEPROM.put(EEAddr_next_4,minLight);  // Stores the value of minLight to EEPROM
  EEPROM.commit();                     // To save changes to flash/EEPROM
}

// Gets called when slider attched to virtual pin 15 in the Blynk app changes, sets the value of the slider to pumpTime.
// This is the setpoint level for the activation of the LED light.
BLYNK_WRITE(VPIN_SLIDER_PUMPTIME)
{
  pumpTime = param.asInt();          // Assigning incoming value from pin V14 as an int to the minLIght variable
  EEPROM.put(EEAddr_next_5, pumpTime);  // Stores the value of minLight to EEPROM
  EEPROM.commit();                     // To save changes to flash/EEPROM
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void setup(){

  EEPROM.begin(512);    // Initialise EEPROM for storing values

  //Define fans connected to the RELAY_PIN_1 as an output and set the pin off initially
  pinMode(RELAY_PIN_1, OUTPUT);
  digitalWrite(RELAY_PIN_1, HIGH);

  //Define LED light connected to the RELAY_PIN_2 as an output and set the pin off initially
  pinMode(RELAY_PIN_2, OUTPUT);
  digitalWrite(RELAY_PIN_2, HIGH);

  //Define heating mat connected to the RELAY_PIN_3 as an output and set the pin off initially
  pinMode(RELAY_PIN_3, OUTPUT);
  digitalWrite(RELAY_PIN_3, HIGH);

  //Define water pump connected to the RELAY_PIN_4 as an output and set the pin off initially
  pinMode(RELAY_PIN_4, OUTPUT);
  digitalWrite(RELAY_PIN_4, HIGH);

  Serial.begin(9600);
  //Connect to wifi
  initWiFi();
  //Connect to Blynk server
  Blynk.config(auth);

  //Inititalize DHT22, the Wire library, and BH1750.
  dht.begin();
  Wire.begin();
  lightMeter.begin();

  dryLevel = EEPROM.get(EEAddr_next_1, dryLevel);  // Get the value of dryLevel stored in the EEPROM
  maxTemp = EEPROM.get(EEAddr, maxTemp);           // Get the value of maxTemp stored in the EEPROM
  minTemp = EEPROM.get(EEAddr_next_2, minTemp);  // Get the value of minTemp stored in the EEPROM
  maxHumidity = EEPROM.get(EEAddr_next_3, maxHumidity);  // Get the value of maxHumidity stored in the EEPROM
  minLight = EEPROM.get(EEAddr_next_4, minLight);  // Get the value of minLight stored in the EEPROM
  pumpTime = EEPROM.get(EEAddr_next_5, pumpTime);  // Get the value of minLight stored in the EEPROM

  // Setup a function to be called every second
  timer.setInterval(1000L, sendData);

}


void loop() {

  unsigned long currentMillis = millis();
  // if WiFi is disconnected, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }

  Blynk.run();
  timer.run();


}
