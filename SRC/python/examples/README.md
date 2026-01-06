# Python Test Doors for DayDream BBS

This directory contains example/test Python doors demonstrating both Python 2 and Python 3 API usage.

## Files

- **test_py2.py** - Python 2 test door (compatible with Python 2.7)
- **test_py3.py** - Python 3 test door (compatible with Python 3.0+, uses `.format()` instead of f-strings for older versions)

## Features Demonstrated

Both test doors demonstrate:

### Basic Functions
- Door initialization and cleanup
- User information retrieval
- System information display
- Status changes
- Screen control (clear, cursor positioning)
- Text centering

### ANSI/Color Functions
- Foreground/background colors
- Pipe code parsing
- ANSI code manipulation
- String length calculation (with/without ANSI)

### String Functions
- `parsepipes()` - Parse pipe codes
- `strippipes()` - Remove pipe codes
- `stripansi()` - Remove ANSI codes
- `strlenansi()` - Get visible string length
- `stripcrlf()` - Strip line endings

### Input Functions
- `hotkey()` - Single key input with arrow key support
- `prompt()` - Text input prompts
- Yes/No prompts

### Message Functions
- Last Read Pointers (LRP)
- Message Pointers
- FidoNet unique ID generation
- Message base information

### File Functions
- File display (`typefile`)
- Log writing

### Python Version Specific Features

**Python 2 door (`test_py2.py`):**
- Uses Python 2 string formatting
- Uses `print` statements
- Classic Python 2 syntax

**Python 3 door (`test_py3.py`):**
- Uses f-strings (Python 3.6+)
- Uses `print()` function
- Unicode box drawing characters (╔═╗║╚╝)
- Modern Python 3 features
- Additional message function tests

## Usage

### From Command Line (Testing)

```bash
# Python 2
python2 examples/test_py2.py 1

# Python 3
python3 examples/test_py3.py 1
```

### From BBS

1. Copy the scripts to your doors directory:
```bash
cp examples/test_py*.py /path/to/daydream/doors/
```

2. Add to your BBS menu system:

**For test_py2.py:**
```
Name: Python 2 API Test
Command: python2 doors/test_py2.py %N
```

**For test_py3.py:**
```
Name: Python 3 API Test
Command: python3 doors/test_py3.py %N
```

Where `%N` is replaced with the node number by your BBS software.

## Interactive Menu

Both doors provide an interactive menu with these options:

- **[T]** Test cursor positioning - Demonstrates ANSI cursor control
- **[F]** Test file display - Shows file display functionality
- **[L]** Write to log - Tests log writing capabilities
- **[Y]** Yes/No prompt test - Shows Y/N prompts
- **[M]** Test message functions - (Python 3 only) Additional message tests
- **[Q]** Quit - Exit the door

## Code Structure

Both doors follow the same structure:

```python
def main():
    # Initialize door
    dd.initdoor(node)
    
    try:
        # Display information and menus
        # Handle user interaction
        pass
    finally:
        # Always cleanup
        dd.closedoor()
```

This ensures the door connection is properly closed even if an error occurs.

## API Functions Demonstrated

### Output
- `print()` / `sendstring()` - Send text with pipe code parsing
- `sendstring_noparse()` - Send text without parsing
- `clrscr()` - Clear screen
- `center()` - Center text

### Input
- `hotkey(flags)` - Get single keypress
  - `HOTKEY_CURSOR` - Enable arrow keys
  - `HOTKEY_YESNO` - Y/N prompt
  - `HOTKEY_NOYES` - N/Y prompt
- `prompt(buffer, length, flags)` - Get text input

### Variables
- `getvar(id)` - Get system/user variable
  - `BBS_NAME`, `BBS_SYSOP`
  - `USER_HANDLE`, `USER_REALNAME`, `USER_SECURITYLEVEL`
  - `CONF_NAME`, `CONF_MSGBASES`, `CONF_FILEAREAS`
  - `MSGBASE_NAME`

### ANSI/Color
- `ansi_fg(color)` - Get foreground color code (0-7)
- `ansi_bg(color)` - Get background color code (0-7)
- `ansipos(x, y)` - Position cursor

### String Manipulation
- `strlenansi(s)` - Length without ANSI codes
- `parsepipes(s)` - Convert pipe codes to ANSI
- `strippipes(s)` - Remove pipe codes
- `stripansi(s)` - Remove ANSI codes
- `stripcrlf(s)` - Remove line endings

### Message Functions
- `getlprs()` - Get Last Read Pointers (returns tuple)
- `getmprs()` - Get Message Pointers (returns tuple)
- `getfidounique()` - Get unique message ID

### System Functions
- `changestatus(status)` - Change node status
- `writelog(message)` - Write to log
- `typefile(filename, flags)` - Display text file
- `pause()` - Wait for keypress

## Color Codes (Pipe Codes)

The doors use pipe codes for colors:

- `|00` - Black
- `|01` - Red (Dark)
- `|02` - Green (Dark)
- `|03` - Yellow (Brown)
- `|04` - Blue (Dark)
- `|05` - Magenta (Dark)
- `|06` - Cyan (Dark)
- `|07` - White (Gray)
- `|08` - Black (Bright)
- `|09` - Red (Bright)
- `|10` - Green (Bright)
- `|11` - Yellow (Bright)
- `|12` - Blue (Bright)
- `|13` - Magenta (Bright)
- `|14` - Cyan (Bright)
- `|15` - White (Bright)
- `|16` - Black background
- `|17` - Blue background
- etc...

## Differences Between Python 2 and Python 3 Versions

### Syntax Differences
```python
# Python 2
print "Hello"
dd.print("Value: " + str(x))

# Python 3
print("Hello")
dd.print(f"Value: {x}")
```

### String Handling
- Python 2: Strings are bytes by default
- Python 3: Strings are Unicode by default

### API Behavior
Both versions use the **same DayDream API** - the differences are only in Python syntax.

## Error Handling

Both doors use try/finally to ensure cleanup:

```python
try:
    # Door code
    pass
finally:
    dd.closedoor()  # Always cleanup
```

This prevents leaving the door connection open if an error occurs.

## Extending These Examples

To create your own door:

1. Copy one of these examples
2. Modify the header and title
3. Add your custom functionality
4. Keep the initialization/cleanup structure
5. Test with both Python versions if possible

## Troubleshooting

### Module Not Found
Ensure the path is correct:
```python
sys.path.insert(0, os.path.join(os.environ.get('DAYDREAM', '/usr/local/daydream'), 'python'))
```

### Door Won't Initialize
Check that:
- Node number is valid
- BBS is running
- Module files (ddpmodule.so / ddp*.so) are in the python directory

### ANSI Colors Not Working
- Ensure user has ANSI enabled
- Check that pipe codes are properly formatted: `|XX`

## License

These examples are provided as part of the DayDream BBS project.

