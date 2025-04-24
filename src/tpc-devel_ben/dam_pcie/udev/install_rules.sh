#!/bin/bash
cp -v ./*.rules /etc/udev/rules.d/.
udevadm control --reload-rules
