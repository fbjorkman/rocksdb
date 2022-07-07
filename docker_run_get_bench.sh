#!/bin/bash

cd /home/fredrik/rocksdb/examples || exit
make rocksdb_fill_keys
make rocksdb_get_test
make rocksdb_get_test_mixed

./rocksdb_fill_keys
./rocksdb_get_test
./rocksdb_get_test_mixed 0.5 mixed
./rocksdb_get_test_mixed 1.0 common