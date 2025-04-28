#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD Address (usually 0x27 or 0x3F) Look for LCD documentation
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Definitions in arduino
const int buttonPin = 2;
const int waterFloatPin = 3;
const int soapFloatPin = 4;
const int sanitizerFloatPin = 5;
const int waterPumpPin = 8;
const int soapPumpPin = 9;
const int sanitizerPumpPin = 10;
const int fan1Pin = 11;
const int fan2Pin = 12;

// Time Durations (in milliseconds)
const unsigned long waterSprayTime1 = 5000; // Initial water spray
const unsigned long soapSprayTime = 5000; // Soap Spray
const unsigned long waterSprayTime2 = 5000; // Water rinse after soap
const unsigned long sanitizerSprayTime = 5000; // Sanitizer Spray
const unsigned long fanRunTime = 600000; // 10 minutes (adjust if needed)
const unsigned long sprayDelay = 3000; // 3 seconds delay between sprays

// Variables
enum CleaningState { IDLE, WATER1, SOAP_DELAY, SOAP, WATER2_DELAY, WATER2, SANITIZER_DELAY, SANITIZER, DRYING, COMPLETE, LOW_LIQUID };
CleaningState currentCleaningState = IDLE;
unsigned long startTime = 0;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  displayLoadingAnimation(); // Show loading animation

  displayReadyMessage();

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(waterFloatPin, INPUT_PULLUP);
  pinMode(soapFloatPin, INPUT_PULLUP);
  pinMode(sanitizerFloatPin, INPUT_PULLUP);
  pinMode(waterPumpPin, OUTPUT);
  pinMode(soapPumpPin, OUTPUT);
  pinMode(sanitizerPumpPin, OUTPUT);
  pinMode(fan1Pin, OUTPUT);
  pinMode(fan2Pin, OUTPUT);

  // Turn off all pumps and fans initially (HIGH is off for active LOW relays)
  digitalWrite(waterPumpPin, HIGH);
  digitalWrite(soapPumpPin, HIGH);
  digitalWrite(sanitizerPumpPin, HIGH);
  digitalWrite(fan1Pin, HIGH);
  digitalWrite(fan2Pin, HIGH);
}

void loop() {
  if (digitalRead(buttonPin) == LOW) {

    
    delay(50); // Debounce
    if (digitalRead(buttonPin) == LOW && currentCleaningState == IDLE) {
      displayPrepairingAnimation();
      startCleaningCycle();
    }
  }

  runCleaningCycle();
}

void displayLoadingAnimation() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("");

  for (int i = 0; i < 3; i++) {
    lcd.print(".");
    delay(500); // Adjust delay for animation speed
  }
  delay(500); // Short delay after dots
}

void displayPrepairingAnimation() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Prepairing");

  for (int i = 0; i < 3; i++) {
    lcd.print(".");
    delay(500); // Adjust delay for animation speed
  }
  delay(500); // Short delay after dots
}

void displayReadyMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Ready to Use ");
  lcd.setCursor(0, 1);
  lcd.print("  The System! ");
}

void startCleaningCycle() {
  if (digitalRead(waterFloatPin) == LOW ||
      digitalRead(soapFloatPin) == LOW ||
      digitalRead(sanitizerFloatPin) == LOW) {
    currentCleaningState = LOW_LIQUID;
    displayLowLiquidMessage();
    return;
  }

  currentCleaningState = WATER1;
  startTime = millis();
  lcd.clear();
}

void displayLowLiquidMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Low Liquid Level!");
  lcd.setCursor(0, 1);
  if (digitalRead(waterFloatPin) == LOW) lcd.print("WATER! ");
  if (digitalRead(soapFloatPin) == LOW) lcd.print("SOAP! ");
  if (digitalRead(sanitizerFloatPin) == LOW) lcd.print("SANITIZER!");
  delay(3000);
  displayReadyMessage();
  currentCleaningState = IDLE; // Reset state
}

void runCleaningCycle() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - startTime;

  switch (currentCleaningState) {
    case WATER1:
      lcd.setCursor(0, 0);
      lcd.print("Spraying Water...");
      digitalWrite(waterPumpPin, LOW);
      if (elapsedTime >= waterSprayTime1) {
        digitalWrite(waterPumpPin, HIGH);
        currentCleaningState = SOAP_DELAY;
        startTime = millis(); // Reset timer for next stage
      }
      break;

    case SOAP_DELAY:
      lcd.setCursor(0, 0);
      lcd.print("Wait....");
      if(elapsedTime >= sprayDelay){
        currentCleaningState = SOAP;
        startTime = millis();
      }
      break;

    case SOAP:
      lcd.setCursor(0, 0);
      lcd.print("Spraying Soap...");
      digitalWrite(soapPumpPin, LOW);
      if (elapsedTime >= soapSprayTime) {
        digitalWrite(soapPumpPin, HIGH);
        currentCleaningState = WATER2_DELAY;
        startTime = millis();
      }
      break;

    case WATER2_DELAY:
      lcd.setCursor(0, 0);
      lcd.print("Wait....");
      if(elapsedTime >= sprayDelay){
        currentCleaningState = WATER2;
        startTime = millis();
      }
      break;

    case WATER2:
      lcd.setCursor(0, 0);
      lcd.print("Rinsing..");
      digitalWrite(waterPumpPin, LOW);
      if (elapsedTime >= waterSprayTime2) {
        digitalWrite(waterPumpPin, HIGH);
        currentCleaningState = SANITIZER_DELAY;
        startTime = millis();
      }
      break;
    case SANITIZER_DELAY:
      lcd.setCursor(0, 0);
      lcd.print("Wait....");
      if(elapsedTime >= sprayDelay){
        currentCleaningState = SANITIZER;
        startTime = millis();
      }
      break;

    case SANITIZER:
      lcd.setCursor(0, 0);
      lcd.print("Spraying Sntzr.");
      digitalWrite(sanitizerPumpPin, LOW);
      if (elapsedTime >= sanitizerSprayTime) {
        digitalWrite(sanitizerPumpPin, HIGH);
        currentCleaningState = DRYING;
        startTime = millis();
      }
      break;

    case DRYING:
      lcd.setCursor(0, 0);
      lcd.print("Drying...");
      lcd.setCursor(0, 1);
      long timeLeft = fanRunTime - elapsedTime; // Note: signed long

      if (timeLeft <= 0) {
        digitalWrite(fan1Pin, HIGH); // Turn off fans
        digitalWrite(fan2Pin, HIGH);
        currentCleaningState = COMPLETE;
        displayCleaningComplete();
      } else {
        lcd.print("Time Left: ");
        unsigned long minutes = abs(timeLeft) / 60000; // Use abs() to get positive value
        unsigned long seconds = (abs(timeLeft) % 60000) / 1000;

        // Pad with leading zeros if needed
        if (minutes < 10) {
          lcd.print("0");
        }
        lcd.print(minutes);
        lcd.print("min");

        if (seconds < 10) {
          lcd.print("0");
        }
        lcd.print(seconds);
        lcd.print("sec");

        digitalWrite(fan1Pin, LOW);
        digitalWrite(fan2Pin, LOW);
      }
      break;

    case COMPLETE:
      break;

    case LOW_LIQUID:
      displayLowLiquidMessage();
      break;

    case IDLE:
// Do nothing, waiting for button press
      break;
  }
}

void displayCleaningComplete() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CleaningTrashcan");
  lcd.setCursor(0, 1);
  lcd.print("Complete!");
  delay(3000); // Show message for 3 seconds
  lcd.clear();
  displayReadyMessage(); // Show ready message
  currentCleaningState = IDLE; // Go back to IDLE state
}
