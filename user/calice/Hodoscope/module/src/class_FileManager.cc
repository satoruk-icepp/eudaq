#include "class_FileManager.hh"
#include <iostream>
#include <sstream>

FileManager *FileManager::_Myself = NULL;

//Functions -----------------------------------------------------------------
ParameterManager* FileManager::GetParameterManager(const char* Name){
  std::string UserName = Name;
  for(int i = 0; i<sizeParamManArray; ++i){
    Array_ForParam *ThisComponent = ParamManArray.at(i);
    if(UserName == ThisComponent->ParamManName){
      return ThisComponent->ParamMan;
    }
  }
  std::cerr << "FileManager ParameterManager get error" << std::endl;
  return NULL;
}

StringManager* FileManager::GetStringManager(const char* Name){
  std::string UserName = Name;
  for(int i = 0; i<sizeStringManArray; ++i){
    Array_ForString *ThisComponent = StringManArray.at(i);
    if(UserName == ThisComponent->StringManName){
      return ThisComponent->StringManPtr;
    }
  }

  std::cerr << "FileManager StringMan get error" << std::endl;
  return NULL;
}

//Controller ----------------------------------------------------------------
FileManager::FileManager(std::ifstream& roFile){
  std::string linebuf;
  while(getline(roFile, linebuf)){
    std::istringstream LineStream(linebuf);

    std::string wordbuf;
    while(LineStream >> wordbuf){
      if(wordbuf.at(0) == '#'){
        break;
      }
      if(wordbuf == "NameParam"){
        LineStream >> wordbuf;
        std::string tempParamManName = wordbuf;
        
        std::string templinebuf;
        getline(roFile, templinebuf);
        std::istringstream tempLineStream(templinebuf);
        tempLineStream >> wordbuf;
        std::ifstream ParamFile(wordbuf.c_str());
        ParameterManager *OneParamMan  = new ParameterManager(ParamFile);
        Array_ForParam   *OneComponent = new Array_ForParam;
        OneComponent->ParamManName = tempParamManName;
        OneComponent->ParamMan     = OneParamMan;

        ParamManArray.push_back(OneComponent);

        break;
      }

      if(wordbuf == "NameString"){
        LineStream >> wordbuf;
        std::string tempStringManName = wordbuf;
        
        std::string templinebuf;
        getline(roFile, templinebuf);
        std::istringstream tempLineStream(templinebuf);
        tempLineStream >> wordbuf;
        std::ifstream StringFile(wordbuf.c_str());
        StringManager   *OneStringMan = new StringManager(StringFile);
        Array_ForString *OneComponent = new Array_ForString;
        OneComponent->StringManName = tempStringManName;
        OneComponent->StringManPtr  = OneStringMan;

        StringManArray.push_back(OneComponent);

        break;
      }
    }
  }

  sizeParamManArray  = ParamManArray.size();
  sizeStringManArray = StringManArray.size();

  _Myself = this;
  
  return;
}

FileManager::~FileManager(){
  for(int i = 0; i<sizeParamManArray; ++i){
    Array_ForParam *OneComponent = ParamManArray.at(i);
    delete OneComponent->ParamMan;
    OneComponent->ParamMan = NULL;
    
    delete OneComponent;
    OneComponent = NULL;
  }

  ParamManArray.clear();

  for(int i = 0; i<sizeStringManArray; ++i){
    Array_ForString *OneComponent = StringManArray.at(i);
    delete OneComponent->StringManPtr;
    OneComponent->StringManPtr  = NULL;
    
    delete OneComponent;
    OneComponent = NULL;
  }

  StringManArray.clear();

  _Myself = NULL;
}
