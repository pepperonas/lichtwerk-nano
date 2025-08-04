# LED Strip Controller - Professional Edition

## Übersicht
Professioneller LED-Strip Controller für 600 LEDs mit 20 vollständig implementierten Effekten, FastLED-Optimierungen, WiFi-Steuerung, Preset-System und persistenter Flash-Speicherung.

## 🚀 Quick Start

**Sofort loslegen in 3 Schritten:**
1. **Setup**: `secrets_template.h` → `secrets.h` mit deinen WiFi-Daten
2. **Upload**: Code auf Arduino Nano 33 IoT hochladen  
3. **Control**: `python3 wifi_control.py` oder `./test_all_effects.sh`

**Erste Befehle:**
```bash
# Alle Effekte testen (vollautomatisch)
./test_all_effects.sh

# Interaktive Steuerung
python3 wifi_control.py
LED> party      # Party-Modus
LED> relax      # Entspannung
LED> fire       # Kamin-Atmosphäre
```

## ✨ Professional Features
- **🎨 20 vollständige Effekte** - alle hochwertig implementiert mit FastLED-Best-Practices
- **⚙️ Experten-Parameter** - jeder Effekt hat 6 präzise konfigurierbare Parameter
- **💾 10 Preset-Speicherplätze** für Lieblings-Einstellungen
- **🎛️ Zonen-System** - bis zu 3 verschiedene Effekte gleichzeitig
- **📱 Dual-Protocol** - WiFi + Bluetooth Low Energy für maximale Erreichbarkeit
- **🔄 Flash-Speicherung** - alle Einstellungen überleben Neustart
- **⚡ 50fps Performance** - Non-blocking Timing für flüssige Animationen
- **🧠 Intelligentes State-Management** - Optimiert für 600 LEDs
- **🌟 Spektakuläre Startup-Animation** - WiFi-Erfolg wird mit Dual-Meteor-Show gefeiert
- **🎨 Hybrid-Optimierung** - Dual-Power + adaptive Farbkorrektur für perfekte Farben

## Professional Effekt-Bibliothek

### 20 vollständig implementierte Effekte (0-19)

**Atmosphäre & Natur (0-9):**
- 0: **Lightning** - Realistische Blitzeinschläge mit variabler Intensität
- 1: **Meteor** - Laufender Schweif mit Physik-basiertem Trailing
- 2: **Strobe** - Hochfrequenz-Stroboskop mit präzisem Timing
- 3: **Firework** - Explodierende Feuerwerke mit Radius-Animation
- 4: **Confetti** - Zufällige Farbpunkte mit intelligentem Fading
- 5: **Rainbow Glitter** - Regenbogen-Hintergrund mit Glitzer-Overlay
- 6: **Rapid Color Change** - Schnelle Farbwechsel in verschiedenen Modi
- 7: **Police** - Blaulicht-Effekt mit realistischen Mustern
- 8: **Energy Wave** - Sinusförmige Energiewellen mit Farbgradienten
- 9: **Solid Color** - Präzise Farbwiedergabe für Basisbeleuchtung

**Animation & Bewegung (10-19):**
- 10: **Theater Chase** - Lauflichter mit konfigurierbaren Lücken
- 11: **Breathing** - Sanftes Atmen mit Sinus-Kurven-Mathematik
- 12: **Color Wipe** - Progressives Füllen mit Farbverläufen
- 13: **Fire** - Realistische Feuersimulation mit Heat-Diffusion-Algorithmus
- 14: **Plasma** - Mathematische Interferenzmuster (3 Sinus-Wellen)
- 15: **Sparkle** - Intelligentes Funkeln mit natürlicher Verteilung
- 16: **Running Lights** - Laufende Segmente mit Helligkeitsverläufen
- 17: **Twinkle** - Sternen-Himmel-Simulation mit realistischem Flackern
- 18: **Bouncing Balls** - Physik-Engine mit Gravitation und Dämpfung
- 19: **Matrix Rain** - Digitaler Regen mit authentischen Trails

### Professional Parameter-System
Jeder Effekt verfügt über 6 präzise konfigurierbare Parameter:

- **speed** (0-100): Animation-Geschwindigkeit (mathematisch gemappt für optimale Sichtbarkeit)
- **intensity** (0-100): Effekt-Intensität/Dichte (z.B. Anzahl Partikel, Helligkeit)
- **size** (0-100): Element-Größe (Meteor-Länge, Feuer-Höhe, Ball-Anzahl)
- **direction** (0/1): Bewegungsrichtung (vorwärts/rückwärts)
- **colorMode** (0-2): Farbmodus
  - 0 = **Manual**: Verwendung der manuell gesetzten RGB-Farbe
  - 1 = **Rainbow**: Automatische Regenbogen-Farbzyklen
  - 2 = **Special**: Effekt-spezifische Modi (weiß für Sparkle, etc.)
- **fadeSpeed** (0-100): Ausblend-Geschwindigkeit für Trails und Übergänge

### Technische Optimierungen
- **Non-blocking Design**: Alle Effekte laufen ohne `delay()` - keine Blockierung der Hauptschleife
- **State-Management**: Globale `EffectState` Struktur verhindert Konflikte zwischen Effekten
- **Memory-Efficiency**: Optimiert für 600 LEDs ohne Speicher-Überschreitung
- **50fps Rendering**: Konstante 20ms Updatezyklen für flüssige Animationen
- **FastLED Integration**: Nutzt `fadeToBlackBy()`, `sin8()`, `HeatColor()` für Performance
- **Dual-Power Optimized**: Hardware-Design für zwei Stromquellen bei langen LED-Strips

### WiFi Befehle

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

## Professional Anwendungsbeispiele

### 🎉 Party-Spektakel
```bash
AUTO:0
EFFECT:3          # Firework-Explosionen
PARAM:speed:90    # Sehr schnelle Explosionen
PARAM:intensity:85 # Hohe Dichte
PARAM:size:70     # Große Explosionen
PARAM:colorMode:1 # Rainbow-Modus
```

### 🔥 Kamin-Atmosphäre
```bash
AUTO:0
EFFECT:13         # Realistische Fire-Simulation
PARAM:speed:30    # Langsame, natürliche Bewegung
PARAM:intensity:60 # Moderate Flammen-Intensität
PARAM:size:40     # Mittlere Flammen-Höhe
PARAM:colorMode:0 # Klassische Feuer-Farben
```

### 🌊 Entspannungs-Wellen
```bash
AUTO:0
EFFECT:11         # Breathing-Effekt
PARAM:speed:15    # Sehr langsames Atmen
PARAM:intensity:25 # Sanfte Übergänge
PARAM:colorMode:0 # Beruhigende blaue Farbe
COLOR:0,50,150
```

### 🎬 Matrix-Cyber-Zone
```bash
AUTO:0
EFFECT:19         # Matrix Rain
PARAM:speed:75    # Schnelle Datenströme
PARAM:intensity:80 # Viele Tropfen
PARAM:size:60     # Lange Trails
PARAM:colorMode:0 # Klassisch grün
COLOR:0,255,0
```

### 🏀 Bouncing Balls Party
```bash
AUTO:0
EFFECT:18         # Bouncing Balls mit Physik
PARAM:speed:80    # Schnelle Bewegung
PARAM:intensity:70 # 3-4 Bälle gleichzeitig
PARAM:size:50     # Moderate Gravitation
PARAM:colorMode:1 # Rainbow-Bälle
```

### 🚔 Notfall-Simulation
```bash
AUTO:0
EFFECT:7          # Police-Lights
PARAM:speed:95    # Sehr schnelle Wechsel
PARAM:intensity:100 # Maximale Intensität
```

### ⭐ Sternenhimmel
```bash
AUTO:0
EFFECT:17         # Twinkle-Sterne
PARAM:speed:25    # Langsames Funkeln
PARAM:intensity:40 # Moderate Sternen-Dichte
PARAM:fadeSpeed:15 # Langsames Ausblenden
PARAM:colorMode:2 # Weißes Licht
```

### 🎭 Theater-Show
```bash
# Zone-basierte Show mit 3 gleichzeitigen Effekten
ZONE:0:10         # Theater Chase vorne
ZONE:1:14         # Plasma-Muster Mitte
ZONE:2:5          # Rainbow Glitter hinten
PARAM:speed:60    # Synchrone mittlere Geschwindigkeit
```

### 🎨 Statische Farben (Solid Color)
```bash
# Über Python Controller (einfachster Weg)
LED> solid 255,255,255       # Perfektes Weiß
LED> solid 255,0,0           # Klassisches Rot
LED> solid 0,255,0           # Grün
LED> solid 0,0,255           # Blau
LED> solid 255 255 0         # Gelb (alternative Syntax)

# Über WiFi-Befehle
AUTO:0
EFFECT:9          # Effekt 9 = Solid Color
COLOR:255,255,255 # Weiße Farbe
```

## 🔧 Hybrid-Optimierung: Hardware + Software

### Dual-Power Setup mit Feinabstimmung
**Problem:** Selbst mit zwei Stromquellen können minimale Farbunterschiede zwischen den Strip-Hälften auftreten, bedingt durch LED-Toleranzen oder unterschiedliche Kabellängen.

**Lösung:** Kombination aus Hardware- und Software-Optimierung:

#### Hardware: Dual-Power Setup
- **Erste Hälfte (LEDs 0-299)**: Stromeinspeisung am Anfang des Strips
- **Zweite Hälfte (LEDs 300-599)**: Zusätzliche Stromeinspeisung in der Mitte

**Hardware-Setup:**
```
Arduino Nano 33 IoT ──► LED 0 ──► LED 299
                         ↓         ↑
                    Netzteil 1  Netzteil 2
                                  ↓
                              LED 300 ──► LED 599
```

#### Software: Adaptive Farbkorrektur
**Feinabstimmung für perfekte Farbharmonie:**
- **LEDs 0-299**: Originalfarben (erste Strip-Hälfte)
- **LEDs 300-599**: Sanfte Korrektur (zweite Strip-Hälfte)
- **Algorithmus**: Minimale Anpassung für einheitliches Erscheinungsbild

**Konfigurierbar im Code:**
```cpp
#define CORRECTION_RED_FACTOR   0.90    // -10% Rot
#define CORRECTION_GREEN_FACTOR 0.85    // -15% Grün  
#define CORRECTION_BLUE_FACTOR  1.0     // Blau unverändert
```

**Vorteile der Hybrid-Lösung:**
- Hauptlast durch Hardware-Optimierung (Dual-Power)
- Feinabstimmung durch Software für perfekte Ergebnisse
- Anpassbar an individuelle LED-Strip-Charakteristiken
- Zukunftssicher und flexibel konfigurierbar

**Technische Spezifikationen:**
- **Netzteil 1 & 2**: Je 5V/5A (empfohlen für optimale Performance)
- **Datenleitung**: Bleibt durchgehend vom Arduino bis LED 599
- **Ground-Verbindung**: Alle Netzteile und Arduino müssen gemeinsames GND haben

## Hardware & Installation

### System-Spezifikationen
- **Mikrocontroller**: Arduino Nano 33 IoT (ARM Cortex-M0+, 48MHz)
- **LEDs**: 600x WS2812B addressierbare RGB-LEDs
- **Netzteil**: 5V/10A (empfohlen für 600 LEDs bei maximaler Helligkeit)
- **Wireless**: WiFi 2.4GHz + Bluetooth Low Energy
- **Speicher**: 
  - Programm: 93,396 bytes (35% von 256KB) 
  - RAM: 10,344 bytes (31% von 32KB)
  - **Optimiert für 600 LEDs ohne Memory-Overflow**

### Installation der Bibliotheken
1. **FastLED**: Arduino IDE → Sketch → Include Library → Manage Libraries → "FastLED"
2. **WiFiNINA**: Meist vorinstalliert für Nano 33 IoT
3. **ArduinoBLE**: Arduino IDE → Sketch → Include Library → Manage Libraries → "ArduinoBLE"
4. **FlashStorage**: Arduino IDE → Sketch → Include Library → Manage Libraries → "FlashStorage"

### Pinbelegung
- **LED Data Pin**: Digital Pin 5
- **Power**: 5V/GND (externes Netzteil empfohlen)
- **WiFi**: Onboard Nano 33 IoT

### Startup-Sequenz
**Beim Einschalten erlebst du eine spektakuläre Startup-Show:**
1. **500ms Weißes Aufleuchten** - Bestätigt LED-Initialisierung
2. **WiFi-Verbindung** - Sucht automatisch dein Netzwerk
3. **Bei WiFi-Erfolg**: 
   - ⚡ Zwei weiße Meteors rasen von beiden Enden zur Mitte
   - ✨ 3 Sekunden weißes Funkeln wenn sie sich treffen
   - 🎯 Strip geht aus und ist bereit für Steuerung

### Erste Schritte
1. **WiFi konfigurieren**: 
   - `secrets_template.h` zu `secrets.h` kopieren
   - Deine WiFi-Daten in `secrets.h` eintragen
2. **LED-Anzahl prüfen** (NUM_LEDS = 600)
3. **Code hochladen**
4. **Startup-Animation genießen** 📺
5. **Steuerung wählen**:
   - **WiFi**: `python3 wifi_control.py` oder Web-Browser
   - **BLE**: Mobile App oder BLE Terminal
   - **Auto-Test**: `./test_all_effects.sh`

### 🔐 Sicherheit & Performance
- **WiFi-Credentials**: Sichere Speicherung in `secrets.h` (Git-ignoriert)
- **Dual-Protocol**: WiFi + Bluetooth Low Energy für maximale Erreichbarkeit
- **Flash-Persistenz**: Alle Einstellungen überleben Stromausfall
- **Memory-Safe**: Buffer-Overflow-Schutz für 600-LED-Arrays
- **Non-blocking**: Keine `delay()`-Aufrufe blockieren WiFi-Communication
- **Error-Handling**: Robuste Fehlerbehandlung für Netzwerk-Verbindungen
- **Auto-Reconnect**: Intelligente WiFi-Wiederverbindung bei Verbindungsabbrüchen
- **Professional Power**: Ausgelegt für Dual-Power-Setup bei 600 LEDs

### 🚀 Performance-Metriken
- **Framerate**: Konstante 50fps (20ms Update-Zyklen)
- **Kompilierungszeit**: ~5-10 Sekunden
- **Boot-Zeit**: ~3 Sekunden bis WiFi-Verbindung
- **Response-Zeit**: <50ms für WiFi-Befehle
- **Memory-Footprint**: 69% freier Speicher für weitere Features

## 🛠️ Control Tools & Utilities

### 🐚 Bash Test Script (`test_all_effects.sh`)
Automatisierter Test aller 20 Effekte mit optimierten Parametern:
```bash
chmod +x test_all_effects.sh
./test_all_effects.sh
```
- **Vollautomatisch**: Testet alle Effekte für je 10 Sekunden
- **Optimierte Parameter**: Jeder Effekt mit best-practice Einstellungen
- **Netzwerk-Test**: Prüft Verbindung vor dem Start
- **Live-Countdown**: Zeigt verbleibende Zeit pro Effekt

### 🐍 Python WiFi Controller (`wifi_control.py`)
Professionelle interaktive Steuerung über WiFi:
```bash
python3 wifi_control.py [IP-Adresse]
```

**Features:**
- **Interaktiver Modus**: Live-Kommandozeile mit Auto-Completion
- **Scenario-Presets**: Vorgefertigte Szenarien (party, relax, fire, matrix...)  
- **Parameter-Control**: Alle 6 Effekt-Parameter steuerbar
- **Static Colors**: `solid <r,g,b>` Shortcut für statische Farben
- **Status-Monitoring**: Live-Status mit JSON-API
- **Bulk-Testing**: Automatischer Test aller Effekte

**Beispiel-Commands:**
```
LED> party                    # Aktiviert Party-Szenario
LED> effect 13                # Feuer-Effekt
LED> speed 80                 # Geschwindigkeit auf 80%
LED> color 255,0,0           # Rote Farbe
LED> solid 255,255,255       # Statische weiße Farbe (Shortcut)
LED> brightness 200          # Helligkeit auf 200
LED> status                  # Zeigt aktuellen Status
```

### 📱 Bluetooth Low Energy (BLE)
- **Device Name**: "LED Controller 600"
- **Gleiche Befehle**: Identische Commands wie WiFi
- **Backup-Connection**: Funktioniert auch ohne WiFi
- **Mobile Apps**: Kompatibel mit BLE Terminal Apps

### 🛠️ Entwickler-Features
- **Modular Design**: Effekte einfach erweiterbar
- **Debug-Output**: Serial Monitor zeigt detaillierte Status-Infos
- **Parameter-Mapping**: Mathematisch optimierte Wertebereiche
- **FastLED-Integration**: Native Nutzung aller FastLED-Funktionen