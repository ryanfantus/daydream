/*
 * DD-Tick
 * BSO routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include <iomanip>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "headers.hpp"

string Outbound::GetFlo(FidoAddr const& f, Priority p) {
  struct stat s;
  stringstream ss;
  string result;
  
  ss << cfg.Outbound;
  if(f.zone != cfg.DefaultZone) {
    ss << ".";
    ss << setw(3) << setfill('0') << hex << f.zone;

    if(stat(ss.str().c_str(), &s) == -1) {
      mkdir(ss.str().c_str(), 0777);
    }
  }
  
  ss << "/";
  ss << setw(4) << setfill('0') << hex << f.net;
  ss << setw(4) << setfill('0') << hex << f.node;

  if(f.point != 0) {
    ss << ".pnt";
  
    if(stat(ss.str().c_str(), &s) == -1) {
      mkdir(ss.str().c_str(), 0777);
    }
  
    ss << "/";
    ss << setw(8) << setfill('0') << hex << f.point;
  }

  switch(p) {
  case P_NORMAL:
    ss << ".flo"; break;
  case P_HOLD:
    ss << ".hlo"; break;
  case P_CRASH:
    ss << ".clo"; break;
  }

  return ss.str();
}

string Outbound::GetTicName() {
  struct stat s;
  int c;
  
  string path = cfg.Outbound + "/ddtick.cnt";
  ifstream i(path.c_str(), ios::in);
  if(i.fail()) {
    c = time(NULL);
    ofstream o(path.c_str(), ios::out);
    o << c;
    o.close();
  }
  else {
    /* Read */
    i >> c;
    i.close();
    /* Increase */
    c++;
    /* Write */
    ofstream o(path.c_str(), ios::out);
    o << c;
    o.close();
  }

  stringstream ss;
  string result;
  
  ss << setw(8) << setfill('0') << hex << c << ".tic";
  ss >> result;
  
  return result;
}
  
void Outbound::AddToFlo(FidoAddr const& a, string f, bool del, Priority p) {
  string flofile = (*this).GetFlo(a, p);
    
  ofstream o(flofile.c_str(), ios::app);
    
  if(o.fail()) {
    throw runtime_error("Cannot write to " + flofile);
  }
    
  if(del) {
    o << "^"; // Kill, truncate is #..
  }

  o << f << endl;

  o.close();
}

#ifdef OUTBOUND_UNITTEST

#include <cassert>

CFG cfg;
int main() {
  Outbound o;
  cfg.Outbound = "/home/bbs/fido/out";
  cfg.DefaultZone = 2;
  FidoAddr f;
  f = "2:236/100.100";
  assert(o.GetFlo(FidoAddr(f), P_HOLD) == "/home/bbs/fido/out/00ec0064.pnt/00000064.hlo");
  FidoAddr f2;
  f2 = "1:261/38";
  assert(o.GetFlo(FidoAddr(f2), P_CRASH) == "/home/bbs/fido/out.001/01050026.clo");
  FidoAddr f3;
  f3 = "99:99/0";
  assert(o.GetFlo(FidoAddr(f3), P_NORMAL) == "/home/bbs/fido/out.063/00630000.flo");
  cout << "Outbound unittest complete" << endl;
}

#endif
