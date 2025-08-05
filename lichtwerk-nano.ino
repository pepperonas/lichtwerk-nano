#include <FastLED.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>
#include <FlashStorage.h>
#include "secrets.h"

// LED Konfiguration - deine bewährte Config
#define LED_PIN     5       
#define NUM_LEDS    600     
#define BRIGHTNESS  255     
#define LED_TYPE    WS2812B 
#define COLOR_ORDER GRB

// Farbkorrektur für zweite Strip-Hälfte (LEDs 300-599)
// Feinabstimmung auch mit Dual-Power Setup
#define SECOND_HALF_START 300
#define CORRECTION_RED_FACTOR   0.90    // Leichte Rot-Korrektur
#define CORRECTION_GREEN_FACTOR 0.85    // Grün etwas mehr reduzieren
#define CORRECTION_BLUE_FACTOR  1.0     // Blau unverändert     

CRGB leds[NUM_LEDS];

// WiFi Config
char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASSWORD;
WiFiServer server(80);

// BLE Config
BLEService ledService("12345678-1234-1234-1234-123456789abc");
BLEStringCharacteristic ledCommand("87654321-4321-4321-4321-cba987654321", BLERead | BLEWrite, 50);
bool bleEnabled = false;

// Effekt-Parameter Struktur
struct EffectConfig {
  uint8_t speed = 50;        // 0-100 (wird auf effektspezifische Werte gemappt)
  uint8_t intensity = 50;    // 0-100 (Intensität/Dichte des Effekts)
  uint8_t size = 50;         // 0-100 (Größe von Elementen)
  bool direction = true;     // Richtung (vorwärts/rückwärts)
  uint8_t colorMode = 0;     // 0=manual, 1=rainbow, 2=random
  uint8_t fadeSpeed = 20;    // Übergangsgeschwindigkeit
};

// Struktur für Presets
struct Preset {
  uint8_t effect;
  uint8_t brightness;
  uint8_t r, g, b;
  EffectConfig config;
  bool valid = false;
};

// Control State
struct ControlState {
  bool autoMode = false;         // Auto-Effekte oder manuell - standardmäßig aus
  uint8_t currentEffect = 0;     // Aktueller Effekt (0-19 mit neuen Effekten)
  uint8_t brightness = 255;
  uint8_t r = 255, g = 255, b = 255;
  bool isOn = true;
  uint32_t effectDuration = 5000; // ms pro Effekt im Auto-Modus
  uint32_t lastEffectChange = 0;
  EffectConfig effectConfigs[20]; // Konfiguration für jeden Effekt
  uint8_t transitionTime = 0;    // 0=instant, >0 = fade time in 100ms steps
  uint8_t activeZones = 1;       // 1-3 Zonen
  uint8_t zoneEffects[3] = {0, 0, 0}; // Effekt pro Zone
} controlState;

// Preset Speicher
Preset presets[10];

// WiFi Stabilisierungs-Variablen für Nano 33 IoT
static uint32_t lastWiFiStabilize = 0;
static uint8_t wifiResetCounter = 0;

// FlashStorage Konfiguration
typedef struct {
  boolean valid;
  ControlState state;
  Preset presets[10];
} StorageData;

FlashStorage(flashStorage, StorageData);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("LED Strip 600 - BLE/WiFi Controller");
  
  // LED Setup - deine bewährte Konfiguration
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);
  FastLED.clear();
  FastLED.show();
  
  // Startup-Animation: 500ms weißes Aufleuchten (mit Farbkorrektur)
  Serial.println("💡 Startup animation...");
  fillCorrectedSolid(0, NUM_LEDS, 255, 255, 255);
  FastLED.show();
  delay(500);
  FastLED.clear();
  FastLED.show();
  
  // BLE initialisieren
  Serial.print("🔵 Initializing BLE...");
  if (!BLE.begin()) {
    Serial.println(" ❌ FAILED - continuing with WiFi only");
    bleEnabled = false;
  } else {
    BLE.setLocalName("LED Controller 600");
    BLE.setAdvertisedService(ledService);
    ledService.addCharacteristic(ledCommand);
    BLE.addService(ledService);
    BLE.advertise();
    bleEnabled = true;
    Serial.println(" ✅ SUCCESS - BLE advertising as 'LED Controller 600'");
  }
  
  // WiFi verbinden - ORIGINAL funktionierendes Setup
  Serial.print("Connecting WiFi");
  WiFi.begin(ssid, pass);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    server.begin();
    WiFi.noLowPowerMode();
    Serial.println("WiFi connected successfully");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    // Stop all current effects before WiFi animation
    controlState.isOn = false;
    controlState.autoMode = false;
    FastLED.clear();
    FastLED.show();
    delay(100);
    
    // First test basic brightness variation
    testBrightnessVariation();
    
    // WiFi Success Animation: Doppel-Meteor von beiden Enden zur Mitte
    wifiSuccessAnimation();
    
    // Resume normal operation after animation
    controlState.isOn = true;
  } else {
    Serial.println("\\nWiFi connection failed - continuing with BLE only");
  }
  
  // EEPROM initialisieren und Einstellungen laden
  loadSettings();
}

void loop() {
  static uint32_t lastEffectUpdate = 0;
  static uint32_t lastWiFiCheck = 0;
  uint32_t now = millis();
  
  // WiFi-Verbindung alle 10 Sekunden prüfen und stabilisieren
  if (now - lastWiFiCheck >= 10000) {
    lastWiFiCheck = now;
    checkAndReconnectWiFi();
  }
  
  // Nano 33 IoT spezifische WiFi-Stabilisierung alle 5 Minuten
  if (now - lastWiFiStabilize >= 300000) {
    lastWiFiStabilize = now;
    stabilizeWiFiConnection();
  }
  
  // BLE Commands verarbeiten (nur wenn aktiv)
  if (bleEnabled) {
    BLE.poll();
    if (ledCommand.written()) {
      String cmd = ledCommand.value();
      processCommand(cmd);
    }
  }
  
  // WiFi-Client nur bedienen wenn verbunden
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client = server.available();
    if (client) {
      handleWebRequest(client);
    }
  }
  
  // Effekte ausführen - 50fps (alle 20ms)
  if (now - lastEffectUpdate >= 20) {
    lastEffectUpdate = now;
    
    if (controlState.isOn) {
      // Auto-Modus: Effekt wechseln
      if (controlState.autoMode) {
        if (now - controlState.lastEffectChange >= controlState.effectDuration) {
          controlState.currentEffect = (controlState.currentEffect + 1) % 20;
          controlState.lastEffectChange = now;
          Serial.println("Auto-Effekt: " + String(controlState.currentEffect));
        }
      }
      
      // Aktuellen Effekt ausführen
      runCurrentEffect(now);
      
      // LEDs einmal pro Frame aktualisieren
      FastLED.show();
      
    } else {
      // Ausgeschaltet: LEDs löschen
      FastLED.clear();
      FastLED.show();
    }
  }
}

void processCommand(String cmd) {
  Serial.println("Command received: " + cmd);
  
  int colonIndex = cmd.indexOf(':');
  if (colonIndex == -1) return;
  
  String command = cmd.substring(0, colonIndex);
  String value = cmd.substring(colonIndex + 1);
  
  if (command == "ON") {
    controlState.isOn = true;
  } else if (command == "OFF") {
    controlState.isOn = false;
  } else if (command == "AUTO") {
    controlState.autoMode = (value == "1");
  } else if (command == "EFFECT") {
    controlState.currentEffect = value.toInt();
    controlState.autoMode = false; // Manueller Modus
    controlState.lastEffectChange = millis(); // Reset Timer
    Serial.println("Effect set to: " + String(controlState.currentEffect) + ", Auto-Mode OFF");
  } else if (command == "BRIGHT") {
    controlState.brightness = value.toInt();
    FastLED.setBrightness(controlState.brightness);
  } else if (command == "COLOR") {
    parseColor(value);
  } else if (command == "DURATION") {
    controlState.effectDuration = value.toInt() * 1000; // Sekunden zu ms
  } else if (command == "PARAM") {
    // Format: PARAM:<param>:<value>
    int paramSep = value.indexOf(':');
    if (paramSep != -1) {
      String param = value.substring(0, paramSep);
      int paramValue = value.substring(paramSep + 1).toInt();
      controlState.autoMode = false; // Parameter-Änderung = Manueller Modus
      setEffectParam(param, paramValue);
    }
  } else if (command == "PRESET") {
    // Format: PRESET:<action>:<slot>
    int actionSep = value.indexOf(':');
    if (actionSep != -1) {
      String action = value.substring(0, actionSep);
      int slot = value.substring(actionSep + 1).toInt();
      if (action == "SAVE") {
        savePreset(slot);
      } else if (action == "LOAD") {
        loadPreset(slot);
      }
    }
  } else if (command == "ZONE") {
    // Format: ZONE:<zone>:<effect>
    int zoneSep = value.indexOf(':');
    if (zoneSep != -1) {
      int zone = value.substring(0, zoneSep).toInt();
      int effect = value.substring(zoneSep + 1).toInt();
      if (zone >= 0 && zone < 3) {
        controlState.zoneEffects[zone] = effect;
      }
    }
  } else if (command == "FADE") {
    controlState.transitionTime = value.toInt();
  }
}

void parseColor(String colorStr) {
  int comma1 = colorStr.indexOf(',');
  int comma2 = colorStr.indexOf(',', comma1 + 1);
  
  if (comma1 != -1 && comma2 != -1) {
    controlState.r = colorStr.substring(0, comma1).toInt();
    controlState.g = colorStr.substring(comma1 + 1, comma2).toInt();
    controlState.b = colorStr.substring(comma2 + 1).toInt();
  }
}

void handleWebRequest(WiFiClient client) {
  String request = client.readStringUntil('\r');
  
  // JSON API für Status
  if (request.indexOf("GET /api/status") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.print("{\"on\":");
    client.print(controlState.isOn ? "true" : "false");
    client.print(",\"auto\":");
    client.print(controlState.autoMode ? "true" : "false"); 
    client.print(",\"effect\":");
    client.print(controlState.currentEffect);
    client.print(",\"brightness\":");
    client.print(controlState.brightness);
    client.print(",\"color\":[");
    client.print(controlState.r);
    client.print(",");
    client.print(controlState.g);
    client.print(",");
    client.print(controlState.b);
    client.print("],\"zones\":");
    client.print(controlState.activeZones);
    client.print(",\"effectConfig\":{");
    client.print("\"speed\":");
    client.print(controlState.effectConfigs[controlState.currentEffect].speed);
    client.print(",\"intensity\":");
    client.print(controlState.effectConfigs[controlState.currentEffect].intensity);
    client.print(",\"size\":");
    client.print(controlState.effectConfigs[controlState.currentEffect].size);
    client.print(",\"direction\":");
    client.print(controlState.effectConfigs[controlState.currentEffect].direction ? "true" : "false");
    client.print(",\"colorMode\":");
    client.print(controlState.effectConfigs[controlState.currentEffect].colorMode);
    client.print(",\"fadeSpeed\":");
    client.print(controlState.effectConfigs[controlState.currentEffect].fadeSpeed);
    client.print("}}");
  }
  // POST Commands über URL Parameter (vereinfacht)
  else if (request.indexOf("GET /cmd?") >= 0) {
    int paramStart = request.indexOf("cmd=") + 4;
    int paramEnd = request.indexOf(" ", paramStart);
    if (paramEnd == -1) paramEnd = request.indexOf("&", paramStart);
    if (paramEnd == -1) paramEnd = request.length();
    
    String cmd = request.substring(paramStart, paramEnd);
    cmd.replace("%3A", ":");
    cmd.replace("%2C", ",");
    processCommand(cmd);
    
    client.println("HTTP/1.1 200 OK");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.println("OK");
  }
  
  client.stop();
}

// Erweiterte Effekt-State Struktur für komplexe Animationen
struct EffectState {
  uint32_t lastUpdate = 0;
  uint32_t phase = 0;
  uint8_t step = 0;
  float position = 0.0;
  bool direction = true;
  
  // Für Meteor/Trail-Effekte
  int meteorPos = 0;
  uint8_t meteorHue = 0;
  
  // Für Fire-Effekt
  uint8_t heat[NUM_LEDS];
  
  // Für Bouncing Balls
  float ballHeight[5];
  float ballVelocity[5];
  uint8_t ballHue[5];
  bool ballsInitialized = false;
  
  // Für Matrix Rain
  uint8_t dropPos[20];
  uint8_t dropSpeed[20];
  bool dropsInitialized = false;
  
  // Für Sparkle/Twinkle
  uint32_t sparkleTimer = 0;
  
  // Für Running Lights
  uint8_t runPosition = 0;
  
  // Für Color Wipe
  int wipePos = 0;
  
  // Für Theater Chase
  uint8_t chasePos = 0;
  uint32_t chaseTimer = 0;
} effectState;

void runCurrentEffect(uint32_t now) {
  static uint8_t lastEffect = 255;
  
  // Effekt-Wechsel: State zurücksetzen
  if (controlState.currentEffect != lastEffect) {
    Serial.println("Switching to effect " + String(controlState.currentEffect));
    effectState = EffectState(); // Reset all state
    effectState.lastUpdate = now - 1000; // Force immediate start
    lastEffect = controlState.currentEffect;
  }
  
  // Aktuellen Effekt ausführen
  switch (controlState.currentEffect) {
    case 0: effect_lightning(now, 0, NUM_LEDS); break;
    case 1: effect_meteor(now, 0, NUM_LEDS); break;
    case 2: effect_strobe(now, 0, NUM_LEDS); break;
    case 3: effect_firework(now, 0, NUM_LEDS); break;
    case 4: effect_confetti(now, 0, NUM_LEDS); break;
    case 5: effect_rainbowGlitter(now, 0, NUM_LEDS); break;
    case 6: effect_rapidColorChange(now, 0, NUM_LEDS); break;
    case 7: effect_police(now, 0, NUM_LEDS); break;
    case 8: effect_energyWave(now, 0, NUM_LEDS); break;
    case 9: effect_solidColor(now, 0, NUM_LEDS); break;
    case 10: effect_theaterChase(now, 0, NUM_LEDS); break;
    case 11: effect_breathing(now, 0, NUM_LEDS); break;
    case 12: effect_colorWipe(now, 0, NUM_LEDS); break;
    case 13: effect_fire(now, 0, NUM_LEDS); break;
    case 14: effect_plasma(now, 0, NUM_LEDS); break;
    case 15: effect_sparkle(now, 0, NUM_LEDS); break;
    case 16: effect_runningLights(now, 0, NUM_LEDS); break;
    case 17: effect_twinkle(now, 0, NUM_LEDS); break;
    case 18: effect_bouncingBalls(now, 0, NUM_LEDS); break;
    case 19: effect_matrixRain(now, 0, NUM_LEDS); break;
  }
}

// === NEUE EINHEITLICHE EFFEKTE ===

// Effekt 9: Solid Color - Einfarbig (mit Farbkorrektur)
void effect_solidColor(uint32_t now, int startLed, int endLed) {
  fillCorrectedSolid(startLed, endLed - startLed, controlState.r, controlState.g, controlState.b);
}

// Effekt 11: Breathing - Sanftes Atmen  
void effect_breathing(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[11];
  
  // Kontinuierliche Zeitbasierte Phase für flüssige Animation
  uint16_t cycleTime = map(config.speed, 1, 100, 8000, 800); // 8s bis 0.8s pro Atemzug
  
  // Glatte Sinus-Kurve basierend auf aktueller Zeit
  float phase = (now % cycleTime) * 2.0 * PI / cycleTime;
  uint8_t brightness = (sin(phase) + 1.0) * 127.5; // 0-255, glatte Kurve
  
  // Intensität bestimmt minimale Helligkeit (für tiefes Atmen)
  uint8_t minBright = map(config.intensity, 0, 100, 0, 5);
  brightness = map(brightness, 0, 255, minBright, 255);
  
  CRGB color;
  if (config.colorMode == 1) {
    // Rainbow-Modus: Farbe ändert sich sehr langsam
    uint8_t hue = (now / 100) % 256; // Langsame Farbänderung
    color = CHSV(hue, 255, brightness);
  } else {
    // Manuelle Farbe mit variabler Helligkeit
    color = CRGB(
      (controlState.r * brightness) / 255,
      (controlState.g * brightness) / 255,
      (controlState.b * brightness) / 255
    );
  }
  
  fill_solid(&leds[startLed], endLed - startLed, color);
}

// Effekt 0: Lightning - Blitzeffekt
void effect_lightning(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[0];
  
  // Geschwindigkeit bestimmt Blitz-Häufigkeit
  uint16_t flashInterval = map(config.speed, 1, 100, 3000, 200);
  
  if (now - effectState.lastUpdate >= flashInterval) {
    effectState.lastUpdate = now;
    
    // Blitz-Position und -Länge
    int flashPos = random(startLed, endLed - 30);
    int flashLength = map(config.size, 0, 100, 5, 50);
    
    // Blitz-Helligkeit basierend auf Intensität
    uint8_t brightness = map(config.intensity, 0, 100, 100, 255);
    
    // Nur den betroffenen Bereich löschen, nicht alle LEDs
    fill_solid(&leds[startLed], endLed - startLed, CRGB::Black);
    for(int i = 0; i < flashLength && (flashPos + i) < endLed; i++) {
      leds[flashPos + i] = CRGB(brightness, brightness, brightness);
    }
  } else if (now - effectState.lastUpdate >= 50) {
    // Blitz nach 50ms löschen - nur den Bereich
    fill_solid(&leds[startLed], endLed - startLed, CRGB::Black);
  }
}

// Effekt 2: Strobe - Stroboskop
void effect_strobe(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[2];
  
  // Geschwindigkeit bestimmt Strobe-Frequenz
  uint16_t strobeInterval = map(config.speed, 1, 100, 500, 50);
  
  if (now - effectState.lastUpdate >= strobeInterval) {
    effectState.lastUpdate = now;
    effectState.step = (effectState.step + 1) % 2;
    
    if (effectState.step == 0) {
      // Strobe an
      CRGB color = CRGB(controlState.r, controlState.g, controlState.b);
      fill_solid(&leds[startLed], endLed - startLed, color);
    } else {
      // Strobe aus
      fill_solid(&leds[startLed], endLed - startLed, CRGB::Black);
    }
  }
}

// Effekt 7: Police - Polizeilicht
void effect_police(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[7];
  
  // Geschwindigkeit bestimmt Wechsel-Frequenz
  uint16_t switchInterval = map(config.speed, 1, 100, 1000, 100);
  
  if (now - effectState.lastUpdate >= switchInterval) {
    effectState.lastUpdate = now;
    effectState.step = (effectState.step + 1) % 2;
    
    int halfLeds = (endLed - startLed) / 2;
    
    // Polizei: Rot und Blau - mit korrigierter Farbzuordnung
    CRGB policeRed = CRGB(255, 0, 0);   // Rot: R=255, G=0, B=0
    CRGB policeBlue = CRGB(0, 0, 255);  // Blau: R=0, G=0, B=255
    
    if (effectState.step == 0) {
      // Rot links, Blau rechts
      fill_solid(&leds[startLed], halfLeds, policeRed);
      fill_solid(&leds[startLed + halfLeds], halfLeds, policeBlue);
    } else {
      // Blau links, Rot rechts  
      fill_solid(&leds[startLed], halfLeds, policeBlue);
      fill_solid(&leds[startLed + halfLeds], halfLeds, policeRed);
    }
  }
}

// === VOLLSTÄNDIG IMPLEMENTIERTE EFFEKTE ===

// Effekt 1: Meteor - Laufender Schweif
void effect_meteor(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[1];
  
  uint16_t speed = map(config.speed, 1, 100, 100, 10);
  
  if (now - effectState.lastUpdate >= speed) {
    effectState.lastUpdate = now;
    
    // Schweif verblassen lassen
    uint8_t decay = map(config.intensity, 0, 100, 50, 5);
    fadeToBlackBy(&leds[startLed], endLed - startLed, decay);
    
    // Meteor-Größe
    uint8_t meteorSize = map(config.size, 0, 100, 3, 15);
    
    // Meteor zeichnen
    for(int i = 0; i < meteorSize; i++) {
      int pos = effectState.meteorPos - i;
      if(pos >= startLed && pos < endLed) {
        uint8_t brightness = 255 - (i * 255 / meteorSize);
        
        if(config.colorMode == 1) {
          // Rainbow
          leds[pos] = CHSV(effectState.meteorHue + (i * 10), 255, brightness);
        } else {
          // Manual color
          leds[pos] = CRGB(
            (controlState.r * brightness) / 255,
            (controlState.g * brightness) / 255,
            (controlState.b * brightness) / 255
          );
        }
      }
    }
    
    // Position bewegen
    if(config.direction) {
      effectState.meteorPos++;
      if(effectState.meteorPos >= endLed + meteorSize) effectState.meteorPos = startLed;
    } else {
      effectState.meteorPos--;
      if(effectState.meteorPos < startLed - meteorSize) effectState.meteorPos = endLed;
    }
    
    effectState.meteorHue += 2;
  }
}

// Effekt 3: Firework - Feuerwerk-Explosionen
void effect_firework(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[3];
  
  // Geschwindigkeit bestimmt Explosions-Häufigkeit
  uint16_t explosionInterval = map(config.speed, 1, 100, 2000, 300);
  
  // Hintergrund ausblenden
  fadeToBlackBy(&leds[startLed], endLed - startLed, 20);
  
  if (now - effectState.lastUpdate >= explosionInterval) {
    effectState.lastUpdate = now;
    
    // Neue Explosion starten
    effectState.position = random(startLed + 20, endLed - 20);
    effectState.phase = 0;
    effectState.step = random(256); // Zufällige Farbe
  }
  
  // Explosion zeichnen
  if(effectState.phase < 50) {
    uint8_t radius = map(effectState.phase, 0, 49, 0, map(config.size, 0, 100, 10, 40));
    uint8_t brightness = map(effectState.phase, 0, 49, 255, 0);
    
    for(int i = 0; i <= radius; i++) {
      uint8_t sparkBrightness = brightness * (radius - i) / radius;
      
      // Explosion nach links und rechts
      int leftPos = effectState.position - i;
      int rightPos = effectState.position + i;
      
      if(leftPos >= startLed && leftPos < endLed) {
        if(config.colorMode == 1) {
          leds[leftPos] = CHSV(effectState.step + (i * 5), 255, sparkBrightness);
        } else {
          leds[leftPos] = CRGB(
            (controlState.r * sparkBrightness) / 255,
            (controlState.g * sparkBrightness) / 255,
            (controlState.b * sparkBrightness) / 255
          );
        }
      }
      
      if(rightPos >= startLed && rightPos < endLed && rightPos != leftPos) {
        if(config.colorMode == 1) {
          leds[rightPos] = CHSV(effectState.step + (i * 5), 255, sparkBrightness);
        } else {
          leds[rightPos] = CRGB(
            (controlState.r * sparkBrightness) / 255,
            (controlState.g * sparkBrightness) / 255,
            (controlState.b * sparkBrightness) / 255
          );
        }
      }
    }
    effectState.phase++;
  }
}

// Effekt 4: Confetti - Zufällige bunte Punkte
void effect_confetti(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[4];
  
  uint16_t fadeSpeed = map(config.fadeSpeed, 0, 100, 5, 50);
  fadeToBlackBy(&leds[startLed], endLed - startLed, fadeSpeed);
  
  uint8_t sparkleCount = map(config.intensity, 0, 100, 1, 15);
  
  for(int i = 0; i < sparkleCount; i++) {
    int pos = random(startLed, endLed);
    if(config.colorMode == 1) {
      leds[pos] += CHSV(random8(), 200, 255);
    } else if(config.colorMode == 2) {
      leds[pos] += CHSV(random8(), 255, random8(100, 255));
    } else {
      leds[pos] += CRGB(controlState.r, controlState.g, controlState.b);
    }
  }
}

// Effekt 5: Rainbow Glitter - Regenbogen mit Glitzer
void effect_rainbowGlitter(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[5];
  
  uint8_t speed = map(config.speed, 1, 100, 1, 10);
  
  if (now - effectState.lastUpdate >= 50) {
    effectState.lastUpdate = now;
    
    // Regenbogen-Hintergrund
    uint8_t deltaHue = map(config.size, 0, 100, 3, 15);
    fill_rainbow(&leds[startLed], endLed - startLed, effectState.phase, deltaHue);
    
    // Glitzer hinzufügen
    uint8_t glitterChance = map(config.intensity, 0, 100, 20, 80);
    if(random8() < glitterChance) {
      for(int i = 0; i < 5; i++) {
        int pos = random(startLed, endLed);
        leds[pos] += CRGB::White;
      }
    }
    
    effectState.phase += speed;
  }
}

// Effekt 6: Rapid Color Change - Schneller Farbwechsel 
void effect_rapidColorChange(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[6];
  
  uint16_t changeInterval = map(config.speed, 1, 100, 1000, 50);
  
  if (now - effectState.lastUpdate >= changeInterval) {
    effectState.lastUpdate = now;
    
    CRGB colors[] = {CRGB::Red, CRGB::Blue, CRGB::Green, CRGB::Yellow, 
                     CRGB::Purple, CRGB::Cyan, CRGB::White, CRGB::Orange,
                     CRGB::Pink, CRGB::Lime, CRGB::Aqua, CRGB::Magenta};
    
    if(config.colorMode == 1) {
      // Rainbow mode
      CRGB color = CHSV(effectState.phase * 10, 255, 255);
      fill_solid(&leds[startLed], endLed - startLed, color);
    } else if(config.colorMode == 2) {
      // Predefined colors
      CRGB color = colors[effectState.step % 12];
      fill_solid(&leds[startLed], endLed - startLed, color);
    } else {
      // Manual color with brightness variation
      uint8_t brightness = map(effectState.step % 4, 0, 3, 50, 255);
      CRGB color = CRGB(
        (controlState.r * brightness) / 255,
        (controlState.g * brightness) / 255,
        (controlState.b * brightness) / 255
      );
      fill_solid(&leds[startLed], endLed - startLed, color);
    }
    
    effectState.phase++;
    effectState.step++;
  }
}

// Effekt 8: Energy Wave - Energiewelle
void effect_energyWave(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[8];
  
  uint16_t speed = map(config.speed, 1, 100, 50, 5);
  
  if (now - effectState.lastUpdate >= speed) {
    effectState.lastUpdate = now;
    
    for(int i = startLed; i < endLed; i++) {
      uint8_t brightness = sin8((effectState.phase * 2) + ((i - startLed) * map(config.size, 0, 100, 5, 20)));
      uint8_t hue = (effectState.phase + ((i - startLed) * 2)) % 256;
      
      if(config.colorMode == 1) {
        leds[i] = CHSV(hue, 255, brightness);
      } else {
        uint8_t intensity = map(config.intensity, 0, 100, 50, 255);
        brightness = map(brightness, 0, 255, 0, intensity);
        leds[i] = CRGB(
          (controlState.r * brightness) / 255,
          (controlState.g * brightness) / 255,
          (controlState.b * brightness) / 255
        );
      }
    }
    
    effectState.phase += map(config.intensity, 0, 100, 1, 5);
  }
}

// Effekt 10: Theater Chase - Laufende Lichter mit Lücken
void effect_theaterChase(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[10];
  
  uint16_t speed = map(config.speed, 1, 100, 200, 20);
  
  if (now - effectState.chaseTimer >= speed) {
    effectState.chaseTimer = now;
    
    // Alle LEDs ausblenden
    fadeToBlackBy(&leds[startLed], endLed - startLed, 100);
    
    uint8_t spacing = map(config.size, 0, 100, 3, 10);
    
    for(int i = startLed; i < endLed; i += spacing) {
      int offset = config.direction ? effectState.chasePos : -effectState.chasePos;
      int pos = ((i - startLed + offset) % (endLed - startLed)) + startLed;
      
      // Sicherstellen dass pos im gültigen Bereich ist
      while(pos < startLed) pos += (endLed - startLed);
      while(pos >= endLed) pos -= (endLed - startLed);
      
      if(config.colorMode == 1) {
        leds[pos] = CHSV(((i - startLed) * 255 / (endLed - startLed) + effectState.chasePos * 10) % 256, 255, 255);
      } else {
        leds[pos] = CRGB(controlState.r, controlState.g, controlState.b);
      }
    }
    
    effectState.chasePos = (effectState.chasePos + 1) % spacing;
  }
}

// Effekt 12: Color Wipe - Farbwelle
void effect_colorWipe(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[12];
  
  uint16_t speed = map(config.speed, 1, 100, 100, 10);
  
  if (now - effectState.lastUpdate >= speed) {
    effectState.lastUpdate = now;
    
    if(config.direction) {
      // Vorwärts wischen
      if(effectState.wipePos < (endLed - startLed)) {
        int pos = startLed + effectState.wipePos;
        if(config.colorMode == 1) {
          leds[pos] = CHSV((effectState.wipePos * 255 / (endLed - startLed)) % 256, 255, 255);
        } else {
          leds[pos] = CRGB(controlState.r, controlState.g, controlState.b);
        }
        effectState.wipePos++;
      } else {
        // Reset und löschen
        effectState.wipePos = 0;
        fadeToBlackBy(&leds[startLed], endLed - startLed, 50);
      }
    } else {
      // Rückwärts wischen
      if(effectState.wipePos < (endLed - startLed)) {
        int pos = endLed - 1 - effectState.wipePos;
        if(pos >= startLed) {
          if(config.colorMode == 1) {
            leds[pos] = CHSV((effectState.wipePos * 255 / (endLed - startLed)) % 256, 255, 255);
          } else {
            leds[pos] = CRGB(controlState.r, controlState.g, controlState.b);
          }
        }
        effectState.wipePos++;
      } else {
        // Reset und löschen
        effectState.wipePos = 0;
        fadeToBlackBy(&leds[startLed], endLed - startLed, 50);
      }
    }
  }
}

// Effekt 13: Fire - Realistisches Feuer
void effect_fire(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[13];
  
  uint16_t speed = map(config.speed, 1, 100, 100, 10);
  
  if (now - effectState.lastUpdate >= speed) {
    effectState.lastUpdate = now;
    
    uint8_t cooling = map(config.intensity, 0, 100, 20, 100);
    uint8_t sparking = map(config.size, 0, 100, 50, 200);
    
    // Cool down every cell
    for(int i = startLed; i < endLed; i++) {
      effectState.heat[i] = qsub8(effectState.heat[i], random8(0, ((cooling * 10) / (endLed - startLed)) + 2));
    }
    
    // Heat drifts up and diffuses
    for(int k = endLed - 1; k >= startLed + 2; k--) {
      effectState.heat[k] = (effectState.heat[k - 1] + effectState.heat[k - 2] + effectState.heat[k - 2]) / 3;
    }
    
    // Randomly ignite new sparks
    if(random8() < sparking) {
      int y = random8(7) + startLed;
      if(y < endLed) effectState.heat[y] = qadd8(effectState.heat[y], random8(160, 255));
    }
    
    // Convert heat to LED colors
    for(int j = startLed; j < endLed; j++) {
      if(config.colorMode == 1) {
        // Cool fire (blue-white)
        uint8_t colorindex = scale8(effectState.heat[j], 240);
        leds[j] = CHSV(160 + (colorindex / 3), 255 - colorindex, colorindex);
      } else if(config.colorMode == 2) {
        // Green fire
        uint8_t colorindex = scale8(effectState.heat[j], 240);
        leds[j] = CHSV(96, 255, colorindex);
      } else {
        // Classic orange/red fire
        uint8_t colorindex = scale8(effectState.heat[j], 240);
        leds[j] = HeatColor(colorindex);
      }
    }
  }
}

// Effekt 14: Plasma - Mathematische Wellenformen
void effect_plasma(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[14];
  
  uint16_t speed = map(config.speed, 1, 100, 50, 5);
  
  if (now - effectState.lastUpdate >= speed) {
    effectState.lastUpdate = now;
    
    uint8_t waveScale = map(config.size, 0, 100, 10, 50);
    uint8_t intensity = map(config.intensity, 0, 100, 100, 255);
    
    for(int i = startLed; i < endLed; i++) {
      uint8_t pixelHue = 0;
      pixelHue += sin8(((i - startLed) * waveScale) + effectState.phase);
      pixelHue += cos8(((i - startLed) * (waveScale/2)) + effectState.phase * 2);
      pixelHue += sin8(((i - startLed) * (waveScale/4)) + effectState.phase * 3);
      
      if(config.colorMode == 0) {
        // Manual color with plasma brightness modulation  
        uint8_t brightness = sin8(pixelHue) / 2 + 127;
        brightness = map(brightness, 0, 255, 50, intensity);
        leds[i] = CRGB(
          (controlState.r * brightness) / 255,
          (controlState.g * brightness) / 255,
          (controlState.b * brightness) / 255
        );
      } else {
        // Full spectrum plasma
        leds[i] = CHSV(pixelHue, 255, intensity);
      }
    }
    
    effectState.phase += 2;
  }
}

// Effekt 15: Sparkle - Funkeln
void effect_sparkle(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[15];
  
  uint16_t fadeSpeed = map(config.fadeSpeed, 0, 100, 5, 50);
  fadeToBlackBy(&leds[startLed], endLed - startLed, fadeSpeed);
  
  uint16_t sparkleInterval = map(config.speed, 1, 100, 200, 20);
  
  if (now - effectState.sparkleTimer >= sparkleInterval) {
    effectState.sparkleTimer = now;
    
    uint8_t sparkleCount = map(config.intensity, 0, 100, 1, 10);
    
    for(int i = 0; i < sparkleCount; i++) {
      int pos = random(startLed, endLed);
      uint8_t brightness = random8(100, 255);
      
      if(config.colorMode == 1) {
        leds[pos] = CHSV(random8(), 255, brightness);
      } else if(config.colorMode == 2) {
        leds[pos] = CRGB(brightness, brightness, brightness);
      } else {
        leds[pos] = CRGB(
          (controlState.r * brightness) / 255,
          (controlState.g * brightness) / 255,
          (controlState.b * brightness) / 255
        );
      }
    }
  }
}

// Effekt 16: Running Lights - Laufende Segmente
void effect_runningLights(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[16];
  
  uint16_t speed = map(config.speed, 1, 100, 100, 10);
  
  if (now - effectState.lastUpdate >= speed) {
    effectState.lastUpdate = now;
    
    uint8_t segmentSize = map(config.size, 0, 100, 5, 30);
    uint8_t fadeAmount = map(config.fadeSpeed, 0, 100, 10, 100);
    
    fadeToBlackBy(&leds[startLed], endLed - startLed, fadeAmount);
    
    for(int i = 0; i < segmentSize; i++) {
      int pos;
      if(config.direction) {
        pos = ((effectState.runPosition + i) % (endLed - startLed)) + startLed;
      } else {
        pos = ((effectState.runPosition - i + (endLed - startLed)) % (endLed - startLed)) + startLed;
      }
      
      uint8_t brightness = 255 - (i * 255 / segmentSize);
      
      if(config.colorMode == 1) {
        leds[pos] = CHSV((effectState.runPosition * 3) % 256, 255, brightness);
      } else {
        leds[pos] = CRGB(
          (controlState.r * brightness) / 255,
          (controlState.g * brightness) / 255,
          (controlState.b * brightness) / 255
        );
      }
    }
    
    effectState.runPosition = (effectState.runPosition + 1) % (endLed - startLed);
  }
}

// Effekt 17: Twinkle - Sterne funkeln
void effect_twinkle(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[17];
  
  uint8_t fadeAmount = map(config.fadeSpeed, 0, 100, 5, 30);
  fadeToBlackBy(&leds[startLed], endLed - startLed, fadeAmount);
  
  uint16_t twinkleInterval = map(config.speed, 1, 100, 200, 30);
  
  if (now - effectState.lastUpdate >= twinkleInterval) {
    effectState.lastUpdate = now;
    
    uint8_t density = map(config.intensity, 0, 100, 1, 15);
    
    for(int i = 0; i < density; i++) {
      int pos = random(startLed, endLed);
      uint8_t brightness = random8(50, 255);
      
      if(config.colorMode == 1) {
        leds[pos] = CHSV(random8(), 255, brightness);
      } else if(config.colorMode == 2) {
        leds[pos] = CRGB(brightness, brightness, brightness);
      } else {
        leds[pos] = CRGB(
          (controlState.r * brightness) / 255,
          (controlState.g * brightness) / 255,
          (controlState.b * brightness) / 255
        );
      }
    }
  }
}

// Effekt 18: Bouncing Balls - Springende Bälle mit Physik
void effect_bouncingBalls(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[18];
  
  // Initialisierung beim ersten Aufruf
  if(!effectState.ballsInitialized) {
    for(int i = 0; i < 5; i++) {
      effectState.ballHeight[i] = startLed; // Alle Bälle starten am Boden
      effectState.ballVelocity[i] = -2.0 - random8(10) / 10.0; // Negative Geschwindigkeit = nach oben
      effectState.ballHue[i] = random8();
    }
    effectState.ballsInitialized = true;
  }
  
  uint16_t speed = map(config.speed, 1, 100, 50, 5);
  
  if (now - effectState.lastUpdate >= speed) {
    effectState.lastUpdate = now;
    
    uint8_t fadeAmount = map(config.fadeSpeed, 0, 100, 10, 50);
    fadeToBlackBy(&leds[startLed], endLed - startLed, fadeAmount);
    
    uint8_t ballCount = map(config.intensity, 0, 100, 1, 5);
    float gravity = map(config.size, 0, 100, 10, 100) / 1000.0;
    
    for(int i = 0; i < ballCount; i++) {
      effectState.ballVelocity[i] += gravity; // Schwerkraft nach unten (positiv)
      effectState.ballHeight[i] += effectState.ballVelocity[i];
      
      // Bounce detection
      if(effectState.ballHeight[i] < startLed) {
        effectState.ballHeight[i] = startLed;
        effectState.ballVelocity[i] = -effectState.ballVelocity[i] * 0.8; // Damping
        
        if(config.colorMode == 1) {
          effectState.ballHue[i] = random8();
        }
      }
      
      // Bounce off ceiling
      if(effectState.ballHeight[i] >= endLed - 1) {
        effectState.ballHeight[i] = endLed - 1;
        effectState.ballVelocity[i] = -effectState.ballVelocity[i] * 0.8;
        
        if(config.colorMode == 1) {
          effectState.ballHue[i] = random8();
        }
      }
      
      // Draw ball
      int pos = (int)effectState.ballHeight[i];
      if(pos >= startLed && pos < endLed) {
        if(config.colorMode == 1) {
          leds[pos] = CHSV(effectState.ballHue[i], 255, 255);
        } else if(config.colorMode == 2) {
          leds[pos] = CRGB::White;
        } else {
          leds[pos] = CRGB(controlState.r, controlState.g, controlState.b);
        }
      }
    }
  }
}

// Effekt 19: Matrix Rain - Digitaler Regen
void effect_matrixRain(uint32_t now, int startLed, int endLed) {
  EffectConfig &config = controlState.effectConfigs[19];
  
  // Initialisierung
  if(!effectState.dropsInitialized) {
    for(int i = 0; i < 20; i++) {
      effectState.dropPos[i] = random8(endLed - startLed);
      effectState.dropSpeed[i] = random8(1, 6);
    }
    effectState.dropsInitialized = true;
  }
  
  uint16_t speed = map(config.speed, 1, 100, 100, 10);
  
  if (now - effectState.lastUpdate >= speed) {
    effectState.lastUpdate = now;
    
    uint8_t fadeAmount = map(config.fadeSpeed, 0, 100, 10, 50);
    fadeToBlackBy(&leds[startLed], endLed - startLed, fadeAmount);
    
    uint8_t dropCount = map(config.intensity, 0, 100, 5, 20);
    uint8_t trailLength = map(config.size, 0, 100, 3, 15);
    
    for(int i = 0; i < dropCount; i++) {
      // Draw trail
      for(int j = 0; j < trailLength; j++) {
        int pos = startLed + ((effectState.dropPos[i] - j * 2) % (endLed - startLed));
        if(pos < startLed) pos += (endLed - startLed);
        
        uint8_t brightness = 255 - (j * 255 / trailLength);
        if(j == 0) brightness = 255; // Head of drop
        
        if(config.colorMode == 1) {
          // Colorful matrix
          leds[pos] = CHSV(96 + (i * 15), 255, brightness);
        } else if(config.colorMode == 2) {
          // Blue matrix
          leds[pos] = CRGB(0, 0, brightness);
        } else {
          // Classic green matrix
          leds[pos] = CRGB(0, brightness, 0);
        }
      }
      
      // Move drop
      effectState.dropPos[i] = (effectState.dropPos[i] + effectState.dropSpeed[i]) % (endLed - startLed);
      
      // Randomly reset drops
      if(random8() < 5) {
        effectState.dropPos[i] = 0;
        effectState.dropSpeed[i] = random8(1, 6);
      }
    }
  }
}

// Arduino Nano 33 IoT spezifische WiFi-Wiederverbindung
void checkAndReconnectWiFi() {
  static bool lastConnectionState = true;
  static uint32_t lastReconnectAttempt = 0;
  static uint8_t reconnectAttempts = 0;
  
  bool currentlyConnected = (WiFi.status() == WL_CONNECTED);
  uint32_t now = millis();
  
  // Verbindung verloren?
  if (!currentlyConnected && lastConnectionState) {
    Serial.println("📡 WiFi disconnected - starting reconnect sequence");
    lastConnectionState = false;
    reconnectAttempts = 0;
    lastReconnectAttempt = now;
  }
  
  // Reconnect-Versuche
  if (!currentlyConnected && (now - lastReconnectAttempt >= 5000)) {
    lastReconnectAttempt = now;
    reconnectAttempts++;
    
    Serial.print("🔄 WiFi reconnect attempt ");
    Serial.print(reconnectAttempts);
    Serial.println("/3");
    
    if (reconnectAttempts <= 3) {
      // Nano 33 IoT optimierte Wiederverbindung
      WiFi.disconnect();
      delay(100);
      WiFi.begin(ssid, pass);
      
      // Power Management Fix nach Reconnect
      delay(200);
      WiFi.noLowPowerMode();
      delay(100);
      WiFi.lowPowerMode();
      delay(50);
      WiFi.noLowPowerMode();
      
      // Status LED
      leds[0] = CRGB::Orange;
      FastLED.show();
      
    } else {
      Serial.println(bleEnabled ? "❌ WiFi reconnect failed - continuing with BLE" : "❌ WiFi reconnect failed - no BLE available");
      reconnectAttempts = 0; // Reset für nächsten Zyklus
      
      // Blaue LED = BLE-only
      leds[0] = CRGB::Blue;
      FastLED.show();
    }
  }
  
  // Verbindung wiederhergestellt?
  if (currentlyConnected && !lastConnectionState) {
    Serial.print("✅ WiFi reconnected! IP: ");
    Serial.println(WiFi.localIP());
    lastConnectionState = true;
    reconnectAttempts = 0;
    
    // Grüne LED
    leds[0] = CRGB::Green;
    FastLED.show();
    delay(500);
    leds[0] = CRGB::Black;
  }
}

// Nano 33 IoT WiFi-Stabilisierung (alle 5 Minuten)
void stabilizeWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("🔧 WiFi maintenance - applying Nano 33 IoT stability fixes");
    
    // Bekannter Workaround für Nano 33 IoT WiFi-Modul
    WiFi.noLowPowerMode();
    delay(100);
    WiFi.lowPowerMode();  
    delay(50);
    WiFi.noLowPowerMode();
    
    // Reset-Counter für Diagnostik
    wifiResetCounter++;
    Serial.print("WiFi stability reset #");
    Serial.println(wifiResetCounter);
    
    // Bei zu vielen Resets: Hard Reset
    if (wifiResetCounter >= 20) {
      Serial.println("⚠️  Too many WiFi resets - performing hard reset");
      WiFi.end();
      delay(1000);
      WiFi.begin(ssid, pass);
      WiFi.noLowPowerMode();
      wifiResetCounter = 0;
    }
  }
}

// Test function for brightness variation
void testBrightnessVariation() {
  Serial.println("🧪 Testing brightness variation...");
  
  // Clear all LEDs
  FastLED.clear();
  
  // Test pattern: alternating brightness levels
  for(int i = 0; i < 100; i += 4) {
    leds[i] = CRGB(255, 255, 255);     // Full brightness
    leds[i+1] = CRGB(180, 180, 180);   // 70% brightness
    leds[i+2] = CRGB(100, 100, 100);   // 40% brightness  
    leds[i+3] = CRGB(50, 50, 50);      // 20% brightness
  }
  
  FastLED.show();
  delay(5000); // Show for 5 seconds
  
  FastLED.clear();
  FastLED.show();
  Serial.println("🧪 Brightness test complete");
}

// WiFi Success Animation: Dual Meteors treffen sich in der Mitte
void wifiSuccessAnimation() {
  Serial.println("🌟 WiFi Success Animation");
  
  int center = NUM_LEDS / 2;
  int meteorLength = 20;
  
  // Phase 1: Meteors laufen von beiden Enden zur Mitte
  for(int pos = 0; pos <= center; pos++) {
    FastLED.clear();
    
    // Linker Meteor (von 0 zur Mitte) - mit Farbkorrektur
    for(int i = 0; i < meteorLength && (pos + i) < NUM_LEDS; i++) {
      int ledPos = pos + i;
      uint8_t brightness = 255 - (i * 255 / meteorLength);
      setCorrectedRGB(ledPos, brightness, brightness, brightness);
    }
    
    // Rechter Meteor (von Ende zur Mitte) - mit Farbkorrektur
    for(int i = 0; i < meteorLength && (NUM_LEDS - 1 - pos - i) >= 0; i++) {
      int ledPos = NUM_LEDS - 1 - pos - i;
      uint8_t brightness = 255 - (i * 255 / meteorLength);
      setCorrectedRGB(ledPos, brightness, brightness, brightness);
    }
    
    FastLED.show();
    delay(8); // Viel schneller!
    
    // Stoppen wenn sich die Meteors treffen
    if(pos >= center - meteorLength) break;
  }
  
  // Phase 2: 5 Sekunden weißes Funkeln wenn sie sich treffen
  Serial.println("✨ Sparkle phase - 5 seconds with varying brightness");
  uint32_t sparkleStart = millis();
  
  // Arrays für persistente Partikel
  const int maxParticles = 60;
  int particlePos[maxParticles];
  uint8_t particleBrightness[maxParticles];
  
  // Initialisiere Partikel mit zufälligen Positionen und Helligkeiten
  for(int i = 0; i < maxParticles; i++) {
    particlePos[i] = random(NUM_LEDS);
    // Verschiedene Helligkeitsstufen für plastischen Effekt
    uint8_t level = random8(100);
    if(level < 40) {
      particleBrightness[i] = random8(30, 80);    // 40% dunkel
    } else if(level < 70) {
      particleBrightness[i] = random8(80, 150);   // 30% mittel
    } else if(level < 90) {
      particleBrightness[i] = random8(150, 200);  // 20% hell
    } else {
      particleBrightness[i] = random8(200, 255);  // 10% sehr hell
    }
  }
  
  while(millis() - sparkleStart < 5000) {
    // Hintergrund schwarz
    FastLED.clear();
    
    // Zeichne alle Partikel mit ihren individuellen Helligkeiten
    for(int i = 0; i < maxParticles; i++) {
      // Langsames Fading für natürlichen Effekt
      if(particleBrightness[i] > 5) {
        setCorrectedRGB(particlePos[i], particleBrightness[i], particleBrightness[i], particleBrightness[i]);
        
        // Gelegentlich neue Position und Helligkeit
        if(random8(100) < 20) {
          particlePos[i] = random(NUM_LEDS);
          uint8_t level = random8(100);
          if(level < 40) {
            particleBrightness[i] = random8(30, 80);
          } else if(level < 70) {
            particleBrightness[i] = random8(80, 150);
          } else if(level < 90) {
            particleBrightness[i] = random8(150, 200);
          } else {
            particleBrightness[i] = random8(200, 255);
          }
        } else {
          // Leichtes Fading
          particleBrightness[i] = particleBrightness[i] * 0.95;
        }
      }
    }
    
    FastLED.show();
    delay(50); // Flüssige Animation
  }
  
  // Phase 3: Ausblenden
  FastLED.clear();
  FastLED.show();
  Serial.println("🎯 Animation complete");
}

// === FARBKORREKTUR-FUNKTIONEN ===

// Zentrale Funktion für farbkorrigierte LED-Zuweisung
void setCorrectedRGB(int ledIndex, uint8_t r, uint8_t g, uint8_t b) {
  if (ledIndex >= SECOND_HALF_START && ledIndex < NUM_LEDS) {
    // Zweite Hälfte: Sanfte Farbkorrektur anwenden
    r = (uint8_t)(r * CORRECTION_RED_FACTOR);
    g = (uint8_t)(g * CORRECTION_GREEN_FACTOR);
    b = (uint8_t)(b * CORRECTION_BLUE_FACTOR);
  }
  leds[ledIndex].setRGB(r, g, b);
}

// Hilfsfunktion für Bereichs-Füllung mit Farbkorrektur
void fillCorrectedSolid(int startLed, int count, uint8_t r, uint8_t g, uint8_t b) {
  for(int i = 0; i < count; i++) {
    int ledIndex = startLed + i;
    if(ledIndex < NUM_LEDS) {
      setCorrectedRGB(ledIndex, r, g, b);
    }
  }
}

// === HILFSFUNKTIONEN ===

void setEffectParam(String param, int value) {
  uint8_t currentEffect = controlState.currentEffect;
  Serial.println("Setting param '" + param + "' to " + String(value) + " for effect " + String(currentEffect));
  
  if(param == "speed") {
    controlState.effectConfigs[currentEffect].speed = constrain(value, 0, 100);
    Serial.println("Speed updated to: " + String(controlState.effectConfigs[currentEffect].speed));
  } else if(param == "intensity") {
    controlState.effectConfigs[currentEffect].intensity = constrain(value, 0, 100);
  } else if(param == "size") {
    controlState.effectConfigs[currentEffect].size = constrain(value, 0, 100);
  } else if(param == "direction") {
    controlState.effectConfigs[currentEffect].direction = value > 0;
  } else if(param == "colorMode") {
    controlState.effectConfigs[currentEffect].colorMode = constrain(value, 0, 2);
  } else if(param == "fadeSpeed") {
    controlState.effectConfigs[currentEffect].fadeSpeed = constrain(value, 0, 100);
  }
  
  saveSettings(); // Auto-save bei Parameter-Änderung
}

void savePreset(uint8_t slot) {
  if(slot >= 10) return;
  
  presets[slot].effect = controlState.currentEffect;
  presets[slot].brightness = controlState.brightness;
  presets[slot].r = controlState.r;
  presets[slot].g = controlState.g;
  presets[slot].b = controlState.b;
  presets[slot].config = controlState.effectConfigs[controlState.currentEffect];
  presets[slot].valid = true;
  
  // In Flash speichern
  saveSettings();
  
  Serial.println("Preset " + String(slot) + " saved");
}

void loadPreset(uint8_t slot) {
  if(slot >= 10 || !presets[slot].valid) return;
  
  controlState.currentEffect = presets[slot].effect;
  controlState.brightness = presets[slot].brightness;
  controlState.r = presets[slot].r;
  controlState.g = presets[slot].g;
  controlState.b = presets[slot].b;
  controlState.effectConfigs[controlState.currentEffect] = presets[slot].config;
  
  FastLED.setBrightness(controlState.brightness);
  
  Serial.println("Preset " + String(slot) + " loaded");
}

void saveSettings() {
  StorageData data;
  data.valid = true;
  data.state = controlState;
  for(int i = 0; i < 10; i++) {
    data.presets[i] = presets[i];
  }
  flashStorage.write(data);
}

void loadSettings() {
  StorageData data = flashStorage.read();
  
  if(data.valid) {
    controlState = data.state;
    
    // Presets laden
    for(int i = 0; i < 10; i++) {
      presets[i] = data.presets[i];
    }
    
    FastLED.setBrightness(controlState.brightness);
    Serial.println("Settings loaded from Flash");
  } else {
    // Erste Initialisierung - Defaults setzen
    for(int i = 0; i < 20; i++) {
      controlState.effectConfigs[i] = EffectConfig();
    }
    saveSettings();
    Serial.println("Settings initialized");
  }
}