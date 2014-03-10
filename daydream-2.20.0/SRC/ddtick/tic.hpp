/*
 * DD-Tick
 * TIC handling routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */


#ifndef TIC_HPP
#define TIC_HPP

class TIC {
public:
  TIC();
  TIC(string _tic_file, bool create_file_obj = true);
  TIC(TIC const& tic, string _tic_file);
  void Dump();

  template <typename C>
  void PrintSummary(C& c) {
    c << "* TIC: " 
      << (*this).tic_file.substr((*this).tic_file.find_last_of('/')+1, (*this).tic_file.size()) 
      << ", From: " 
      << string((*this).from) 
      << ", Origin: " 
      << string((*this).origin) 
      << ", Area: " 
      << (*this).area 
      << endl;

    c << "* File: " 
      << (*this).file 
      << ", Size: " 
      << (*this).PrintableSize((*this).size) 
      << endl;
  }

  template <typename L>
  void LogSummary(L& l) {
    string _tic_name = (*this).tic_file.substr((*this).tic_file.find_last_of('/')+1, (*this).tic_file.size());
    string _from = string((*this).from);
    string _origin = string((*this).origin);

    l << "* TIC: " << _tic_name 
      << ", From: " << _from 
      << ", Origin: " << _origin 
      << l.endl;  

    string _area = (*this).area;
    string _file = (*this).file;
    string _size = (*this).PrintableSize((*this).size);

    l << "* Area: " << _area 
      << ", File: " << _file 
      << ", Size: " << _size 
      << l.endl;
  }

  void Write();
  void MoveFile(string new_path);
  string PrintableSize() const;
  bool CheckFileIntegrity();
private:
  string PrintableSize(int size) const;
  
  template <typename C>
  void Write(C& c) {
    c << "Created by " << version << endl;
    c << "File " << (*this).file << endl;
    c << "Area " << (*this).area << endl;
    for(int i = 0; i < (*this).desc.size(); ++i) {
      c << "Desc " << (*this).desc[i] << endl;
    }
    c << "From " << string((*this).from) << endl;
    if(cfg.UseTo) {
      c << "To " << string((*this).to) << endl;
    }
    c << "Origin " << string((*this).origin) << endl;
    c << "Size " << (*this).size << endl;
    c << "CRC " << IntToHexString((*this).crc) << endl;

    for(int i = 0; i < (*this).tic_lines.size(); ++i) {
      c << (*this).tic_lines[i] << endl;
    }   

    for(int i = 0; i < (*this).paths.size(); ++i) {
      c << "Path " << (*this).paths[i] << endl;
    }

    for(set<FidoAddr>::iterator it = (*this).seenbys.begin();
        it != (*this).seenbys.end();
	++it) {
      c << "Seenby " << string(*it) << endl;
    }
   
    c << "Pw " << (*this).pw << endl;
  }

  void Read();

public:
  string tic_file;
  string file;
  File file_obj;
  string area;
  vector<string> desc;
  FidoAddr from;
  FidoAddr to;
  FidoAddr origin;
  size_t size;
  int crc;
  string pw;
  set<FidoAddr> seenbys;
  vector<string> paths;  
  vector<string> tic_lines;
};

#endif