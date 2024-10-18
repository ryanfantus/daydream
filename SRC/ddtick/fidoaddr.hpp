/* 
 * DD-Tick
 * Fido address handling routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#ifndef FIDOADDR_HPP
#define FIDOADDR_HPP

class FidoAddr {
public:
  FidoAddr();
  FidoAddr(FidoAddr const& f);
  FidoAddr(string const& s);
  void operator=(string s);
  operator string() const;
  bool operator==(FidoAddr const& f);
  bool operator!=(FidoAddr const& f);
  bool operator<(FidoAddr const& f) const;
  bool isEmpty();

  int zone;
  int net;
  int node;
  int point;
};

#endif

