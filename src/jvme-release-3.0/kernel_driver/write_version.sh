#!/bin/bash
GIT_COMMIT=$(git describe --always --dirty --match 'NOT A TAG')

cat << EOF > version.h
#ifndef GIT_COMMIT
#define GIT_COMMIT "${GIT_COMMIT}"
#endif
EOF
