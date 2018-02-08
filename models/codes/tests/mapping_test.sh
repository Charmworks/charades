#!/bin/bash

tst=.
set -e

syncs="1 2 3"

for s in $syncs
do
  $tst/mapping_test --sync=$s --codes-config=$tst/conf/mapping_test.conf \
      2> mapping_test.err \
      1| grep TEST > mapping_test.out

  diff $tst/expected/mapping_test.out mapping_test.out
  err=$?

  if [ -s mapping_test.err ] ; then
      echo ERROR: see mapping_test.err
      exit 1
  fi

  if [ "$err" -eq 0 ]; then
      echo "mapping_test sync $s PASS!"
      rm mapping_test.out mapping_test.err
  else
      echo "mapping_test sync $s PASS!"
      exit $err
  fi
done
