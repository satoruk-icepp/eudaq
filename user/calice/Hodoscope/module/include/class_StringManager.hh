#ifndef _STRMAN_H_
#define _STRMAN_H_

#include <fstream>
#include <vector>
#include <string>

class StringManager{
public:
  //Functions ---------------------------------------------------------------
  const char* GetString(int ArrayNum, const char* roName);
  int   GetVectorSize(const char* roName);
  
  //Controller --------------------------------------------------------------
  StringManager(std::ifstream& roFile);
  ~StringManager();
  
private:
  StringManager();
  StringManager(const StringManager& object);
  StringManager& operator = (const StringManager& object);

  typedef std::vector<std::string> StringContainor;
  struct _Data{
    std::string     Name;
    StringContainor StringArray;
  };

  typedef struct _Data Data;
  typedef std::vector<Data*> DataContainor;

  DataContainor DataVector;
  
  int         Memory_LastStringNum;
  int         Memory_LastDataNum;
  std::string Memory_LastName;
};

#endif

