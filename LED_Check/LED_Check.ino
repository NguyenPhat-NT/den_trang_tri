// Optional low-output wiring check. Upload DenDeco again after this check.
// Uses the same pins; it does not erase or update the saved DenDeco NVS.
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(20,5,NEO_GRB+NEO_KHZ800);
const uint8_t touchPins[]={0,1,3,4};
const char* labels[]={"Power","Minus","Plus","Mode"};
bool oldTouch[4]={};
uint32_t lastStep=0;
int step=-1;
void setup(){Serial.begin(115200);for(auto pin:touchPins)pinMode(pin,INPUT);pixels.begin();pixels.clear();pixels.show();}
void loop(){
 uint32_t now=millis();
 for(int i=0;i<4;i++){bool down=digitalRead(touchPins[i])==HIGH;if(down!=oldTouch[i]){oldTouch[i]=down;Serial.printf("Touch %s: %s\n",labels[i],down?"HIGH":"LOW");}}
 uint32_t interval=step>=7?600:2000;
 if(step<0||uint32_t(now-lastStep)>=interval){
  lastStep=now;step=(step+1)%17;pixels.clear();
  if(step<3){for(int i=0;i<20;i++)pixels.setPixelColor(i,step==0?12:0,step==1?12:0,step==2?12:0);Serial.printf("All LEDs: %s\n",step==0?"RED":step==1?"GREEN":"BLUE");}
  else if(step<7){const int indices[]={0,9,10,19};int i=indices[step-3];pixels.setPixelColor(i,12,5,2);Serial.printf("Only LED %d\n",i);}
  else {int h=step-7;pixels.setPixelColor(h,12,5,2);pixels.setPixelColor(19-h,12,5,2);Serial.printf("Height pair %d and %d\n",h,19-h);}
  pixels.show();
 }
 delay(1);
}
