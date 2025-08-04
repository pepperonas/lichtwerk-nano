# LED Strip Controller - Erweiterte Version

## Übersicht
Erweiterter LED-Strip Controller für 600 LEDs mit 20 konfigurierbaren Effekten, BLE/WiFi-Steuerung, Preset-System und persistenter Flash-Speicherung.

## ✨ Neue Features dieser Version
- **🎨 20 Effekte** statt 10 (inkl. Fire, Matrix Rain, Bouncing Balls)
- **⚙️ Vollständig konfigurierbar** - jeder Effekt hat 6 Parameter
- **💾 10 Preset-Speicherplätze** für Lieblings-Einstellungen
- **🎛️ Zonen-System** - bis zu 3 verschiedene Effekte gleichzeitig
- **📱 Erweiterte APIs** - BLE + WiFi mit detaillierten Status-Infos
- **🔄 Flash-Speicherung** - alle Einstellungen überleben Neustart

## Neue Features

### 20 Effekte (0-19)
**Original-Effekte (0-9):**
- 0: Lightning - Blitzeffekt
- 1: Meteor - Meteor mit Schweif
- 2: Strobe - Stroboskop
- 3: Firework - Feuerwerk
- 4: Confetti - Konfetti
- 5: Rainbow Glitter - Regenbogen mit Glitzer
- 6: Rapid Color Change - Schneller Farbwechsel
- 7: Police - Polizeilicht
- 8: Energy Wave - Energiewelle
- 9: Solid Color - Einfarbig

**Neue Effekte (10-19):**
- 10: Theater Chase - Lauflichter mit Lücken
- 11: Breathing - Sanftes Atmen
- 12: Color Wipe - Farbwelle
- 13: Fire Effect - Feuer-Simulation
- 14: Plasma - Mathematische Muster
- 15: Sparkle - Funkeln
- 16: Running Lights - Laufende Segmente
- 17: Twinkle - Sterne
- 18: Bouncing Balls - Springende Bälle
- 19: Matrix Rain - Digital-Regen

### Effekt-Parameter
Jeder Effekt hat konfigurierbare Parameter:
- **speed** (0-100): Geschwindigkeit
- **intensity** (0-100): Intensität/Dichte
- **size** (0-100): Größe der Elemente
- **direction** (0/1): Richtung
- **colorMode** (0-2): 0=Manuell, 1=Rainbow, 2=Random/Special
- **fadeSpeed** (0-100): Ausblendgeschwindigkeit

### BLE/WiFi Befehle

#### Basis-Befehle
- `ON:1` - LEDs einschalten
- `OFF:1` - LEDs ausschalten
- `AUTO:1` - Auto-Modus ein (0=aus)
- `EFFECT:<nr>` - Effekt wählen (0-19)
- `BRIGHT:<wert>` - Helligkeit (0-255)
- `COLOR:<r>,<g>,<b>` - Farbe setzen (je 0-255)
- `DURATION:<sek>` - Auto-Effekt Dauer

#### Neue Befehle
- `PARAM:<param>:<wert>` - Parameter für aktuellen Effekt
  - Beispiel: `PARAM:speed:75`
  - Beispiel: `PARAM:colorMode:1`
  
- `PRESET:SAVE:<slot>` - Einstellung speichern (0-9)
- `PRESET:LOAD:<slot>` - Einstellung laden (0-9)

- `ZONE:<zone>:<effect>` - Effekt pro Zone (0-2)
  - Beispiel: `ZONE:0:5` - Zone 0 auf Effekt 5
  
- `FADE:<zeit>` - Übergangszeit (x100ms)

### Web-API

#### Status abrufen
```
GET http://<IP>/api/status

Response:
{
  "on": true,
  "auto": false,
  "effect": 10,
  "brightness": 255,
  "color": [255, 0, 0],
  "zones": 1,
  "effectConfig": {
    "speed": 50,
    "intensity": 50,
    "size": 50,
    "direction": true,
    "colorMode": 0,
    "fadeSpeed": 20
  }
}
```

#### Befehl senden
```
GET http://<IP>/cmd?cmd=EFFECT:15
GET http://<IP>/cmd?cmd=PARAM:speed:80
GET http://<IP>/cmd?cmd=COLOR:0,255,0
GET http://<IP>/cmd?cmd=PRESET:SAVE:1
```

### Zonen-Modus
Der Strip kann in bis zu 3 Zonen unterteilt werden:
- Zone 0: LEDs 0-199
- Zone 1: LEDs 200-399  
- Zone 2: LEDs 400-599

Verschiedene Effekte können gleichzeitig in verschiedenen Zonen laufen.

### Presets
- 10 Speicherplätze für Lieblings-Einstellungen
- Speichert: Effekt, Farbe, Helligkeit und alle Parameter
- Persistent im Flash-Speicher gespeichert

### Flash-Speicherung
Beim Neustart werden automatisch geladen:
- Letzter Zustand (Ein/Aus)
- Aktueller Effekt und Parameter
- Helligkeit und Farbe
- Alle Presets

**Benötigte Bibliotheken:**
- FastLED
- WiFiNINA
- ArduinoBLE
- FlashStorage (für Arduino Nano 33 IoT)

## Beispiel-Sequenzen

### Party-Modus
```
AUTO:0
EFFECT:2
PARAM:speed:90
PARAM:intensity:80
COLOR:255,0,255
```

### Entspannungs-Modus
```
AUTO:0
EFFECT:11
PARAM:speed:20
PARAM:intensity:30
COLOR:0,100,255
```

### Multi-Zonen Demo
```
ZONE:0:13
ZONE:1:5
ZONE:2:18
PARAM:speed:50
```

## Hardware & Installation
- **Arduino Nano 33 IoT**
- **600x WS2812B LEDs**
- **5V/10A Netzteil**
- **WiFi: 2.4GHz**
- **BLE: Low Energy**

### Installation der Bibliotheken
1. **FastLED**: Arduino IDE → Sketch → Include Library → Manage Libraries → "FastLED"
2. **WiFiNINA**: Meist vorinstalliert für Nano 33 IoT
3. **ArduinoBLE**: Meist vorinstalliert für Nano 33 IoT  
4. **FlashStorage**: Arduino IDE → Sketch → Include Library → Manage Libraries → "FlashStorage"

### Pinbelegung
- **LED Data Pin**: Digital Pin 5
- **Power**: 5V/GND (externes Netzteil empfohlen)
- **WiFi/BLE**: Onboard Nano 33 IoT

### Erste Schritte
1. **WiFi konfigurieren**: 
   - `secrets_template.h` zu `secrets.h` kopieren
   - Deine WiFi-Daten in `secrets.h` eintragen
2. **LED-Anzahl prüfen** (NUM_LEDS = 600)
3. **Code hochladen**
4. **Serial Monitor öffnen** für IP-Adresse
5. **BLE-App oder Web-Browser** für Steuerung nutzen

### 🔐 Sicherheit
- WiFi-Passwörter werden in `secrets.h` gespeichert
- Diese Datei wird automatisch von Git ignoriert
- Nur `secrets_template.h` wird in Repository committed