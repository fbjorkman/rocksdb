//
// Created by fredrik on 2022-05-16.
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

int getKeyIndexUniform(int setSize){
  unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
  default_random_engine randomEngine(seed);
  return randomEngine() % setSize;
}

bool keyIsCommon(double prob){
  unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
  default_random_engine randomEngine(seed);
  uniform_real_distribution<> uniform_zero_to_one(0.0, 1.0);
  return uniform_zero_to_one(randomEngine) <= prob;
}

void shuffleKeys(vector<string> &keys){
  auto rng = default_random_engine {};
  shuffle(begin(keys), end(keys), rng);
}

vector<string> selectCommonKeys(vector<string> &commonKeys, int numKeys){
  shuffleKeys(commonKeys);
  auto first = commonKeys.begin();
  auto last = commonKeys.begin() + numKeys;
  vector<string> selectedCommonKeys(first, last);
  return selectedCommonKeys;
}

vector<rocksdb::Slice> generateKeySlices(const vector<string>& keys){
  vector<rocksdb::Slice> keySlices;
  for(const string& key : keys){
    keySlices.emplace_back(key);
  }
  return keySlices;
}

vector<string> generateRandomTestKeys(int numOfKeysInSet, int totkeys){
  unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
  default_random_engine randomEngine(seed);
  vector<string> testKeys;
  for(int i = 0; i < numOfKeysInSet; i++){
    testKeys.emplace_back(getKey(randomEngine()%totkeys));
  }
  return testKeys;
}

vector<string> generateTestKeys(int amount, vector<string> &commonKeys, int totalKeys, double prob){
  vector<string> testKeys;
  for(int i = 0; i < amount; i++){
    if(keyIsCommon(prob)){
      testKeys.emplace_back(commonKeys.at(getKeyIndexUniform(commonKeys.size())));
    } else{
      testKeys.emplace_back(getKey(getKeyIndexUniform(totalKeys)));
    }
  }
  return testKeys;
}

vector<string> generateTestKeysNoMultiples(int amount, vector<string> &commonKeys, int totalKeys, double prob){
  vector<string> getKeys;
  vector<string> mulKeys;
  vector<string> testKeys;
  int commonKeyCount = 0;
  int randomKeyCount = 0;
  for(int i = 0; i < amount; i++){
    if(keyIsCommon(prob)){
      commonKeyCount++;
    } else{
      randomKeyCount++;
    }
  }
  vector<string> randomKeysSelection = generateRandomTestKeys(randomKeyCount*2, totalKeys);
  vector<string> commonKeysSelection = selectCommonKeys(commonKeys, commonKeyCount*2);

  getKeys.reserve(randomKeyCount + commonKeyCount);
  getKeys.insert(getKeys.end(), randomKeysSelection.begin(), randomKeysSelection.begin() + randomKeyCount);
  getKeys.insert(getKeys.end(), commonKeysSelection.begin(), commonKeysSelection.begin() + commonKeyCount);

  mulKeys.reserve(randomKeyCount + commonKeyCount);
  mulKeys.insert(mulKeys.end(), randomKeysSelection.begin() + randomKeyCount, randomKeysSelection.end());
  mulKeys.insert(mulKeys.end(), commonKeysSelection.begin() + commonKeyCount, commonKeysSelection.end());

  shuffleKeys(getKeys);
  shuffleKeys(mulKeys);

  testKeys.reserve(getKeys.size() + mulKeys.size());
  testKeys.insert(testKeys.end(), getKeys.begin(), getKeys.end());
  testKeys.insert(testKeys.end(), mulKeys.begin(), mulKeys.end());

  return testKeys;
}

void doWarmup(rocksdb::DB* db, string &value, rocksdb::Status &status,
              vector<rocksdb::PinnableSlice> &values, vector<rocksdb::Status> &statuses,
              double prob, int totKeys, vector<string> &commonKeys,
              int numFetches){

//  for(int i = 0; i < 100; i++){
//    vector<string> getKeys = generateTestKeys(numFetches, commonKeys, totKeys, prob);
//    vector<string> mulKeys = generateTestKeys(numFetches, commonKeys, totKeys, prob);
//    vector<rocksdb::Slice> getKeySlices = generateKeySlices(getKeys);
//    vector<rocksdb::Slice> mulKeySlices = generateKeySlices(mulKeys);
//    get(db, getKeySlices, value, status);
//    multiGet(db, mulKeySlices, values, statuses);
//  }
  for(string &key : commonKeys){
    status = db->Get(rocksdb::ReadOptions(), key, &value);
    assert(status.ok());
  }
  vector<string> testKeys = generateTestKeysNoMultiples(numFetches, commonKeys, totKeys, prob);
  vector<string> getKeys(testKeys.begin(), testKeys.begin() + testKeys.size()/2);
  vector<string> mulKeys(testKeys.begin() + testKeys.size()/2, testKeys.end());
  vector<rocksdb::Slice> getKeySlices = generateKeySlices(getKeys);
  vector<rocksdb::Slice> mulKeySlices = generateKeySlices(mulKeys);

  get(db, getKeySlices, value, status);
  multiGet(db, mulKeySlices, values, statuses);
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
  const int NUM_OF_KEYS[] = {10, 100, 1000, 10000, 100000};
  const int NUM_OF_RUNS = 10;
  const int TOTAL_KEYS = 1000000000;
  double COMMON_KEY_PROB;
  string file_name;
  if(argc >= 2){
    COMMON_KEY_PROB = stod(argv[1]);
    file_name = argv[2];
  } else{
    COMMON_KEY_PROB = 0.5;
    file_name = "mixed";
  }

  rocksdb::DB* db;
  rocksdb::Options options;
  options.create_if_missing = true;
  options.statistics = rocksdb::CreateDBStatistics();

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

  for (int numFetches : NUM_OF_KEYS) {
    double totGetTime = 0;
    double totMulTime = 0;
    vector<double> rawGetRow;
    vector<double> rawMultiGetRow;
    values.resize(numFetches);
    statuses.resize(numFetches);
    vector<string> commonKeys = generateRandomTestKeys(numFetches*10, TOTAL_KEYS);
//     Warmup
    cout << "Starting warmup" << endl;
    doWarmup(db, value, status, values, statuses, COMMON_KEY_PROB,
             TOTAL_KEYS, commonKeys, numFetches);
    cout << "Warmup finished" << endl;

//    outfile.open("1000keyStats.txt", std::ios_base::app);
//    outfile << "After Warmup" << endl;
//    outfile << options.statistics->ToString() << endl;

    for (int i = 0; i < NUM_OF_RUNS; i++) {
      vector<string> testKeys = generateTestKeysNoMultiples(numFetches, commonKeys, TOTAL_KEYS, COMMON_KEY_PROB);
      vector<string> getKeys(testKeys.begin(), testKeys.begin() + testKeys.size()/2);
      vector<string> mulKeys(testKeys.begin() + testKeys.size()/2, testKeys.end());

      vector<rocksdb::Slice> getKeySlices = generateKeySlices(getKeys);
      vector<rocksdb::Slice> mulKeySlices = generateKeySlices(mulKeys);
      double mulTime = multiGet(db, mulKeySlices, values, statuses);
      double getTime = get(db, getKeySlices, value, status);
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
//  db->GetProperty("rocksdb.block-cache-capacity", &value);
//  cout << value << endl;
//  cout << options.statistics->ToString() << endl;
//  outfile << "After Run" << endl;
//  outfile << options.statistics->ToString() << endl;
//  outfile.close();

  outfile.open("raw_data/get_rawdata_" + file_name + ".txt");
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

  outfile.open("raw_data/multiget_rawdata_" + file_name + ".txt");
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
