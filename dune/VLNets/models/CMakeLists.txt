include_directories( $ENV{TENSORFLOW_INC}/eigen )
include_directories( $ENV{TENSORFLOW_INC}/absl )

art_make_library(
    LIBRARY_NAME VLNModels
    SOURCE
        tf_model/ModelConfig.cxx
        tf_model/TFModel.cxx
        zoo/VLNEnergyModel.cxx
    LIBRARIES
        ${TENSORFLOW}
)

install_headers(SUBDIRS tf_model zoo)
install_source(SUBDIRS tf_model zoo)

