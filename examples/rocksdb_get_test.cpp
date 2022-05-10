//
// Created by fredrik on 2022-04-07.
//

#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"

using namespace std;



double get(rocksdb::DB* db, const vector<rocksdb::Slice>& keys, string &value, rocksdb::Status &status){
  auto start = std::chrono::high_resolution_clock::now();
  for (rocksdb::Slice key : keys) {
    status = db->Get(rocksdb::ReadOptions(), key, &value);
    assert(status.ok());
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> float_ms = end - start;
  return float_ms.count();
}

double multiGet(rocksdb::DB* db, vector<rocksdb::Slice> &keys, vector<rocksdb::PinnableSlice> &values, vector<rocksdb::Status> &statuses){
  auto start = std::chrono::high_resolution_clock::now();
  db->MultiGet(rocksdb::ReadOptions(), db->DefaultColumnFamily(),
               keys.size(), keys.data(), values.data(),
               statuses.data());
  for (const rocksdb::Status& status_check : statuses) {
    assert(status_check.ok());
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> float_ms = end - start;
  return float_ms.count();
}

string getKey(int i){
  return "key" + to_string(i);
}

vector<rocksdb::Slice> generateKeySlices(const vector<string>& keys){
  vector<rocksdb::Slice> keySlices;
  for(const string& key : keys){
    keySlices.emplace_back(key);
  }
  return keySlices;
}

vector<string> generateTestKeys(int totkeys){
  vector<string> testKeys;
  testKeys.emplace_back(getKey(0));
  testKeys.emplace_back(getKey(totkeys-1));
  testKeys.emplace_back(getKey(5000));
  testKeys.emplace_back(getKey(1000));
  testKeys.emplace_back(getKey(totkeys/2-1));
  testKeys.emplace_back(getKey(7000));
  return testKeys;
}

vector<string> generateRandomTestKeys(int amount, int totkeys){
  unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
  default_random_engine randomEngine(seed);
  vector<string> testKeys;
  for(int i = 0; i < amount; i++){
    testKeys.emplace_back(getKey(randomEngine()%totkeys));
  }
  return testKeys;
}

void evictCacheValues(rocksdb::DB* db, int numOfKeys, rocksdb::Status &status){
  string value;
  for (int i = 0; i < numOfKeys; i++){
    string key = getKey(i);
    status = db->Get(rocksdb::ReadOptions(), key, &value);
    assert(status.ok());
  }
}

void dbFillKeys(rocksdb::DB* db, int keyamount, rocksdb::Status &status){
  int p = 0;
  for(int i = 0; i < keyamount; i++){
    string key = getKey(i);
    string value = "value" + to_string(i);
    status = db->Put(rocksdb::WriteOptions(), key, value);
    assert(status.ok());
    if(i%(keyamount/100)==0){
      cout << "[" + to_string(p++) + "%]" << endl;
    }
  }
  db->Flush(rocksdb::FlushOptions());
}

int main(int argc, char** argv) {
  const int NUM_OF_KEYS[] = {5, 10, 50, 100, 500, 1000};
  const int NUM_OF_RUNS = 100;
  const int TOTAL_KEYS = 1000000000;

  rocksdb::DB* db;
  rocksdb::Options options;
  options.create_if_missing = true;

  rocksdb::Status status =
      rocksdb::DB::Open(options, "/home/fredrik/testdb", &db);
  assert(status.ok());

  vector<vector<rocksdb::Slice>> list_slice_keys;
  vector<rocksdb::PinnableSlice> values;
  vector<rocksdb::Status> statuses;
  vector<vector<double>> getRawData;
  vector<vector<double>> multiGetRawData;

  string value;
  ofstream outfile;

  // Flush the data from the memtable to disk
  db->Flush(rocksdb::FlushOptions());

  // Warmup
  cout << "Starting warmup" << endl;
  values.resize(5);
  statuses.resize(5);
  for(int i = 0; i < 100; i++){
    vector<string> getKeys = generateRandomTestKeys(5, TOTAL_KEYS);
    vector<string> mulKeys = generateRandomTestKeys(5, TOTAL_KEYS);
    vector<rocksdb::Slice> getKeySlices = generateKeySlices(getKeys);
    vector<rocksdb::Slice> mulKeySlices = generateKeySlices(mulKeys);
    get(db, getKeySlices, value, status);
    multiGet(db, mulKeySlices, values, statuses);
  }
  cout << "Warmup finished" << endl;

  for (int numFetches : NUM_OF_KEYS) {
    double totGetTime = 0;
    double totMulTime = 0;
    vector<double> rawGetRow;
    vector<double> rawMultiGetRow;
    values.resize(numFetches);
    statuses.resize(numFetches);

    for (int i = 0; i < NUM_OF_RUNS; i++) {
      vector<string> getKeys = generateRandomTestKeys(numFetches, TOTAL_KEYS);
      vector<string> mulKeys = generateRandomTestKeys(numFetches, TOTAL_KEYS);
      vector<rocksdb::Slice> getKeySlices = generateKeySlices(getKeys);
      vector<rocksdb::Slice> mulKeySlices = generateKeySlices(mulKeys);
      double getTime = get(db, getKeySlices, value, status);
      double mulTime = multiGet(db, mulKeySlices, values, statuses);
      totGetTime += getTime;
      totMulTime += mulTime;
      rawGetRow.emplace_back(getTime);
      rawMultiGetRow.emplace_back(mulTime);
    }
    getRawData.emplace_back(rawGetRow);
    multiGetRawData.emplace_back(rawMultiGetRow);

    cout << "Avg get: " + to_string(totGetTime/NUM_OF_RUNS) << endl;
    cout << "Avg mul: " + to_string(totMulTime/NUM_OF_RUNS) << endl;
    cout << endl;
  }

  outfile.open("get_rawdata_random.txt");
  if(outfile.is_open()) {
    for(const vector<double>& getRow : getRawData){
      for(double rawGetData : getRow){
        outfile << rawGetData << " ";
      }
      outfile << endl;
    }
    outfile.close();
  }
  else{
    cout << "Unable to open file" << endl;
  }

  outfile.open("multiget_rawdata_random.txt");
  if(outfile.is_open()) {
    for(const vector<double>& multiGetRow : multiGetRawData){
      for(double rawMultiGetData : multiGetRow){
        outfile << rawMultiGetData << " ";
      }
      outfile << endl;
    }
    outfile.close();
  }
  else{
    cout << "Unable to open file" << endl;
  }

  delete db;

  return 0;
}