#!/bin/bash
# Voice Report Generator for Karnix Spine Screening
# Parses results and generates spoken summary

TEMP_OUTPUT="/tmp/spine_scan_output.txt"
REPORT_FILE="/tmp/spine_voice_report.txt"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

say_report() {
    echo "$1"
    say -v Daniel "$1"
}

generate_voice_report() {
    local output_file="$1"
    
    # Extract key metrics
    local total_images=$(grep "Total Images Processed:" "$output_file" | awk '{print $4}')
    local abnormal=$(grep "Abnormalities Detected:" "$output_file" | awk '{print $3}')
    local records=$(grep "Database Records:" "$output_file" | awk '{print $3}')
    
    # Count risk categories
    local high_risk=$(grep "Risk Category:.*HIGH" "$output_file" | wc -l | tr -d ' ')
    local moderate_risk=$(grep "Risk Category:.*MODERATE" "$output_file" | wc -l | tr -d ' ')
    local low_risk=$(grep "Risk Category:.*LOW" "$output_file" | wc -l | tr -d ' ')
    
    # Generate report
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  VOICE SUMMARY REPORT"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    
    sleep 1
    
    say_report "Spine screening analysis complete."
    sleep 0.5
    
    if [ -n "$total_images" ] && [ "$total_images" -gt 0 ]; then
        say_report "Processed $total_images images."
        sleep 0.5
        
        if [ -n "$abnormal" ] && [ "$abnormal" -gt 0 ]; then
            say_report "Deep learning analysis detected $abnormal potential abnormalities."
            sleep 0.5
        else
            say_report "No abnormalities detected by deep learning analysis."
            sleep 0.5
        fi
        
        # Risk category summary
        if [ "$high_risk" -gt 0 ]; then
            say_report "Alert: $high_risk cases classified as high risk."
            echo -e "${RED}⚠️  HIGH RISK: $high_risk case(s)${NC}"
            sleep 0.5
            say_report "Immediate clinical evaluation recommended."
        fi
        
        if [ "$moderate_risk" -gt 0 ]; then
            say_report "$moderate_risk cases classified as moderate risk."
            echo -e "${YELLOW}⚠️  MODERATE RISK: $moderate_risk case(s)${NC}"
            sleep 0.5
            say_report "Follow up imaging recommended in 6 to 12 months."
        fi
        
        if [ "$low_risk" -gt 0 ]; then
            say_report "$low_risk cases classified as low risk."
            echo -e "${GREEN}✓ LOW RISK: $low_risk case(s)${NC}"
            sleep 0.5
        fi
        
        # Database update
        if [ -n "$records" ]; then
            say_report "Total database now contains $records patient records."
            sleep 0.5
        fi
        
        # Clinical findings summary
        local disk_hernia=$(grep "Pathology:.*Disk Hernia" "$output_file" | wc -l | tr -d ' ')
        local spondylo=$(grep "Pathology:.*Spondylolisthesis" "$output_file" | wc -l | tr -d ' ')
        local scoliosis=$(grep "Scoliosis:.*Mild\|Moderate\|Severe" "$output_file" | wc -l | tr -d ' ')
        local fractures=$(grep "Compression Fracture:.*Grade [1-3]" "$output_file" | wc -l | tr -d ' ')
        
        if [ "$disk_hernia" -gt 0 ]; then
            say_report "U C I analysis identified $disk_hernia cases of disk hernia."
            sleep 0.5
        fi
        
        if [ "$spondylo" -gt 0 ]; then
            say_report "U C I analysis identified $spondylo cases of spondylolisthesis."
            sleep 0.5
        fi
        
        if [ "$scoliosis" -gt 0 ]; then
            say_report "Clinical assessment detected scoliosis in $scoliosis cases."
            sleep 0.5
        fi
        
        if [ "$fractures" -gt 0 ]; then
            say_report "Compression fractures detected in $fractures cases."
            sleep 0.5
        fi
        
        # Final recommendation
        echo ""
        if [ "$high_risk" -gt 0 ]; then
            say_report "Urgent: Please review high risk cases immediately."
        elif [ "$moderate_risk" -gt 0 ]; then
            say_report "Recommendation: Schedule follow up for moderate risk cases."
        else
            say_report "All cases within normal parameters. Routine follow up recommended."
        fi
        
    else
        say_report "No images were processed. Please check the image directory."
    fi
    
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    
    say_report "End of report."
}

# Main execution
if [ "$1" == "--from-file" ]; then
    # Parse existing output file
    generate_voice_report "$2"
else
    # Run spine_cli and capture output
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  KARNIX SPINE SCREENING WITH VOICE REPORT"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    
    say -v Daniel "Starting spine screening analysis" &
    
    # Run spine_cli and save output
    ./spine_cli 2>&1 | tee "$TEMP_OUTPUT"
    
    # Generate voice report from output
    sleep 1
    generate_voice_report "$TEMP_OUTPUT"
    
    # Cleanup
    rm -f "$TEMP_OUTPUT" 2>/dev/null
fi
