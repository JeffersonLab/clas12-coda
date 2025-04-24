# xilinx_docker

## Tested on:

Ubuntu 20.04.2 LTS

Docker version 20.10.7, build f0df350

## Quick Start:

    sudo mkdir -p /tools/Xilinx
    sudo chown -R <username>:<usergroup> /tools/

    make build-docker
    make start-docker-container MOUNTPATH=<absolute file path>
    make log-into-continer

## Programming the fabric from the container
Use the script

    cd /tools/Xilinx/Vivado_Lab/2020.1/data/xicom/cable_drivers/lin64/install_script/install_drivers/

    sudo ./install_drivers

to install the driver USB drivers.  These drivers need to be installed outside of the docker container. "Vivado_Lab" in the path above may need to be replaced with just "Vivado"
