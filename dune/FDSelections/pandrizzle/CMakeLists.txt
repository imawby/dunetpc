art_make( LIBRARY_NAME PandrizzleAlg
	  LIB_LIBRARIES larcorealg_Geometry
	  	        larcore_Geometry_Geometry_service
		          larsim_Simulation nug4_ParticleNavigation lardataobj_Simulation
 	  	       larsim_MCCheater_BackTrackerService_service
         	  larsim_MCCheater_ParticleInventoryService_service
	  	        lardata_Utilities
	  	        larevt_Filters
	  	        lardataobj_RawData
	  	        lardataobj_RecoBase
	  	        lardata_RecoObjects 
#larreco_Deprecated
	  	        larreco_RecoAlg
         			larreco_Calorimetry
	  	        larpandora_LArPandoraInterface
	  	        SignalShapingServiceDUNE10kt_service
	  	        SignalShapingServiceDUNE35t_service
          	        nusimdata_SimulationBase
          	        ${LARRECO_LIB}
          	        ${LARDATA_LIB}
	  	        ${MF_MESSAGELOGGER}
	  	        ${MF_UTILITIES}
	  	        ${FHICLCPP}
	  	        cetlib cetlib_except
          	        ${ROOT_GEOM}
          	        ${ROOT_XMLIO}
          	        ${ROOT_GDML}
	  	        ${ROOT_BASIC_LIB_LIST}
            ${ROOT_TMVA}
            dune_TrackPID_tf
            larsim_Utils
            NeutrinoEnergyRecoAlg

MODULE_LIBRARIES PandrizzleAlg
#dune_TrackPID
dune_TrackPID_algorithms
dune_TrackPID_products
          	           ${ART_FRAMEWORK_CORE}
	  	           ${ART_FRAMEWORK_PRINCIPAL}
		       	   ${ART_FRAMEWORK_SERVICES_REGISTRY}
	  	       	   ${ART_FRAMEWORK_SERVICES_OPTIONAL}
			   ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
	  	           art_Persistency_Common canvas
	  	           art_Persistency_Provenance canvas
	  	           art_Utilities canvas
        )

install_headers()
install_fhicl()
install_source()
install_scripts()
