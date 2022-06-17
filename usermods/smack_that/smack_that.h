#pragma once

#include "wled.h"

#define MAX_SMACKS 10

// Extra Features, Enabled with Serial Output Level
//   * 99 - Will send a constant invert-adjusted reading from sensor

// TODO
//  Improve memory use with PROGMEM 
//  Secret Knock Mode
//  Millis rollover?
//  optimize definitions
//  check current preset to new preset    vs    just set the preset, even if same



class smackThatUsermod : public Usermod {
  private:

    int8_t sensorPin;           //  pin to be used by sensor
    int    activationTimeout;   //  time after most recent sensor activation to end session and apply presets
    int    bounceDelay;         //  cooldown time after each sensor activation is first detected, helps with sensor bounce and timing
    bool   invertSensorHL;      //  invert HIGH/LOW for sensor
    bool   enabled;             //  enable / disable Smack That Usermod
    int    serialOutputLevel;   //  0: disabled, 1: data when preset applied, 2: data on smacks with preset if applied. 99: raw feed from sensor
    int    tripwireTimeout;     //  Timeout from last Tripwire detection, if exceeded second Tripwire preset will be applied
    int    tripwirePresets[2];  //  holds the trip and untrip presets used in Tripwire mode

    bool sensorIsActive      = false;  //  Holds the invert-adjusted reading from the sensor
    bool sensorWasActive     = false;  //  Is the sensor active right now
    bool tripped             = false;  //  Is Tripwire tripped or untripped
    int  activationCount     = 0;      //  The number of sensor activations in this session, resets after activationTimeout
    unsigned long lastActive = 0;      //  millis time of most recent sensor activation detected
    int  smacksToPreset[MAX_SMACKS];   //  Stores sensor activation count per session to preset lookup



  public:

    void setup(){
      if (sensorPin < 0 || !pinManager.allocatePin(sensorPin, false, PinOwner::UM_Unspecified)){
        sensorPin = -1;
        return;
      }
    }



    void loop() {

      // If Smack That is not enabled, don't do anything
      if (!enabled) return;

      // Read from sensor, adjust for invert setting
      sensorIsActive = invertSensorHL?digitalRead(sensorPin):!digitalRead(sensorPin);

      //Extra Feature: Send out invert-adjusted feed from sensor over serial
      if(serialOutputLevel == 99){
        Serial.println(sensorIsActive);
      }

      // New sensor activation detected
      if (sensorIsActive && !sensorWasActive && millis() - lastActive >= bounceDelay){
        activationCount++;
        lastActive = millis();
        sensorWasActive = true;
      }

      // Sensor is inactive
      else if (!sensorIsActive){

        // If previous sensor activation has ended
        if (sensorWasActive) {
          sensorWasActive = false;
        }
        
        else{

          // Check if session has ended
          if (activationCount > 0 && millis() - lastActive >= activationTimeout){

            // Check if number of activations matches any preset settings and apply
            if (activationCount <= MAX_SMACKS && smacksToPreset[activationCount] > 0){
              applyPreset(smacksToPreset[activationCount]);
              serialOutput(activationCount, smacksToPreset[activationCount]);
            }
            else{
              serialOutput(activationCount, 0);
            }

           activationCount = 0;
          }
        }
      }

      // Check Tripwire Preset settings and apply
      if (sensorIsActive && !tripped){
        tripped = true;
        if (applyPreset(tripwirePresets[0])){
          if(serialOutputLevel > 0){
            tripwireSerialOutput(true, tripwirePresets[0]);
          }
        }
      }

      // Check Tripwire Reset and timeout settings and apply
      else if(!sensorIsActive && tripped && millis() - lastActive >= tripwireTimeout){
        tripped = false;
        if(applyPreset(tripwirePresets[1])){
          if(serialOutputLevel > 0){
            tripwireSerialOutput(false, tripwirePresets[1]);
          }
        }
      }

    } // end loop



    void addToConfig(JsonObject& root)
    {
      JsonObject top = root.createNestedObject("Smack That Usermod");

      top["Enable"]                    = enabled;
      top["Smack Timeout (ms)"]        = activationTimeout;
      top["Bounce Delay (ms)"]         = bounceDelay;
      top["Serial Output Level (0-2)"] = serialOutputLevel;
      top["Pin"]                       = sensorPin;
      top["Invert"]                    = invertSensorHL;

      for (int i = 1; i <= MAX_SMACKS; i++) {
        top[getKey(i)] = smacksToPreset[i];
      }

      top["Tripwire Preset"]       = tripwirePresets[0];
      top["Tripwire Reset"]        = tripwirePresets[1];
      top["Tripwire Timeout (ms)"] = tripwireTimeout;
    }



    bool readFromConfig(JsonObject& root)
    {
      JsonObject top = root["Smack That Usermod"];

      bool configComplete = !top.isNull();

      configComplete &= getJsonValue(top["Enable"],                    enabled,           true);
      configComplete &= getJsonValue(top["Smack Timeout (ms)"],        activationTimeout, 250);
      configComplete &= getJsonValue(top["Bounce Delay (ms)"],         bounceDelay,       150);
      configComplete &= getJsonValue(top["Serial Output Level (0-2)"], serialOutputLevel, 0);
      configComplete &= getJsonValue(top["Pin"],                       sensorPin,         -1);
      configComplete &= getJsonValue(top["Invert"],                    invertSensorHL,    false);

      for (int i = 1; i <= MAX_SMACKS; i++) {
        configComplete &= getJsonValue(top[getKey(i)], smacksToPreset[i], 0);
        }

      configComplete &= getJsonValue(top["Tripwire Preset"],       tripwirePresets[0], 0);
      configComplete &= getJsonValue(top["Tripwire Reset"],        tripwirePresets[1], 0);
      configComplete &= getJsonValue(top["Tripwire Timeout (ms)"], tripwireTimeout,    60000); // default to 1 minute

      return configComplete;
    }



    // Generate JSON Key
    String getKey(uint8_t i) {
      if (i == 1) return "1 Smack";
      else return String(i) + " Smacks";
    }



    // Send JSON blobs with smack and preset data out over serial if enabled
    void serialOutput(int smacks, int preset){
      if (serialOutputLevel >= 2 || (serialOutputLevel == 1 && preset > 0)){
        Serial.write("{\"smacks\":");
        Serial.print(smacks);
        if (preset>0){
          Serial.write(",\"preset\":");
          Serial.print(preset);
        }
        Serial.println("}");
      }
    }



  // Send JSON blob with Tripwire events
  void tripwireSerialOutput(bool tripped, int preset){
    Serial.write("{\"tripped\":");
    Serial.print(tripped);
    if (preset > 0){
      Serial.write(",\"preset\":");
      Serial.print(preset);
    }
    Serial.println("}");
  }



    uint16_t getId(){return USERMOD_ID_SMACK_THAT;}



    //void connected() {}
    //void addToJsonInfo(JsonObject& root){}
    //void addToJsonState(JsonObject& root){}
    //void readFromJsonState(JsonObject& root){}
    //void handleOverlayDraw(){}
};

