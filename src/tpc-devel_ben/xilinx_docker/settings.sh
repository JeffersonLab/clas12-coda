MOUNTPATH=$(pwd)
CWD=$(pwd)
HOSTNAME=$(hostname)
IMAGE_NAME=ubuntu:bionic_vitis
CONTAINER_NAME=bionic_vitis
X11_UNIX_SOCKET_PATH=/tmp/.X11-unix
X11_DOCKER_MOUNT_PATH=${X11_UNIX_SOCKET_PATH}:/tmp/.X11-unix
