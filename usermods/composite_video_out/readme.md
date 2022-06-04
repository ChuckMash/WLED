esp32 only
connect composite video cable to pin 25 and ground

Currently extremely unstable

```
[platformio]

default_envs = esp32dev

[env:esp32dev]
board = esp32dev
platform = ${esp32.platform}
platform_packages = ${esp32.platform_packages}
build_unflags = ${common.build_unflags}
build_flags = ${common.build_flags_esp32} 
  -D WLED_RELEASE_NAME=ESP32
  -D WLED_DISABLE_OTA
  -D WLED_DISABLE_ALEXA
  -D WLED_DISABLE_BLYNK
  -D WLED_DISABLE_HUESYNC
  -D WLED_DISABLE_INFRARED
  -D WLED_DISABLE_WEBSOCKETS
  -D WLED_DISABLE_BROWNOUT_DET
  -D WLED_DEBUG 
  -D USERMOD_COMPOSITE_VIDEO_OUT
lib_deps = ${esp32.lib_deps}
         roger-random/ESP_8_BIT Color Composite Video Library@^1.3.1
         adafruit/Adafruit GFX Library @ ^1.11.1
         adafruit/Adafruit BusIO@^1.11.1

monitor_filters = esp32_exception_decoder
board_build.partitions = ${esp32.default_partitions}
```