/*
 * DD-Tick
 * Logfile class
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#ifndef LOG_HPP
#define LOG_HPP

class Log {
public:
  class Endl {
  
  };

  Log();
  ~Log();
  void open(string filename);
  void flush();

  stringstream s;
  Endl endl;

private:
  void logit(string logline);

  ofstream o;
  bool valid;
};

extern Log logfh;

Log& operator<<(Log& log, Log::Endl);

template <typename T>
Log& operator<<(Log& log, T const& value) {
    log.s << value;
    return log;
}

#endif
