simple_plugin(IcebergTpcMonitor "module"
              larcorealg_Geometry
              larcore_Geometry_Geometry_service
              lardataalg_DetectorInfo
              lardataobj_RawData
              nusimdata_SimulationBase
              dune-raw-data_Services_ChannelMap_IcebergChannelMapService_service
              ${ART_FRAMEWORK_CORE}
              ${ART_FRAMEWORK_PRINCIPAL}
              ${ART_FRAMEWORK_SERVICES_REGISTRY}
              ${ART_ROOT_IO_TFILE_SUPPORT} ${ROOT_CORE}
              ${ART_ROOT_IO_TFILESERVICE_SERVICE}
              ${ART_PERSISTENCY_PROVENANCE}
              ${MF_MESSAGELOGGER}
              ${CETLIB}
              ${ROOT_BASIC_LIB_LIST}
              BASENAME_ONLY)

install_headers()
install_fhicl()
install_source()
install_scripts()
