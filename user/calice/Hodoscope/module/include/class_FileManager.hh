#ifndef _FILEMAN_H_
#define _FILEMAN_H_

#include "class_ParameterManager.hh"
#include "class_StringManager.hh"
#include <fstream>
#include <vector>
#include <string>

class FileManager{
public:
  //Functions ----------------------------------------------------------------
  ParameterManager*    GetParameterManager(const char* Name);
  StringManager*       GetStringManager(const char* Name);
  static FileManager&  GetFileManager();
  
  //Controller ---------------------------------------------------------------
  FileManager(std::ifstream& roFile);
  ~FileManager();

private:
  //FileManager --------------------------------------------------------------
  static FileManager *_Myself;
  FileManager();
  FileManager(const FileManager& object);
  FileManager& operator =(const FileManager& object);
  
  //ParameterManager ---------------------------------------------------------
  struct Array_ForParam{
    std::string       ParamManName;
    ParameterManager *ParamMan;
  };
  std::vector<Array_ForParam*> ParamManArray;
  
  int   sizeParamManArray;

  //StringManager ------------------------------------------------------------
  struct Array_ForString{
    std::string    StringManName;
    StringManager *StringManPtr;
  };
  std::vector<Array_ForString*> StringManArray;
  
  int   sizeStringManArray;

};

inline FileManager& FileManager::GetFileManager(){
  return *_Myself;
}

#endif
