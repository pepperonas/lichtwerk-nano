#!/bin/bash

# LED Strip Controller - Alle Effekte Tester
# Testet alle 20 Effekte für je 10 Sekunden

# IP-Adresse des LED Controllers
LED_IP="192.168.2.136"

echo "🎨 LED Strip Controller - Effekte-Test gestartet"
echo "📡 Controller IP: $LED_IP"
echo "⏱️  Jeder Effekt läuft 10 Sekunden"
echo ""

# Auto-Modus ausschalten für manuellen Test
echo "🔧 Auto-Modus deaktivieren..."
curl -s --connect-timeout 5 --max-time 10 "http://$LED_IP/cmd?cmd=AUTO:0" > /dev/null
if [ $? -ne 0 ]; then
    echo "❌ Fehler: Kann Controller unter $LED_IP nicht erreichen!"
    echo "💡 Prüfe:"
    echo "   1. Ist der Controller eingeschaltet?"
    echo "   2. Ist die IP-Adresse korrekt?"
    echo "   3. Bist du im gleichen WiFi-Netzwerk?"
    echo ""
    echo "🔍 Teste Verbindung:"
    ping -c 1 $LED_IP
    exit 1
fi

# LEDs einschalten
echo "💡 LEDs einschalten..."
curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=ON:1" > /dev/null

# Helligkeit auf Maximum
echo "🔆 Helligkeit auf Maximum..."
curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=BRIGHT:255" > /dev/null

# Standard-Farbe setzen (Weiß)
echo "🎨 Standard-Farbe setzen (Weiß)..."
curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=COLOR:255,255,255" > /dev/null

echo ""
echo "🚀 Effekte-Test beginnt in 3 Sekunden..."
sleep 3

# Array mit Effekt-Namen für bessere Anzeige
effects=(
    "Lightning - Blitzeffekt"
    "Meteor - Laufender Schweif" 
    "Strobe - Stroboskop"
    "Firework - Feuerwerk-Explosionen"
    "Confetti - Bunte Konfetti-Punkte"
    "Rainbow Glitter - Regenbogen mit Glitzer"
    "Rapid Color Change - Schnelle Farbwechsel"
    "Police - Blaulicht-Effekt"
    "Energy Wave - Sinusförmige Wellen"
    "Solid Color - Einfarbig"
    "Theater Chase - Lauflichter mit Lücken"
    "Breathing - Sanftes Atmen"
    "Color Wipe - Progressive Farbfüllung"
    "Fire - Realistische Feuersimulation"
    "Plasma - Mathematische Interferenzmuster"
    "Sparkle - Intelligentes Funkeln"
    "Running Lights - Laufende Segmente"
    "Twinkle - Sternen-Himmel"
    "Bouncing Balls - Physik-Bälle"
    "Matrix Rain - Digitaler Regen"
)

# Alle 20 Effekte durchlaufen (0-19)
for i in {0..19}; do
    echo "🎭 Effekt $i: ${effects[$i]}"
    
    # Effekt aktivieren
    curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=EFFECT:$i" > /dev/null
    
    # Spezielle Parameter für bestimmte Effekte
    case $i in
        11) # Breathing - optimiert für sichtbaren Test
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:speed:60" > /dev/null
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:intensity:30" > /dev/null
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=COLOR:0,100,255" > /dev/null
            ;;
        13) # Fire - optimiert für sichtbares Feuer
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:speed:40" > /dev/null
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:intensity:70" > /dev/null
            ;;
        19) # Matrix Rain - klassisch grün
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=COLOR:0,255,0" > /dev/null
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:speed:60" > /dev/null
            ;;
        3) # Firework - Rainbow-Modus für bunte Explosionen
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:colorMode:1" > /dev/null
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:speed:70" > /dev/null
            ;;
        18) # Bouncing Balls - Rainbow-Modus
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:colorMode:1" > /dev/null
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:intensity:60" > /dev/null
            ;;
        *)
            # Standard-Parameter für die meisten Effekte
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:speed:50" > /dev/null
            curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:intensity:60" > /dev/null
            ;;
    esac
    
    # 10 Sekunden warten und Countdown anzeigen
    for countdown in {10..1}; do
        echo -ne "   ⏱️  $countdown Sekunden verbleibend...\r"
        sleep 1
    done
    echo -ne "   ✅ Effekt $i abgeschlossen!                    \n"
    echo ""
done

echo "🎉 Alle 20 Effekte erfolgreich getestet!"
echo ""
echo "💡 Tipp: Lieblings-Effekt gefunden? Speichere ihn als Preset:"
echo "   curl \"http://$LED_IP/cmd?cmd=PRESET:SAVE:0\""
echo ""
echo "🔄 LEDs auf sanftes Breathing umschalten..."
curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=EFFECT:11" > /dev/null
curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:speed:20" > /dev/null
curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=PARAM:intensity:25" > /dev/null
curl -s --connect-timeout 3 --max-time 5 "http://$LED_IP/cmd?cmd=COLOR:0,50,150" > /dev/null

echo "✨ Test abgeschlossen - Controller auf entspannten Breathing-Modus gesetzt"