#!/usr/bin/env python3
"""
Professional WiFi LED Controller für Arduino Nano 33 IoT
Vollständige Steuerung aller 20 Effekte mit allen Parametern
"""

import requests
import json
import time
import sys

class WiFiLEDController:
    def __init__(self, ip="192.168.2.136"):
        self.ip = ip
        self.base_url = f"http://{ip}"
        
    def send_command(self, cmd):
        """Befehl senden"""
        try:
            response = requests.get(f"{self.base_url}/cmd?cmd={cmd}", timeout=5)
            if response.status_code == 200:
                print(f"📤 {cmd} ✅")
                return True
            else:
                print(f"📤 {cmd} ❌ ({response.status_code})")
                return False
        except Exception as e:
            print(f"📤 {cmd} ❌ ({e})")
            return False
    
    def get_status(self):
        """Status abrufen"""
        try:
            response = requests.get(f"{self.base_url}/api/status", timeout=5)
            if response.status_code == 200:
                return response.json()
        except:
            pass
        return None
    
    def test_connection(self):
        """Verbindung testen"""
        print(f"🔍 Testing connection to {self.ip}...")
        status = self.get_status()
        if status:
            print("✅ Connection successful!")
            return True
        else:
            print(f"❌ Cannot reach Arduino at {self.ip}")
            return False

# Effekt-Definitionen (gleich wie BLE Version)
EFFECTS = [
    {"id": 0, "name": "Lightning", "desc": "Blitzeffekte", "cat": "Atmosphäre"},
    {"id": 1, "name": "Meteor", "desc": "Laufender Schweif", "cat": "Bewegung"},
    {"id": 2, "name": "Strobe", "desc": "Stroboskop", "cat": "Party"},
    {"id": 3, "name": "Firework", "desc": "Feuerwerk-Explosionen", "cat": "Party"},
    {"id": 4, "name": "Confetti", "desc": "Bunte Konfetti", "cat": "Party"},
    {"id": 5, "name": "Rainbow Glitter", "desc": "Regenbogen mit Glitzer", "cat": "Farben"},
    {"id": 6, "name": "Rapid Color Change", "desc": "Schnelle Farbwechsel", "cat": "Farben"},
    {"id": 7, "name": "Police", "desc": "Blaulicht", "cat": "Simulation"},
    {"id": 8, "name": "Energy Wave", "desc": "Energiewellen", "cat": "Sci-Fi"},
    {"id": 9, "name": "Solid Color", "desc": "Einfarbig", "cat": "Basic"},
    {"id": 10, "name": "Theater Chase", "desc": "Lauflichter", "cat": "Bewegung"},
    {"id": 11, "name": "Breathing", "desc": "Sanftes Atmen", "cat": "Entspannung"},
    {"id": 12, "name": "Color Wipe", "desc": "Farbwelle", "cat": "Bewegung"},
    {"id": 13, "name": "Fire", "desc": "Feuer-Simulation", "cat": "Simulation"},
    {"id": 14, "name": "Plasma", "desc": "Plasma-Muster", "cat": "Sci-Fi"},
    {"id": 15, "name": "Sparkle", "desc": "Funkeln", "cat": "Atmosphäre"},
    {"id": 16, "name": "Running Lights", "desc": "Laufende Segmente", "cat": "Bewegung"},
    {"id": 17, "name": "Twinkle", "desc": "Sterne", "cat": "Atmosphäre"},
    {"id": 18, "name": "Bouncing Balls", "desc": "Springende Bälle", "cat": "Physik"},
    {"id": 19, "name": "Matrix Rain", "desc": "Digitaler Regen", "cat": "Sci-Fi"}
]

# Szenarien (gleich wie BLE Version)
SCENARIOS = {
    "party": ["AUTO:0", "EFFECT:3", "PARAM:speed:85", "PARAM:colorMode:1", "BRIGHT:255"],
    "relax": ["AUTO:0", "EFFECT:11", "PARAM:speed:15", "PARAM:intensity:10", "COLOR:0,50,150"],
    "fire": ["AUTO:0", "EFFECT:13", "PARAM:speed:30", "PARAM:intensity:60", "PARAM:colorMode:0"],
    "matrix": ["AUTO:0", "EFFECT:19", "COLOR:0,255,0", "PARAM:speed:70", "PARAM:intensity:80"],
    "police": ["AUTO:0", "EFFECT:7", "PARAM:speed:95", "PARAM:intensity:100", "BRIGHT:255"],
    "rainbow": ["AUTO:0", "EFFECT:5", "PARAM:speed:50", "PARAM:colorMode:1", "PARAM:intensity:70"]
}

def interactive_mode(controller):
    """Interaktiver Steuerungsmodus"""
    print(f"\n🎛️  WiFi LED Controller - {controller.ip}")
    print("📡 Fast & Reliable WiFi Control")
    print("=" * 50)
    
    show_help()
    
    while True:
        try:
            cmd = input("\n💡 LED> ").strip()
            
            if not cmd:
                continue
                
            if cmd.lower() in ["quit", "exit", "q"]:
                break
            elif cmd.lower() in ["help", "h", "?"]:
                show_help()
            elif cmd.lower() == "effects":
                show_effects()
            elif cmd.lower() == "scenarios":
                show_scenarios()
            elif cmd.lower() == "status":
                show_status(controller)
            elif cmd.lower() == "on":
                controller.send_command("ON:1")
            elif cmd.lower() == "off":
                controller.send_command("OFF:1")
            elif cmd.lower() == "auto":
                controller.send_command("AUTO:1")
            elif cmd.lower() == "manual":
                controller.send_command("AUTO:0")
            elif " " in cmd:
                handle_parameter_command(controller, cmd)
            elif cmd.lower() in SCENARIOS:
                activate_scenario(controller, cmd.lower())
            elif cmd.lower() == "test":
                test_all_effects(controller)
            else:
                print(f"❌ Unknown command: '{cmd}'")
                print("💡 Type 'help' for available commands")
                
        except KeyboardInterrupt:
            print("\n👋 Exiting...")
            break
        except Exception as e:
            print(f"❌ Error: {e}")

def show_help():
    """Hilfe anzeigen"""
    print("\n📖 Available Commands:")
    print("Basic: on/off, auto/manual, brightness <N>, effect <N>")
    print("Params: speed <N>, intensity <N>, color <r,g,b>, direction <0|1>")
    print("Scenarios: party, relax, fire, matrix, police, rainbow")
    print("Info: effects, status, test, help, quit")

def show_effects():
    """Alle Effekte anzeigen"""
    print("\n🎭 Available Effects:")
    for effect in EFFECTS:
        print(f"   {effect['id']:2d}: {effect['name']} - {effect['desc']}")

def show_scenarios():
    """Szenarien anzeigen"""
    print("\n🎬 Preset Scenarios:")
    scenario_names = {
        "party": "🎉 Party Mode",
        "relax": "🌊 Entspannung", 
        "fire": "🔥 Kamin",
        "matrix": "🎬 Matrix",
        "police": "🚔 Alarm",
        "rainbow": "🌈 Regenbogen"
    }
    for key in SCENARIOS.keys():
        print(f"   {key:8s} - {scenario_names.get(key, key)}")

def show_status(controller):
    """Status anzeigen"""
    status = controller.get_status()
    if status:
        print("\n📊 Current Status:")
        print(f"   Power:      {'🟢 On' if status['on'] else '🔴 Off'}")
        print(f"   Mode:       {'🔄 Auto' if status['auto'] else '🎛️  Manual'}")
        print(f"   Effect:     {status['effect']} ({EFFECTS[status['effect']]['name']})")
        print(f"   Brightness: {status['brightness']}")
        print(f"   Color:      RGB({status['color'][0]}, {status['color'][1]}, {status['color'][2]})")
        config = status['effectConfig']
        print(f"   Speed:      {config['speed']}")
        print(f"   Intensity:  {config['intensity']}")
    else:
        print("❌ Cannot get status")

def handle_parameter_command(controller, cmd):
    """Parameter-Befehle verarbeiten"""
    parts = cmd.split()
    if len(parts) < 2:
        print("❌ Missing parameter value")
        return
        
    param = parts[0].lower()
    value = parts[1]
    
    try:
        if param == "brightness":
            val = int(value)
            if 0 <= val <= 255:
                controller.send_command(f"BRIGHT:{val}")
        elif param == "effect":
            val = int(value)
            if 0 <= val <= 19:
                controller.send_command(f"EFFECT:{val}")
                print(f"🎭 Activated: {EFFECTS[val]['name']}")
        elif param in ["speed", "intensity", "size", "colormode", "fadespeed"]:
            val = int(value)
            if 0 <= val <= 100:
                controller.send_command(f"PARAM:{param}:{val}")
        elif param == "direction":
            val = int(value)
            if val in [0, 1]:
                controller.send_command(f"PARAM:direction:{val}")
        elif param == "color":
            if "," in value:
                rgb = [int(x.strip()) for x in value.split(",")]
                if len(rgb) == 3 and all(0 <= x <= 255 for x in rgb):
                    controller.send_command(f"COLOR:{rgb[0]},{rgb[1]},{rgb[2]}")
        else:
            print(f"❌ Unknown parameter: {param}")
    except ValueError:
        print("❌ Invalid number format")

def activate_scenario(controller, scenario_key):
    """Szenario aktivieren"""
    commands = SCENARIOS[scenario_key]
    print(f"🎬 Activating scenario: {scenario_key}")
    
    for cmd in commands:
        controller.send_command(cmd)
        time.sleep(0.2)
    print("✅ Scenario activated!")

def test_all_effects(controller):
    """Alle Effekte testen"""
    print("🧪 Testing all effects (3 seconds each)...")
    
    controller.send_command("AUTO:0")
    controller.send_command("ON:1")
    controller.send_command("BRIGHT:255")
    
    for effect in EFFECTS:
        print(f"🎭 Testing Effect {effect['id']:2d}: {effect['name']}")
        controller.send_command(f"EFFECT:{effect['id']}")
        time.sleep(3)
    
    print("✅ Test sequence completed!")

def main():
    """Hauptfunktion"""
    print("🎨 Professional WiFi LED Controller")
    print("📡 Fast & Reliable WiFi Control for Arduino Nano 33 IoT")
    print("=" * 60)
    
    # IP aus Argumenten oder Standard
    ip = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith('-') else "192.168.2.136"
    
    controller = WiFiLEDController(ip)
    
    if len(sys.argv) > 1:
        mode = sys.argv[1].lower()
        
        if mode == "test":
            if controller.test_connection():
                test_all_effects(controller)
            return
        elif mode == "status":
            if controller.test_connection():
                show_status(controller)
            return
        elif mode in ["help", "-h", "--help"]:
            print("\n📖 Usage:")
            print("   python3 wifi_control.py [IP]      # Interactive mode")
            print("   python3 wifi_control.py test      # Test all effects") 
            print("   python3 wifi_control.py status    # Show status")
            print("   python3 wifi_control.py help      # Show this help")
            return
    
    # Verbindung testen und interaktiver Modus
    if controller.test_connection():
        interactive_mode(controller)
    else:
        print("💡 Make sure the Arduino is powered on and connected to WiFi")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n👋 Goodbye!")
    except Exception as e:
        print(f"\n❌ Error: {e}")