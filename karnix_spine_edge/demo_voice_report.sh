#!/bin/bash
# Quick demo of voice reporting using sample data

echo ""
echo "══════════════════════════════════════════════════════════════"
echo "  VOICE REPORT DEMO"
echo "══════════════════════════════════════════════════════════════"
echo ""
echo "This demonstrates what the voice report sounds like."
echo "After processing images, the system will speak:"
echo ""
sleep 1

echo "🔊 \"Spine screening analysis complete.\""
say -v Daniel "Spine screening analysis complete."
sleep 1

echo "🔊 \"Processed 11 images.\""
say -v Daniel "Processed 11 images."
sleep 1

echo "🔊 \"Deep learning analysis detected 4 potential abnormalities.\""
say -v Daniel "Deep learning analysis detected 4 potential abnormalities."
sleep 1

echo "🔊 \"2 cases classified as moderate risk.\""
say -v Daniel "2 cases classified as moderate risk."
sleep 1

echo "🔊 \"Follow up imaging recommended in 6 to 12 months.\""
say -v Daniel "Follow up imaging recommended in 6 to 12 months."
sleep 1

echo "🔊 \"9 cases classified as low risk.\""
say -v Daniel "9 cases classified as low risk."
sleep 1

echo "🔊 \"Total database now contains 117 patient records.\""
say -v Daniel "Total database now contains 117 patient records."
sleep 1

echo "🔊 \"Recommendation: Schedule follow up for moderate risk cases.\""
say -v Daniel "Recommendation: Schedule follow up for moderate risk cases."
sleep 1

echo "🔊 \"End of report.\""
say -v Daniel "End of report."

echo ""
echo "══════════════════════════════════════════════════════════════"
echo ""
echo "To use with real scans: ./voice.sh scan"
echo ""
