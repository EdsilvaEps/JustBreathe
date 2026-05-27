#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define D1 5
#define D2 4

// Define RX and TX pins
SoftwareSerial mySerial(D2, D1); // RX, TX (ESP perspective)

DFRobotDFPlayerMini player;

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  Serial.println("Initializing DFPlayer...");

  if (!player.begin(mySerial)) {
    Serial.println("DFPlayer Mini not detected!");
    while (true);
  }

  Serial.println("DFPlayer ready.");

  player.volume(25);  // Volume: 0–30
  player.play(1);     // Play first MP3 (0001.mp3)
}

void loop() {
  // Example: play track every 10 seconds
  delay(30000);
  player.next();  // play next track
}