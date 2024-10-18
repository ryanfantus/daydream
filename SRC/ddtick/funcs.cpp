/*
 * DD-Tick
 * Misc. functions
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include <sstream>
#include <iomanip>

#include "headers.hpp"

unsigned int HexStringToInt(string s) {
  unsigned int result;
  stringstream ss;
  ss << hex << s;
  ss >> result;
  return result;
}

string IntToHexString(unsigned int a) {
  stringstream ss;
  ss << hex << a;
  return ss.str();
}

unsigned int StringToInt(string s) {
  stringstream ss;
  unsigned int a;
  ss << s;
  ss >> a;
  return a;
}

string IntToString(unsigned int a) {
  stringstream ss;
  ss << a;
  return ss.str();
}

void SplitLine(string s, vector<string>& l, char delim, int times) {
  string::size_type i = 0;
  string::size_type last = 0;
  
  bool ignore = false;

  while(i < s.size()) {
    if(s[i] == '"') {

      if(!ignore) {
	ignore = true;
	last = i + 1;
      }
      else {
	string new_str = s.substr(last, i-last);
	l.push_back(new_str);
	last = i + 2;
	i += 2;
	ignore = false;
      }
    }
    else if(s[i] == delim && !ignore) {
      string new_str = s.substr(last, i - last);
      l.push_back(new_str);
      last = i+1;
    }

    ++i;
    
    if(times > 0 && l.size() >= times) {
      break;
    } 
  }
  if(s.size() - last > 0) {
    string new_str = s.substr(last, s.size() - last);
    l.push_back(new_str);
  }
}

string LowerStr(string s) {
    for(string::iterator it = s.begin(); it != s.end(); ++it) {
      *it = tolower(*it);
    }
    return s;
}

void RemoveCrLf(string& s) {
    string::iterator it;

    for(it = s.begin(); it != s.end(); ++it) {
      if(*it == '\r' || *it == '\n')
        break;
    }
    
    if(it != s.end()) {
      s.erase(it, s.end());
    }
}

string Center(string s) {
  stringstream ss;
  unsigned int pos = 40 - ((s.size() - 1) / 2);

  ss << setw(pos) << "";
  ss << s;
  ss << setw(79 - (pos + s.size())) << "";

  return ss.str();
}

#ifdef CENTER_TEST
int main() {
  string ss = "hej hej hej";
  string s = Center(ss);
  cout << s << endl;
}

#endif