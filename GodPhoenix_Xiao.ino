/**
 * God Phoenix L.E.D work
 *
 * @author painnick@gmail.com
 * @see https://smile-dental-clinic.info/wordpress/?p=11441
 **/
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include <TimerTC3.h>
#include "DFMiniMp3.h"
#include "Effects.h"

#define _DEBUG 1

#ifdef _DEBUG
#define STRIP_BRIGHT_NORMAL 20
#define STRIP_BRIGHT_HIGH 100
#else
#define STRIP_BRIGHT_NORMAL 100
#define STRIP_BRIGHT_HIGH 250
#endif

#define VOLUME 10

#define HEAD_PIN 0      // #1 기수 앞 부분의 NeoPixel
#define COCKPIT_PIN 1   // #2 콕핏 부분의 NeoPixel
#define TOP_PIN 2       // #3 상단의 원형 클리어 안에 심은 LED
#define TAILSIDE_PIN 4  // #5 엔진 좌우의 클리어 안에 심은 LED
#define ENGINE_PIN 5    // #6 엔진 클리어 안에 심은 LED
#define TX_PIN 6
#define RX_PIN 7
#define BUTTON_PIN 8  // #9 신호 입력용 버튼 스위치
#define VOLUME_PIN 9

#define COLOR_BLACK 0x0

uint32_t normalColor = 0x0000;
Adafruit_NeoPixel head_strip = Adafruit_NeoPixel(11 * 2, HEAD_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel cockpit_strip = Adafruit_NeoPixel(7, COCKPIT_PIN, NEO_GRB + NEO_KHZ800);

class Mp3Notify;
typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3;
SoftwareSerial mySerial(RX_PIN, TX_PIN);
DfMp3 dfmp3(mySerial);

//--------------------------------------------------------------------------
void setup() {
#ifdef _DEBUG
  Serial.begin(9600);
  delay(1000 * 2);
  Serial.println("===== Start Setup =====");
#endif

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(VOLUME_PIN, INPUT);

  head_strip.begin();
  cockpit_strip.begin();
  dfmp3.begin(9600, 1000);

  dfmp3.reset();
  waitMilliseconds(100);

  dfmp3.setVolume(VOLUME);
  waitMilliseconds(100);

  normalColor = head_strip.Color(255, 140, 0);

  TimerTc3.initialize(1000000);
  TimerTc3.attachInterrupt(timerISR);

#ifdef _DEBUG
  Serial.println("----- Setup end -----");
#endif
}

enum PROCESSOR {
  PROCESSOR_NORMAL = 0,
  PROCESSOR_PHOENIX,
  PROCESSOR_NOSONG
};

PROCESSOR processor = PROCESSOR::PROCESSOR_NOSONG;
int oldState = -1;
int normalStep = -1;
int phoenixStep = -1;
int nosongStep = 0;
void loop() {
  waitMilliseconds(500);

  int newState = digitalRead(BUTTON_PIN) == HIGH ? LOW : HIGH;

  switch (processor) {
    case PROCESSOR::PROCESSOR_NORMAL:  // == LOW
      if (newState == HIGH) {
#ifdef _DEBUG
        Serial.println("Button On");
#endif
        processor = PROCESSOR::PROCESSOR_PHOENIX;
        phoenixStep = 0;
      }
      break;
    case PROCESSOR::PROCESSOR_PHOENIX:  // == HIGH
      if (newState == LOW) {
#ifdef _DEBUG
        Serial.println("Button Off");
#endif
        processor = PROCESSOR::PROCESSOR_NORMAL;
        normalStep = 0;
      }
      break;
    default:
      if (newState != oldState) {
        if (newState == LOW) {
#ifdef _DEBUG
          Serial.println("Button Off(First)");
#endif
          processor = PROCESSOR::PROCESSOR_NORMAL;
          normalStep = 0;
        } else {
#ifdef _DEBUG
          Serial.println("Button On(First)");
#endif
          processor = PROCESSOR::PROCESSOR_PHOENIX;
          phoenixStep = 0;
        }
      }
      break;
  }

  oldState = newState;

  switch (processor) {
    case PROCESSOR::PROCESSOR_NORMAL:
      normal_form(normalStep);
      normalStep++;
      break;
    case PROCESSOR::PROCESSOR_PHOENIX:
      phoenix_form(phoenixStep);
      phoenixStep++;
      break;
    case PROCESSOR::PROCESSOR_NOSONG:
      no_song_form(nosongStep);
      nosongStep++;
    default:
      break;
  }
}

void waitMilliseconds(uint16_t msWait) {
  uint32_t start = millis();

  while ((millis() - start) < msWait) {
    // if you have loops with delays, its important to
    // call dfmp3.loop() periodically so it allows for notifications
    // to be handled without interrupts
    dfmp3.loop();
    delay(1);
  }
}

//----------------------------------------------------------------------------------
/**
 * 노말 모드
 **/
void normal_form(int &step) {
#ifdef _DEBUG
  Serial.println("Process - Normal - Step#" + String(step));
#endif
  switch (step) {
    case 0:  // mp3 재생 시작
      head_strip.setBrightness(STRIP_BRIGHT_NORMAL);
      cockpit_strip.setBrightness(STRIP_BRIGHT_NORMAL);

      colorWipe(&head_strip, normalColor, 1, true);
      colorWipe(&cockpit_strip, normalColor, 1, false);

      dfmp3.playMp3FolderTrack(1);
      waitMilliseconds(100);

      analogWrite(TOP_PIN, 100);
      normal_engine(ENGINE_PIN, TAILSIDE_PIN);

      break;
    case 1:
      after_burner(ENGINE_PIN, TAILSIDE_PIN, false);

      head_strip.setBrightness(STRIP_BRIGHT_NORMAL);
      cockpit_strip.setBrightness(STRIP_BRIGHT_NORMAL);

      // 3번 깜박임. 종료 시점에 불이 켜져 있는 상태
      for (int i = 0; i < 6; i++) {
        if (i % 2 == 0) {
          // Screen Off
          colorWipe(&head_strip, COLOR_BLACK, 10, true);
          colorWipe(&cockpit_strip, COLOR_BLACK, 10, false);
        } else {
          colorWipe(&head_strip, normalColor, 30, true);
          colorWipe(&cockpit_strip, normalColor, 30, false);
        }
        waitMilliseconds(200);
      }
      break;
    case 2:
      for (int i = 0; i < 6; i++) {
        COLOR_FILL_TYPE targetType = COLOR_FILL_TYPE::ALL;
        COLOR_FILL_TYPE blackType = COLOR_FILL_TYPE::ALL;
        switch (i % 2) {
          case 0:
            targetType = COLOR_FILL_TYPE::ODD;
            blackType = COLOR_FILL_TYPE::EVEN;
            break;
          case 1:
            targetType = COLOR_FILL_TYPE::EVEN;
            blackType = COLOR_FILL_TYPE::ODD;
            break;
        }

        // Screen Off
        colorWipe(&head_strip, COLOR_BLACK, 1, true, blackType);
        colorWipe(&cockpit_strip, COLOR_BLACK, 1, false, blackType);

        colorWipe(&head_strip, normalColor, 1, true, targetType);
        colorWipe(&cockpit_strip, normalColor, 1, false, targetType);

        waitMilliseconds(200);
      }

      // Screen Off
      colorWipe(&head_strip, COLOR_BLACK, 1, true, COLOR_FILL_TYPE::ALL);
      colorWipe(&cockpit_strip, COLOR_BLACK, 1, false, COLOR_FILL_TYPE::ALL);

      waitMilliseconds(200);

      colorWipe(&head_strip, normalColor, 1, true, COLOR_FILL_TYPE::ALL);
      colorWipe(&cockpit_strip, normalColor, 1, false, COLOR_FILL_TYPE::ALL);

      break;
    case 3:
      break;
    case 4:
      break;
    case 5:
      // Screen Off
      colorWipe(&head_strip, COLOR_BLACK, 1, true, COLOR_FILL_TYPE::ALL);
      colorWipe(&cockpit_strip, COLOR_BLACK, 1, false, COLOR_FILL_TYPE::ALL);

      waitMilliseconds(200);

      colorWipe(&head_strip, normalColor, 1, true, COLOR_FILL_TYPE::ALL);
      colorWipe(&cockpit_strip, normalColor, 1, false, COLOR_FILL_TYPE::ALL);
      break;
    case 6:
      analogWrite(TOP_PIN, 0);

      head_strip.setBrightness(0);
      cockpit_strip.setBrightness(0);
    case 7:
      break;
    default:
      step = 0;  // Skip Case #0
      break;
  }
}

/**
 * 피닉스 모드
 * 레인보우 효과와 함께 일반 LED들의 밝기도 최대로 한다.
 **/
void phoenix_form(int &step) {
#ifdef _DEBUG
  Serial.println("Process - PHOENIX! - Step#" + String(step));
#endif
  switch (step) {
    case 0:
      head_strip.setBrightness(STRIP_BRIGHT_HIGH);
      cockpit_strip.setBrightness(STRIP_BRIGHT_HIGH);

      colorWipe(&head_strip, normalColor, 1, true);
      colorWipe(&cockpit_strip, normalColor, 1, false);

      dfmp3.playMp3FolderTrack(2);
      waitMilliseconds(100);

      analogWrite(TOP_PIN, 250);
      normal_engine(ENGINE_PIN, TAILSIDE_PIN);
      break;
    case 1:
      analogWrite(TOP_PIN, 250);
      after_burner(ENGINE_PIN, TAILSIDE_PIN, true);
      break;
    case 2:
      head_strip.setBrightness(STRIP_BRIGHT_HIGH);
      cockpit_strip.setBrightness(STRIP_BRIGHT_HIGH);

      for (int i = 0; i < 5; i++) {
        rainbowCycle(&head_strip, 1, true);
        rainbowCycle(&cockpit_strip, 1, false);
      }
      break;
    case 3:
      for (int i = 0; i < 3; i++) {
        blink(&head_strip, &cockpit_strip, 200);
        waitMilliseconds(200);
      }
      break;
    default:
      step = 0;
      break;
  }
}

void no_song_form(int &step) {
#ifdef _DEBUG
  Serial.println("Process - No Song - Step#" + String(step));
#endif
  switch (step) {
    case 0:
      head_strip.setBrightness(STRIP_BRIGHT_NORMAL);
      cockpit_strip.setBrightness(STRIP_BRIGHT_NORMAL);

      colorWipe(&head_strip, normalColor, 1, true);
      colorWipe(&cockpit_strip, normalColor, 1, false);

      analogWrite(TOP_PIN, 50);
      normal_engine(ENGINE_PIN, TAILSIDE_PIN);
      break;
    case 1:
      colorWipe(&head_strip, COLOR_BLACK, 1, true);
      colorWipe(&cockpit_strip, COLOR_BLACK, 1, false);

      waitMilliseconds(200);

      colorWipe(&head_strip, normalColor, 30, true);
      colorWipe(&cockpit_strip, normalColor, 30, false);
      break;
    case 2:
    case 3:
    default:
      step = 0;
      break;
  }
}

int lastVolume = -1;
void timerISR() {
  int val = analogRead(VOLUME_PIN) + 1;
  int volume = map(val, 0, 1023, 0, 30);

  if (lastVolume != volume) {
#ifdef _DEBUG
    Serial.print("Volume = ");
    Serial.println(volume);
#endif
    dfmp3.setVolume(volume);
    lastVolume = volume;
  }
}

//----------------------------------------------------------------------------------
class Mp3Notify {
public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char *action) {
#ifdef _DEBUG
    if (source & DfMp3_PlaySources_Sd) {
      Serial.print("SD Card, ");
    }
    if (source & DfMp3_PlaySources_Usb) {
      Serial.print("USB Disk, ");
    }
    if (source & DfMp3_PlaySources_Flash) {
      Serial.print("Flash, ");
    }
    Serial.println(action);
#endif
  }
  static void OnError(DfMp3 &mp3, uint16_t errorCode) {
#ifdef _DEBUG
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    switch (errorCode) {
      case DfMp3_Error_Busy:
        Serial.println("Busy");
        break;
      case DfMp3_Error_Sleeping:
        Serial.println("Sleeping");
        break;
      case DfMp3_Error_SerialWrongStack:
        Serial.println("Serial Wrong Stack");
        break;

      case DfMp3_Error_RxTimeout:
        Serial.println("Rx Timeout!!!");
        break;
      case DfMp3_Error_PacketSize:
        Serial.println("Wrong Packet Size!!!");
        break;
      case DfMp3_Error_PacketHeader:
        Serial.println("Wrong Packet Header!!!");
        break;
      case DfMp3_Error_PacketChecksum:
        Serial.println("Wrong Packet Checksum!!!");
        break;

      default:
        Serial.println(errorCode, HEX);
        break;
    }
#endif
  }
  static void OnPlayFinished(DfMp3 &mp3, DfMp3_PlaySources source, uint16_t track) {
#ifdef _DEBUG
    Serial.print("Play finished for #");
    Serial.println(track);
#endif
    processor = PROCESSOR::PROCESSOR_NOSONG;
  }
  static void OnPlaySourceOnline(DfMp3 &mp3, DfMp3_PlaySources source) {
    PrintlnSourceAction(source, "online");
  }
  static void OnPlaySourceInserted(DfMp3 &mp3, DfMp3_PlaySources source) {
    PrintlnSourceAction(source, "inserted");
  }
  static void OnPlaySourceRemoved(DfMp3 &mp3, DfMp3_PlaySources source) {
    PrintlnSourceAction(source, "removed");
  }
};