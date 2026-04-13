#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h> // biblioteka z menedżera "WebSockets" by Links2004
#include <SocketIOclient.h>
#include <ArduinoJson.h>      // biblioteka "ArduinoJson" by Benoit Blanchon
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "mbedtls/base64.h"   // wbudowana biblioteka do dekodowania base64

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

const int speedPWM = 150; // Prędkość jazdy (odpowiednik testDriveSpeed)
const int freq = 5000;
const int resolution = 8;

const int kickSpeed = 255;  
const int holdSpeed = 100;  
const int kickTime = 150;

// Ekran I2C (SDA=27, SCL=14)
#define I2C_SDA 27
#define I2C_SCL 14
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Głośnik (PAM8403) — pin 25 to DAC1 na ESP32
#define AUDIO_PIN 25

// --- Bufor audio i odtwarzanie TTS ---
#define AUDIO_BUFFER_SIZE 32000  // Max ~4 sekundy audio (8000 Hz * 4s)
volatile uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
volatile size_t audioLength = 0;
volatile size_t audioIndex = 0;
volatile bool audioPlaying = false;
hw_timer_t *audioTimer = NULL;

// Tabela dekodowania μ-law → 16-bit signed PCM (skompresowana do 8-bit unsigned dla DAC)
// μ-law to standardowy format kompresji audio (ITU-T G.711)
static const int16_t ulaw_decode_table[256] = {
    -32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956,
    -23932,-22908,-21884,-20860,-19836,-18812,-17788,-16764,
    -15996,-15484,-14972,-14460,-13948,-13436,-12924,-12412,
    -11900,-11388,-10876,-10364, -9852, -9340, -8828, -8316,
     -7932, -7676, -7420, -7164, -6908, -6652, -6396, -6140,
     -5884, -5628, -5372, -5116, -4860, -4604, -4348, -4092,
     -3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004,
     -2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980,
     -1884, -1820, -1756, -1692, -1628, -1564, -1500, -1436,
     -1372, -1308, -1244, -1180, -1116, -1052,  -988,  -924,
      -876,  -844,  -812,  -780,  -748,  -716,  -684,  -652,
      -620,  -588,  -556,  -524,  -492,  -460,  -428,  -396,
      -372,  -356,  -340,  -324,  -308,  -292,  -276,  -260,
      -244,  -228,  -212,  -196,  -180,  -164,  -148,  -132,
      -120,  -112,  -104,   -96,   -88,   -80,   -72,   -64,
       -56,   -48,   -40,   -32,   -24,   -16,    -8,     0,
     32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956,
     23932, 22908, 21884, 20860, 19836, 18812, 17788, 16764,
     15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412,
     11900, 11388, 10876, 10364,  9852,  9340,  8828,  8316,
      7932,  7676,  7420,  7164,  6908,  6652,  6396,  6140,
      5884,  5628,  5372,  5116,  4860,  4604,  4348,  4092,
      3900,  3772,  3644,  3516,  3388,  3260,  3132,  3004,
      2876,  2748,  2620,  2492,  2364,  2236,  2108,  1980,
      1884,  1820,  1756,  1692,  1628,  1564,  1500,  1436,
      1372,  1308,  1244,  1180,  1116,  1052,   988,   924,
       876,   844,   812,   780,   748,   716,   684,   652,
       620,   588,   556,   524,   492,   460,   428,   396,
       372,   356,   340,   324,   308,   292,   276,   260,
       244,   228,   212,   196,   180,   164,   148,   132,
       120,   112,   104,    96,    88,    80,    72,    64,
        56,    48,    40,    32,    24,    16,     8,     0
};

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

// --- Przerwanie timera do odtwarzania audio (8000 Hz) ---
void IRAM_ATTR onAudioTimer() {
    if (audioPlaying && audioIndex < audioLength) {
        // Dekoduj μ-law sample do 16-bit, potem przeskaluj do 8-bit unsigned (0-255) dla DAC
        int16_t pcm16 = ulaw_decode_table[audioBuffer[audioIndex]];
        // Przeskaluj z zakresu -32124..32124 do 0..255
        uint8_t dacValue = (uint8_t)((pcm16 + 32124) * 255 / 64248);
        dacWrite(AUDIO_PIN, dacValue);
        audioIndex++;
    } else if (audioPlaying && audioIndex >= audioLength) {
        audioPlaying = false;
        dacWrite(AUDIO_PIN, 128); // Cisza (środek zakresu DAC)
    }
}

// Dekoduj base64 i załaduj audio do bufora
void playAudioFromBase64(const char* b64Data, size_t b64Len) {
    // Zatrzymaj bieżące odtwarzanie
    audioPlaying = false;
    audioIndex = 0;
    audioLength = 0;
    
    // Bufor tymczasowy na zdekodowane dane
    size_t decodedLen = 0;
    
    // Oblicz wymagany rozmiar bufora
    int ret = mbedtls_base64_decode(
        NULL, 0, &decodedLen,
        (const unsigned char*)b64Data, b64Len
    );
    
    if (decodedLen > AUDIO_BUFFER_SIZE) {
        Serial.printf("Audio za duże! %d > %d bajtów. Przycinam.\n", decodedLen, AUDIO_BUFFER_SIZE);
        decodedLen = AUDIO_BUFFER_SIZE;
    }
    
    // Dekoduj base64 do bufora audio
    size_t actualLen = 0;
    ret = mbedtls_base64_decode(
        (unsigned char*)audioBuffer, AUDIO_BUFFER_SIZE, &actualLen,
        (const unsigned char*)b64Data, b64Len
    );
    
    if (ret != 0) {
        Serial.printf("Błąd dekodowania base64: %d\n", ret);
        return;
    }
    
    Serial.printf("Audio zdekodowane: %d bajtów (%.1f sek)\n", actualLen, actualLen / 8000.0);
    audioLength = actualLen;
    audioIndex = 0;
    audioPlaying = true;
}

// --- Funkcje ruchu ---
void wyprostujKola() {
    ledcWrite(ENB, 0);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
}

void stopMotors() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, 0);
    wyprostujKola();
    isMoving = false;
    isTurning = false;
}

void moveForward(int durationMs) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, speedPWM);
    
    // Ustawienie blokady skrętu do zera żeby jechał prosto
    wyprostujKola();

    actionEndTime = millis() + durationMs;
    isMoving = true;
    isTurning = false;
}

void moveBackward(int durationMs) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    ledcWrite(ENA, speedPWM);

    wyprostujKola();

    actionEndTime = millis() + durationMs;
    isMoving = true;
    isTurning = false;
}

void turnLeft(int durationMs) {
    // Skręt (silnik przedni w lewo)
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWrite(ENB, kickSpeed);
    delay(kickTime);
    ledcWrite(ENB, holdSpeed);
    
    // W trakcie jazdy skręca: jedzie do przodu
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, speedPWM);

    actionEndTime = millis() + (durationMs > kickTime ? durationMs - kickTime : 0);
    isMoving = true;
    isTurning = true;
}

void turnRight(int durationMs) {
    // Skręt (silnik przedni w prawo)
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWrite(ENB, kickSpeed);
    delay(kickTime);
    ledcWrite(ENB, holdSpeed);
    
    // W trakcie jazdy skręca: jedzie do przodu
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, speedPWM);

    actionEndTime = millis() + (durationMs > kickTime ? durationMs - kickTime : 0);
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
            
            DynamicJsonDocument doc(48000); // Duży bufor na dane audio base64
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
                // Beep nie jest potrzebny — zaraz odtworzy się TTS
            }
            else if(eventName == "audio_response") {
                // Odtwarzanie audio TTS z ElevenLabs
                const char* audioB64 = data["audio"];
                if (audioB64) {
                    size_t b64Len = strlen(audioB64);
                    Serial.printf("Odebrano audio TTS: %d znaków base64\n", b64Len);
                    updateDisplay("Mowie...");
                    playAudioFromBase64(audioB64, b64Len);
                }
            }
            break;
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Konfiguracja pinów mostka L298N
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    ledcAttach(ENA, freq, resolution);
    ledcAttach(ENB, freq, resolution);
    stopMotors();

    // Konfiguracja pinu audio (DAC)
    pinMode(AUDIO_PIN, OUTPUT);
    dacWrite(AUDIO_PIN, 128); // Cisza (środek zakresu)

    // Inicjalizacja timera sprzętowego do odtwarzania audio (8000 Hz)
    audioTimer = timerBegin(1000000); // Timer z bazą 1 MHz
    timerAttachInterrupt(audioTimer, &onAudioTimer);
    timerAlarm(audioTimer, 125, true, 0); // 1000000/8000 = 125 μs = 8kHz, auto-reload

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
