/* 
 * DD-Tick
 * Configuration file reader and parser
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.hpp"

#include <iomanip>

Announce::Announce(string _areamask, string _msgbase, string _msg_from, 
           string _aka, string _msg_to, string _msg_subject, 
           string _templ, string _origin) : areamask(), msgbase(), msg_from(_msg_from), 
           aka(_aka), msg_to(_msg_to), msg_subject(_msg_subject), templ(_templ), 
           origin(_origin), local(false) { 
  vector<string> v;
  SplitLine(_areamask, (*this).areamask, ',');
  SplitLine(_msgbase, v, ':');
  (*this).msgbase.first = StringToInt(v[0]);
  (*this).msgbase.second = StringToInt(v[1]);
    
  if((*this).aka.isEmpty()) {
    (*this).local = true;
  }
}

Export::Export(FidoAddr const& _ex, bool _send = true, bool _recv = true) {
  (*this).ex = _ex;
  (*this).send = _send;
  (*this).recv = _recv;
}

Area::Area() { }
Area::Area(string _name, string _conference_path, int _directory, 
       FidoAddr _ouraka, bool _request) : 
         name(_name), conference_path(_conference_path), 
         directory(_directory), ouraka(_ouraka), request(_request) { }

bool Area::isAddrSubscribed(FidoAddr f) {
    for(vector<Export>::iterator it = (*this).exports.begin(); 
        it != (*this).exports.end();
        ++it) {
        
      if((*it).ex == f) {
        return true;
      }
    }
    
    return false;
}

string Area::get_directory() {
  string filename = (*this).conference_path + "/data/paths.dat";
  ifstream is(filename.c_str(), ios::in);
    
  if(is.fail()) {
    throw runtime_error("Cannot read " + filename);
  }
    
  char buf[256];
  for(int i = 1; i <= (*this).directory; ++i) {
    is.getline(buf, 256);
     
    if(is.fail() || is.eof()) {
      throw runtime_error("Cannot find dir for " + (*this).conference_path + " " + IntToString(i));
    }
  }
  
  return buf;
}
  
string Area::get_directory_file() {
  stringstream ss;
  string result;
  
  ss << (*this).conference_path;
  ss << "/data/directory.";
  ss << setw(3) << setfill('0') << (*this).directory;
  
  return ss.str();
}

void CFG::Parse(string file) {

  ifstream fh(file.c_str(), ifstream::in);
  char buf[256];
  string buffer;
  int line = 0;
  vector<string> v;

  (*this).Conferences.resize(64);
    
  if(fh.fail())
    throw std::runtime_error("Cannot open config file '" + file + "'");

  while (!fh.eof()) {
    fh.getline(buf, 256);
    buffer = buf;
      
    line++;
      
    if(buffer.size() == 0) 
      continue;
      
    /* Comments */
    if(buffer[0] == ';') 
      continue;
    if(buffer[0] == '#') 
      continue;
 
    // We can still have carrige returns I guess.
    RemoveCrLf(buffer);
 
    v.clear();

    SplitLine(buffer, v);
      
    if(v.size() < 2) {
      throw std::runtime_error("No space found in line " + IntToString(line));
    }
      
    string key = LowerStr(v[0]);
    if(key == "aka") {
      FidoAddr f;
      f = v[1];
      (*this).Akas.push_back(f);
    }
    else if(key == "inbound") {
      (*this).Inbound = v[1];
    }
    else if(key == "outbound") {
      (*this).Outbound = v[1];
    }
    else if(key == "logfile") {
      (*this).LogFile = v[1];
    }
    else if(key == "announcequeue") {
      (*this).AnnounceQueue = v[1];
    }
    else if(key == "freqresptempl") {
      (*this).FreqRespTempl = v[1];
    }
    else if(key == "defzone") {
      (*this).DefaultZone = StringToInt(v[1]);
    }
    else if(key == "useto") {
      if(LowerStr(v[1]) == "yes") {
        (*this).UseTo = true;
      }
      else {
        (*this).UseTo = false;
      }
    }
    else if(key == "conference") {
      int number = StringToInt(v[1]);
      if(number > 0 && number < 64) {
        (*this).Conferences[number] = v[2];
      }
    }
    else if(key == "ticpw") {
      TicPasswordType t;
        
      if(v.size() < 3) {
        throw std::runtime_error("Error in TicPw " + IntToString(line));
      }
        
      t.first = v[1];
      t.second = v[2];
      (*this).TicPasswords.push_back(t);
    }
    else if(key == "flavour") {
      FlavType fl;
      fl.first = v[1];
      string tmp = LowerStr(v[2]);
      if(tmp == "hold") {
        fl.second = P_HOLD;
      }
      else if(tmp == "crash") {
        fl.second = P_CRASH;
      }
      else {
        fl.second = P_NORMAL;
      }
      (*this).Flavours.push_back(fl);
    }
    else if(key == "area") {
      string area = v[1];
      if(v.size() < 3) {
        throw std::runtime_error("Error in Area " + IntToString(line));
      }

      vector<string> v2;
      SplitLine(v[2], v2, ':');
      int conference = StringToInt(v2[0]);
      int directory = StringToInt(v2[1]);
        
      FidoAddr ouraka;
      bool request;
      vector<Export> exports;
        
      for(int i = 3; i < v.size(); ++i) {
        if(v[i] == "-a") {
          ouraka = v[i+1];
          ++i;
        } 
        else if(v[i] == "-r") {
          request = true;
        }
        else {
          if(v[i][0] == '%') {
            // send only
            FidoAddr f;
            f = v[i].substr(1);
            Export e(f, true, false);
            exports.push_back(e);
          }
          else if(v[i][0] == '&') {
	    // recv only
	    FidoAddr f;
	    f = v[i].substr(1);
	    Export e(f, false, true);
	    exports.push_back(e);
	  }
	  else {
	    FidoAddr f;
	    f = v[i];
	    Export e(f);
	    exports.push_back(e);
	  }
        }
      } 
        
      Area a(area, (*this).Conferences[conference], directory, ouraka, request);
      a.exports = exports;
      (*this).Areas.push_back(a);
    }     
    else if(key == "announce") {
      Announce a(v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]);
      (*this).Announces.push_back(a);
    }
  }

  fh.close();
}

void CFG::Dump() {
  cerr << "Inbound: " << (*this).Inbound << endl;
  cerr << "Outbound: " << (*this).Outbound << endl;
  cerr << "AnnounceQueue: " << (*this).AnnounceQueue << endl;
  cerr << "Logfile: " << (*this).LogFile << endl;
  cerr << "FreqRespTempl: " << (*this).FreqRespTempl << endl;
    
  for(size_t i = 0; i < (*this).Akas.size(); ++i) {
    cerr << "Aka: " << string((*this).Akas[i]) << endl;
  }
  for(size_t i = 0; i < (*this).TicPasswords.size(); ++i) {
    cerr << "Link: " 
         << string((*this).TicPasswords[i].first) 
         << ",Tic Password: " 
         << (*this).TicPasswords[i].second 
         << endl;
  }
  for(size_t i = 0; i < (*this).Flavours.size(); ++i) {
    cerr << "Link: " 
         << string((*this).Flavours[i].first) 
         << ",Flav: " 
         << (*this).Flavours[i].second 
         << endl;
  }
  for(size_t i = 0; i < (*this).Areas.size(); ++i) {
    cerr << "Area: " 
         << (*this).Areas[i].name
         << ",Conference: " 
         << (*this).Areas[i].conference_path
         << ",Directory: "  
         << (*this).Areas[i].directory
         << ",Our aka: " 
         << string((*this).Areas[i].ouraka)
         << endl;
    for(size_t j = 0; j < (*this).Areas[i].exports.size(); ++j) {
      cerr << "Export: " 
           << string((*this).Areas[i].exports[j].ex) 
           << ((*this).Areas[i].exports[j].send ? ", Send" : ", NoSend")
	   << ((*this).Areas[i].exports[j].recv ? ", Recv" : ", NoRecv")
	   << endl;
    }
  }

  for(size_t i = 0; i < (*this).Conferences.size(); ++i) {
    if((*this).Conferences[i] != "")
    cerr << "Conference: " << i << ", " << (*this).Conferences[i] << endl;
  }

  for(size_t i = 0; i < (*this).Announces.size(); ++i) {
    cerr << "Areamask: ";
    for(int j = 0; j < (*this).Announces[i].areamask.size(); j++)
      cerr << (*this).Announces[i].areamask[j] << " ";

    cerr << " Conference: " 
         << (*this).Announces[i].msgbase.first
         << " Msgbase: " 
         << (*this).Announces[i].msgbase.second
         << " Msg From: " 
         << (*this).Announces[i].msg_from
         << " Aka: "
	 << string((*this).Announces[i].aka)
         << " Msg To: "
         << (*this).Announces[i].msg_to
         << " Msg Subject: "
         << (*this).Announces[i].msg_subject
         << " Template: " 
         << (*this).Announces[i].templ
         << " Origin: "
         << (*this).Announces[i].origin
         << endl;
  }
}

bool CFG::LinkExists(FidoAddr f) {
  for(int i=0; i < (*this).TicPasswords.size(); ++i) {
    if(f == (*this).TicPasswords[i].first) {
      return true;
    }
  }
  return false;
}

CFG::TicPasswordType& CFG::FindLink(FidoAddr f) {
  for(int i=0; i < (*this).TicPasswords.size(); ++i) {
    if(f == (*this).TicPasswords[i].first) {
      return (*this).TicPasswords[i];
    }
  }
}

bool CFG::AreaExists(string a) {
  for(int i=0; i < (*this).Areas.size(); ++i) {
    if(a == (*this).Areas[i].name) {
      return true;
    }
  }
  return false;
}

Area& CFG::FindArea(string a) {
  for(int i=0; i < (*this).Areas.size(); ++i) {
    if(a == (*this).Areas[i].name) {
      return (*this).Areas[i];
    }
  }
}
  
bool CFG::FlavourExists(FidoAddr f) {
  for(int i=0; i < (*this).Flavours.size(); ++i) {
    if(f == (*this).Flavours[i].first) {
      return true;
    }
  }
  return false;
}
  
CFG::FlavType CFG::FindFlavour(FidoAddr f) {
  for(int i=0; i < (*this).Flavours.size(); ++i) {
    if(f == (*this).Flavours[i].first) {
      return (*this).Flavours[i];
    }
  }
}


