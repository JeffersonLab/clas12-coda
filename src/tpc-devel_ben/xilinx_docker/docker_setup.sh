#!/bin/bash
source ./settings.sh

copy_x11_auth () {
    X_COOKIE=$(xauth list ${DISPLAY} | awk '{print $NF}')
    xauth -f ~/.Xauthority add ${HOSTNAME}-docker/unix${DISPLAY} . ${X_COOKIE}
}

docker_run () {
    copy_x11_auth

    docker run -d --shm-size=1024m --name ${CONTAINER_NAME} \
    -e DISPLAY=${DISPLAY} \
    -v ${X11_DOCKER_MOUNT_PATH} \
    -v ${HOME}/.Xauthority:/home/vitisuser/.Xauthority \
    --rm -it \
    --mount type=bind,source="${MOUNTPATH}",target=/home/vitisuser \
    --mount type=bind,source=/tools/Xilinx,target=/tools/Xilinx \
    --hostname ${HOSTNAME}-docker \
    --privileged -v /dev/bus/usb:/dev/bus/usb \
    ${IMAGE_NAME}

    docker exec -it ${CONTAINER_NAME} bash -c 'cd . '
    docker cp .bashrc ${CONTAINER_NAME}:/home/vitisuser
}

docker_login () {
	docker exec -it -e DISPLAY=${DISPLAY} ${CONTAINER_NAME} bash
}

if [[ $DISPLAY == *"localhost"* ]]; then
    DISPLAY_N=$(echo ${DISPLAY} | cut -d. -f1 | cut -d: -f2)
    export DISPLAY=:${DISPLAY_N}
    X11_UNIX_SOCKET_PATH=display/socket
    X11_DOCKER_MOUNT_PATH=${CWD}/${X11_UNIX_SOCKET_PATH}:/tmp/.X11-unix
    if [[ ! -S ${X11_UNIX_SOCKET_PATH}/X${DISPLAY_N} ]]; then
        socat TCP4:localhost:60${DISPLAY_N} UNIX-LISTEN:${X11_UNIX_SOCKET_PATH}/X${DISPLAY_N} &
    fi
fi

if [[ $# -eq 1 ]]; then
    case "$1" in
        "start-docker-container" ) docker_run;;
        "log-into-container" ) docker_login;;
        *) echo >&2 "Invalid argument: $@"; exit 1;;
    esac
else
    echo >&2 "Argument required"; exit 1;
fi

