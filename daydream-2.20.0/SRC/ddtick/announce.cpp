/* 
 * DD-Tick
 * TIC Announce routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.hpp"

#include <map>
#include <iomanip>
#include <ctime>

#include <sys/stat.h>
#include <fcntl.h>

#include "functors.hpp"
#include "scandir.hpp"
#include "fdstream.hpp"

extern "C" {
  #include "ddmsglib.h"
}

template <typename S>
void AnnounceThem(map<string, vector<TIC> > const& m, S& s) {
  for(map<string, vector<TIC> >::const_iterator it = m.begin();
      it != m.end();
      ++it) {

    s << endl;
    s << Center("--->>> Area " + (*it).first + " <<<---") << endl;

    s << setw(79) << setfill('-') << "" << endl;
    
    s << setw(20) << setfill(' ') << left << "File"
      << setw(10) << left << "Size"
      << "Description" << endl;

    s << setw(79) << setfill('-') << "" << endl;

    for(vector<TIC>::const_iterator it2 = (*it).second.begin(); 
        it2 != (*it).second.end();
        ++it2) {

      s << setw(20) << setfill(' ') << left << (*it2).file 
        << setw(10) << left << (*it2).PrintableSize();

      bool first = true;
 
      for(int i = 0; i < (*it2).desc.size(); ++i) {
        vector<string> v;
        string tmp = "";
        
        SplitLine((*it2).desc[i], v);
        
        for(int j=0; j < v.size(); ++j) {
          if(tmp.size() + v[j].size() < 45) {
            tmp += v[j] + " ";
          }
          else {
            if(first) {
              s << setw(49) << left << tmp << endl;
              first = false;
            } else {
              s << setw(30) << left << " " 
                << setw(49) << left << tmp << endl;
            }
            tmp = v[j] + " ";
          }
        }
        if(tmp.size() > 0) {
          if(first) {
            s << setw(49) << left << tmp << endl;
            first = false;
          } else {
            s << setw(30) << left << " " 
              << setw(49) << left << tmp << endl;
          }
        }
      }

    }
  }
}

void SendAnnounce(Announce const& a, map<string, vector<TIC> > const& m) {
  DayDream_Message dm;

  memset(&dm, '\0', sizeof(DayDream_Message));

  strncpy(dm.MSG_AUTHOR, a.msg_from.c_str(), 25);
  strncpy(dm.MSG_RECEIVER, a.msg_to.c_str(), 25);
  strncpy(dm.MSG_SUBJECT, a.msg_subject.c_str(), 25);
  dm.MSG_CREATION = time(NULL);
  dm.MSG_RECEIVED = time(NULL);
  dm.MSG_FLAGS = MSG_FLAGS_LOCAL;
  
  if(!a.local) {
    dm.MSG_FN_ORIG_ZONE = a.aka.zone;
    dm.MSG_FN_ORIG_NET = a.aka.net;
    dm.MSG_FN_ORIG_NODE = a.aka.node;
    dm.MSG_FN_ORIG_POINT = a.aka.point;
  }
  
  /* We open the base first to ensure it's lock while fixing the pointers */
  int fd = ddmsg_open_base((char*) cfg.Conferences[a.msgbase.first].c_str(), 
			   a.msgbase.second, O_RDWR | O_CREAT | O_APPEND, 0666);
  DayDream_MsgPointers ptrs;

  ddmsg_getptrs((char*) cfg.Conferences[a.msgbase.first].c_str(), 
		a.msgbase.second,
		&ptrs);

  if(ptrs.msp_low == 0 && ptrs.msp_high == 0) {
    dm.MSG_NUMBER = 1;
    ptrs.msp_low = 1;
    ptrs.msp_high = 1;
  }
  else {
    ptrs.msp_high++;
    dm.MSG_NUMBER = ptrs.msp_high;
  }

  ddmsg_setptrs((char*) cfg.Conferences[a.msgbase.first].c_str(), 
		a.msgbase.second,
		&ptrs);

  write(fd, &dm, sizeof(DayDream_Message));
  ddmsg_close_base(fd);

  fd = ddmsg_open_msg((char*) cfg.Conferences[a.msgbase.first].c_str(), 
		      a.msgbase.second, dm.MSG_NUMBER, O_RDWR | O_CREAT, 0666);
  
  
  boost::fdostream s(fd);

  if(!a.local) {
    s << "\1MSGID: " << string(a.aka) << " ";
    s << IntToHexString(time(NULL)) << endl;
    s << endl;
  }

  ifstream itp(a.templ.c_str(), ios::in);

  while(!itp.fail() && !itp.eof()) {
    char buf[256];
    itp.getline(buf, 256);
    
    if(!strcasecmp(buf, "@@FILES@@")) {
      AnnounceThem(m, s);
    }
    else {
      s << buf << endl;
    }
  }
  
  itp.close();

  if(!a.local) {
    s << endl << "--- " << version << endl << " * Origin: " << a.origin;
    s << " (" << string(a.aka) << ")" << endl;
    s << "SEEN-BY: " << a.aka.net << "/" << a.aka.node << endl;
  } else {
    s << endl << "Report created by " << version << endl;
  }
  
  ddmsg_close_msg(fd);
}

void announce() {
  ScandirFunctor f;
  ScanDir<ScandirFunctor> s(cfg.AnnounceQueue, false, string("tic"), f);
  map<string, vector<TIC> > m;
  vector<TIC> v;

  s.Scan();
  for(vector<string>::iterator it = f.l.begin(); it != f.l.end(); ++it) {
    TIC tic(*it, false);
    
    v.push_back(tic);

  }  

  for(int i = 0; i < cfg.Announces.size(); ++i) {
    for(int j = 0; j < v.size(); ++j) {
      for(int k = 0; k < cfg.Announces[i].areamask.size(); ++k) {
	if(patmat(v[j].area.c_str(), cfg.Announces[i].areamask[k].c_str())) {
	  m[v[j].area].push_back(v[j]);
	}
      }
    }
    if(m.size() != 0) {
      SendAnnounce(cfg.Announces[i], m);
      logfh << "Announced in " << cfg.Announces[i].msgbase.first
            << " " << cfg.Announces[i].msgbase.second << logfh.endl;
    }
  }

  v.clear();

  for(vector<string>::iterator it = f.l.begin(); it != f.l.end(); ++it) {
    unlink((*it).c_str());
  }
}
