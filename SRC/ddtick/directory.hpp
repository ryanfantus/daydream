/*
 * DD-Tick
 * Class for handling daydream directory files 
 *   (confs/<conference>/data/directory.XXX)
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#ifndef DIRECTORY_HPP
#define DIRECTORY_HPP

class Directory {
public:
  Directory(string _file);
  void WriteEntry(TIC& tic);
private:
  string file;

};

#endif