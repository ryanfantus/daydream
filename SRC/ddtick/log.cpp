/*
 * DD-Tick
 * Logfile class
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include <iomanip>

#include "headers.hpp"

Log::Log() { }

void Log::open(string filename) {
  (*this).o.open(filename.c_str(), ios::app);
  (*this).valid = true;
  (*this).logit(string(version) + " started");
}

Log::~Log() {
  if((*this).valid) {
    (*this).logit(string(version) + " ended"); 
  }
}

void Log::logit(string logline) {
  std::cout << logline << std::endl;
  
  if(valid) {
    stringstream ss;
    time_t tt;
    struct tm * st = NULL;
    
    tt = time(NULL);
    st = localtime(&tt);

    ss << setw(2) << setfill('0') << (*st).tm_hour << ":"
       << setw(2) << setfill('0') << (*st).tm_min << ":"
       << setw(2) << setfill('0') << (*st).tm_sec << " "
       << setw(2) << setfill('0') << (*st).tm_mday << "-"
       << setw(2) << setfill('0') << (*st).tm_mon+1 << "-"
       << setw(2) << setfill('0') << (*st).tm_year-100;

    string datestr = ss.str();
    (*this).o << datestr << " " << logline << std::endl;
  }
}

void Log::flush() {
    string tmp = (*this).s.str();
    (*this).logit(tmp);
    (*this).s.str("");
}

Log& operator<<(Log& log, Log::Endl) {
    log.flush();
    return log;
}

