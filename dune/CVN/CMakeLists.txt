#if(DEFINED ENV{CAFFE_LIB} )
add_subdirectory(levelDB)
#endif()
# not ready for c2 yet
if(( DEFINED ENV{TENSORFLOW_DIR} ) AND ( DEFINED ENV{TRTIS_CLIENTS_DIR} ))
  add_subdirectory(art)
  add_subdirectory(adcutils)
  add_subdirectory(tf)
endif ()
add_subdirectory(func)
add_subdirectory(fcl)

