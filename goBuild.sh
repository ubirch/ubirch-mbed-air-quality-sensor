#!/bin/bash -x

MBED_CONTAINER_VERSION="latest"



function init() {

  DEPENDENCY_LABEL=$GO_DEPENDENCY_LABEL_MBED_CONTAINER


  if [ -z ${DEPENDENCY_LABEL} ]; then
    MBED_CONTAINER_VERSION="latest"
  else
    MBED_CONTAINER_VERSION="v${DEPENDENCY_LABEL}"
  fi

}

function build_software() {

  	docker run --user `id -u`:`id -g`  --volume=${PWD}:/opt ubirch/ubirch-mbed-build:${MBED_CONTAINER_VERSION} /opt/build.sh


  if [ $? -ne 0 ]; then
	    echo "Docker build failed"
      exit 1
  fi
}



case "$1" in
    build)
        init
        build_software
        ;;
    *)
        echo "Usage: $0 {build}"
        exit 1
esac

exit 0
