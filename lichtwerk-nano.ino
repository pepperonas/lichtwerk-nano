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

CRGB leds[NUM_LEDS];

// WiFi Config
char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASSWORD;
WiFiServer server(80);

// BLE Config
BLEService ledService("12345678-1234-1234-1234-123456789abc");
BLEStringCharacteristic ledCommand("87654321-4321-4321-4321-cba987654321", BLERead | BLEWrite, 50);

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
  bool autoMode = true;          // Auto-Effekte oder manuell
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
  
  // BLE initialisieren
  if (!BLE.begin()) {
    Serial.println("BLE failed!");
    while (1);
  }
  
  BLE.setLocalName("LED Controller 600");
  BLE.setAdvertisedService(ledService);
  ledService.addCharacteristic(ledCommand);
  BLE.addService(ledService);
  BLE.advertise();
  Serial.println("BLE ready");
  
  // WiFi verbinden
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    Serial.print(".");
    delay(5000);
  }
  server.begin();
  
  Serial.print("\nWiFi connected! IP: ");
  Serial.println(WiFi.localIP());
  
  // EEPROM initialisieren und Einstellungen laden
  loadSettings();
  
  // Status-Anzeige: Grün = bereit
  wifiStatusReady();
}

void loop() {
  BLE.poll();
  
  // BLE Commands verarbeiten
  if (ledCommand.written()) {
    String cmd = ledCommand.value();
    processCommand(cmd);
  }
  
  // WiFi Commands verarbeiten
  WiFiClient client = server.available();
  if (client) {
    handleWebRequest(client);
  }
  
  // Effekte ausführen
  if (controlState.isOn) {
    if (controlState.autoMode) {
      runAutoEffects();
    } else {
      runManualEffect();
    }
  } else {
    FastLED.clear();
    FastLED.show();
    delay(100);
  }
}

void processCommand(String cmd) {
  Serial.println("Command: " + cmd);
  
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

void runAutoEffects() {
  uint32_t now = millis();
  
  // Effekt wechseln nach Zeit
  if (now - controlState.lastEffectChange >= controlState.effectDuration) {
    controlState.currentEffect = (controlState.currentEffect + 1) % 20; // Jetzt 20 Effekte
    controlState.lastEffectChange = now;
    Serial.println("Auto-Effekt: " + String(controlState.currentEffect));
  }
  
  runManualEffect();
}

void runManualEffect() {
  static uint32_t effectTimer = 0;
  static uint16_t effectStep = 0;
  
  // Alle 20ms Effekt-Update (50fps)
  if (millis() - effectTimer >= 20) {
    effectTimer = millis();
    effectStep++;
    
    // Zonen-basierte Effekte
    if (controlState.activeZones > 1) {
      runZonedEffects();
    } else {
      // Einzelner Effekt über alle LEDs
      runSingleEffect(controlState.currentEffect, 0, NUM_LEDS);
    }
  }
}

void runZonedEffects() {
  int ledsPerZone = NUM_LEDS / controlState.activeZones;
  for (int zone = 0; zone < controlState.activeZones; zone++) {
    int startLed = zone * ledsPerZone;
    int endLed = (zone == controlState.activeZones - 1) ? NUM_LEDS : (zone + 1) * ledsPerZone;
    runSingleEffect(controlState.zoneEffects[zone], startLed, endLed);
  }
  FastLED.show();
}

void runSingleEffect(uint8_t effect, int startLed, int endLed) {
  static uint16_t effectStep = 0;
  effectStep++;
  
  switch (effect) {
    // Original Effekte
    case 0: lightning(); break;
    case 1: meteor(CRGB(controlState.r, controlState.g, controlState.b), 10, 64, true); break;
    case 2: strobe(CRGB(controlState.r, controlState.g, controlState.b), 1, 50, 50); break;
    case 3: 
      if (effectStep % 10 == 0) firework(); 
      break;
    case 4: confetti(); break;
    case 5: rainbowGlitter(); break;
    case 6: rapidColorChange(); break;
    case 7: police(); break;
    case 8: energyWave(); break;
    case 9: 
      fill_solid(&leds[startLed], endLed - startLed, CRGB(controlState.r, controlState.g, controlState.b));
      if (startLed == 0) FastLED.show();
      break;
      
    // Neue Effekte
    case 10: theaterChase(startLed, endLed); break;
    case 11: breathing(startLed, endLed); break;
    case 12: colorWipe(startLed, endLed); break;
    case 13: fireEffect(startLed, endLed); break;
    case 14: plasma(startLed, endLed); break;
    case 15: sparkle(startLed, endLed); break;
    case 16: runningLights(startLed, endLed); break;
    case 17: twinkle(startLed, endLed); break;
    case 18: bouncingBalls(startLed, endLed); break;
    case 19: matrixRain(startLed, endLed); break;
  }
}

// Deine Original-Effekte (optimiert für Steuerung)

void lightning() {
  static uint32_t lastFlash = 0;
  if (millis() - lastFlash < random(500, 2000)) return;
  lastFlash = millis();
  
  int flashPos = random(NUM_LEDS - 20);
  int flashLength = random(10, 30);
  
  for(int i = 0; i < flashLength; i++) {
    leds[flashPos + i] = CRGB(200, 200, 255);
  }
  FastLED.show();
  delay(20);
  FastLED.clear();
  FastLED.show();
}

void meteor(CRGB color, int meteorSize, int meteorTrailDecay, boolean meteorRandomDecay) {
  static int meteorPos = 0;
  
  // Schweif ausblenden
  for(int j = 0; j < NUM_LEDS; j++) {
    if((!meteorRandomDecay) || (random(10) > 5)) {
      leds[j].fadeToBlackBy(meteorTrailDecay);
    }
  }
  
  // Meteor zeichnen
  for(int j = 0; j < meteorSize; j++) {
    if((meteorPos - j < NUM_LEDS) && (meteorPos - j >= 0)) {
      leds[meteorPos - j] = color;
    }
  }
  
  FastLED.show();
  meteorPos = (meteorPos + 1) % (NUM_LEDS + meteorSize);
}

void strobe(CRGB color, int strobeCount, int flashDelay, int endPause) {
  static bool strobeState = false;
  static uint32_t strobeTimer = 0;
  
  if (millis() - strobeTimer >= flashDelay) {
    strobeState = !strobeState;
    strobeTimer = millis();
    
    if (strobeState) {
      fill_solid(leds, NUM_LEDS, color);
    } else {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
    }
    FastLED.show();
  }
}

void firework() {
  // Vereinfachte Version für kontinuierliche Steuerung
  static int phase = 0;
  static int explosionPos = 0;
  static CRGB explosionColor;
  
  if (phase == 0) {
    explosionPos = random(50, NUM_LEDS - 50);
    explosionColor = CHSV(random(256), 255, 255);
    phase = 1;
  }
  
  fadeToBlackBy(leds, NUM_LEDS, 30);
  
  for(int radius = 0; radius < 40; radius += 5) {
    if(explosionPos - radius >= 0) leds[explosionPos - radius] = explosionColor;
    if(explosionPos + radius < NUM_LEDS) leds[explosionPos + radius] = explosionColor;
  }
  
  FastLED.show();
  
  if (random(100) < 10) phase = 0; // Neue Explosion
}

void confetti() {
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random(NUM_LEDS);
  leds[pos] += CHSV(random(256), 200, 255);
  FastLED.show();
}

void rainbowGlitter() {
  static uint8_t hue = 0;
  
  fill_rainbow(leds, NUM_LEDS, hue, 7);
  
  if(random(100) < 80) {
    for(int i = 0; i < 10; i++) {
      leds[random(NUM_LEDS)] += CRGB::White;
    }
  }
  
  FastLED.show();
  hue++;
}

void rapidColorChange() {
  static uint8_t colorIndex = 0;
  static uint32_t colorTimer = 0;
  
  CRGB colors[] = {CRGB::Red, CRGB::Blue, CRGB::Green, CRGB::Yellow, 
                   CRGB::Purple, CRGB::Cyan, CRGB::White, CRGB::Orange};
  
  if (millis() - colorTimer >= 200) {
    colorTimer = millis();
    colorIndex = (colorIndex + 1) % 8;
  }
  
  fill_solid(leds, NUM_LEDS, colors[colorIndex]);
  FastLED.show();
}

void police() {
  static bool policeState = false;
  static uint32_t policeTimer = 0;
  
  if (millis() - policeTimer >= 150) {
    policeTimer = millis();
    policeState = !policeState;
    
    if (policeState) {
      for(int i = 0; i < NUM_LEDS/2; i++) leds[i] = CRGB::Red;
      for(int i = NUM_LEDS/2; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
    } else {
      for(int i = 0; i < NUM_LEDS/2; i++) leds[i] = CRGB::Black;
      for(int i = NUM_LEDS/2; i < NUM_LEDS; i++) leds[i] = CRGB::Blue;
    }
    FastLED.show();
  }
}

void energyWave() {
  static uint8_t energy = 0;
  
  for(int i = 0; i < NUM_LEDS; i++) {
    uint8_t brightness = sin8((energy * 2) + (i * 10));
    uint8_t hue = (energy + (i * 2)) % 256;
    leds[i] = CHSV(hue, 255, brightness);
  }
  
  FastLED.show();
  energy += 3;
}

void wifiStatusReady() {
  for(int i = 0; i < 10; i++) {
    leds[i] = CRGB::Green;
  }
  FastLED.show();
  delay(1000);
  FastLED.clear();
  FastLED.show();
}

// === NEUE EFFEKTE ===

// Theater Chase - Laufende Lichter mit Lücken
void theaterChase(int startLed, int endLed) {
  static uint8_t chasePos = 0;
  uint8_t spacing = map(controlState.effectConfigs[10].size, 0, 100, 2, 10);
  uint16_t delayTime = map(controlState.effectConfigs[10].speed, 0, 100, 200, 10);
  
  fadeToBlackBy(&leds[startLed], endLed - startLed, 50);
  
  for(int i = startLed; i < endLed; i += spacing) {
    if((i + chasePos) % spacing == 0) {
      if(controlState.effectConfigs[10].colorMode == 1) {
        leds[i] = CHSV(((i - startLed) * 255 / (endLed - startLed) + chasePos * 10) % 256, 255, 255);
      } else {
        leds[i] = CRGB(controlState.r, controlState.g, controlState.b);
      }
    }
  }
  
  chasePos = (chasePos + (controlState.effectConfigs[10].direction ? 1 : -1)) % spacing;
  FastLED.show();
  delay(delayTime);
}

// Breathing - Sanftes Ein-/Ausblenden
void breathing(int startLed, int endLed) {
  static uint8_t breathPhase = 0;
  uint16_t speed = map(controlState.effectConfigs[11].speed, 0, 100, 50, 1);
  
  uint8_t brightness = sin8(breathPhase);
  brightness = map(brightness, 0, 255, 
                  map(controlState.effectConfigs[11].intensity, 0, 100, 0, 50),
                  map(controlState.effectConfigs[11].intensity, 0, 100, 100, 255));
  
  CRGB color;
  if(controlState.effectConfigs[11].colorMode == 1) {
    color = CHSV(breathPhase, 255, brightness);
  } else {
    color = CRGB(
      (controlState.r * brightness) / 255,
      (controlState.g * brightness) / 255,
      (controlState.b * brightness) / 255
    );
  }
  
  fill_solid(&leds[startLed], endLed - startLed, color);
  breathPhase += speed;
  FastLED.show();
}

// Color Wipe - Farbwelle durchlaufen lassen
void colorWipe(int startLed, int endLed) {
  static int wipePos = 0;
  uint16_t speed = map(controlState.effectConfigs[12].speed, 0, 100, 1, 10);
  
  if(controlState.effectConfigs[12].direction) {
    for(int i = 0; i < speed && wipePos < (endLed - startLed); i++, wipePos++) {
      if(controlState.effectConfigs[12].colorMode == 1) {
        leds[startLed + wipePos] = CHSV((wipePos * 255 / (endLed - startLed)) % 256, 255, 255);
      } else {
        leds[startLed + wipePos] = CRGB(controlState.r, controlState.g, controlState.b);
      }
    }
    if(wipePos >= (endLed - startLed)) {
      wipePos = 0;
      fadeToBlackBy(&leds[startLed], endLed - startLed, 20);
    }
  }
  FastLED.show();
}

// Fire Effect - Realistisches Feuer
void fireEffect(int startLed, int endLed) {
  static byte heat[NUM_LEDS];
  uint8_t cooling = map(controlState.effectConfigs[13].intensity, 0, 100, 20, 80);
  uint8_t sparking = map(controlState.effectConfigs[13].speed, 0, 100, 50, 200);
  
  // Cool down every cell a little
  for(int i = startLed; i < endLed; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((cooling * 10) / (endLed - startLed)) + 2));
  }
  
  // Heat from each cell drifts up and diffuses
  for(int k = endLed - 1; k >= startLed + 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  
  // Randomly ignite new sparks near the bottom
  if(random8() < sparking) {
    int y = random8(7) + startLed;
    if(y < endLed) heat[y] = qadd8(heat[y], random8(160, 255));
  }
  
  // Convert heat to LED colors
  for(int j = startLed; j < endLed; j++) {
    byte colorindex = scale8(heat[j], 240);
    CRGB color = HeatColor(colorindex);
    leds[j] = color;
  }
  FastLED.show();
}

// Plasma - Mathematische Farbmuster
void plasma(int startLed, int endLed) {
  static uint8_t plasmaTime = 0;
  uint8_t speed = map(controlState.effectConfigs[14].speed, 0, 100, 1, 5);
  
  for(int i = startLed; i < endLed; i++) {
    uint8_t pixelHue = 0;
    pixelHue += sin8(((i - startLed) * controlState.effectConfigs[14].size) + plasmaTime);
    pixelHue += cos8(((i - startLed) * controlState.effectConfigs[14].intensity / 10) + plasmaTime * 2);
    leds[i] = CHSV(pixelHue, 255, 255);
  }
  
  plasmaTime += speed;
  FastLED.show();
}

// Sparkle - Zufälliges Funkeln
void sparkle(int startLed, int endLed) {
  fadeToBlackBy(&leds[startLed], endLed - startLed, controlState.effectConfigs[15].fadeSpeed);
  
  uint8_t chance = map(controlState.effectConfigs[15].intensity, 0, 100, 1, 50);
  for(int i = 0; i < chance; i++) {
    int pos = random(startLed, endLed);
    if(controlState.effectConfigs[15].colorMode == 1) {
      leds[pos] = CHSV(random8(), 255, 255);
    } else if(controlState.effectConfigs[15].colorMode == 2) {
      leds[pos] = CRGB::White;
    } else {
      leds[pos] = CRGB(controlState.r, controlState.g, controlState.b);
    }
  }
  FastLED.show();
}

// Running Lights - Laufende Lichtsegmente
void runningLights(int startLed, int endLed) {
  static uint8_t position = 0;
  uint8_t segmentSize = map(controlState.effectConfigs[16].size, 0, 100, 5, 50);
  uint8_t speed = map(controlState.effectConfigs[16].speed, 0, 100, 1, 10);
  
  fadeToBlackBy(&leds[startLed], endLed - startLed, controlState.effectConfigs[16].fadeSpeed);
  
  for(int i = 0; i < segmentSize; i++) {
    int pos = (position + i) % (endLed - startLed) + startLed;
    uint8_t brightness = 255 - (i * 255 / segmentSize);
    
    if(controlState.effectConfigs[16].colorMode == 1) {
      leds[pos] = CHSV((position * 2) % 256, 255, brightness);
    } else {
      leds[pos] = CRGB(
        (controlState.r * brightness) / 255,
        (controlState.g * brightness) / 255,
        (controlState.b * brightness) / 255
      );
    }
  }
  
  position = (position + speed) % (endLed - startLed);
  FastLED.show();
}

// Twinkle - Sterne-Effekt
void twinkle(int startLed, int endLed) {
  static uint8_t twinkleDensity = 0;
  
  fadeToBlackBy(&leds[startLed], endLed - startLed, controlState.effectConfigs[17].fadeSpeed);
  
  uint8_t density = map(controlState.effectConfigs[17].intensity, 0, 100, 1, 20);
  for(int i = 0; i < density; i++) {
    int pos = random(startLed, endLed);
    uint8_t brightness = random8(50, 255);
    
    if(controlState.effectConfigs[17].colorMode == 1) {
      leds[pos] = CHSV(random8(), 255, brightness);
    } else if(controlState.effectConfigs[17].colorMode == 2) {
      leds[pos] = CRGB(brightness, brightness, brightness);
    } else {
      leds[pos] = CRGB(
        (controlState.r * brightness) / 255,
        (controlState.g * brightness) / 255,
        (controlState.b * brightness) / 255
      );
    }
  }
  FastLED.show();
}

// Bouncing Balls - Springende Bälle
void bouncingBalls(int startLed, int endLed) {
  static float ballHeight[5];
  static float ballVelocity[5];
  static CRGB ballColors[5];
  static bool initialized = false;
  
  if(!initialized) {
    for(int i = 0; i < 5; i++) {
      ballHeight[i] = random(startLed, endLed);
      ballVelocity[i] = 0;
      ballColors[i] = CHSV(random8(), 255, 255);
    }
    initialized = true;
  }
  
  fadeToBlackBy(&leds[startLed], endLed - startLed, controlState.effectConfigs[18].fadeSpeed);
  
  uint8_t ballCount = map(controlState.effectConfigs[18].intensity, 0, 100, 1, 5);
  float gravity = map(controlState.effectConfigs[18].speed, 0, 100, 0.05, 0.5);
  
  for(int i = 0; i < ballCount; i++) {
    ballVelocity[i] -= gravity;
    ballHeight[i] += ballVelocity[i];
    
    if(ballHeight[i] < startLed) {
      ballHeight[i] = startLed;
      ballVelocity[i] = -ballVelocity[i] * 0.9;
      if(controlState.effectConfigs[18].colorMode == 1) {
        ballColors[i] = CHSV(random8(), 255, 255);
      }
    }
    
    int pos = (int)ballHeight[i];
    if(pos >= startLed && pos < endLed) {
      if(controlState.effectConfigs[18].colorMode == 0) {
        leds[pos] = CRGB(controlState.r, controlState.g, controlState.b);
      } else {
        leds[pos] = ballColors[i];
      }
    }
  }
  FastLED.show();
}

// Matrix Rain - Digital Regen
void matrixRain(int startLed, int endLed) {
  static uint8_t dropPos[20];
  static uint8_t dropSpeed[20];
  static bool initialized = false;
  
  if(!initialized) {
    for(int i = 0; i < 20; i++) {
      dropPos[i] = random8();
      dropSpeed[i] = random8(1, 5);
    }
    initialized = true;
  }
  
  fadeToBlackBy(&leds[startLed], endLed - startLed, controlState.effectConfigs[19].fadeSpeed);
  
  uint8_t dropCount = map(controlState.effectConfigs[19].intensity, 0, 100, 5, 20);
  uint8_t trailLength = map(controlState.effectConfigs[19].size, 0, 100, 5, 30);
  
  for(int i = 0; i < dropCount; i++) {
    for(int j = 0; j < trailLength; j++) {
      int pos = startLed + ((dropPos[i] - j * 5) % (endLed - startLed));
      if(pos >= startLed && pos < endLed) {
        uint8_t brightness = 255 - (j * 255 / trailLength);
        if(j == 0) brightness = 255;
        
        if(controlState.effectConfigs[19].colorMode == 0) {
          leds[pos] = CRGB(0, brightness, 0);
        } else if(controlState.effectConfigs[19].colorMode == 1) {
          leds[pos] = CHSV(96 + (i * 10), 255, brightness);
        } else {
          leds[pos] = CRGB(
            (controlState.r * brightness) / 255,
            (controlState.g * brightness) / 255,
            (controlState.b * brightness) / 255
          );
        }
      }
    }
    
    dropPos[i] = (dropPos[i] + dropSpeed[i]) % (endLed - startLed);
  }
  FastLED.show();
}

// === HILFSFUNKTIONEN ===

void setEffectParam(String param, int value) {
  uint8_t currentEffect = controlState.currentEffect;
  
  if(param == "speed") {
    controlState.effectConfigs[currentEffect].speed = constrain(value, 0, 100);
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