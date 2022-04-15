//
// Created by fredrik on 2022-04-07.
//

#include <cassert>
#include <string>
#include <iostream>
#include <fstream>
#include "rocksdb/db.h"
#include "rocksdb/options.h"

using namespace std;

int main(int argc, char** argv) {
  const int NUM_OF_KEYS[] = {5, 10, 100, 1000, 10000};
  const int NUM_OF_RUNS = 1000;
  const int NUM_OF_KEY_AMOUNT = std::extent<decltype(NUM_OF_KEYS)>::value;
  const int TOTAL_KEYS = NUM_OF_KEYS[NUM_OF_KEY_AMOUNT-1];

  rocksdb::DB* db;
  rocksdb::Options options;
  options.create_if_missing = true;
  rocksdb::Status status =
      rocksdb::DB::Open(options, "/tmp/testdb", &db);
  assert(status.ok());

  string keys[TOTAL_KEYS];
  vector<vector<rocksdb::Slice>> list_slice_keys;
  vector<rocksdb::PinnableSlice> values;
  vector<rocksdb::Status> statuses;
  double raw_get_data[NUM_OF_KEY_AMOUNT][NUM_OF_RUNS];
  double raw_multiget_data[NUM_OF_KEY_AMOUNT][NUM_OF_RUNS];

  ofstream outfile;

  // Insert the right amount of elements
  for (int i = 0; i < TOTAL_KEYS; i++) {
    string key = "key" + to_string(i);
    string value = "value" + to_string(i);
    status = db->Put(rocksdb::WriteOptions(), key, value);
    keys[i] = key;
    assert(status.ok());
  }

  for (int i : NUM_OF_KEYS) {
    vector<rocksdb::Slice> slice_keys;
    for(int j = 0; j < i; j++) {
      slice_keys.emplace_back(keys[j]);
    }
    list_slice_keys.emplace_back(slice_keys);
  }

  // Flush the data from the memtable to disk
  db->Flush(rocksdb::FlushOptions());

  // Read back values
  // For each instance of the NUM_OF_KEYS array, run NUM_OF_RUNS iterations
  // where for each run the specified number of keys (the instance of the NUM_OF_KEYS array)
  // will be fetched by Get()-calls in a loop and a MultiGet() with the same keys.

  string value;
  double time1;
  double time2;

  for (int i = 0; i < NUM_OF_KEY_AMOUNT; i++) {
    time1 = 0;
    time2 = 0;
    values.resize(list_slice_keys[i].size());
    statuses.resize(list_slice_keys[i].size());
    double raw_get_data_row[NUM_OF_RUNS];
    double raw_multiget_data_row[NUM_OF_RUNS];

    for (int j = 0; j < NUM_OF_RUNS; j++) {
      auto start1 = std::chrono::high_resolution_clock::now();
      for (int key_index = 0; key_index < NUM_OF_KEYS[i]; key_index++) {
        status = db->Get(rocksdb::ReadOptions(), keys[key_index], &value);
        assert(status.ok());
      }
      auto end1 = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::milli> float_ms1 = end1 - start1;
      time1 += float_ms1.count();
      raw_get_data_row[j] = float_ms1.count();

      auto start2 = std::chrono::high_resolution_clock::now();
      db->MultiGet(rocksdb::ReadOptions(), db->DefaultColumnFamily(),
                   list_slice_keys[i].size(), list_slice_keys[i].data(), values.data(),
                   statuses.data());
      for (const rocksdb::Status& status_check : statuses) {
        assert(status_check.ok());
      }
      auto end2 = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::milli> float_ms2 = end2 - start2;
      time2 += float_ms2.count();
      raw_multiget_data_row[j] = float_ms2.count();
    }

    // Not the nicest solution to copy the array, but it was an easy solution
    copy(begin(raw_get_data_row), end(raw_get_data_row), begin(raw_get_data[i]));
    copy(begin(raw_multiget_data_row), end(raw_multiget_data_row), begin(raw_multiget_data[i]));

    cout << "The Get() read with " + to_string(NUM_OF_KEYS[i]) +" took on average "
              << time1 / NUM_OF_RUNS << " milliseconds" << endl;
    cout << "The MultiGet() read with " + to_string(NUM_OF_KEYS[i]) + " took on average "
              << time2 / NUM_OF_RUNS << " milliseconds" << endl;
  }

  outfile.open("get_vs_multiget_rawdata.txt");
  if(outfile.is_open()) {
    for (int i = 0; i < NUM_OF_KEY_AMOUNT; i++) {
      for (int j = 0; j < NUM_OF_RUNS-1; j++) {
        outfile << raw_get_data[i][j] << ",";
      }
      outfile << raw_get_data[i][NUM_OF_RUNS-1] << endl;

      for (int j = 0; j < NUM_OF_RUNS-1; j++) {
        outfile << raw_multiget_data[i][j] << ",";
      }
      outfile << raw_multiget_data[i][NUM_OF_RUNS-1] << endl;
    }
    outfile.close();
  }
  else{
    cout << "Unable to open file" << endl;
  }

  delete db;

  return 0;
}