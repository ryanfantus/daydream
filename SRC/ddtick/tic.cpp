#include "headers.hpp"

#include <cmath>
#include <ctime>

TIC::TIC() : tic_file(), file(), area(), file_obj(), desc(), 
             from(), to(), origin(), size(), crc(), seenbys(), 
             paths(), pw(), tic_lines() {}

TIC::TIC(string _tic_file, bool create_file_obj) 
  : tic_file(_tic_file) {
  (*this).Read();

  size_t pos = (*this).tic_file.find_last_of('/');
   
  string file_str = (*this).tic_file.substr(0, pos);
  file_str += "/";
  file_str += (*this).file;
    
  if(create_file_obj)
    (*this).file_obj = File(file_str);
}

TIC::TIC(TIC const& tic, string _tic_file) : tic_file(_tic_file), 
  file(tic.file), area(tic.area), file_obj(tic.file_obj), 
  desc(tic.desc), from(tic.from), to(tic.to), origin(tic.origin), 
  size(tic.size), crc(tic.crc), seenbys(tic.seenbys), 
  paths(tic.paths), pw(tic.pw), tic_lines(tic.tic_lines) {}

void TIC::Dump() {
  cerr << "-------------- TIC File -------------------" << endl;
  cerr << "TIC Filename: " << (*this).tic_file << endl;
  cerr << "File: " << (*this).file << endl;
  cerr << "Area: " << (*this).area << endl;
  for(int i = 0; i < (*this).desc.size(); ++i) {
    cerr << "Desc: " << (*this).desc[i] << endl;
  }
  cerr << "From: " << string((*this).from) << endl;
  cerr << "To: " << string((*this).to) << endl;
  cerr << "Origin: " << string((*this).origin) << endl;
  cerr << "Size: " << (*this).size << endl;
  cerr << "CRC: " << (*this).crc << endl;
  cerr << "Seenbys: ";
  for(set<FidoAddr>::iterator it = (*this).seenbys.begin();
      it != (*this).seenbys.end();
      ++it) {
    cerr << string(*it) << " ";
  }
   
  cerr << endl;
  for(int i = 0; i < (*this).paths.size(); ++i) {
    cerr << "Path: " << (*this).paths[i] << endl;
  }

  cerr << "Pw: " << (*this).pw << endl;

  cerr << "*** MISC LINES ***" << endl;
  for(int i = 0; i < (*this).tic_lines.size(); ++i) {
    cerr << "L: " << (*this).tic_lines[i] << endl;
  }   
  cerr << "-------------- End TIC File ---------------" << endl;
}

bool TIC::CheckFileIntegrity() {
  if((*this).size != (*this).file_obj.Size()) {
    throw std::runtime_error("Sizes does not match");
  }
  if((*this).crc != (*this).file_obj.CRC()) {
    throw std::runtime_error("CRC does not match");
  }
}

void TIC::Write() {
  ofstream o((*this).tic_file.c_str(), ios::out);
  if(o.fail()) {
    throw runtime_error("Cannot write to file " + (*this).tic_file);
  }
  
  (*this).Write(o);
  o.close();
}

void TIC::MoveFile(string new_path) {
  (*this).file_obj = (*this).file_obj.Move(new_path);    
}

string TIC::PrintableSize() const {
  return (*this).PrintableSize((*this).size);
}

string TIC::PrintableSize(int size) const {
  string result;
  int tmp = 0;
  if((tmp = size / pow(10.0, 9)) > 0) {
    result = IntToString(tmp) + " GB";
  }
  else if((tmp = size / pow(10.0, 6)) > 0) {
    result = IntToString(tmp) + " MB";
  }
  else if((tmp = size / pow(10.0, 3)) > 0) {
    result = IntToString(tmp) + " KB";
  }
  else {
    result = IntToString(tmp) + " B";
  }

  return result;
}

void TIC::Read() {
  ifstream fh((*this).tic_file.c_str(), ifstream::in);
  char buf[256];
  string buffer;
  vector<string> v;

  if(fh.fail()) {
    throw std::runtime_error("Cannot open tic file '" + (*this).tic_file + "'");
  }

  while(!fh.eof()) {
    fh.getline(buf, 256);
    buffer = buf;
    
    if(buffer.size() == 0) {
      continue;
    }
      
    RemoveCrLf(buffer);
     
    if(LowerStr(buffer.substr(0, 10)) == "created by") {
      continue;

      }
      
    v.clear();
    SplitLine(buffer, v, ' ', 1);
    string key = LowerStr(v[0]);
      
    if(key == "file") {
      (*this).file = v[1];
    }
    else if(key == "area") {
      (*this).area = v[1];
    }
    else if(key == "desc") {
      (*this).desc.push_back(v[1]);
    } 
    else if(key == "from") {
      (*this).from = v[1];
    }
    else if(key == "to") {
      (*this).to = v[1];
    }
    else if(key == "origin") {
      (*this).origin = v[1];
    }
    else if(key == "size") {
      (*this).size= StringToInt(v[1]);
    }
    else if(key == "crc") {
      (*this).crc = HexStringToInt(v[1]);
    }
    else if(key == "pw") {
      (*this).pw = v[1];
    }
    else if(key == "seenby") {
      FidoAddr f;
      f = v[1];
      (*this).seenbys.insert(f);
    }
    else if(key == "path") {
      (*this).paths.push_back(v[1]);
    }
    else {
      (*this).tic_lines.push_back(buffer);
    }
  }
}

