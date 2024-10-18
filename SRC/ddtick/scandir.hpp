/* 
 * DD-Tick
 * "Generic" dir scanning routine (the author is proud of this one :))
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include <dirent.h>
#include <sys/types.h>
#include <errno.h>

template <typename F>
class ScanDir {
public:
  ScanDir(string _dirname, bool _recursive, string _extension, F& _functor) : dirname(_dirname), recursive(_recursive), extension(_extension), functor(&_functor) {}
  void Scan() {
    (*this)._Scan((*this).dirname);
  }

private:
  string ErrorToString(int err) {
    switch(err) {
    case EACCES:
      return "Permissions denied";
    case ENOTDIR:
      return "Not a directory";
    case ENOENT:
      return "Directory does not exist";
    default:
      return string("Unkown error ") + IntToString(err);
    }
  }

  void _Scan(string _dirname) {
    DIR* dir;
    struct dirent* d;

    dir = opendir(_dirname.c_str());

    if(dir == 0) {
      throw std::runtime_error(ErrorToString(errno) + " " + _dirname);
    }

    while((d = readdir(dir))) {
      if((*d).d_type == DT_DIR) {
	if((*this).recursive) {
	  string tmp = _dirname;
	  tmp += (*d).d_name;
	  (*this)._Scan(tmp);
	}
      }
      else {
	string tmp = (*d).d_name;
	size_t pos = tmp.find_last_of('.');
	string a = LowerStr(tmp.substr(pos+1, tmp.size()));
	if((*this).extension == "" || a == (*this).extension) {
	
	  string tmpstr = dirname;
	  tmpstr += "/";
	  tmpstr += tmp;
	  
	  (*(*this).functor)(tmpstr);
	}
      }
    }

    closedir(dir);
  }


  string dirname;
  bool recursive;
  string extension;
  F* functor;
};
