Smack That Usermod!
---
Control WLED with Smacks, Claps, Taps, Knocks, Jostles, Nudges, and more!


Settings
---
* Enable
  * Enable/Disable Smack That.
* Smack Timeout (ms)
  * How much you can delay between smacks before Smack That will assume the smacking has ended and act accordingly.
* Bounce Delay (ms)
  * Helps remove multiple activations in a very short time due to bounce.
  * Acts as a cooldown after a smack is detected. If unsure, leave as is.
* Serial Output Level
  * Sends Smack That updates over serial connection as JSON.
  * Useful for debugging and determining required timing for Smack Timeout and Bounce Delay
  * 0: Disabled
  * 1: Send Smack That update when preset is applied.
    * Example: ```{"smacks":3,"preset":1}```
    * Example: ```{"tripped":1,"preset":1}``` (Tripwire Mode)
    * Example: ```{"tripped":0,"preset":2}``` (Tripwire Mode)
  * 2: Send Smack That update even when preset is not applied.
    * Example: ```{"smacks":2}```
    * Example: ```{"smacks":3,"preset":1}```
    * Example: ```{"tripped":1}``` (Tripwire Mode)
* Pin
  * Pin to read from for sensor
* Invert
  * Default is detect on LOW, invert to detect on HIGH
* N Smacks(s)
  * Specify a preset to load when N smacks(s) are detected
* Use Tripwire Mode
  * Overrides default behavior with Tripwire Mode.
  * On sensor activation, applies Tripped Preset.
  * After timeout (resets on each sensor activation) applies Untripped Preset.
* Tripwire Timeout (ms)
  * The amount of time, in milliseconds, after last sensor trip that the Untripped Preset will be applied.
* Tripped Preset
  * Preset to apply when sensor is tripped.
* Untripped Preset
  * Preset to apply when sensor has not been tripped for the timeout period.

Supported Sensors
---
* **FC-04 Sound Sensor Module. (Recommended)**
* **SW-420 Vibration Sensor Module. (Recommended)**
* **SR602 PIR Motion Sensor Module. (Recommended, Tripwire Mode)**
* MAX4466 Microphone Amplifier Module.
* SW-18010P Spring Vibration Sensor (not in module). Works, but not recommended.
* Any other sensor that pulls LOW when activated. (or HIGH if Invert is enabled)


Demos
---
[![Smack That: Clapper Mode](https://img.youtube.com/vi/mRhMShXGT5s/0.jpg)](https://www.youtube.com/watch?v=mRhMShXGT5s)

[Default (Clapper) Mode](https://www.youtube.com/watch?v=mRhMShXGT5s)

---

[![Smack That: Tripwire Mode configured for semi-sound reactive](https://img.youtube.com/vi/cBBUQdeMTcY/0.jpg)](https://www.youtube.com/watch?v=cBBUQdeMTcY)

[Tripwire Mode (configured for semi-sound reactive)](https://www.youtube.com/watch?v=cBBUQdeMTcY)



PlatformIO Example
---
```
[platformio]
default_envs = d1_mini
[env:d1_mini]
board = d1_mini
platform = ${common.platform_wled_default}
platform_packages = ${common.platform_packages}
upload_speed = 115200
board_build.ldscript = ${common.ldscript_4m1m}
build_unflags = ${common.build_unflags}
build_flags = ${common.build_flags_esp8266} -D USERMOD_SMACK_THAT
lib_deps = ${esp8266.lib_deps}
monitor_filters = esp8266_exception_decoder
```
