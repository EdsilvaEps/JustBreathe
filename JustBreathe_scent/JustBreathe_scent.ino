#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "RTClib.h"

#define DF_RX 12   // D6 - DF RX   
#define DF_TX 13   // D7 -  DF TX
#define RELAY_PIN 2 // pin D4/GPIO4
#define BUTTON_PIN  0 // pin D3/GPIO0

#define PIR_PIN  14 // D5 - PIR sensor output
#define POT_PIN A0 // Analog input for potentiometer

SoftwareSerial mySerial(DF_TX, DF_RX);
DFRobotDFPlayerMini player;
RTC_DS1307 rtc; // D1 (GPIO5/SCL) and D2 (GPIO4/SDA)

//-- sound state variables --
bool isPlaying = false;
DateTime playStartTime;

int currentTrack = 1;
const int TOTAL_TRACKS = 5;
const int PLAY_DURATION_SECONDS = 180; // 3 minutes

//-- pump state variables --
bool pumpRunning = false;
unsigned long pumpStartTime = 0;
const unsigned long PUMP_DURATION = 4000; // 4 secs

//-- button interrupt --
volatile bool buttonPressed = false; // flag set by ISR
volatile unsigned long lastISRms = 0;
#define DEBOUNCETIME 300

// Interrupt Service Routine
void IRAM_ATTR handleButtonPress()
{
  unsigned long nowMs = millis();
  // simple debounce: ignored edges within 300ms
  if(nowMs - lastISRms > DEBOUNCETIME)
  {
    buttonPressed = true;
    lastISRms = nowMs;
  }
}

void selectNextTrack() { // move player to next track
  currentTrack = (currentTrack + 1) % TOTAL_TRACKS;
  if(currentTrack == 0) currentTrack++;
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  Wire.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);
  
  digitalWrite(RELAY_PIN, LOW); // relay off

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
    Serial.print("Motion detected → Playing soundtrack ");
    Serial.println(currentTrack);
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

  // -- pump logic --

  // start pump
  if(buttonPressed && !pumpRunning) {
    Serial.println("Pump ON");
    digitalWrite(RELAY_PIN, HIGH); // relay on

    pumpStartTime = millis();
    pumpRunning = true;
    buttonPressed = false;
  }

  // stop pumping after the set time has elapsed 
  if(pumpRunning && millis() - pumpStartTime >= PUMP_DURATION) {
    Serial.println("Pump OFF");
    digitalWrite(RELAY_PIN, LOW); 
    pumpRunning = false;
  }

  if(pumpRunning) {
    Serial.print("pump running for :  ");
    Serial.print(millis());
    Serial.println(" ms");
    if(buttonPressed) buttonPressed = false; // prevent retriggering during pump run
  }
  

  //delay(500); // remove the delay to improve button performance
  delay(10); // small debounce
}
