#include <DS3231.h>
#include <Wire.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Key.h>
#include <Keypad.h>
#include <stdlib.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 12
#define LDR_PIN A0
#define LM35_PIN A1

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

DS3231 myRTC;

const byte rows = 4;
const byte cols = 4;
char keys[rows][cols] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPin[rows] = {9,8,7,6};
byte colPin[cols] = {5,4,3,2};
Keypad keypad = Keypad(makeKeymap(keys), rowPin, colPin, rows, cols);
char key;
int whichAlarm;
byte alarm[6][3] = {};
byte durbuf[2] = {};
int i = 0;

byte hour;
byte minute;
byte second;

bool century = false;
bool h12Flag = false;
bool pmFlag = false;
bool pengaturan = false;
bool aturAlarm = false;
bool enterDuration = false;
bool enterJam = false;
bool enterMenit = false;
bool done[5];

const byte WAIT = 60;

const String DASH = "-";
const String SPACE = " ";
const String COLON = ":";
const String EMPTY_STR = "";

//String DISPLAY[3];
char nama[] = "Nandaffa Rizky P.";
char nrp[] = "07211940000007";
char namanrp[] = "Nandaffa-007";

unsigned long timeAlive = 0;
unsigned long intensityThrottle = 0;

float temperature;
unsigned int ledIntensity = 0;

char buf[100];

void setup() {
    Serial.begin(9600);
    Wire.begin();
    myDisplay.begin();
    myDisplay.setIntensity(0);
    myDisplay.displayClear();

    myDisplay.setTextAlignment(PA_CENTER);
    memset(done, false, 5);
    setTime();
}

String toBePrinted = EMPTY_STR;

void loop() {
    ledIntensitySelect(LDR_PIN);
    myDisplay.setIntensity(ledIntensity);

    if(!pengaturan){
        toBePrinted = EMPTY_STR;
    }

    byte curSecond = myRTC.getSecond();
    key = keypad.getKey();

    if(!pengaturan) {
        for(int a = 1; a <= 4; a++) {
            if((myRTC.getHour(h12Flag, pmFlag) == alarm[a][1]) && (myRTC.getMinute() == alarm[a][2]) && (!done[a])) {
                myDisplay.print("");
                if(a == 1){
                    myDisplay.displayScroll(nama, PA_CENTER, PA_SCROLL_LEFT, 50);
                }
                else if(a == 2){
                    myDisplay.displayScroll(nrp, PA_CENTER, PA_SCROLL_LEFT, 50);
                }
                else if(a == 3){
                    myDisplay.displayScroll(namanrp, PA_CENTER, PA_SCROLL_LEFT, 50);
                }
                else if(a == 5){
                    myDisplay.displayScroll(setAlarm(alarm[a][1], alarm[a][2]), PA_CENTER, PA_SCROLL_LEFT, 50);
                }
                byte durasiFix = curSecond + alarm[a][0];
                while(myRTC.getSecond() < durasiFix){
                    if(myDisplay.displayAnimate()) {
                        myDisplay.displayReset();
                    }
                }
                done[a] = true;
                break;
            }
        }
    }

    if (((curSecond >= 10 && curSecond <= 15) || (curSecond >= 40 && curSecond <= 45)) && (!pengaturan)) {
        toBePrinted += getTemp();
    } 
    else if (key && !pengaturan) {
        if (key == 'A'){
            myDisplay.setTextAlignment(PA_LEFT);
            myDisplay.print(EMPTY_STR);
            toBePrinted = EMPTY_STR;
            toBePrinted += "Alrm:";
            key = NULL;
            pengaturan = true;
            aturAlarm = true;
            whichAlarm = 0;
        }
    }
    else if (key && pengaturan && aturAlarm) {
        if(whichAlarm == 0){
            if(key == '1' || key == '2' || key == '3' || key == '4' || key == '5'){
                whichAlarm = key - 48;
                done[whichAlarm] = false;
                //alarm[key][0] = key;
                toBePrinted += key;
            } 
        }
        else if(key == '#' && whichAlarm != 0 && enterDuration == false){
            toBePrinted = EMPTY_STR + "Dur:";
            enterDuration = true;
            key = NULL;
            i = 0;
        }
        else if(key == '#' && enterDuration == true && durbuf[0] && !enterJam) {
            alarm[whichAlarm][0] = atoi(durbuf);
            toBePrinted = EMPTY_STR;
            toBePrinted += "Jam:";
            enterJam = true;
            key = NULL;
            memset(durbuf, NULL, 2);
            i = 0;
        }
        else if(enterDuration && i < 2 && !enterJam){
            toBePrinted += key;
            durbuf[i] = key;
            i++;
        }
        else if (key == '#' && enterJam == true && durbuf[0] && !enterMenit) {
            alarm[whichAlarm][1] = atoi(durbuf);
            toBePrinted = EMPTY_STR + "Mnt:";
            enterMenit = true;
            key = NULL;
            memset(durbuf, NULL, 2);
            i = 0;
        }
        else if(enterJam && i < 2 && !enterMenit) {
            toBePrinted += key;
            durbuf[i] = key;
            i++;
        }
        else if (key == '#' && durbuf[0] && enterMenit) {
            alarm[whichAlarm][2] = atoi(durbuf);
            toBePrinted = "SET OK";
            delay(1000);
            key = NULL;
            memset(durbuf, NULL, 2);
            i = 0;
            pengaturan = false;
            enterDuration = false;
            enterJam = false;
            enterMenit = false;
            myDisplay.setTextAlignment(PA_CENTER);
            Serial.println(whichAlarm);
            Serial.println(alarm[whichAlarm][0]);
            Serial.println(alarm[whichAlarm][1]);
            Serial.println(alarm[whichAlarm][2]);
            Serial.println(setAlarm(alarm[whichAlarm][1], alarm[whichAlarm][2]));
        }
        else if(enterMenit && i < 2) {
            toBePrinted += key;
            durbuf[i] = key;
            i++;
        }
    }
    else if(!key && !pengaturan) {
        toBePrinted += getTime();
    }

    myDisplay.print(toBePrinted);

    delay(WAIT);
    //byte alarm1[2] = []
    //input jam
    //alarm1[0] = input
    //input mnt
    //alarm1[1] = input
    //String alarmSatu = setAlarm();
}

String getTemp() {
    unsigned long timeNow = millis();
    if (timeAlive == 0 || timeNow - timeAlive >= 1000) {
        temperature = (float)analogRead(LM35_PIN) / 2.0479;
        timeAlive = timeNow;
    }
    String res;
    res.concat(temperature);
    res = res.substring(0, 4);
    res.concat(" ");
    res.concat("C");
    return res;
}

void setTemp() {
    String t = EMPTY_STR;
    t += getTemp();
    Serial.println(t);
    t.toCharArray(buf, 100);
}

/*void setNama() {
    String n = NAMA;
    n.toCharArray(buf, 100);
}*/

String getTime() {
    char buf[30];
    byte hour = myRTC.getHour(h12Flag, pmFlag);
    sprintf(buf, "%02d", hour);
    String time = EMPTY_STR + buf;
    time += COLON;
    byte minute = myRTC.getMinute();
    sprintf(buf, "%02d", minute); 
    time += buf;

    return time;
}

String setAlarm(byte jam, byte menit) {
    char buf[30];
    byte hour = jam;
    sprintf(buf, "%02d", hour);
    String time = EMPTY_STR + buf;
    time += COLON;
    byte minute = menit;
    sprintf(buf, "%02d", minute);
    time += buf;

    return time;
}

void setTime() {
    String time = getTime();
    time.toCharArray(buf, 100);
}

byte getLedIntensity(int light) {
    int a = abs(light - 1023) * 16;
    a /= 1023;
    --a;
    return abs(a);
}

void ledIntensitySelect(uint8_t ldrPin) {
    unsigned long timeNow = millis();
    if (intensityThrottle == 0 || timeNow - intensityThrottle >= 1000) {
        int light = analogRead(ldrPin);
        ledIntensity = getLedIntensity(light);
        intensityThrottle = timeNow;
    }
}
