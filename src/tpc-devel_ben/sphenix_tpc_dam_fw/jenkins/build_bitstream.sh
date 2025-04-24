#!/bin/bash
set -x
chmod 600 ./.ssh/id_rsa
python3 -m venv venv
source venv/bin/activate
python -m pip install bnl-daq-dockpy
dockpy build
dockpy stop
dockpy start
docker exec -t --env-file=docker_env.txt sphenix_tpc_dam_fw_jenkins bash -c "\
make -j8 ip && \
make tpc_dam_top.bit"
