simple_plugin(RandomChannelStatusService "service"
              larcorealg_Geometry
              ${ROOT_BASIC_LIB_LIST}
              ${ROOT_GEOM}
              ${ART_FRAMEWORK_CORE}
              ${ART_FRAMEWORK_PRINCIPAL}
              ${ART_FRAMEWORK_SERVICES_REGISTRY}
              ${ART_FRAMEWORK_SERVICES_BASIC}
              ${ART_ROOT_IO_TFILE_SUPPORT} ${ROOT_CORE}
              )

install_fhicl()
