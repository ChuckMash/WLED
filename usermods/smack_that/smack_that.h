#pragma once

#include "wled.h"

#define MAX_SMACKS 10


// TODO
//  Improve memory use with PROGMEM 
//  Secret Knock Mode
//  Millis rollover?



class smackThatUsermod : public Usermod {
  private:

    int8_t sensorPin;           //  pin to be used by sensor
    int    activationTimeout;   //  time after most recent sensor activation to end session and apply presets
    int    bounceDelay;         //  cooldown time after each sensor activation is first detected, helps with sensor bounce and timing
    bool   invertSensorHL;      //  invert HIGH/LOW for sensor
    bool   enabled;             //  enable / disable Smack That Usermod
    int    serialOutputLevel;   //  0: disabled, 1: data when preset applied, 2: data on smacks with preset if applied. 99: raw feed from sensor
    bool   enableTripwire;      //  enable / disable Tripwire mode
    int    tripwireTimeout;     //  Timeout from last Tripwire detection, if exceeded second Tripwire preset will be applied
    int    tripwirePresets[2];  //  holds the trip and untrip presets used in Tripwire mode

    bool sensorActive        = false;  //  Is the sensor active right now
    int  activationCount     = 0;      //  The number of sensor activations in this session, resets after activationTimeout
    int  sensorReading       = 0;      //  Holds the reading from the sensor
    unsigned long lastActive = 0;      //  millis time of most recent sensor activation detected
    int  sensorHL            = LOW;    //  default trigger setting for sensor, can be inverted with "Invert" in usermod setting page    
    int  loadPreset          = 0;      //  holds preset loaded last
    int  smacksToPreset[MAX_SMACKS];   //  Stores sensor activation count per session to preset lookup



  public:

    void setup(){
      if (sensorPin < 0 || !pinManager.allocatePin(sensorPin, false, PinOwner::UM_Unspecified)){
        sensorPin = -1;
        return;
      }
    }



    void loop() {

      // If not enabled, don't do anything
      if (!enabled) return;

      // Read from sensor
      sensorReading = digitalRead(sensorPin);

      // If secret serial output level, send out raw feed from sensor
      if(serialOutputLevel == 99){
        Serial.println(sensorReading);
      }

      // Use Tripwire Mode if enabled
      if (enableTripwire) tripwireLoop();

      // else default to Clapper mode
      else clapperLoop();
    }



    void clapperLoop() {
      // If new sensor activation detected
      if (sensorReading == (invertSensorHL?!sensorHL:sensorHL) && !sensorActive && millis() - lastActive >= bounceDelay){
        activationCount++;
        lastActive = millis();
        sensorActive = true;
      }

      // Sensor is inactive
      else if (sensorReading == (invertSensorHL?sensorHL:!sensorHL)){

        // If previous sensor activation has ended
        if (sensorActive) {
          sensorActive = false;
        }

        else{
          // Check if session has ended
          if (activationCount > 0 && millis() - lastActive >= activationTimeout){
            for (int i=1; i<=MAX_SMACKS; i++){        
              if (activationCount == i && smacksToPreset[i] > 0){
                applyPreset(smacksToPreset[i]);
                loadPreset = smacksToPreset[i];
                break;
              }
            }
           
           if (serialOutputLevel > 0){
            serialOutput(activationCount, loadPreset);
           }

           activationCount = 0;
           loadPreset = 0;
          }
        }
      }
    } // end clapperLoop()



    void tripwireLoop(){
      if (sensorReading == (invertSensorHL?!sensorHL:sensorHL) && millis() - lastActive >= bounceDelay){
        lastActive = millis();

        if (currentPreset != tripwirePresets[0]){
          applyPreset(tripwirePresets[0]);
          if(serialOutputLevel > 0){
            tripwireSerialOutput(true, tripwirePresets[0]);
          }
        }

        // If serial output level is 2+, and not applying a tripped/untripped preset, send tripped message
        else if (serialOutputLevel > 1){
          tripwireSerialOutput(true, 0);
        }

      }

      else if (sensorReading == (invertSensorHL?sensorHL:!sensorHL) && currentPreset != tripwirePresets[1] && millis() - lastActive >= tripwireTimeout){
        applyPreset(tripwirePresets[1]);
        if(serialOutputLevel > 0){
          tripwireSerialOutput(false, tripwirePresets[1]);
        }
      }
    }



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

      top["Use Tripwire Mode"]     = enableTripwire;
      top["Tripwire Timeout (ms)"] = tripwireTimeout;
      top["Tripped Preset"]        = tripwirePresets[0];
      top["Untripped Preset"]      = tripwirePresets[1];
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

      configComplete &= getJsonValue(top["Use Tripwire Mode"],     enableTripwire,     false);
      configComplete &= getJsonValue(top["Tripwire Timeout (ms)"], tripwireTimeout,    60000); // default to 1 minute
      configComplete &= getJsonValue(top["Tripped Preset"],        tripwirePresets[0], 0);
      configComplete &= getJsonValue(top["Untripped Preset"],      tripwirePresets[1], 0);

      return configComplete;
    }



    // Generate JSON Key
    String getKey(uint8_t i) {
      if (i == 1) return "1 Smack";
      else return String(i) + " Smacks";
    }



    // Send JSON blobs with smack and preset data out over serial if enabled
    void serialOutput(int smacks, int preset){
      if (serialOutputLevel>=2 || (serialOutputLevel==1 && preset>0)){
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

