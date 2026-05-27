
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "RTClib.h"

#define DF_RX 12   // D6 - ESP RX (to DF TX)  
#define DF_TX 13   // D7 - ESP TX (to DF RX) 

#define PIR_PIN  14 // D5 - PIR sensor output
#define POT_PIN A0 // Analog input for potentiometer

SoftwareSerial mySerial(DF_TX, DF_RX);
DFRobotDFPlayerMini player;
RTC_DS1307 rtc;

bool isPlaying = false;
DateTime playStartTime;

int currentTrack = 1;
const int TOTAL_TRACKS = 7;
const int PLAY_DURATION_SECONDS = 180; // 3 minutes


void selectNextTrack() { // move player to next track
  currentTrack = (currentTrack + 1) % TOTAL_TRACKS;
  if(currentTrack == 0) currentTrack++;
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  Wire.begin();

  pinMode(PIR_PIN, INPUT);

  //-- RTC init --
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1) {
      delay(10);
    }
  }
  else {
    Serial.println("RTC connected");
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, setting the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  else {
    Serial.println("RTC is running");
  }

  //----------

  Serial.println("Initializing DFPlayer...");

  if (!player.begin(mySerial)) {
    Serial.println("DFPlayer Mini not detected!");
    while (true);
  }


  Serial.println("DFPlayer ready.");

  player.volume(20);  // initial volume
}

void loop() {

  DateTime now = rtc.now();

  // ---- Read PIR sensor ----
  int motion = digitalRead(PIR_PIN);

  // ---- Read potentiometer (0–1023) and map to volume (0–30) ----
  int potValue = analogRead(POT_PIN);
  int volume = map(potValue, 0, 1023, 0, 30);

  player.volume(volume);

  Serial.print("Time: ");
  Serial.print(now.timestamp());
  Serial.print(" | Motion: ");
  Serial.print(motion);
  Serial.print(" | Volume: ");
  Serial.println(volume);

  // ---- Motion logic ----
  if (motion == HIGH && !isPlaying) {
    Serial.println("Motion detected → Playing sound");
    player.play(currentTrack);  // play currently selected track

    playStartTime = now; // store start time
    isPlaying = true;
    
  } 

  // -- if currently playing, check elapsed time --
  if (isPlaying) {
    TimeSpan elapsed = now - playStartTime;

    if(elapsed.totalseconds() >= PLAY_DURATION_SECONDS){
      Serial.println("time elapsed, stop playback");
      player.stop();
      selectNextTrack(); // move player selector to next track
      isPlaying = false;
    }
  }
  

  delay(500);
}
