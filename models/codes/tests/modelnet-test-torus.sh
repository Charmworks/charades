#!/bin/bash

syncs="1 2 3"

for s in $syncs
do
  ./modelnet-test --sync=$s --codes-config=conf/modelnet-test-torus.conf --expected-events=5968 \
      2>modelnet_test_torus.err \
      1>modelnet_test_torus.out
  err=$?
  if [ "$err" -eq 0 ]; then
    echo "modelnet-test-torus sync $s PASS!"
    rm modelnet_test_torus.err modelnet_test_torus.out
  else
    echo "modelnet-test-torus sync $s FAIL!"
    exit $err
  fi
done
