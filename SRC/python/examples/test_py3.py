#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DayDream BBS - Python 3 Test Door
Demonstrates the Python 3 API functionality
"""

import sys
import os

# Add the Python module path
sys.path.insert(0, os.path.join(os.environ.get('DAYDREAM', '/usr/local/daydream'), 'python'))

import dd

def main():
    """Main entry point for the Python 3 test door"""
    # Initialize door with node number from command line
    if len(sys.argv) < 2:
        print("Usage: test_py3.py <node>")
        return 1
    
    node = sys.argv[1]
    
    if not dd.initdoor(node):
        print("Failed to initialize door!")
        return 1
    
    try:
        # Change status
        dd.changestatus("Python 3 Test")
        
        # Clear screen
        dd.clrscr()
        
        # Display header with ANSI colors
        dd.sendstring("|09|17" + " " * 80 + "|07|00\r\n")
        dd.center("|15|09|17 DayDream BBS - Python 3 API Test |07|00")
        dd.sendstring("|09|17" + " " * 80 + "|07|00\r\n\r\n")
        
        # Get and display user information
        dd.sendstring("|14Welcome to the Python 3 test door, |15")
        dd.sendstring(dd.getvar(dd.USER_HANDLE))
        dd.sendstring("|07!\r\n\r\n")
        
        # Display system information
        dd.sendstring("|11System Information:|07\r\n")
        dd.sendstring("  BBS Name: |14{}|07\r\n".format(dd.getvar(dd.BBS_NAME)))
        dd.sendstring("  SysOp: |14{}|07\r\n".format(dd.getvar(dd.BBS_SYSOP)))
        dd.sendstring("  Your Name: |14{}|07\r\n".format(dd.getvar(dd.USER_REALNAME)))
        dd.sendstring("  Security: |14{}|07\r\n".format(dd.getvar(dd.USER_SECURITYLEVEL)))
        dd.sendstring("  Time Left: |14{}|07 minutes\r\n".format(dd.getvar(dd.USER_TIMELEFT)))
        dd.sendstring("\r\n")
        
        # Test ANSI functions
        dd.sendstring("|11ANSI Color Test:|07\r\n")
        colors = ["Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White"]
        for i, color_name in enumerate(colors):
            fg_code = dd.ansi_fg(i)
            dd.sendstring("{}{} ".format(fg_code, color_name))
        dd.sendstring("|07\r\n\r\n")
        
        # Test string functions
        dd.sendstring("|11String Function Tests:|07\r\n")
        test_string = "|12Hello |15World|07 with \x1b[1mpipes\x1b[0m and ANSI"
        dd.sendstring("  Original: {}\r\n".format(test_string))
        dd.sendstring("  Length with ANSI: {}\r\n".format(len(test_string)))
        dd.sendstring("  Length without ANSI: {}\r\n".format(dd.strlenansi(test_string)))
        dd.sendstring("  Stripped pipes: {}\r\n".format(dd.strippipes(test_string)))
        dd.sendstring("  Stripped ANSI: {}\r\n".format(dd.stripansi(test_string)))
        dd.sendstring("\r\n")
        
        # Test message base info
        dd.sendstring("|11Conference Information:|07\r\n")
        conf_name = dd.getvar(dd.CONF_NAME)
        if conf_name:
            dd.sendstring("  Conference: |14{}|07\r\n".format(conf_name))
            dd.sendstring("  Message Bases: |14{}|07\r\n".format(dd.getvar(dd.CONF_MSGBASES)))
            dd.sendstring("  File Areas: |14{}|07\r\n".format(dd.getvar(dd.CONF_FILEAREAS)))
        dd.sendstring("\r\n")
        
        # Test LRP (Last Read Pointers)
        dd.sendstring("|11Message Pointers:|07\r\n")
        lrp = dd.getlprs()
        if lrp:
            dd.sendstring("  Last Read: |14{}|07\r\n".format(lrp[0]))
            dd.sendstring("  Last Scan: |14{}|07\r\n".format(lrp[1]))
        dd.sendstring("\r\n")
        
        # Test new Python 3 features
        dd.sendstring("|11Python 3 Features:|07\r\n")
        dd.sendstring("  Unicode support: |14Available|07\r\n")
        dd.sendstring("  String formatting: |14.format() method|07\r\n")
        dd.sendstring("  Type hints: |14Available|07\r\n")
        dd.sendstring("\r\n")
        
        # Interactive menu
        while True:
            dd.sendstring("|15" + "=" * 40 + "|07\r\n")
            dd.sendstring("|11Menu Options:|07\r\n")
            dd.sendstring("  |14[T]|07 Test cursor positioning\r\n")
            dd.sendstring("  |14[F]|07 Test file display\r\n")
            dd.sendstring("  |14[L]|07 Write to log\r\n")
            dd.sendstring("  |14[Y]|07 Yes/No prompt test\r\n")
            dd.sendstring("  |14[M]|07 Test message functions\r\n")
            dd.sendstring("  |14[Q]|07 Quit\r\n")
            dd.sendstring("|15" + "=" * 40 + "|07\r\n")
            dd.sendstring("|14Your choice: |07")
            
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
            elif key in (ord('M'), ord('m')):
                test_messages()
            else:
                dd.sendstring("|12Invalid choice!|07\r\n\r\n")
                dd.pause()
        
        # Goodbye message
        dd.sendstring("\r\n|11Thanks for testing the Python 3 API!|07\r\n")
        dd.sendstring("|14Press any key to exit...|07")
        dd.hotkey(0)
        
    finally:
        # Always close the door
        dd.closedoor()
    
    return 0

def test_cursor():
    """Test cursor positioning"""
    dd.clrscr()
    dd.sendstring("|11Cursor Positioning Test|07\r\n\r\n")
    
    # Draw a fancy box
    dd.ansipos(10, 5)
    dd.sendstring("|14╔" + "═" * 30 + "╗")
    for i in range(3):
        dd.ansipos(10, 6 + i)
        dd.sendstring("|14║" + " " * 30 + "║")
    dd.ansipos(10, 9)
    dd.sendstring("|14╚" + "═" * 30 + "╝")
    
    # Put text in the box
    dd.ansipos(15, 7)
    dd.sendstring("|15✓ Centered Text in Box ✓|07")
    
    dd.ansipos(1, 11)
    dd.sendstring("|07Press any key to continue...")
    dd.hotkey(0)

def test_file():
    """Test file display"""
    dd.sendstring("|11Testing file display...|07\r\n\r\n")
    
    # Try to display a file
    result = dd.typefile("welcome", dd.TYPE_WARN)
    if result:
        dd.sendstring("\r\n|14File displayed successfully!|07\r\n")
    else:
        dd.sendstring("\r\n|12File not found or error.|07\r\n")
    
    dd.sendstring("\r\n|07Press any key to continue...")
    dd.hotkey(0)

def test_log():
    """Test log writing"""
    dd.sendstring("|11Enter a message to write to the log:|07\r\n")
    msg = dd.prompt("", 60, 0)
    if msg:
        dd.writelog("Python 3 Test: {}".format(msg))
        dd.sendstring("\r\n|14Message written to log!|07\r\n")
    else:
        dd.sendstring("\r\n|12Cancelled.|07\r\n")
    dd.sendstring("\r\n|07Press any key to continue...")
    dd.hotkey(0)

def test_yesno():
    """Test Yes/No prompt"""
    dd.sendstring("|11Yes/No Prompt Test|07\r\n\r\n")
    dd.sendstring("|14Do you like Python 3? (Y/N): |07")
    
    result = dd.hotkey(dd.HOTKEY_YESNO)
    
    if result == 1:  # Yes
        dd.sendstring("\r\n|14Excellent! Python 3 is modern and powerful!|07\r\n")
    else:
        dd.sendstring("\r\n|12That's okay, both versions work here!|07\r\n")
    
    dd.sendstring("\r\n|07Press any key to continue...")
    dd.hotkey(0)

def test_messages():
    """Test message-related functions"""
    dd.sendstring("|11Message Functions Test|07\r\n\r\n")
    
    # Get message pointers
    mprs = dd.getmprs()
    if mprs:
        dd.sendstring("  Message Base Range: |14{}|07 to |14{}|07\r\n".format(mprs[0], mprs[1]))
    
    # Get conference data
    base_name = dd.getvar(dd.MSGBASE_NAME)
    if base_name:
        dd.sendstring("  Current Base: |14{}|07\r\n".format(base_name))
    
    # Get unique ID
    unique_id = dd.getfidounique()
    dd.sendstring("  Unique ID: |14{}|07\r\n".format(unique_id))
    
    dd.sendstring("\r\n|07Press any key to continue...")
    dd.hotkey(0)

if __name__ == "__main__":
    sys.exit(main())

