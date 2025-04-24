#!/bin/sh
tmux new-session -d 'vim -p src/dam_drv.c src/dam_drv.h'
tmux split-window -v 'sudo dmesg -w'
tmux split-window -v 'sudo su'
tmux split-window -v
tmux -2 attach-session -d
