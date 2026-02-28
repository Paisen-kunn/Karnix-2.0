#!/bin/bash
# Simple Voice Control Wrapper for Karnix Spine Screening
# Uses macOS 'say' for voice feedback

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

say_feedback() {
    echo -e "${BLUE}🔊 $1${NC}"
    # Enable voice feedback (runs in background)
    ( say -v Daniel "$1" 2>/dev/null & )
}

show_help() {
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  KARNIX SPINE SCREENING - VOICE COMMANDS"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    echo "  Available commands:"
    echo ""
    echo "  ${GREEN}./voice.sh setup${NC}      - Run system setup"
    echo "  ${GREEN}./voice.sh scan${NC}       - Start screening with voice report"
    echo "  ${GREEN}./voice.sh test${NC}       - Run validation tests"
    echo "  ${GREEN}./voice.sh compile${NC}    - Compile the system"
    echo "  ${GREEN}./voice.sh status${NC}     - Show system status"
    echo "  ${GREEN}./voice.sh help${NC}       - Show this help"
    echo ""
    echo "  Interactive mode:"
    echo "  ${GREEN}./voice.sh${NC}            - Start interactive voice control"
    echo ""
    echo "  ${BLUE}NEW:${NC} Voice report automatically speaks summary after scan!"
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo ""
}

run_setup() {
    say_feedback "Starting system setup"
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  RUNNING SETUP"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    ./setup.sh
    if [ $? -eq 0 ]; then
        say_feedback "Setup completed successfully"
    else
        say_feedback "Setup failed. Please check the output"
    fi
}

run_scan() {
    say_feedback "Starting spine screening analysis"
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  RUNNING SPINE SCREENING WITH VOICE REPORT"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    
    # Run with voice report
    if [ -f "./voice_report.sh" ]; then
        ./voice_report.sh
    else
        # Fallback to regular scan
        ./spine_cli
        if [ $? -eq 0 ]; then
            say_feedback "Screening completed successfully"
        else
            say_feedback "Screening failed. Please check the output"
        fi
    fi
}

run_test() {
    say_feedback "Running system validation tests"
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  RUNNING VALIDATION TESTS"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    if [ -f "./test_uci_vertebral" ]; then
        ./test_uci_vertebral
        say_feedback "Validation completed"
    else
        echo -e "${RED}✗ Test program not found. Compiling...${NC}"
        clang++ -std=c++14 -O2 test_uci_vertebral.c++ -o test_uci_vertebral
        ./test_uci_vertebral
    fi
}

run_compile() {
    say_feedback "Compiling spine screening system"
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  COMPILING SYSTEM"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    clang++ -std=c++14 -O3 spine_cli.c++ -o spine_cli
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Compilation successful${NC}"
        say_feedback "Compilation successful"
    else
        echo -e "${RED}✗ Compilation failed${NC}"
        say_feedback "Compilation failed"
    fi
}

show_status() {
    say_feedback "Checking system status"
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  SYSTEM STATUS"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    
    # Check compiled binary
    if [ -f "./spine_cli" ]; then
        echo -e "${GREEN}✓${NC} spine_cli: Compiled"
    else
        echo -e "${RED}✗${NC} spine_cli: Not compiled"
    fi
    
    # Check images
    if [ -d "./data/images" ]; then
        IMAGE_COUNT=$(find ./data/images -type f \( -name "*.jpg" -o -name "*.jpeg" -o -name "*.png" -o -name "*.ppm" -o -name "*.pgm" \) | wc -l | tr -d ' ')
        echo -e "${GREEN}✓${NC} Images: $IMAGE_COUNT files found"
    else
        echo -e "${RED}✗${NC} Images: Directory not found"
        IMAGE_COUNT=0
    fi
    
    # Check database
    if [ -f "./data/spine_records.json" ]; then
        RECORD_COUNT=$(grep -o '"patient_id"' ./data/spine_records.json | wc -l | tr -d ' ')
        echo -e "${GREEN}✓${NC} Database: $RECORD_COUNT records"
    else
        echo -e "${YELLOW}!${NC} Database: Not found (will be created on first run)"
    fi
    
    # Check models
    if [ -f "./models/resnet50_mura.onnx" ]; then
        echo -e "${GREEN}✓${NC} ONNX Model: Found"
    else
        echo -e "${YELLOW}!${NC} ONNX Model: Not found (using simulation mode)"
    fi
    
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    
    say_feedback "System has $IMAGE_COUNT images ready for processing"
}

interactive_mode() {
    say_feedback "Voice control activated. Say help for commands"
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  KARNIX SPINE SCREENING - INTERACTIVE VOICE CONTROL"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    echo "  Type commands or press Ctrl+C to exit"
    echo "  Available: setup, scan, test, compile, status, help, exit"
    echo ""
    
    while true; do
        echo -ne "${YELLOW}🎤 Command:${NC} "
        read -r command
        
        command=$(echo "$command" | tr '[:upper:]' '[:lower:]' | xargs)
        
        case "$command" in
            setup|install)
                run_setup
                ;;
            scan|run|process|start)
                run_scan
                ;;
            test|validate|check)
                run_test
                ;;
            compile|build)
                run_compile
                ;;
            status|info)
                show_status
                ;;
            help|commands)
                show_help
                ;;
            exit|quit|stop|bye)
                say_feedback "Goodbye"
                echo ""
                echo "══════════════════════════════════════════════════════════════"
                echo "  VOICE CONTROL ENDED"
                echo "══════════════════════════════════════════════════════════════"
                echo ""
                break
                ;;
            "")
                # Empty command, continue
                ;;
            *)
                echo -e "${RED}❌ Unknown command: '$command'${NC}"
                echo "   Type 'help' for available commands"
                say_feedback "Command not recognized"
                ;;
        esac
        
        echo ""
    done
}

# Main script
cd "$(dirname "$0")" || exit 1

case "$1" in
    setup|install)
        run_setup
        ;;
    scan|run|process|start)
        run_scan
        ;;
    test|validate|check)
        run_test
        ;;
    compile|build)
        run_compile
        ;;
    status|info)
        show_status
        ;;
    help|--help|-h)
        show_help
        ;;
    "")
        # No argument - start interactive mode
        interactive_mode
        ;;
    *)
        echo -e "${RED}❌ Unknown command: '$1'${NC}"
        echo ""
        show_help
        exit 1
        ;;
esac
