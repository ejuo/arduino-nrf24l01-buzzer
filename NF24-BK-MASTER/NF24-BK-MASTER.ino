#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "D:/1/arduino-nrf24l01-buzzer-master/NF24-BK/NF24BK.h" // change path! Relative path not works in Win.

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

SoftwareSerial mySoftwareSerial(3, 2); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

RF24 radio(9, 10);

#define CLEAR_BTN_PIN 5

byte buzzerIds[] = {1,2, 3, 4, 5, 6, 7, 8, 9}; // list all your existing buzzers client ids (see buzzerId in NF24-BK-BUZZER.ino)

void setup() {
  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  myDFPlayer.volume(30);
  Serial.println(F("DFPlayer Mini online."));
  
  Serial.begin(57600);

  printf_begin();
  printf("*** MASTER ***\n\r");

  initRadio(radio);
  radio.setPALevel(RF24_PA_LOW);			// Set power level for stable connecting (RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX)
  radio.setPayloadSize(4);               
  setPipes(radio, pipes[1], pipes[0]);
  radio.startListening();

  pinMode(CLEAR_BTN_PIN, INPUT_PULLUP);
}

long lastPress = 0;
long currentBuzzer = 0;

void loop(void) {
  if (isClearButtonPressed() && millis() - lastPress > 1000) {
    Serial.print("Clear Button Pressed: ");
    Serial.println(isClearButtonPressed());

    currentBuzzer = 0;

    myDFPlayer.pause();
    
    radio.stopListening();
    int i;
    for (i = 0; i < sizeof(buzzerIds); i++) {
      bk_msg.device = buzzerIds[i];
      bk_msg.cmd = BK_LIGHT_OFF;
      radio.write(&bk_msg, sizeof(bk_msg));
    }
    radio.startListening();

    lastPress = millis();
  }

  if (radio.available()) {
    printf("Receive data ...\n\r");
    radio.read(&bk_msg, sizeof(bk_msg));

    printf("Data received! %2d, %2d\n\r", bk_msg.device, bk_msg.cmd);

    if (currentBuzzer == 0) {
      boolean send = false;
      radio.stopListening();
      delay(150);

      switch (bk_msg.cmd) {
        case BK_BTN_PRESSED:
          printf("try to send it ...\n\r");

          currentBuzzer = bk_msg.device;
          bk_msg.cmd = BK_LIGHT_ON;

          send = radio.write(&bk_msg, sizeof(bk_msg));
          sendSound(currentBuzzer);
          break;
      }

      if (send) {
        printf("Send CMD!\n\r");
      } else {
        printf("Send CMD failed!\n\r");
      }
      setPipes(radio, pipes[1], pipes[0]);
      radio.startListening();
    }
  }
  delay(50);
}

boolean isClearButtonPressed() {
  return digitalRead(CLEAR_BTN_PIN) == LOW;
}

boolean sendCmd(byte pipeNo, byte sw, byte cmd) {
  bk_msg.device = sw;
  bk_msg.cmd = BK_BTN_PRESSED;
  radio.writeAckPayload(pipeNo, &bk_msg, sizeof(bk_msg));
  return true;
  //radio.write(&bk_msg, sizeof(bk_msg));
}

void sendSound(int team) {
  Serial.println("Playing Sound");
  myDFPlayer.play(team);
}

