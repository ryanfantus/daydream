Installation instructions:

(These are admittedly pretty sparse! It's a work in progress!)

- Initial installation -

move into daydream-2.20.0 directory
as root, sh install.sh
check out docs/ directory for specifics regarding inetd, xinetd, etc

- Upgrade from 2.14.9 -

this may break! pretty untested!
As root, sh upgrade.sh

This will ONLY copy over the new sets of binaries.  No cfg, gfx, data,
docs, or any other files will be replaced.  I suggest you do a full backup
before you attempt this!  Now, everything *should* be backwards compatible
with your old daydream setup.  However, I make no guarantees!  ;)

You should manually do the following:

1:

In your display/iso/edituser.txt|gfx files, add a selection:
12 - Change message signature

This will launch the door doors/autosig %N

Feel free to add it as a main menu option as well.

Also, either copy the autosigtop.txt|gfx files from this archive,
or create your own, and put it in the display/iso dir as well.

2:

To use the new FR / NEW door, add FR and NEW as door options to
daydream.cfg.  Make them both similar to this:

```
DOOR_COMMAND.. NEW
DOOR_TYPE..... 1
DOOR_SECURITY. 5
DOOR_EXECUTE.. /home/bbs/doors/new %N
DOOR_CONFS1... XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
DOOR_CONFS2... XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
DOOR_PASSWD... -
+
DOOR_COMMAND.. FR
DOOR_TYPE..... 1
DOOR_SECURITY. 5
DOOR_EXECUTE.. /home/bbs/doors/new %N
DOOR_CONFS1... XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
DOOR_CONFS2... XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
DOOR_PASSWD... -
```

That's it! Enjoy!
