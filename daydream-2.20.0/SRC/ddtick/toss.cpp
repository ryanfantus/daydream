/*
 * DD-Tick
 * Tossing routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.hpp"
#include "functors.hpp"
#include "scandir.hpp"
#include <unistd.h>

void CheckSecurity(TIC& tic) {
  if(!cfg.LinkExists(tic.from)) {
    throw runtime_error("Link is not known " + string(tic.from));
  }
  
  CFG::TicPasswordType link = cfg.FindLink(tic.from);

  if(tic.pw != link.second) {
    throw runtime_error("Password is not correct");
  }

  Area a = cfg.FindArea(tic.area);

  bool found = false;
  for(int i = 0; i < a.exports.size(); ++i) {
    if(a.exports[i].ex == tic.from && a.exports[i].recv) {
      found = true;
    }
  }

  if(!found) {
    throw runtime_error("Link is not subscribed to area");
  }
}

void PrepareExport(TIC& tic) {
  Area a = cfg.FindArea(tic.area);  
  for(int i = 0; i < a.exports.size(); ++i) {
    if(a.exports[i].send) {
      tic.seenbys.insert(a.exports[i].ex);
    }
  }

  /*
    2:237/53 1263785401 Mon Jan 18 03:30:01 2010 UTC HTick/lnx 1.4.0-sta 15-01-08
  */

  time_t t = time(NULL);
  string path;
  
  string timestr = string(ctime(&t));
  timestr[timestr.size()-1] = ' ';
  timestr += "UTC";

  path = string(a.ouraka) + " ";
  path += IntToString(t) + " ";
  path += timestr + " ";
  path += version;
  
  tic.paths.push_back(path);
}

void ExportTic(FidoAddr const& exp, Area const& a, TIC const& tic) {
  try {
    logfh << "Exporting to " << string(exp) << logfh.endl;
    CFG::TicPasswordType link = cfg.FindLink(exp);
    Outbound o;
    string ticname = cfg.Outbound + "/" + o.GetTicName();
    
    TIC newtic(tic, ticname);
    
    newtic.from = a.ouraka;
    newtic.to = exp;
    newtic.pw = link.second;
    
    newtic.Dump();
    newtic.Write();
    
    Priority p = P_NORMAL;
    if(cfg.FlavourExists(newtic.to)) {
      p = cfg.FindFlavour(newtic.to).second;
    }
    
    o.AddToFlo(newtic.to, ticname, true, p);
    o.AddToFlo(newtic.to, newtic.file_obj.GetFilename(), false, p);
    
  }
  catch(runtime_error e) {
    logfh << "Cannot export to node: " << e.what() << logfh.endl;
  }
}

void hatch(string file, string area) {
  try {
    TIC tic;

    while(!cin.eof()) {
      char buf[256];
      cin.getline(buf, 256);
      tic.desc.push_back(buf);
    }

    Area a = cfg.FindArea(area);
    string filebase = file.substr(file.find_last_of('/')+1);
    File f(file);

    File nf = f.Copy(a.get_directory() + "/" + filebase);

    tic.area = area;
    tic.file = filebase;
    tic.file_obj = nf;
    tic.crc = tic.file_obj.CRC();
    tic.size = tic.file_obj.Size();
    tic.origin = a.ouraka;

    Directory d(a.get_directory_file());
    d.WriteEntry(tic);

    PrepareExport(tic);

    for(int i = 0; i < a.exports.size(); ++i) {
      if(a.exports[i].send) {
	ExportTic(a.exports[i].ex, a, tic);
      }
    }          
    if(cfg.AnnounceQueue.size() != 0) {
      Outbound o;
      string annticname = cfg.AnnounceQueue + "/" + o.GetTicName();
      TIC anntic(tic, annticname);
      anntic.Write();
    }
    logfh << "Successfully hatched " << file 
          << " to " << area.c_str()
          << logfh.endl;
  }
  catch(runtime_error e) {
    logfh << "Error: " << e.what() << logfh.endl;
  }
}


void toss() {
  ScandirFunctor f;
  ScanDir<ScandirFunctor> s(cfg.Inbound, false, string("tic"), f);

  s.Scan();
  for(vector<string>::iterator it = f.l.begin(); it != f.l.end(); ++it) {
    try {
      TIC tic(*it);
      Area a;
      
      if(cfg.AreaExists(tic.area)) {
        a = cfg.FindArea(tic.area);
      }
      else {
        throw runtime_error("Cannot find area " + tic.area);
      }
      
      try {
        tic.CheckFileIntegrity();
        CheckSecurity(tic);
        tic.LogSummary(logfh);
        tic.MoveFile(a.get_directory() + "/" + tic.file);
        Directory d(a.get_directory_file());
        d.WriteEntry(tic);
      
      } 
      catch(runtime_error e) {
        logfh << "Bad tic..: " << e.what() << logfh.endl;
        continue;
      }

      PrepareExport(tic);

      for(int i = 0; i < a.exports.size(); ++i) {
        if(a.exports[i].ex != tic.from && a.exports[i].send) {
          try {
	    ExportTic(a.exports[i].ex, a, tic);
	  }
	  catch(runtime_error e) {
	    logfh << "Cannot export to node" 
	          << string(a.exports[i].ex)
	          << ":" << e.what() << logfh.endl;
	  }
        }
      }      

      try {
        if(cfg.AnnounceQueue.size() != 0) {
          string basename = tic.tic_file.substr(tic.tic_file.find_last_of('/') + 1, tic.tic_file.size());
          string annticname = cfg.AnnounceQueue + basename;
          TIC anntic(tic, annticname);
          anntic.Write();
        }
      }
      catch(runtime_error e) {
        logfh << "Cannot write announce file " << e.what() << logfh.endl;
      }
      
      unlink((*it).c_str());
    }
    catch(runtime_error e) {
      logfh << "Cannot read ticfile: " << e.what() << logfh.endl;
    }
  }
}
