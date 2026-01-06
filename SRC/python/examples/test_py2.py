#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
DayDream BBS - Python 2 Test Door
Demonstrates the Python 2 API functionality
"""

import sys
import os

# Add the Python module path
sys.path.insert(0, os.path.join(os.environ.get('DAYDREAM', '/usr/local/daydream'), 'python'))

import dd

def main():
    # Initialize door with node number from command line
    if len(sys.argv) < 2:
        print "Usage: test_py2.py <node>"
        return 1
    
    node = sys.argv[1]
    
    if not dd.initdoor(node):
        print "Failed to initialize door!"
        return 1
    
    try:
        # Change status
        dd.changestatus("Python 2 Test")
        
        # Clear screen
        dd.clrscr()
        
        # Display header with ANSI colors
        dd.sendstring("|15|17" + " " * 80 + "|07|00\r\n")
        dd.center("|15|17 DayDream BBS - Python 2 API Test |07|00")
        dd.sendstring("|15|17" + " " * 80 + "|07|00\r\n\r\n")
        
        # Get and display user information
        dd.sendstring("|11Welcome to the Python 2 test door, |15")
        dd.sendstring(dd.getvar(dd.USER_HANDLE))
        dd.sendstring("|07!\r\n\r\n")
        
        # Display system information
        dd.sendstring("|14System Information:|07\r\n")
        dd.sendstring("  BBS Name: |11" + dd.getvar(dd.BBS_NAME) + "|07\r\n")
        dd.sendstring("  SysOp: |11" + dd.getvar(dd.BBS_SYSOP) + "|07\r\n")
        dd.sendstring("  Your Name: |11" + dd.getvar(dd.USER_REALNAME) + "|07\r\n")
        dd.sendstring("  Security: |11" + str(dd.getvar(dd.USER_SECURITYLEVEL)) + "|07\r\n")
        dd.sendstring("  Time Left: |11" + str(dd.getvar(dd.USER_TIMELEFT)) + "|07 minutes\r\n")
        dd.sendstring("\r\n")
        
        # Test ANSI functions
        dd.sendstring("|14ANSI Color Test:|07\r\n")
        for i in range(8):
            fg_code = dd.ansi_fg(i)
            dd.sendstring(fg_code + "Color " + str(i) + " ")
        dd.sendstring("|07\r\n\r\n")
        
        # Test string functions
        dd.sendstring("|14String Function Tests:|07\r\n")
        test_string = "|12Hello |15World|07 with \x1b[1mpipes\x1b[0m and ANSI"
        dd.sendstring("  Original: " + test_string + "\r\n")
        dd.sendstring("  Length with ANSI: " + str(len(test_string)) + "\r\n")
        dd.sendstring("  Length without ANSI: " + str(dd.strlenansi(test_string)) + "\r\n")
        dd.sendstring("  Stripped pipes: " + dd.strippipes(test_string) + "\r\n")
        dd.sendstring("  Stripped ANSI: " + dd.stripansi(test_string) + "\r\n")
        dd.sendstring("\r\n")
        
        # Test message base info
        dd.sendstring("|14Conference Information:|07\r\n")
        conf_name = dd.getvar(dd.CONF_NAME)
        if conf_name:
            dd.sendstring("  Conference: |11" + conf_name + "|07\r\n")
            dd.sendstring("  Message Bases: |11" + str(dd.getvar(dd.CONF_MSGBASES)) + "|07\r\n")
            dd.sendstring("  File Areas: |11" + str(dd.getvar(dd.CONF_FILEAREAS)) + "|07\r\n")
        dd.sendstring("\r\n")
        
        # Test LRP (Last Read Pointers)
        dd.sendstring("|14Message Pointers:|07\r\n")
        lrp = dd.getlprs()
        if lrp:
            dd.sendstring("  Last Read: |11" + str(lrp[0]) + "|07\r\n")
            dd.sendstring("  Last Scan: |11" + str(lrp[1]) + "|07\r\n")
        dd.sendstring("\r\n")
        
        # Interactive menu
        while True:
            dd.sendstring("|15" + "=" * 40 + "|07\r\n")
            dd.sendstring("|14Menu Options:|07\r\n")
            dd.sendstring("  |11[T]|07 Test cursor positioning\r\n")
            dd.sendstring("  |11[F]|07 Test file display\r\n")
            dd.sendstring("  |11[L]|07 Write to log\r\n")
            dd.sendstring("  |11[Y]|07 Yes/No prompt test\r\n")
            dd.sendstring("  |11[Q]|07 Quit\r\n")
            dd.sendstring("|15" + "=" * 40 + "|07\r\n")
            dd.sendstring("|11Your choice: |07")
            
            key = dd.hotkey(dd.HOTKEY_CURSOR)
            dd.sendstring(chr(key) + "\r\n\r\n")
            
            if key in (ord('Q'), ord('q'), 27):  # Q or ESC
                break
            elif key in (ord('T'), ord('t')):
                test_cursor()
            elif key in (ord('F'), ord('f')):
                test_file()
            elif key in (ord('L'), ord('l')):
                test_log()
            elif key in (ord('Y'), ord('y')):
                test_yesno()
            else:
                dd.sendstring("|12Invalid choice!|07\r\n\r\n")
                dd.pause()
        
        # Goodbye message
        dd.sendstring("\r\n|14Thanks for testing the Python 2 API!|07\r\n")
        dd.sendstring("|11Press any key to exit...|07")
        dd.hotkey(0)
        
    finally:
        # Always close the door
        dd.closedoor()
    
    return 0

def test_cursor():
    """Test cursor positioning"""
    dd.clrscr()
    dd.sendstring("|14Cursor Positioning Test|07\r\n\r\n")
    
    # Draw a box
    dd.ansipos(10, 5)
    dd.sendstring("|11" + "+" + "-" * 30 + "+")
    for i in range(3):
        dd.ansipos(10, 6 + i)
        dd.sendstring("|11|" + " " * 30 + "|")
    dd.ansipos(10, 9)
    dd.sendstring("|11" + "+" + "-" * 30 + "+")
    
    # Put text in the box
    dd.ansipos(15, 7)
    dd.sendstring("|15Centered Text in Box|07")
    
    dd.ansipos(1, 11)
    dd.sendstring("|07Press any key to continue...")
    dd.hotkey(0)

def test_file():
    """Test file display"""
    dd.sendstring("|14Testing file display...|07\r\n\r\n")
    
    # Try to display a file
    result = dd.typefile("welcome", dd.TYPE_WARN)
    if result:
        dd.sendstring("\r\n|11File displayed successfully!|07\r\n")
    else:
        dd.sendstring("\r\n|12File not found or error.|07\r\n")
    
    dd.sendstring("\r\n|07Press any key to continue...")
    dd.hotkey(0)

def test_log():
    """Test log writing"""
    dd.sendstring("|14Enter a message to write to the log:|07\r\n")
    msg = dd.prompt("", 60, 0)
    if msg:
        dd.writelog("Python 2 Test: " + msg)
        dd.sendstring("\r\n|11Message written to log!|07\r\n")
    else:
        dd.sendstring("\r\n|12Cancelled.|07\r\n")
    dd.sendstring("\r\n|07Press any key to continue...")
    dd.hotkey(0)

def test_yesno():
    """Test Yes/No prompt"""
    dd.sendstring("|14Yes/No Prompt Test|07\r\n\r\n")
    dd.sendstring("|11Do you like Python 2? (Y/N): |07")
    
    result = dd.hotkey(dd.HOTKEY_YESNO)
    
    if result == 1:  # Yes
        dd.sendstring("\r\n|11Great! Python 2 is classic!|07\r\n")
    else:
        dd.sendstring("\r\n|12That's okay, Python 3 is nice too!|07\r\n")
    
    dd.sendstring("\r\n|07Press any key to continue...")
    dd.hotkey(0)

if __name__ == "__main__":
    sys.exit(main())




