#!/bin/bash

syncs="1 2 3"

for s in $syncs
do
  ./modelnet-test --sync=$s --codes-config=conf/modelnet-test-slimfly.conf --expected-events=84510 --buffer-size=8192 \
      2>modelnet_test_slimfly.err \
      1>modelnet_test_slimfly.out
  err=$?
  if [ "$err" -eq 0 ]; then
    echo "modelnet-test-slimfly sync $s PASS!"
    rm modelnet_test_slimfly.err modelnet_test_slimfly.out
  else
    echo "modelnet-test-slimfly sync $s FAIL!"
    exit $err
  fi
done
