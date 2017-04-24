SET(hgcal_name "HGCalProducer.exe")

SET(sourcefiles src/IpbusHwController.cc src/TriggerController.cc)
#SET(executables src/HGCalProducer.cxx)

find_package(CACTUS REQUIRED)
INCLUDE_DIRECTORIES( include ${UHAL_LOG_INCLUDE_PREFIX} ${UHAL_GRAMMARS_INCLUDE_PREFIX} ${EXTERN_BOOST_INCLUDE_PREFIX} ${EXTERN_PUGIXML_INCLUDE_PREFIX} ${UHAL_UHAL_INCLUDE_PREFIX} )
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${EXTERN_BOOST_LIB_PREFIX} ${EXTERN_PUGIXML_LIB_PREFIX} ${UHAL_LOG_LIB_PREFIX} ${UHAL_GRAMMARS_LIB_PREFIX} ${UHAL_UHAL_LIB_PREFIX} )

ADD_EXECUTABLE(${hgcal_name} src/HGCalProducer.cxx ${sourcefiles})

TARGET_LINK_LIBRARIES(${hgcal_name} EUDAQ ${EUDAQ_THREADS_LIB} boost_timer boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal)

INSTALL(TARGETS  ${hgcal_name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

