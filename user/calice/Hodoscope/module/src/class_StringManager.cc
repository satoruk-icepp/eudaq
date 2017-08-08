#include "class_StringManager.hh"
#include <iostream>
#include <sstream>

//Functions -----------------------------------------------------------------
const char* StringManager::GetString(int ArrayNum, const char* roName){
  std::string UserName = roName;
  if(!Memory_LastName.empty()){
    if(true
      && UserName == Memory_LastName
      && ArrayNum == Memory_LastStringNum
      ){
      Data* tempData = DataVector.at(Memory_LastDataNum);      
      return tempData->StringArray.at(Memory_LastStringNum).c_str();
    }
  }
  
  for(unsigned int i = 0; i<DataVector.size(); ++i){
    Data* tempData = DataVector.at(i);

    if(UserName == tempData->Name){
      if(ArrayNum > static_cast<int>((tempData->StringArray.size() - 1))){
        std::cerr << "StringManager GetError" << std::endl;
        return "Error";
      }
      else{
        Memory_LastDataNum   = i;
        Memory_LastStringNum = ArrayNum;
        Memory_LastName      = UserName;
        return tempData->StringArray.at(ArrayNum).c_str();
      }
    }
  }

  std::cerr << "StringManager GetError" << std::endl;
  return "Error";
}

int StringManager::GetVectorSize(const char* roName){
  std::string UserName = roName;
  for(unsigned int i = 0; i<DataVector.size(); ++i){
    Data* tempData = DataVector.at(i);
    if(UserName == tempData->Name){
      return tempData->StringArray.size();
    }
  }

  std::cerr << "StringManager GetSizeError" << std::endl;
  return 0;
}

//Controller ----------------------------------------------------------------
StringManager::StringManager(std::ifstream& roFile){
  std::string linebuf;
  while(getline(roFile, linebuf)){
    std::istringstream LineStream(linebuf);

    std::string wordbuf;
    while(LineStream >> wordbuf){
      if(wordbuf.at(0) == '#'){
        break;
      }
      else if(wordbuf == "StartDefinition"){
        if(LineStream >> wordbuf){
          Data *OneStringData = new Data;
          OneStringData->Name = wordbuf;
          while(getline(roFile, linebuf)){
            if(linebuf.at(0) == '#'){
            continue;
            }
            else if(linebuf == "EndDefinition"){
              DataVector.push_back(OneStringData);
              break;
            }
            std::string OneString = linebuf;
            OneStringData->StringArray.push_back(OneString);
          }  
        }else{
          std::cerr << "StringManager Definition Error" << std::endl ; 
          return;
        }
      }
    }
  }
  return;
}

StringManager::~StringManager(){
  for(unsigned int i = 0; i<DataVector.size(); ++i){
    Data* Onedata = DataVector.at(i);
    Onedata->StringArray.clear();
    delete Onedata;
    Onedata = NULL;
  }

  DataVector.clear();
}

//Private functions ---------------------------------------------------------
