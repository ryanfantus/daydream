# python-script to get the latest daydream-version

import dd
import sys
import os

if (dd.initdoor(sys.argv[1])==0):
    print "Ugh. Run me from DD\n"
    sys.exit(-1)
else:
     dd.sendstring("[0mLatest version:\n")
     os.system("grep versionstring /usr/src/daydream/main/dd.h >/tmp/getddfoo")
     dd.typefile("/tmp/getddfoo",0)
     dd.sendstring("\n[36mDo you want it? (Yes/no): [32m")
     goo=dd.hotkey(dd.HOTKEY_YESNO)
     if (goo==1):
        dd.sendfiles("/home/bbs/configs/sping.lst")
     

    
