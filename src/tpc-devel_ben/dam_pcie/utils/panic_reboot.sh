#!/bin/sh

# Sync the fs
echo s > /proc/sysrq-trigger

# Force reboot
echo b >/proc/sysrq-trigger
