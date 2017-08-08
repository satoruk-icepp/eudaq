#ifndef _PARAMMAN_H_
#define _PARAMMAN_H_

#include <fstream>
#include <vector>
#include <string>

typedef int ParamVal;

class ParameterManager{
public:
  //Functions ---------------------------------------------------------------
  ParamVal GetParameter(int ArrayNum, const char* Name);
  int      GetVectorSize();
  
  //Controller --------------------------------------------------------------
  ParameterManager(std::ifstream& roFile);
  ~ParameterManager();

  typedef std::vector<std::string> NameContainor;
  typedef std::vector<ParamVal*>   ValContainor;
  
private:
  ParameterManager(); 
  ParameterManager(const ParameterManager& object);
  ParameterManager& operator = (const ParameterManager& object);
  
  NameContainor NameArray;
  ValContainor  ParamArray;

  int   sizeParamArray;
  int   sizeNameArray;
  
  int         Memory_LastParamNum;
  std::string Memory_LastName;

  bool SearchStartDefinition(std::ifstream& roFile);
  bool SetNameArray_FromFile(std::ifstream& roFile);
  void SetParamArray_FromFile(std::ifstream& roFile);
};

inline int ParameterManager::GetVectorSize(){
  return ParamArray.size();
}

#endif