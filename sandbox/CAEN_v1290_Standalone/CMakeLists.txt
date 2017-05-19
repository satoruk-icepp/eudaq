cmake_minimum_required (VERSION 2.8)

add_definitions(-DLINUX)		#necessary for compilation on LINUX

project (CAEN_v1290_Standalone)

# library name
SET( libname ${PROJECT_NAME} )

# include directories
INCLUDE_DIRECTORIES( ./include )

# add library the other includes as directory
AUX_SOURCE_DIRECTORY( ./src library_sources )
ADD_LIBRARY( ${libname} ${library_sources} )

LINK_LIBRARIES( ${libname} )

target_link_libraries (${libname} libCAENComm.so libCAENDigitizer.so libCAENVME.so)

# make the executable main
ADD_EXECUTABLE( test main.cc )
