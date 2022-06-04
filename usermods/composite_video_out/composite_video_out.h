#pragma once
#include "wled.h"
#include <ESP_8_BIT_GFX.h>

ESP_8_BIT_GFX videoOut(true,8);

class compositeVideoOut : public Usermod {
  
  private:

  public:



    void setup(){

      videoOut.begin();
      videoOut.fillScreen(0x00);
      videoOut.waitForFrame();

      videoOut.fillScreen(0x00);
      videoOut.waitForFrame();

    }



    void loop() {
  
      for (uint16_t i=1; i<=30; i+=1) {
        uint8_t ab = (strip.getPixelColor(i) >> 16) & 0xFF;
        uint8_t ag = (strip.getPixelColor(i) >> 8) & 0xFF;
        uint8_t ar = strip.getPixelColor(i) & 0xFF;

        uint8_t aB = ab >> 6;
        uint8_t aG = ag >> 5;
        uint8_t aR = ar >> 5;
    
        videoOut.fillRect(100, i*10, 50,10, (aB << 6) | ( aG << 3 ) | aR ); // x,y,w,h,color   
       
      }

    videoOut.waitForFrame();
    }
};
