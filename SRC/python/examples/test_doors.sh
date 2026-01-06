#!/bin/bash
# Quick test script for Python test doors
# This simulates running the doors (won't fully work without BBS running)

echo "DayDream Python Test Doors"
echo "=========================="
echo ""
echo "These doors require the BBS to be running."
echo "This script only checks if they can be invoked."
echo ""

# Set DAYDREAM env if not set
if [ -z "$DAYDREAM" ]; then
    export DAYDREAM="/usr/local/daydream"
    echo "Setting DAYDREAM=$DAYDREAM"
fi

echo ""
echo "Testing Python 2 door..."
echo "------------------------"
if command -v python2 &> /dev/null; then
    python2 test_py2.py 1 2>&1 | head -5
    if [ $? -eq 1 ]; then
        echo "Note: Door requires BBS to be running (expected)"
    fi
else
    echo "Python 2 not available"
fi

echo ""
echo "Testing Python 3 door..."
echo "------------------------"
if command -v python3 &> /dev/null; then
    python3 test_py3.py 1 2>&1 | head -5
    if [ $? -eq 1 ]; then
        echo "Note: Door requires BBS to be running (expected)"
    fi
else
    echo "Python 3 not available"
fi

echo ""
echo "To use these doors in your BBS:"
echo "1. Copy them to your BBS doors directory"
echo "2. Add them to your menu system"
echo "3. Command format: python2/python3 test_py2.py %N"
echo ""




