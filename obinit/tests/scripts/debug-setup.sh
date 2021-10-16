#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

propagation=$(findmnt -rn -o PROPAGATION /tmp)

if [[ "$propagation" != "private" ]]; then
  echo -e "Please mark /tmp as private and try again:\n"
  echo -e "sudo mount --make-private /tmp\n"
  exit 1
fi

TEST_TMP_DIR=/tmp/obinit-debug

source "$SCRIPT_DIR/helpers.bash"

set +e

test_setupFakeRamfsRoot

cp -v /etc/mtab $TEST_RAMFS_DIR/etc/mtab

if command -v tree &>/dev/null; then
  tree $TEST_TMP_DIR
fi

echo -e "\nDebug tree ready, use:\n"
echo -e "obinit -r /tmp/obinit-debug/ramfs\n"
echo -e "and perform SINGLE debug session.\n\nPess enter to clean up end exit\n"
read -s

test_cleanup

echo "Done"
