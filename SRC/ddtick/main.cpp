/* 
 * DD-Tick
 * Main routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

using namespace std;

const char* version = "DD-Tick/" UNAME " 0.2";

#include "headers.hpp"

#include <sys/stat.h>
#include <string.h>

CFG cfg;
Log logfh;

void parse_cfg() {
  char* ptr = getenv("DDTICK");
  
  if(ptr == 0) {
    cerr << "Please set your DDTICK environment variable to the location of" << endl;
    cerr << "config file" << endl;
    exit(1);
  }
  
  cfg.Parse(ptr);
  
  if(cfg.LogFile.size() != 0) {
    logfh.open(cfg.LogFile);
  }
}

int main(int argc, char* argv[]) {

  enum { C_TOSS = 1, C_ANNOUNCE = 2, C_HATCH = 4 };
  int command = 0; /* lorte sprog */
  string hatch_file;
  string hatch_area;
  
  umask(0002);

  /* Itraexp-like requester */
  if(argc > 1 && !strcasecmp(argv[1], "req")) {
    parse_cfg();
  
    int i = argc; --i;
    string reply = argv[i]; --i;
    string filelist = argv[i]; --i;
    string request = argv[i]; --i;
    FidoAddr aka;
    aka = argv[i];
    
    process_request(aka, request, filelist, reply);
  } 
  /* SRIF-like requester */
  else if(argc > 1 && !strcasecmp(argv[1], "srif")) {

    if(argc < 3) {
      exit(1);
    }

    parse_cfg();
  
    ifstream i(argv[2], ios::in);
    
    string srifname = argv[2];
    FidoAddr aka;
    string request = "";
    string response = "";
    
    while(!i.eof()) {
      vector<string> v;
      char buffer[256];
      
      i.getline(buffer, 256);
      string tmp = buffer;
      
      RemoveCrLf(tmp);
      SplitLine(tmp, v);
      
      if(v.size() < 2) {
        continue;
      }
      
      string arg = LowerStr(v[0]);
      
      if(arg == "aka") {
        aka = v[1];
      }
      else if(arg == "requestlist") {
        request = v[1];
      } 
      else if(arg == "responselist") {
        response = v[1];
      }
    }
    
    process_request(aka, request, response, "", true);
  }
  /* Regular command */
  else {
    for(int i=1; i < argc; ++i) {
  
      string arg = LowerStr(string(argv[i]));
    
      if(arg == "toss") {
        command |= C_TOSS;
      }
      else if(arg == "announce") {
        command |= C_ANNOUNCE;
      }
      else if(arg == "hatch") {
        command |= C_HATCH;
        hatch_file = argv[i+1];
        hatch_area = argv[i+2];
        i += 2;
      }
    }

    /* We give up */
    if(command == 0) {
      cerr << version << endl;
      cerr << "Copyright Bo Simonsen, 2010" << endl;
      cerr << endl << "Possible commands area:" << endl;
      cerr << endl;
      cerr << "  TOSS - Handles inbound TIC files" << endl;
      cerr << "  ANNOUNCE - Announce queued TIC files" << endl;
      cerr << "  HATCH <file> <area> - Hatches <file> to <area>" << endl;
      cerr << "                        Description is read from stdin" << endl;
      cerr << "  REQ - Itraexp file request processor" << endl;
      cerr << "  SRIF - SRIF file request processor" << endl;
      cerr << endl;
      cerr << "Use the DDTICK environment variable to set the path" << endl;
      cerr << "for the configuration file" << endl;
      cerr << endl;
    }
    else {
      /* We got a command, now process it */
      parse_cfg();
    
      if(command & C_TOSS) {
        toss();
      }
      if(command & C_ANNOUNCE) {
        announce();
      }
      if(command & C_HATCH) {
        hatch(hatch_file, hatch_area);
      }
    }
  }
  return 0;

}
