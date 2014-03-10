/*
 * DD-Tick
 * Class for handling daydream directory files 
 *   (confs/<conference>/data/directory.XXX)
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.hpp"

#include <iomanip>

Directory::Directory(string _file) : file(_file) {}

void Directory::WriteEntry(TIC& tic) {
  time_t t = time(NULL);
  ofstream o((*this).file.c_str(), ios::app);

  if(o.fail()) {
    throw runtime_error("Error writing directory file");
  }

  o << setw(35) << left << tic.file 
    << "N---" 
    << setw(8) << right << tic.size 
    << " "
    << left << ctime(&t);

  for(int i=0; i < tic.desc.size(); ++i) {
    vector<string> v;
    SplitLine(tic.desc[i], v);
      
    string tmp = "";
    for(int j=0; j < v.size(); ++j) {
      if(v[j].size() + tmp.size() < 45) {
        tmp += v[j] + " ";
      }
      else {
        o << setw(35) << "" << tmp << endl;
        tmp = v[j] + " ";
      }
    }
    if(tmp.size() > 0) {
      o << setw(35) << "" << tmp << endl;
    }
  }
   

  o << setw(35) << "" << "[ Imported by " << version << " ]" << endl;
  o.close();
   
}

