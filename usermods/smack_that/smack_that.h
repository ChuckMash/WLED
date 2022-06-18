#pragma once

#include "wled.h"

#define MAX_SMACKS 10

// Extra Features, Enabled with Serial Output Level
//   * 99 - Will send a constant invert-adjusted reading from sensor

// TODO
//  Improve memory use with PROGMEM 
//  Secret Knock Mode
//  Millis rollover?
//  check current preset to new preset    vs    just set the preset, even if same
// tuck tripwire presets into the end of smacks to preset?



class smackThatUsermod : public Usermod {
  private:

    // Settings
    int8_t        sensorPin;           //  Pin to be used by sensor
    uint8_t       serialOutputLevel;   //  0: disabled, 1: data when preset applied, 2: data on smacks with preset if applied. 99: raw feed from sensor
    uint8_t       tripwirePresets[2];  //  Holds the trip and reset presets used in Tripwire mode
    unsigned long activationTimeout;   //  Timeout from most recent sensor activation to end session and apply presets
    unsigned long tripwireTimeout;     //  Timeout from last Tripwire detection, if exceeded second Tripwire preset will be applied
    unsigned long bounceDelay;         //  Cooldown time after each sensor activation is first detected, helps with sensor bounce and timing
    bool          invertSensorHL;      //  Invert HIGH/LOW for sensor
    bool          enabled;             //  enable / disable Smack That Usermod

    // Internal Use
    uint8_t  activationCount = 0;         //  The number of sensor activations in this session, resets after activationTimeout
    uint8_t  smacksToPreset[MAX_SMACKS];  //  Stores sensor activation count per session to preset lookup
    unsigned long lastActive = 0;         //  millis time of most recent sensor activation detected
    bool sensorIsActive      = false;     //  Holds the invert-adjusted reading from the sensor
    bool sensorWasActive     = false;     //  Is the sensor active right now
    bool tripped             = false;     //  Is Tripwire tripped or untripped



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
      if (sensorIsActive && !tripped && tripwirePresets[0]){
        tripped = true;
        applyPreset(tripwirePresets[0]);
        if(serialOutputLevel > 0){
          tripwireSerialOutput(tripwirePresets[0]);
        }
      }

      // Check Tripwire Reset and timeout settings and apply
      else if(!sensorIsActive && tripped && millis() - lastActive >= tripwireTimeout){
        tripped = false;
        applyPreset(tripwirePresets[1]);
        if(serialOutputLevel > 0){
          tripwireSerialOutput(tripwirePresets[1]);
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

      for (uint8_t i = 1; i <= MAX_SMACKS; i++) {
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

      for (uint8_t i = 1; i <= MAX_SMACKS; i++) {
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
    void serialOutput(uint8_t smacks, uint8_t preset){
      if (serialOutputLevel >= 2 || (serialOutputLevel == 1 && preset)){
        Serial.write("{\"smacks\":");
        Serial.print(smacks);
        if (preset){
          Serial.write(",\"preset\":");
          Serial.print(preset);
        }
        Serial.println("}");
      }
    }



  // Send JSON blob with Tripwire events
  void tripwireSerialOutput(uint8_t preset){
    Serial.write("{\"tripped\":");
    Serial.print(tripped);
    if (preset){
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