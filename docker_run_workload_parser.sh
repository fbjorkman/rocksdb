#!/bin/bash

cd /home/fredrik/rocksdb/examples || exit
#cd /home/fredrik/Documents/rocksdb/examples || exit
make rocksdb_fill_keys
make rocksdb_workload_parser
make rocksdb_workload_parser_buffer

./rocksdb_fill_keys

./rocksdb_workload_parser tumbling.all.incremental.w5 0
./rocksdb_workload_parser_buffer 1000 tumbling.all.incremental.w5 3000000
./rocksdb_workload_parser_buffer 10000 tumbling.all.incremental.w5 6000000
./rocksdb_workload_parser_buffer 100000 tumbling.all.incremental.w5 9000000

./rocksdb_workload_parser tumbling.all.incremental.w50 12000000
./rocksdb_workload_parser_buffer 1000 tumbling.all.incremental.w50 15000000
./rocksdb_workload_parser_buffer 10000 tumbling.all.incremental.w50 18000000
./rocksdb_workload_parser_buffer 100000 tumbling.all.incremental.w50 21000000

./rocksdb_workload_parser tumbling.all.holistic.w5 24000000
./rocksdb_workload_parser_buffer 1000 tumbling.all.holistic.w5 27000000
./rocksdb_workload_parser_buffer 10000 tumbling.all.holistic.w5 30000000
./rocksdb_workload_parser_buffer 100000 tumbling.all.holistic.w5 33000000

./rocksdb_workload_parser tumbling.all.holistic.w50 36000000
./rocksdb_workload_parser_buffer 1000 tumbling.all.holistic.w50 39000000
./rocksdb_workload_parser_buffer 10000 tumbling.all.holistic.w50 42000000
./rocksdb_workload_parser_buffer 100000 tumbling.all.holistic.w50 45000000

./rocksdb_workload_parser tumbling.keyed.incremental.w5 48000000
./rocksdb_workload_parser_buffer 1000 tumbling.keyed.incremental.w5 51000000
./rocksdb_workload_parser_buffer 10000 tumbling.keyed.incremental.w5 54000000
./rocksdb_workload_parser_buffer 100000 tumbling.keyed.incremental.w5 57000000

./rocksdb_workload_parser tumbling.keyed.incremental.w50 60000000
./rocksdb_workload_parser_buffer 1000 tumbling.keyed.incremental.w50 63000000
./rocksdb_workload_parser_buffer 10000 tumbling.keyed.incremental.w50 66000000
./rocksdb_workload_parser_buffer 100000 tumbling.keyed.incremental.w50 69000000

./rocksdb_workload_parser tumbling.keyed.holistic.w5 72000000
./rocksdb_workload_parser_buffer 1000 tumbling.keyed.holistic.w5 75000000
./rocksdb_workload_parser_buffer 10000 tumbling.keyed.holistic.w5 78000000
./rocksdb_workload_parser_buffer 100000 tumbling.keyed.holistic.w5 81000000

./rocksdb_workload_parser tumbling.keyed.holistic.w50 84000000
./rocksdb_workload_parser_buffer 1000 tumbling.keyed.holistic.w50 87000000
./rocksdb_workload_parser_buffer 10000 tumbling.keyed.holistic.w50 90000000
./rocksdb_workload_parser_buffer 100000 tumbling.keyed.holistic.w50 93000000

./rocksdb_workload_parser sliding.all.incremental.w5 96000000
./rocksdb_workload_parser_buffer 1000 sliding.all.incremental.w5 99000000
./rocksdb_workload_parser_buffer 10000 sliding.all.incremental.w5 102000000
./rocksdb_workload_parser_buffer 100000 sliding.all.incremental.w5 105000000

./rocksdb_workload_parser sliding.all.incremental.w50 108000000
./rocksdb_workload_parser_buffer 1000 sliding.all.incremental.w50 111000000
./rocksdb_workload_parser_buffer 10000 sliding.all.incremental.w50 114000000
./rocksdb_workload_parser_buffer 100000 sliding.all.incremental.w50 117000000

./rocksdb_workload_parser sliding.all.holistic.w5 120000000
./rocksdb_workload_parser_buffer 1000 sliding.all.holistic.w5 123000000
./rocksdb_workload_parser_buffer 10000 sliding.all.holistic.w5 126000000
./rocksdb_workload_parser_buffer 100000 sliding.all.holistic.w5 129000000

./rocksdb_workload_parser sliding.all.holistic.w50 132000000
./rocksdb_workload_parser_buffer 1000 sliding.all.holistic.w50 135000000
./rocksdb_workload_parser_buffer 10000 sliding.all.holistic.w50 138000000
./rocksdb_workload_parser_buffer 100000 sliding.all.holistic.w50 141000000

./rocksdb_workload_parser sliding.keyed.incremental.w5 144000000
./rocksdb_workload_parser_buffer 1000 sliding.keyed.incremental.w5 147000000
./rocksdb_workload_parser_buffer 10000 sliding.keyed.incremental.w5 150000000
./rocksdb_workload_parser_buffer 100000 sliding.keyed.incremental.w5 153000000

./rocksdb_workload_parser sliding.keyed.incremental.w50 156000000
./rocksdb_workload_parser_buffer 1000 sliding.keyed.incremental.w50 159000000
./rocksdb_workload_parser_buffer 10000 sliding.keyed.incremental.w50 162000000
./rocksdb_workload_parser_buffer 100000 sliding.keyed.incremental.w50 165000000

./rocksdb_workload_parser sliding.keyed.holistic.w5 168000000
./rocksdb_workload_parser_buffer 1000 sliding.keyed.holistic.w5 171000000
./rocksdb_workload_parser_buffer 10000 sliding.keyed.holistic.w5 174000000
./rocksdb_workload_parser_buffer 100000 sliding.keyed.holistic.w5 177000000

./rocksdb_workload_parser sliding.keyed.holistic.w50 180000000
./rocksdb_workload_parser_buffer 1000 sliding.keyed.holistic.w50 183000000
./rocksdb_workload_parser_buffer 10000 sliding.keyed.holistic.w50 186000000
./rocksdb_workload_parser_buffer 100000 sliding.keyed.holistic.w50 189000000