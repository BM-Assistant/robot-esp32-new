#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Piny i ustawienia (zgodne z robot/robot.ino) ---
#define ENA 13
#define IN1 23
#define IN2 4

#define ENB 15
#define IN3 33
#define IN4 32

const int speedPWM = 200;

// Ekran I2C (SDA=27, SCL=14)
#define I2C_SDA 27
#define I2C_SCL 14
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Głośnik
#define AUDIO_PIN 25

// Pomocnicze funkcje
void updateDisplay(const String &text) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("--- TEST ---");
    display.println(text);
    display.display();
    Serial.println(text);
}

void playBeep() {
    #if defined(ESP32) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
        tone(AUDIO_PIN, 1000, 200);
    #else
        // fallback: krótki ręczny sygnał
        for(int i = 0; i < 200; i++) {
            digitalWrite(AUDIO_PIN, HIGH);
            delayMicroseconds(500);
            digitalWrite(AUDIO_PIN, LOW);
            delayMicroseconds(500);
        }
    #endif
}

void stopMotors() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
}

void moveForward(int durationMs) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, speedPWM);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);

    delay(durationMs);
    stopMotors();
}

void moveBackward(int durationMs) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, speedPWM);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);

    delay(durationMs);
    stopMotors();
}

void turnLeft(int durationMs) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, speedPWM);

    // jedzie lekko do przodu podczas skrętu
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, speedPWM);

    delay(durationMs);
    stopMotors();
}

void turnRight(int durationMs) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, speedPWM);

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, speedPWM);

    delay(durationMs);
    stopMotors();
}

void setup() {
    Serial.begin(115200);

    // Piny mostka
    pinMode(ENA, OUTPUT);
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENB, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    stopMotors();

    // Audio
    pinMode(AUDIO_PIN, OUTPUT);
    digitalWrite(AUDIO_PIN, LOW);

    // Ekran
    Wire.begin(I2C_SDA, I2C_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Nie znaleziono ekranu OLED (SSD1306)"));
    } else {
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.cp437(true);
    }

    // Pokaz testowy
    updateDisplay("dziala");
    playBeep();
    delay(500);
}

void loop() {
    // Sekwencja testowa: przód, tył, lewo, prawo
    updateDisplay("Jadę przód");
    moveForward(1000); // 1s
    delay(300);

    updateDisplay("Jadę tył");
    moveBackward(1000);
    delay(300);

    updateDisplay("Skręt w lewo");
    turnLeft(800);
    delay(300);

    updateDisplay("Skręt w prawo");
    turnRight(800);
    delay(300);

    updateDisplay("Koniec cyklu");
    playBeep();
    delay(1500); // chwila przerwy przed kolejnym cyklem
}
