#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27,16,2);

// BUTTONS
int powerButton = 2;
int retryButton = 3;
int topScoreButton = 4;

int gameButtons[3] = {5,6,7};

// LEDS
int leds[3] = {A1, A2, A3};

// BUZZER
int buzzer = 8;

// SOUND SENSOR
int soundSensor = A0;
int soundThreshold = 500;

// GAME STATE
bool systemOn = false;
bool gameReady = false;
bool gameRunning = false;
bool gameOver = false;

int score = 0;
int topScore = 0;

unsigned long lastActionTime = 0;

// track last LED
int lastTarget = -1;

// ================= LED RESET =================
void turnOffAllLEDs(){
  for(int i=0; i<3; i++){
    digitalWrite(leds[i], LOW);
  }
}

// ================= BUZZER =================
void beep(int freq, int dur){
  tone(buzzer, freq, dur);
}

void clickSound(){
  beep(1200, 60);
}

void startSound(){
  beep(1200,100);
  delay(120);
  beep(1500,120);
}

void gameOverSound(){
  beep(500,300);
}

// ================= RESET =================
void resetSystem(){
  systemOn = false;
  gameReady = false;
  gameRunning = false;
  gameOver = false;
  score = 0;

  turnOffAllLEDs();

  lcd.clear();
  lcd.print("System OFF");
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  randomSeed(analogRead(A5));

  lcd.init();
  lcd.backlight();

  pinMode(powerButton, INPUT_PULLUP);
  pinMode(retryButton, INPUT_PULLUP);
  pinMode(topScoreButton, INPUT_PULLUP);

  for(int i=0;i<3;i++){
    pinMode(gameButtons[i], INPUT_PULLUP);
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }

  pinMode(buzzer, OUTPUT);
  pinMode(soundSensor, INPUT);

  lcd.print("Hit n Score Ready!");
  lcd.setCursor(0,1);
  lcd.print("Press Start");
}

// ================= LOOP =================
void loop() {

  // POWER BUTTON
  if(digitalRead(powerButton) == LOW){
    delay(200);
    clickSound();

    if(systemOn){
      resetSystem();
      return;
    } else {
      systemOn = true;
      lcd.clear();
      lcd.print("Scan RFID card");
      beep(1200,150);
    }
  }

  if(!systemOn) return;

  // RFID
  if(!gameReady){
    if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
      gameReady = true;

      beep(1500,150); // RFID SOUND

      lcd.clear();
      lcd.print("Press Start!");
    }
    return;
  }

  // TOP SCORE
  if(digitalRead(topScoreButton) == LOW){
    delay(150);
    clickSound();

    lcd.clear();
    lcd.print("TOP SCORE:");
    lcd.setCursor(0,1);
    lcd.print(topScore);

    delay(1500);

    lcd.clear();
    lcd.print("Ready");
    return;
  }

  // RETRY
  if(gameOver){
    lcd.setCursor(0,0);
    lcd.print("Try again?");
    lcd.setCursor(0,1);
    lcd.print("Press Retry");

    if(digitalRead(retryButton) == LOW){
      delay(200);
      clickSound();

      turnOffAllLEDs();

      gameOver = false;
      gameReady = false;
      score = 0;

      lcd.clear();
      lcd.print("Scan RFID card");
    }
    return;
  }

  // START GAME
  if(gameReady && !gameRunning){
    lcd.setCursor(0,0);
    lcd.print("Press Start!");

    if(digitalRead(powerButton) == LOW){
      delay(200);
      clickSound();

      gameRunning = true;
      score = 0;

      startSound();

      startGame();
    }
    return;
  }
}

// ================= GAME =================
void startGame(){

  lcd.clear();
  lcd.print("Game Start!");
  delay(800);

  while(gameRunning){

    int speed = 1500 - (score * 20);

    // 🔥 DOUBLE SPEED when score >= 20
    if(score >= 20){
      speed = speed / 2;
    }

    if(speed < 200) speed = 200;

    int target;
    do {
      target = random(0,3);
    } while(target == lastTarget);

    lastTarget = target;

    turnOffAllLEDs();
    digitalWrite(leds[target], HIGH);

    lcd.setCursor(0,1);
    lcd.print("Score:");
    lcd.print(score);
    lcd.print("   ");

    lastActionTime = millis();
    bool hit = false;

    while(millis() - lastActionTime < speed){

      // STOP GAME
      if(digitalRead(powerButton) == LOW){
        delay(200);
        clickSound();

        turnOffAllLEDs();

        gameRunning = false;
        gameOver = true;

        gameOverSound();
        return;
      }

      // WRONG BUTTON
      for(int i=0;i<3;i++){
        if(i != target && digitalRead(gameButtons[i]) == LOW){
          delay(30);

          if(digitalRead(gameButtons[i]) == LOW){

            clickSound();

            lcd.clear();
            lcd.print("WRONG BUTTON!");

            beep(400,200);
            delay(500);

            turnOffAllLEDs();

            gameRunning = false;
            gameOver = true;

            gameOverSound();
            return;
          }
        }
      }

      // CORRECT BUTTON
      if(digitalRead(gameButtons[target]) == LOW){
        delay(50);

        if(digitalRead(gameButtons[target]) == LOW){

          clickSound();

          int soundValue = analogRead(soundSensor);

          if(soundValue > soundThreshold){
            beep(1400,50);
          } else {
            beep(1000,80);
          }

          score++;

          if(score > topScore){
            topScore = score;
          }

          hit = true;

          while(digitalRead(gameButtons[target]) == LOW);

          turnOffAllLEDs();
          break;
        }
      }
    }

    turnOffAllLEDs();

    // MISS
    if(!hit){
      gameRunning = false;
      gameOver = true;

      gameOverSound();
      return;
    }
  }
}