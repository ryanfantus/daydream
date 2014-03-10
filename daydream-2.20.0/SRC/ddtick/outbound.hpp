/*
 * DD-Tick
 * BSO routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#ifndef OUTBOUND_HPP
#define OUTBOUND_HPP

class Outbound {
public:
  string GetFlo(FidoAddr const& f, Priority p = P_NORMAL);
  string GetTicName();
  void AddToFlo(FidoAddr const& a, string f, bool del, Priority p = P_NORMAL);
};

#endif