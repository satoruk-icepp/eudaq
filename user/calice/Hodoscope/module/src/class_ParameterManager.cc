#include "class_ParameterManager.hh"
#include <iostream>
#include <sstream>
#include <cstdlib>

//Functions -----------------------------------------------------------------
ParamVal ParameterManager::GetParameter(int ArrayNum, const char* Name){
  if(ArrayNum > (sizeParamArray - 1)){
    std::cerr << "ParameterManager ArrayNum is Too big" << std::endl;
    return 0;
  }
  
  std::string UserName = Name;
  if(!Memory_LastName.empty()){
    if(UserName == Memory_LastName){
      ParamVal *tempParam = ParamArray.at(ArrayNum);
      
      return *(tempParam + Memory_LastParamNum);
    }
  }

  for(int i = 0 ; i<sizeNameArray; ++i){
    if(UserName == NameArray.at(i)){
      Memory_LastName     = UserName;
      Memory_LastParamNum = i;
      
      ParamVal *tempParam = ParamArray.at(ArrayNum);
      
      return *(tempParam + i);
    }
  }

  std::cerr << "ParameterManager GetError" << std::endl;
  return 0;
}


//Controller ----------------------------------------------------------------
ParameterManager::ParameterManager(std::ifstream& roFile){
  bool DefinitionStart = false;
  bool DefinitionEnd   = false;
  DefinitionStart = SearchStartDefinition(roFile);

  if(DefinitionStart){
    DefinitionEnd = SetNameArray_FromFile(roFile);
  }
  
  if(DefinitionEnd){
    SetParamArray_FromFile(roFile);
  }

  return;
}

ParameterManager::~ParameterManager(){
  NameArray.clear();

  for(int i = 0; i<sizeParamArray; ++i){
    ParamVal* tempptr = ParamArray.at(i);
    delete tempptr;
  }
  ParamArray.clear();
}

//Private Functions ---------------------------------------------------------
bool ParameterManager::SearchStartDefinition(std::ifstream& roFile){
  std::string linebuf;
  while(getline(roFile, linebuf)){
    std::istringstream LineStream(linebuf);

    std::string wordbuf;
    while(LineStream >> wordbuf){
      if(wordbuf.at(0) == '#'){
        break;
      }
      else if(wordbuf == "StartDefinition"){
        return true;
      }
    }
  }
  
  std::cerr << "ParameterManager Definition Error" << std::endl ; 
  return false;
}

bool ParameterManager::SetNameArray_FromFile(std::ifstream& roFile){
  std::string linebuf;
  while(getline(roFile, linebuf)){
    std::istringstream linestream(linebuf);

    std::string wordbuf;
    while(linestream >> wordbuf){
      if(wordbuf.at(0) == '#'){
        break;
      }
      else if(wordbuf == "EndDefinition"){
        sizeNameArray = NameArray.size();
        return true;
      }
      std::string Name = wordbuf;
      NameArray.push_back(Name);
    }
  }
  
  std::cerr << "ParameterManager Make NameArray Error" << std::endl;
  return false;
}

void ParameterManager::SetParamArray_FromFile(std::ifstream& roFile){
  std::string linebuf;
  while(getline(roFile, linebuf)){
    if(linebuf.at(0) == '#'){
      continue;
    }
    
    {
      std::istringstream linestream(linebuf);
      
      std::string wordbuf;
      ParamVal *Value = new ParamVal[sizeNameArray];
      for(int i = 0; i<sizeNameArray; ++i){
        linestream >> wordbuf;
        *(Value + i) = atoi(wordbuf.c_str());
      }

      ParamArray.push_back(Value);
    }
  }

  sizeParamArray = ParamArray.size();

  return;
}
