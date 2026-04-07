#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h> // biblioteka z menedżera "WebSockets" by Links2004
#include <SocketIOclient.h>
#include <ArduinoJson.h>      // biblioteka "ArduinoJson" by Benoit Blanchon
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Ustawienia WiFi i serwera
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";

const char* server_ip = "192.168.1.100";  // Zmień na lokalne IP komputera na którym działa serwer (sprawdź przez 'ipconfig')
const uint16_t server_port = 5000;

SocketIOclient socketIO;

// --- Piny i ustawienia z dokumentacji ---
// Mostek L298N
#define ENA 13
#define IN1 23
#define IN2 4

#define ENB 15
#define IN3 33
#define IN4 32

const int speedPWM = 200; // UWAGA TUTAJ!!!!!

// Ekran I2C (SDA=27, SCL=14)
#define I2C_SDA 27
#define I2C_SCL 14
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Głośnik (PAM8403)
#define AUDIO_PIN 25

unsigned long actionEndTime = 0;
bool isMoving = false;
bool isTurning = false;

// --- Funkcje pomocnicze ---
void updateDisplay(String text) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("--- ROBOT ---");
    display.println(text);
    display.display();
    Serial.println(text);
}

void playBeep() {
    // Prosty ciągły dźwięk pipnięcia potwierdzający (1000 Hz przez 200 ms)
    #if defined(ESP32) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
        tone(AUDIO_PIN, 1000, 200); 
    #else
        // Ręczne wygenerowanie fali prostokątnej (jeśli starsza wersja bibliotek)
        for(int i = 0; i < 200; i++) {
            digitalWrite(AUDIO_PIN, HIGH);
            delayMicroseconds(500);
            digitalWrite(AUDIO_PIN, LOW);
            delayMicroseconds(500);
        }
    #endif
}

// --- Funkcje ruchu ---
void stopMotors() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    isMoving = false;
    isTurning = false;
}

void moveForward(int durationMs) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, speedPWM);
    
    // Ustawienie blokady skrętu do zera żeby jechał prosto
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);

    actionEndTime = millis() + durationMs;
    isMoving = true;
    isTurning = false;
}

void moveBackward(int durationMs) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, speedPWM);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);

    actionEndTime = millis() + durationMs;
    isMoving = true;
    isTurning = false;
}

void turnLeft(int durationMs) {
    // Skręt (silnik przedni)
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, speedPWM);
    
    // W trakcie jazdy skręca: jedzie do przodu na silniku skrętu
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, speedPWM);

    actionEndTime = millis() + durationMs;
    isMoving = true;
    isTurning = true;
}

void turnRight(int durationMs) {
    // Skręt (silnik przedni w drugą stronę)
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, speedPWM);
    
    // W trakcie jazdy skręca
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, speedPWM);

    actionEndTime = millis() + durationMs;
    isMoving = true;
    isTurning = true;
}

// --- Obsługa WebSockets / SocketIO ---
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            Serial.printf("[IOc] Odłączono od serwera!\n");
            updateDisplay("Odłączono od API");
            break;

        case sIOtype_CONNECT:
            Serial.printf("[IOc] Połączono z %s\n", payload);
            updateDisplay("Polaczono z serwerem");
            // Połączenie za pomocą domyślnej przestrzeni nazw (Namespace)
            socketIO.send(sIOtype_CONNECT, "/");
            break;

        case sIOtype_EVENT: {
            String payloadStr = (char*)payload;
            Serial.printf("[IOc] Event: %s\n", payloadStr.c_str());
            
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payloadStr);
            if(error) {
                Serial.print(F("Błąd podczas parsowania JSON: "));
                Serial.println(error.f_str());
                return;
            }

            String eventName = doc[0];
            JsonObject data = doc[1];

            if(eventName == "robot_command") {
                String cmd = data["command"]; 
                int val = data["value"];
                if (val == 0) val = 1; // Domyslnie np krok wartosc=1
                
                String msg = "Komenda: " + cmd + "\nWartosc: " + String(val);
                updateDisplay(msg);
                playBeep();
                
                int duration = val * 1000; // Na ten moment: 1 jednostka to 1 sekunda jazdy

                if(cmd == "przód") moveForward(duration);
                else if(cmd == "tył") moveBackward(duration);
                else if(cmd == "lewo") turnLeft(duration);
                else if(cmd == "prawo") turnRight(duration);
                else stopMotors();
            }
            else if(eventName == "chat_response") {
                String text = data["response"];
                updateDisplay("AI:\n" + text);
                playBeep();
            }
            break;
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Konfiguracja pinów mostka L298N
    pinMode(ENA, OUTPUT);
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENB, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    stopMotors();

    // Konfiguracja pinu audio
    pinMode(AUDIO_PIN, OUTPUT);
    digitalWrite(AUDIO_PIN, LOW);

    // Inicjalizacja ekranu OLED (podanie własnych pinów I2C)
    Wire.begin(I2C_SDA, I2C_SCL);
    // Ustawiono adres 0x3C, który jest najpowszechniejszy
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println(F("Nie znaleziono ekranu OLED (SSD1306)"));
    } else {
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.cp437(true); // by pomóc w czcionkach
        updateDisplay("URUCHAMIANIE...");
    }

    // Łączenie z Wi-Fi
    WiFi.begin(ssid, password);
    String dots = "";
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        dots += ".";
        if(dots.length() > 5) dots = ".";
        updateDisplay("Wi-Fi...\n" + dots);
    }

    String connectedInfo = "WiFi Polaczone!\nIP: " + WiFi.localIP().toString();
    updateDisplay(connectedInfo);
    delay(2000);

    // Inicjalizacja webSocket client 
    // Format linku dla Flask-SocketIO wersji 4: /socket.io/?EIO=4
    socketIO.begin(server_ip, server_port, "/socket.io/?EIO=4");
    socketIO.onEvent(socketIOEvent);
}

void loop() {
    socketIO.loop();

    // Automatyczne zatrzymywanie silników
    if (isMoving && millis() >= actionEndTime) {
        stopMotors();
    }
}
