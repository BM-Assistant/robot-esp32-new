"""
Test ElevenLabs TTS — generuje audio i odtwarza na komputerze.
Użycie: python test_elevenlabs.py
"""

import os
import base64
import wave
import struct
from dotenv import load_dotenv
from elevenlabs.client import ElevenLabs

load_dotenv()

API_KEY = os.getenv("ELEVEN_LABS_API_KEY")
if not API_KEY:
    print("❌ Brak ELEVEN_LABS_API_KEY w .env!")
    exit(1)

print(f"🔑 Klucz API: {API_KEY[:10]}...")

# Inicjalizacja klienta
client = ElevenLabs(api_key=API_KEY)
print("✅ Klient ElevenLabs zainicjalizowany")

# Tabela dekodowania μ-law (taka sama jak na ESP32)
ULAW_DECODE = [
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
        56,    48,    40,    32,    24,    16,     8,     0,
]


def test_tts(text):
    """Generuje TTS, zapisuje jako WAV i wyświetla statystyki."""
    print(f"\n🎤 Generuję TTS: \"{text}\"")
    
    # Generowanie audio (ulaw_8000 — taki sam format jak dla robota)
    audio_gen = client.text_to_speech.convert(
        text=text,
        voice_id="JBFqnCBsd6RMkjVDRZzb",  # George
        model_id="eleven_multilingual_v2",
        output_format="ulaw_8000"
    )
    
    # Zbierz chunki
    audio_bytes = b""
    for chunk in audio_gen:
        audio_bytes += chunk
    
    # Statystyki
    duration = len(audio_bytes) / 8000
    b64 = base64.b64encode(audio_bytes).decode('utf-8')
    print(f"📊 Raw audio:  {len(audio_bytes)} bajtów ({duration:.1f} sek)")
    print(f"📊 Base64:     {len(b64)} znaków")
    print(f"📊 Znaków tekstu: {len(text)} (koszt ElevenLabs)")
    
    # Dekoduj μ-law do PCM i zapisz jako WAV (do odsłuchu na komputerze)
    pcm_samples = []
    for byte in audio_bytes:
        pcm_samples.append(ULAW_DECODE[byte])
    
    wav_filename = "test_tts_output.wav"
    with wave.open(wav_filename, 'w') as wav:
        wav.setnchannels(1)       # mono
        wav.setsampwidth(2)       # 16-bit
        wav.setframerate(8000)    # 8kHz
        for sample in pcm_samples:
            wav.writeframes(struct.pack('<h', sample))
    
    print(f"💾 Zapisano: {wav_filename}")
    
    # Sprawdź czy mieści się w buforze ESP32
    if len(audio_bytes) <= 32000:
        print(f"✅ Mieści się w buforze ESP32 (32KB)")
    else:
        print(f"⚠️  Za duże dla ESP32! ({len(audio_bytes)} > 32000)")
    
    return wav_filename


if __name__ == "__main__":
    print("=" * 50)
    print("   TEST ELEVENLABS TTS DLA ROBOTA")
    print("=" * 50)
    
    # Test 1: Krótka komenda (strzałki)
    test_tts("Do przodu")
    
    # Test 2: Potwierdzenie komendy
    test_tts("Jadę do przodu 2 metry")
    
    # Test 3: Odpowiedź na pytanie (chat)
    test_tts("Jestem robotem łazikiem. Nie posiadam czujnika temperatury.")
    
    print("\n" + "=" * 50)
    print("🎧 Otwórz test_tts_output.wav żeby odsłuchać ostatni test!")
    print("=" * 50)
