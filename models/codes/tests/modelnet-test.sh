#!/bin/bash

syncs="1 2 3"
configs="modelnet-test modelnet-test-loggp modelnet-test-simplep2p modelnet-test-torus modelnet-test-dragonfly modelnet-test-slimfly"

declare -A expected
expected[modelnet-test]=1744
expected[modelnet-test-loggp]=528
expected[modelnet-test-simplep2p]=131
expected[modelnet-test-torus]=5968
expected[modelnet-test-dragonfly]=45757
expected[modelnet-test-slimfly]=84510

for c in $configs
do 
  for s in $syncs
  do
    ./modelnet-test --sync=$s --codes-config=conf/$c.conf --buffer-size=8192 --expected-events=${expected[$c]} \
        2>$c.err \
        1>$c.out
    err=$?
    if [ "$err" -eq 0 ]; then
      echo "$c with sync $s PASS!"
      rm $c.err $c.out
    else
      echo "$c with sync $s FAIL!"
      exit $err
    fi
  done
done
