/* 
 * DD-Tick
 * Fido address handling routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include "headers.hpp"

FidoAddr::FidoAddr() : zone(), net(), node(), point() { }

FidoAddr::FidoAddr(FidoAddr const& f) : 
  zone(f.zone), net(f.net), node(f.node), point(f.point) {}

FidoAddr::FidoAddr(string const& s) : 
  zone(), net(), node(), point() {
    (*this).operator=(s);
}
void FidoAddr::operator=(string s) {
    
  size_t z = s.find_first_of(':');
  if(z != string::npos) {
    
    size_t i = z-1;
    while(i > 0) {
      if(s[i] >= '0' && s[i] <= '9') {
        --i;
      }
      else {
        break;
      }
    }
    (*this).zone = StringToInt(s.substr(i, z-i));
    z++;
  }
  else {
    (*this).zone = 0;
    z = 0;
  }

  size_t n = s.find_first_of('/');
  if(n != string::npos) {
    (*this).net = StringToInt(s.substr(z, n-z));
    n++;
    size_t p = s.find_first_of('.');
    if(p == string::npos) {
      (*this).node = StringToInt(s.substr(n, s.size()-n));
    }
    else {
      (*this).node = StringToInt(s.substr(n, p-n));
      p++;
      (*this).point = StringToInt(s.substr(p, s.size()-p));
    }
  } 
  else {
    (*this).net = 0;
    (*this).node = 0;
    (*this).point = 0;
  }
} 

FidoAddr::operator string() const {
  string s;
  s = IntToString((*this).zone) + 
      ":" + 
      IntToString((*this).net) + 
      "/" + 
      IntToString((*this).node);

  if((*this).point != 0) {
    s += "." + IntToString((*this).point);
  }
  return s;
}
bool FidoAddr::operator==(FidoAddr const& f) {
  if(f.zone == (*this).zone &&
     f.net == (*this).net &&
     f.node == (*this).node &&
     f.point == (*this).point) {
       return true;
    }
     
  return false;
}
bool FidoAddr::operator!=(FidoAddr const& f) {
  return !(*this).operator==(f);
}
bool FidoAddr::operator<(FidoAddr const& f) const {
  if((*this).zone != f.zone)
    return (*this).zone < f.zone;
  if((*this).net != f.net)
    return (*this).net < f.net;
  if((*this).node != f.node)
    return (*this).node < f.node;
  if((*this).point != f.point)
    return (*this).point < f.point;
}
bool FidoAddr::isEmpty() {
  return (*this).zone == 0 && 
         (*this).net == 0 && 
         (*this).node == 0 &&
         (*this).point == 0;
}

#ifdef FIDOADDR_UNITTEST
/* 
 * The following unit test was passed, please run
 * it if you change the code! 
 */

#include <cassert>
int main() {
  cout << "One" << endl;
  FidoAddr f1("2:236/100");
  assert(f1.zone == 2 && f1.net == 236 && f1.node == 100 && f1.point == 0);
  cout << "Two" << endl;
  FidoAddr f2("2:236/100.100");
  assert(f2.zone == 2 && f2.net == 236 && f2.node == 100 && f2.point == 100);
  cout << "Three" << endl;
  FidoAddr f3("Bo Simonsen, 2:236/100");
  assert(f3.zone == 2 && f3.net == 236 && f3.node == 100 && f3.point == 0);
  cout << "Four" << endl;
  FidoAddr f4("236/100");
  assert(f4.zone == 0 && f4.net == 236 && f4.node == 100 && f4.point == 0);
  cout << "Five" << endl;
  FidoAddr f5("1:1/0");
  assert(f5.zone == 1 && f5.net == 1 && f5.node == 0 && f4.point == 0);
  cout << "Six" << endl;
  FidoAddr f6("-");
  assert(f6.isEmpty() == true);
}
#endif
