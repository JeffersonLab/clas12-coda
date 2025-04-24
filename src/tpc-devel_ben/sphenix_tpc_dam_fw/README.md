# sPHENIX DAM PCIe Firmware for TPC

 - Uses `dam_wupper` submodule.
 - Fetch by running `git clone --recurse-submodules https://git.racf.bnl.gov/gitea/instrumentation/sphenix_tpc_dam_fw.git`
 - When switching between branches, `git checkout --recurse-submodules <branch>` should be used.
 
## Build Prerequisites

 - Vivado 2020.2
 - Python3 
 - Vivado et al should be sourced and in your path before invoking `make`

## Building and starting docker environment

create the ~/.config/pip/pip.conf and copy the following code into it

    [global]
    extra-index-url = 
        http://192.168.60.108:8081
        https://pypi.sdcc.bnl.gov/

    trusted-host = 
        192.168.60.108
        https://pypi.sdcc.bnl.gov/


then setup the virtual environment and start the docker container


    git clone git@code.bnl.gov:instrumentation/daq/sphenix_tpc_dam_fw.git
    cd sphenix_tpc_dam_fw

    #create the python virtual environment
    python3 -m venv venv
    source venv/bin/activate
    
    #install the DAQ group's unified docker manager
    python -m pip install bnl-daq-dockpy

    #build the image, start container, and login
    dockpy build
    dockpy start
    dockpy login

## Building

```
# inside the docker container
$ make -j8 ip
$ make all 
```

Should result in a `tpc_dam_top.bit` file being generated.
