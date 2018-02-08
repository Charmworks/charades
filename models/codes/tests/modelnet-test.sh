#!/bin/bash

syncs="1 2 3"

for s in $syncs
do
  ./modelnet-test --sync=$s --codes-config=conf/modelnet-test.conf --expected-events=1744 \
      2>modelnet_test.err \
      1>modelnet_test.out
  err=$?
  if [ "$err" -eq 0 ]; then
    echo "modelnet-test sync $s PASS!"
    rm modelnet_test.err modelnet_test.out
  else
    echo "modelnet-test sync $s FAIL!"
    exit $err
  fi
done
