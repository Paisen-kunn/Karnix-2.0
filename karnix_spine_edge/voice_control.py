#!/usr/bin/env python3
"""
Voice Control for Karnix Spine Screening System
Uses macOS built-in speech recognition (offline)
"""

import subprocess
import sys
import os
import time

def speak(text):
    """Use macOS 'say' command for voice feedback"""
    subprocess.run(['say', '-v', 'Daniel', text])

def listen_for_command():
    """
    Listen for voice input using macOS dictation
    Returns the recognized text
    """
    print("\n🎤 Listening for command... (speak now)")
    print("    Say: 'setup', 'run scan', 'test system', 'stop', or 'help'")
    
    # Use osascript to trigger dictation and get text
    script = '''
    tell application "System Events"
        set userText to text returned of (display dialog "Speak your command:" default answer "" giving up after 1)
    end tell
    return userText
    '''
    
    # Alternative: use simple input for now, can be replaced with actual speech recognition
    try:
        # For macOS, we'll use a simpler approach - keyboard input
        # In production, integrate with macOS Speech Recognition API
        command = input("\n🎤 Speak command (or type): ").strip().lower()
        return command
    except KeyboardInterrupt:
        return "stop"
    except Exception as e:
        print(f"Error: {e}")
        return ""

def execute_command(command):
    """Execute the recognized command"""
    command = command.lower().strip()
    
    if not command:
        return True
    
    # Setup commands
    if 'setup' in command or 'install' in command:
        speak("Starting system setup")
        print("\n" + "="*70)
        print("  RUNNING SETUP")
        print("="*70 + "\n")
        result = subprocess.run(['./setup.sh'], cwd='/Users/sudharsans/Orion/karnix_spine_edge')
        if result.returncode == 0:
            speak("Setup completed successfully")
        else:
            speak("Setup failed. Please check the output")
        return True
    
    # Run spine screening
    elif 'scan' in command or 'run' in command or 'process' in command or 'start' in command:
        speak("Starting spine screening analysis")
        print("\n" + "="*70)
        print("  RUNNING SPINE SCREENING")
        print("="*70 + "\n")
        result = subprocess.run(['./spine_cli'], cwd='/Users/sudharsans/Orion/karnix_spine_edge')
        if result.returncode == 0:
            speak("Screening completed successfully")
        else:
            speak("Screening failed. Please check the output")
        return True
    
    # Test validation
    elif 'test' in command or 'validate' in command or 'check' in command:
        speak("Running system validation tests")
        print("\n" + "="*70)
        print("  RUNNING VALIDATION TESTS")
        print("="*70 + "\n")
        result = subprocess.run(['./test_uci_vertebral'], cwd='/Users/sudharsans/Orion/karnix_spine_edge')
        if result.returncode == 0:
            speak("Validation completed")
        else:
            speak("Please compile the test program first")
        return True
    
    # Compile system
    elif 'compile' in command or 'build' in command:
        speak("Compiling spine screening system")
        print("\n" + "="*70)
        print("  COMPILING SYSTEM")
        print("="*70 + "\n")
        result = subprocess.run([
            'clang++', '-std=c++14', '-O3', 'spine_cli.c++', '-o', 'spine_cli'
        ], cwd='/Users/sudharsans/Orion/karnix_spine_edge')
        if result.returncode == 0:
            speak("Compilation successful")
        else:
            speak("Compilation failed. Please check for errors")
        return True
    
    # Show status
    elif 'status' in command or 'info' in command:
        speak("Checking system status")
        print("\n" + "="*70)
        print("  SYSTEM STATUS")
        print("="*70)
        
        # Check if compiled
        if os.path.exists('/Users/sudharsans/Orion/karnix_spine_edge/spine_cli'):
            print("✓ spine_cli: Compiled")
        else:
            print("✗ spine_cli: Not compiled")
        
        # Check images
        images_dir = '/Users/sudharsans/Orion/karnix_spine_edge/data/images'
        if os.path.exists(images_dir):
            image_count = len([f for f in os.listdir(images_dir) if f.endswith(('.jpg', '.jpeg', '.png', '.ppm', '.pgm'))])
            print(f"✓ Images: {image_count} files found")
        else:
            print("✗ Images: Directory not found")
        
        # Check database
        db_path = '/Users/sudharsans/Orion/karnix_spine_edge/data/spine_records.json'
        if os.path.exists(db_path):
            import json
            with open(db_path) as f:
                data = json.load(f)
                print(f"✓ Database: {len(data)} records")
        else:
            print("✗ Database: Not found")
        
        print("="*70)
        speak(f"System has {image_count if 'image_count' in locals() else 0} images ready for processing")
        return True
    
    # Help
    elif 'help' in command or 'commands' in command:
        speak("Available commands")
        print("\n" + "="*70)
        print("  AVAILABLE VOICE COMMANDS")
        print("="*70)
        print("  • 'setup' or 'install'     - Run system setup")
        print("  • 'scan' or 'run'          - Start spine screening")
        print("  • 'test' or 'validate'     - Run validation tests")
        print("  • 'compile' or 'build'     - Compile the system")
        print("  • 'status' or 'info'       - Show system status")
        print("  • 'help' or 'commands'     - Show this help")
        print("  • 'stop' or 'exit' or 'quit' - Exit voice control")
        print("="*70 + "\n")
        return True
    
    # Exit commands
    elif 'stop' in command or 'exit' in command or 'quit' in command or 'bye' in command:
        speak("Stopping voice control. Goodbye")
        return False
    
    else:
        speak("Command not recognized. Say help for available commands")
        print(f"❌ Unknown command: '{command}'")
        print("   Say 'help' for available commands")
        return True

def main():
    """Main voice control loop"""
    print("\n" + "="*70)
    print("  KARNIX SPINE SCREENING - VOICE CONTROL")
    print("="*70)
    print("\n  Voice control is now active!")
    print("  Speak or type commands to control the system.")
    print("  Say 'help' for available commands.\n")
    
    speak("Voice control activated. Say help for commands")
    
    while True:
        try:
            command = listen_for_command()
            
            if not execute_command(command):
                break
                
            time.sleep(0.5)  # Brief pause between commands
            
        except KeyboardInterrupt:
            print("\n\n⚠️  Interrupted by user")
            speak("Voice control stopped")
            break
        except Exception as e:
            print(f"\n❌ Error: {e}")
            speak("An error occurred")
            time.sleep(1)
    
    print("\n" + "="*70)
    print("  VOICE CONTROL ENDED")
    print("="*70 + "\n")

if __name__ == "__main__":
    # Check if running on macOS
    if sys.platform != 'darwin':
        print("⚠️  This voice control system requires macOS")
        sys.exit(1)
    
    # Check if in correct directory
    if not os.path.exists('/Users/sudharsans/Orion/karnix_spine_edge'):
        print("⚠️  Please run from karnix_spine_edge directory")
        sys.exit(1)
    
    main()
