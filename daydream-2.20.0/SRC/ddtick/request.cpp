/* 
 * DD-Tick
 * FREQ processing routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.hpp" 

#include "functors.hpp"
#include "scandir.hpp"

#include <string.h>

void process_request(FidoAddr aka, string request, string filelist, string reply, bool srif) {
  vector<string> requests;
  vector<string> files;

  ifstream i(request.c_str(), ios::in);
  ofstream o(filelist.c_str(), ios::out);
  
  if(i.fail()) {
    throw runtime_error("Cannot open request file " + request);
  }
  
  while(!i.eof() && !i.fail()) {
    char buf[256];
    
    i.getline(buf, 256);
    
    for(int j=0; j < cfg.Areas.size(); ++j) {
      if(cfg.Areas[j].request) {
        RequestFunctor f(buf);
        ScanDir<RequestFunctor> s(cfg.Areas[j].get_directory(), false, "", f);
        s.Scan();

        while(!f.response.empty()) {
          string file = f.response.back();
          f.response.pop_back();
          if(srif) {
            o << "+" << file << endl;
          }
          else {
            o << file << endl;
          }
          files.push_back(file.substr(file.find_last_of('/')+1));
        }
      }
    }
  }
  if(!srif) {
    ifstream ii(cfg.FreqRespTempl.c_str(), ios::in);
    ofstream oo(reply.c_str(), ios::out);
  
    if(ii.fail()) {
      throw runtime_error("Cannot open response template" + cfg.FreqRespTempl);
    }
  
    while(!ii.eof() && !ii.fail()) {
      char buf[256];

      ii.getline(buf, 256);

      if(!strcasecmp(buf, "@@FILES@@")) {
        while(!files.empty()) {
          oo << files.back() << endl;
          files.pop_back();
        }
      }
      else {
        oo << buf << endl;
      }
    }
  
    oo << "___ " << version << endl;
  }
}
