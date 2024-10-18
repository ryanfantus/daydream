/* 
 * DD-Tick
 * Configuration file reader and parser
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#ifndef CFG_HPP
#define CFG_HPP

enum Priority { P_NORMAL = 0, P_HOLD = 1, P_CRASH = 2}; 

class Announce {
public:
  Announce(string _areamask, string _msgbase, string _msg_from, 
           string _aka, string _msg_to, string _msg_subject, 
           string _templ, string _origin);

  vector<string> areamask;
  std::pair<int, int> msgbase;
  string msg_from;
  FidoAddr aka;
  string msg_to;
  string msg_subject;
  string templ;
  string origin;
  bool local;
};

class Export {
public:
  Export(FidoAddr const& _ex, bool _send, bool _recv);

  FidoAddr ex;
  bool send;
  bool recv;
};

class Area {
public:
  Area();
  Area(string _name, string _conference_path, int _directory, 
       FidoAddr _ouraka, bool _request);

  bool isAddrSubscribed(FidoAddr f);
  string get_directory();
  string get_directory_file();

  string name;
  int directory;
  string conference_path;
  FidoAddr ouraka;
  bool request;
  vector<Export> exports;
};

class CFG {
public:
  typedef std::pair<FidoAddr, string> TicPasswordType;
  typedef std::pair<FidoAddr, Priority> FlavType;

  void Parse(string file);
  void Dump();
  bool LinkExists(FidoAddr f);
  TicPasswordType& FindLink(FidoAddr f);
  bool AreaExists(string a);
  Area& FindArea(string a);
  bool FlavourExists(FidoAddr f);
  FlavType FindFlavour(FidoAddr f);

  int DefaultZone;
  bool UseTo;
  string Inbound;
  string Outbound;
  string AnnounceQueue;
  string FreqRespTempl;
  string LogFile;
  vector<FidoAddr> Akas;
  vector<TicPasswordType> TicPasswords;
  vector<FlavType> Flavours;
  vector<Area> Areas;
  vector<Announce> Announces;
  vector<string> Conferences;

};

extern CFG cfg;

#endif
