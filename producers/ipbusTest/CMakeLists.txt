SET(name "IpbusTestProducer.exe")
SET(sourcefiles src/IpbusTestController.cc)
SET(executables src/IpbusTestProducer.cxx)

find_package(CACTUS REQUIRED)
INCLUDE_DIRECTORIES( include ${UHAL_LOG_INCLUDE_PREFIX} ${UHAL_GRAMMARS_INCLUDE_PREFIX} ${EXTERN_BOOST_INCLUDE_PREFIX} ${EXTERN_PUGIXML_INCLUDE_PREFIX} ${UHAL_UHAL_INCLUDE_PREFIX} )
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${EXTERN_BOOST_LIB_PREFIX} ${EXTERN_PUGIXML_LIB_PREFIX} ${UHAL_LOG_LIB_PREFIX} ${UHAL_GRAMMARS_LIB_PREFIX} ${UHAL_UHAL_LIB_PREFIX} )

ADD_EXECUTABLE(${name} ${executables} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name} EUDAQ ${EUDAQ_THREADS_LIB} boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal)

INSTALL(TARGETS  ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

#SET(name "IpbusTestProducer.exe")
#SET(sourcefiles src/IpbusTestProducer.cxx)


  

# ADD_EXECUTABLE(${name} ${sourcefiles})

# target_link_libraries(${sourcefiles} EUDAQ boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal )

# TARGET_LINK_LIBRARIES(${name} EUDAQ ${EUDAQ_THREADS_LIB}  )

# INSTALL(TARGETS  ${name}
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib)
