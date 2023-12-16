#include "si5351.h"
#include "Wire.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "soc/syscon_reg.h"
#include "soc/syscon_struct.h"
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include "BluetoothSerial.h"

String device_name = "ANTENA_ANALYZER_YD1GSE";
BluetoothSerial SerialBT;

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
#define TFT_GREY 0xBDF7

Si5351 si5351;
int_fast32_t freq_manual =  7000000;
int_fast32_t freq_send =  0;
bool onoff=false;

  double REV;  
  double analogVoltsref;  
  double FWD;  
  double analogVoltsfwd; 
  double calb=150;
  double VSWR, EffOhms;

int counter=0;
String message;String dt[10];int i;boolean parsing=false;//serialnya
byte led = 2; bool on_off_led=false;
unsigned long test_freq; 
unsigned long low_freq;
unsigned long high_freq;
unsigned long center_freq; 

int multi_scan = 1; 
String data_grafik;
byte modeplay;

void setup(){
 tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);

  adc1_config_width(ADC_WIDTH_BIT_10); //ADC_WIDTH_BIT_9 ADC_WIDTH_BIT_10 ADC_WIDTH_BIT_11 ADC_WIDTH_BIT_12
  //adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_0);//ADC_ATTEN_DB_0 ADC_ATTEN_DB_2_5 ADC_ATTEN_DB_6 ADC_ATTEN_DB_11
  //adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_0);
  //analogReadResolution(12);
  //analogReference(INTERNAL);  
  
  Serial.begin(115200);
  SerialBT.begin(device_name); 
   
  bool i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  if(!i2c_found){ Serial.println("Device not found on I2C bus!");}
  
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
  //si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
  //si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA);
  
  
  si5351.set_phase(SI5351_CLK0, 0);
  update_freq();

  si5351.update_status();
  delay(500);
  Serial.println(freq_manual);
}

void loop(){
  from_android();  
  if (modeplay==1){Sweeper_on();}
  if (modeplay==3){stand();}
  
  readAdc(); 
  tampilLCD();
  bacaserial();

}

void from_android() {
    if(SerialBT.available()>0) {char inChar = (char)SerialBT.read(); message += inChar;if (inChar == '\n') { parsing = true;} 
    }                             
    if(parsing){parsingData(); parsing=false; message="";}
} 

void parsingData(){
int j=0; Serial.print("data masuk : ");Serial.println(message);dt[j]="";
String cekk=String(message[0])+String(message[1])+String(message[2]);
                    if  (cekk=="@1%"){modeplay=1;Serial.println("Sweeper On");}
                    if  (cekk=="@2%"){modeplay=2; Serial.println("Sweeper Off");digitalWrite(led,LOW);}                
                    if  (cekk=="@3%"){                                  
                              Serial.println("Setting Grid");                   
                              String convert = String(message).substring(3);
                              String pisah[3]; counter=0;int lastIndex = 0;
                              for (int i = 0; i < convert.length(); i++) {
                                if (convert.substring(i, i+1) == ":") {
                                  pisah[counter] = convert.substring(lastIndex, i);
                                  lastIndex = i + 1; counter++;
                                }
                                 if (i == convert.length() - 1) {
                                 pisah[counter] = convert.substring(lastIndex, i);
                                }
                              }
                              low_freq=(unsigned long)pisah[0].toInt();center_freq=(unsigned long)pisah[1].toInt(); high_freq=(unsigned long)pisah[2].toInt();
                              convert = ""; counter = 0;lastIndex = 0;                             
                              multi_scan=high_freq-low_freq; multi_scan=multi_scan/100;                                   
                    }
                    if  (cekk=="@4%"){modeplay=3; Serial.println("Stand Measurement");digitalWrite(led,LOW);}   
} 
void Sweeper_on(){
       data_grafik=""; double FWD5=0; double REV5=0;  int Z = 0 ; int R =0;  
       for (counter=0; counter <100;counter=counter+2){   
          for (int i=counter; i <counter+2; i++){  
              test_freq = low_freq + (i*multi_scan); 
              freq_manual = test_freq * 1000; update_freq(); delay(10); tampilLCD();   
              readAdc(); 
              if (VSWR>6.0) VSWR = 6;     Z = EffOhms;                                        
              //Z = Z + 2; if (Z > 250) Z = 0;     //test dummy Impedancy
              //R = R + 4; if (R > 250) R = 0;     //test dummy Reacktance
                    
              if (test_freq>(high_freq-multi_scan)){test_freq=low_freq;} 
              else {data_grafik = data_grafik + String(test_freq)+"|"+String(VSWR)+"|"+String(Z)+"|"+String(R)+"|";}               
          from_android();
          }
          on_off_led= not on_off_led;digitalWrite(led,on_off_led);
          Serial.println(data_grafik+"END");//test dummy
          SerialBT.println(data_grafik+"END");
          data_grafik=""; Serial.flush(); from_android();delay (100); 
       }
}

void stand(){
         data_grafik="";  int Z=0 ; int R=0; 
         freq_manual = center_freq * 1000; update_freq(); delay(10); tampilLCD();
         readAdc();  
         if (VSWR>6.0) VSWR = 6;  Z = EffOhms;       
                    //Z = Z + 3; if (Z > 250) Z = 0;     //test dummy Impedancy
                    //R = R + 5; if (R > 250) R = 0;     //test dummy Reacktance      
         data_grafik =  String(center_freq)+"|"+String(VSWR)+"|"+String(Z)+"|"+String(R)+"|";               
         from_android();
         on_off_led= not on_off_led;digitalWrite(led,on_off_led);
         Serial.println(data_grafik+"END");
         SerialBT.println(data_grafik+"END");
         data_grafik=""; Serial.flush(); 
         delay (150);    
}

void tampilLCD(){
  char munculFreq[20];char adcr[20];char adcf[20];char a[20];
  sprintf(munculFreq, "%d",freq_manual); 
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(2); tft.drawString(munculFreq, 0, 0, 2); 
  sprintf(adcr, "adcF %f",FWD); 
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(2); tft.drawString(adcr, 0, 30, 2);
  sprintf(adcr, "adcR %f ",REV); 
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(2); tft.drawString(adcr, 0, 60, 2);  
  sprintf(a, "Ohm %f ",EffOhms); 
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(2); tft.drawString(a, 0, 90, 2);  
  sprintf(a, "VSWR %f ",VSWR); 
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(2); tft.drawString(a, 0, 120, 2);  
  //delay(500);
};
  
void readAdc(){
  int repeat = 70;
  for (int i = 0 ; i < repeat ;i++){
    REV = analogRead(36); FWD = analogRead(39);
    REV+=REV; FWD+=FWD;
  }
  REV/=repeat; FWD/=repeat;
  if(REV>=FWD) {VSWR = 999;}  else {VSWR=(FWD+REV)/(FWD-REV);}
  if(FWD>=115) EffOhms = VSWR*47.0;//FWD>=94
  else  EffOhms = 47.0/VSWR;
         
  //Serial.print(REV); Serial.print("|");Serial.println(FWD);
  //Serial.println(VSWR);
}


void update_freq(){
      si5351.set_freq(freq_manual * 100, SI5351_CLK0);
      //si5351.set_freq(freq_manual * 100, SI5351_CLK1);
      //si5351.set_freq(freq_manual * 100, SI5351_CLK2);
      //si5351.set_phase(SI5351_CLK0, 0); 
      //si5351.set_phase(SI5351_CLK1, 90); 
      si5351.update_status();
      Serial.println(freq_manual);
 
      delay(50); 
}

void bacaserial(){
  if (Serial.available() > 0)  {
    char key = Serial.read();
    switch (key){
    case 'a':
      freq_manual+=10;update_freq();
      break;
    case 'z':
      freq_manual-=10;update_freq();    
      break;  
    case 's':
      freq_manual+=100;update_freq();      
      break;
    case 'x':
      freq_manual-=100; update_freq();     
      break;      
    case 'd':
      freq_manual+=1000;update_freq();      
      break;
    case 'c':
      freq_manual-=1000; update_freq(); 
      break; 
    case 'f':
      freq_manual+=10000; update_freq();  
      break;
    case 'v':
      freq_manual-=10000;update_freq();
      break; 
    case 'g':
      freq_manual+=100000; update_freq();  
      break;
    case 'b':
      freq_manual-=100000;update_freq();
      break;  
    case 'h':
      freq_manual+=1000000; update_freq();  
      break;
    case 'n':
      freq_manual-=1000000;update_freq();
      break;       
    case 'y':
      //onoff=!onoff;digitalWrite(txrx,onoff);
      break;       
    case 't':
      si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
      update_freq();
      break; 
      

    }
  }  
}


 
