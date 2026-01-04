# Python 2 and Python 3 Compatibility

The DayDream Python API now supports **both Python 2 and Python 3** using conditional compilation.

## Building

The Makefile automatically detects available Python versions and builds appropriate modules:

```bash
make          # Builds for all available Python versions
make py2      # Build Python 2 only
make py3      # Build Python 3 only
make clean    # Clean all builds
```

## Output Files

- **Python 2**: `ddpmodule.so`
- **Python 3**: `ddpmodule.cpython-3XX-x86_64-linux-gnu.so` (name varies by version)

Python will automatically load the correct module for the interpreter version being used.

## Usage

Same code works in both Python 2 and 3:

```python
import ddp

# Initialize door
ddp.initdoor("1")

# Send output
ddp.print("Hello from Python!")

# Get user input
response = ddp.hotkey(ddp.HOTKEY_YESNO)

# Close door
ddp.closedoor()
```

## Compatibility Details

### What Changed in the C Code

The `_dd.c` file now uses:

1. **Conditional compilation** based on `PY_MAJOR_VERSION`
2. **Compatibility macros** for API differences:
   - `PyInt_FromLong` / `PyLong_FromLong`
   - `PyString_FromString` / `PyUnicode_FromString`
3. **Different module initialization**:
   - Python 2: `initddp()` function
   - Python 3: `PyInit_ddp()` returning `PyModuleDef`
4. **Consistent return values**:
   - `Py_RETURN_NONE` instead of `Py_BuildValue("")`
   - `PyInt_FromLong(n)` instead of `Py_BuildValue("i", n)`
   - `PyString_FromString(s)` instead of `Py_BuildValue("s", s)`

### String Handling

Strings are automatically handled:
- **Python 2**: Accepts both byte strings and unicode
- **Python 3**: All strings are unicode (UTF-8)

### Integer Handling

Integers are automatically converted:
- **Python 2**: int and long types
- **Python 3**: All integers are long type

## Available Functions (60 total)

### Output Functions
- `print(s)` / `sendstring(s)` - Send string with pipe code parsing
- `sendstring_noparse(s)` - Send string without parsing
- `sendfmt(s)` - Send formatted string

### Input Functions
- `hotkey(flags)` - Get single keypress
- `prompt(buffer, length, flags)` - Get user input

### Screen Control
- `clrscr()` - Clear screen
- `ansipos(x, y)` - Position cursor
- `center(s)` - Center text
- `cursoron()` / `cursoroff()` - Cursor visibility

### ANSI/Color Functions
- `ansi_fg(color)` - Get foreground color code
- `ansi_bg(color)` - Get background color code
- `parsepipes(s)` - Parse pipe codes in string
- `strippipes(s)` - Remove pipe codes
- `stripansi(s)` - Remove ANSI codes
- `strlenansi(s)` - Get length excluding ANSI

### File Operations
- `typefile(filename, flags)` - Display text file
- `flagfile(filename, flags)` - Flag file for download
- `flagsingle(filename, flags)` - Flag single file
- `unflagfiles(filename)` - Unflag file
- `findfilestolist(pattern, list)` - Find files matching pattern
- `dumpfilestolist(filename)` - Dump flagged files to list
- `sendfiles(filelist)` - Send files to user
- `getfiles(path)` - Receive files from user
- `isfiletagged(filename)` - Check if file is tagged
- `isfreedl(filename)` - Check if file is free download
- `fileattach()` - Attach files to message

### System Functions
- `initdoor(node)` - Initialize door interface
- `closedoor()` - Close door interface
- `system(command, mode)` - Execute system command
- `cmd(command)` - Execute BBS command
- `writelog(message)` - Write to log file
- `changestatus(status)` - Change node status
- `pause()` - Pause for keypress

### User/Conference Functions
- `getvar(id)` - Get system/user variable
- `setvar(id, value)` - Set system/user variable
- `finduser(username)` - Find user by name
- `joinconf(conf_num, flags)` - Join conference
- `isconfaccess(conf_num)` - Check conference access
- `isanybasestagged(conf_num)` - Check if any bases tagged
- `isconftagged(conf_num)` - Check if conference tagged
- `isbasetagged(conf_num, base_num)` - Check if base tagged
- `getconfdata()` - Get conference data

### Message Functions
- `changemsgbase(base_num, flags)` - Change message base
- `getlprs()` - Get Last Read Pointers (returns tuple)
- `setlprs(read, scan)` - Set Last Read Pointers
- `getmprs()` - Get Message Pointers (returns tuple)
- `setmprs(low, high)` - Set Message Pointers
- `getfidounique()` - Get unique FidoNet ID

### Message Library Functions (Direct Access)
- `ddmsg_open_base(conf, base_num, flags, mode)` - Open message base
- `ddmsg_close_base(fd)` - Close message base
- `ddmsg_getptrs(conf, base_num)` - Get message pointers
- `ddmsg_setptrs(conf, base_num, low, high)` - Set message pointers
- `ddmsg_getfidounique(origdir)` - Get FidoNet unique ID
- `ddmsg_open_msg(conf, base_num, msg_num, flags, mode)` - Open message
- `ddmsg_close_msg(fd)` - Close message
- `ddmsg_delete_msg(conf, base_num, msg_num)` - Delete message
- `ddmsg_rename_msg(conf, base_num, old_num, new_num)` - Rename message

### Utility Functions
- `stripcrlf(s)` - Strip CR/LF from string

## Testing

Both Python versions can be tested with the same code:

```bash
# Test Python 3
python3 -c "import sys; sys.path.insert(0, '.'); import ddp; print('OK')"

# Test Python 2 (if available)
python2 -c "import sys; sys.path.insert(0, '.'); import ddp; print 'OK'"
```

## Migration Notes

If you have existing Python 2 doors:
- They should work without changes in Python 2
- To run in Python 3, update any Python 2-specific syntax:
  - `print "text"` → `print("text")`
  - `except Exception, e:` → `except Exception as e:`
  - String handling differences (bytes vs unicode)

The DayDream API itself is compatible with both versions.

## Note on Missing Functions

- `outputmask()` - Declared in header but not implemented in libdd, so excluded from Python API

## Troubleshooting

### Module Not Found
Ensure the `.so` file is in Python's path or use:
```python
import sys
sys.path.insert(0, '/path/to/module')
import ddp
```

### Wrong Python Version
Check which version was built:
```bash
ls -la ddpmodule*.so
```

### Symbol Errors
Rebuild the module:
```bash
make clean && make
```

## Technical Details

- Source: `_dd.c`
- Python 2 uses: `#include <Python.h>` 
- Python 3 uses: `#include <Python.h>`
- Compatibility macros handle API differences
- Module name: `ddp` (same for both versions)
- Build system: Makefile with auto-detection

