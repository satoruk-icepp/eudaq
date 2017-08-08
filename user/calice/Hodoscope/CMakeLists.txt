cmake_dependent_option(USER_CALICE_BUILD_HODOSCOPE "build user/calice HODOSCOPE" ON
  "USER_CALICE_BUILD" OFF)

get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME}) 

if(NOT USER_CALICE_BUILD_HODOSCOPE)
  message("user/calice Hodoscope is not to be built")
  return() 
endif()

#include_directories(./include)

add_subdirectory(module)