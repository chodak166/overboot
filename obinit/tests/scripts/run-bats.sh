#!/bin/bash

# run this script with OBINIT_BIN variable, eg.:
# OBINIT_BIN=../../build/Debug/bin/obinit ./run-bats.sh

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

propagation=$(findmnt -rn -o PROPAGATION /)

if [[ "$propagation" != "private" ]]; then
    echo "Root propagation is $propagation, please make '/' private"
    exit 1
fi

cd ../functional
sudo -E -S bats -f "$1" . < "$SCRIPT_DIR/secret.tmp"

