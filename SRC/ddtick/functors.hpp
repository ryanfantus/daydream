#ifndef FUNCTORS_HPP
#define FUNCTORS_HPP

class ScandirFunctor {
public:
  void operator()(string filename) {
    (*this).l.push_back(filename);
  }

  vector<string> l;

};

class RequestFunctor {
public:
  RequestFunctor(string _desired_filename) : 
    desired_filename(LowerStr(_desired_filename)) { 
      RemoveCrLf((*this).desired_filename); 
    }

  bool operator()(string filename) {
    int pos = filename.find_last_of('/');
    
    if(patmat(LowerStr(filename.substr(pos+1)).c_str(), (*this).desired_filename.c_str())) {
      response.push_back(filename);
    }
  }
  vector<string> response;
  string desired_filename;
};

class FindFileFunctor { 
public:
  FindFileFunctor(string _desired_filename) : desired_filename(_desired_filename), found(false) { }
  bool operator()(string pathname) {
    string filename = pathname.substr(pathname.find_last_of('/') + 1);
      
    if(LowerStr(filename) == LowerStr((*this).desired_filename)) {
      found_filename = pathname;
      found = true;
    }
      
  }

  bool found;
  string found_filename;

private:
  string desired_filename;
};
#endif
