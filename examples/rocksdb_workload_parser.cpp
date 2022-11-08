//
// Created by fredrik on 2022-05-30.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/merge_operator.h"
#include "string.h"

using namespace std;

enum Operation{GET, PUT, MERGE, DELETE, ERROR};

Operation convert(const string &op){
  if(op == "get") return GET;
  else if (op == "put") return PUT;
  else if (op == "merge") return MERGE;
  else if (op == "delete") return DELETE;
  return ERROR;
}

class Stats{
 private:
  int getok = 0;
  int getnf = 0;
  int putok = 0;
  int putnf = 0;
  int mergeok = 0;
  int mergenf = 0;
  int delok = 0;
  int delnf = 0;
  int emptykey = 0;
  int error = 0;

 public:
  void update(rocksdb::Status &status, const Operation &op){
    switch (op) {
      case GET:
        if(status.ok())
          getok++;
        else
          getnf++;
        break;
      case PUT:
        if(status.ok())
          putok++;
        else
          putnf++;
        break;
      case MERGE:
        if (status.ok())
          mergeok++;
        else
          mergenf++;
      case DELETE:
        if(status.ok())
          delok++;
        else
          delnf++;
        break;
      case ERROR:
        error++;
        break;
    }
  }

  void empty_key(){
    emptykey++;
  }

  void print() const{
    cout << "Operation statistics: " << endl;
    cout << "Get ok: " + to_string(getok)  << endl;
    cout << "Get not found: " + to_string(getnf)  << endl;
    cout << "Put ok: " + to_string(putok)  << endl;
    cout << "Put not found: " + to_string(putnf)  << endl;
    cout << "Merge ok: " + to_string(mergeok) << endl;
    cout << "Merge not found: " + to_string(mergenf) << endl;
    cout << "Delete ok: " + to_string(delok)  << endl;
    cout << "Delete not found: " + to_string(delnf)  << endl;
    cout << "Empty keys: " + to_string(emptykey) << endl;
    cout << "Errors: " + to_string(error) << endl;
  }
};

// A 'model' merge operator where a value is replaced by another value
class MyMergeOperator : public rocksdb::AssociativeMergeOperator {
 public:
  virtual bool Merge(
      const rocksdb::Slice& key,
      const rocksdb::Slice* existing_value,
      const rocksdb::Slice& value,
      std::string* new_value,
      rocksdb::Logger* logger) const override {

    *new_value = value.ToString();
    return true;        // always return true for this, since we treat all errors as "zero".
  }

  virtual const char* Name() const override {
    return "MyMergeOperator";
  }
};

void tokenize(string const &str, const char delim, vector<string> &out){
  out.clear();
  stringstream ss(str);
  string token;
  while (getline(ss, token, delim)) {
    out.push_back(token);
  }
}

void workloadOperation(rocksdb::DB* db, string const &operation, Stats &stats, rocksdb::Slice const &key,
                       string &value, rocksdb::Status &status, vector<string> &deletes, bool omit_deletes,
                       vector<double> &fetch_time, vector<double> &put_delete_time){

  chrono::system_clock::time_point time_start;
  chrono::system_clock::time_point time_end;

  Operation op = convert(operation);
  switch(op){
    case GET:
      time_start = std::chrono::high_resolution_clock::now();
      status = db->Get(rocksdb::ReadOptions(), key, &value);
      time_end = std::chrono::high_resolution_clock::now();
      assert(status.ok() || status.IsNotFound());
      break;
    case PUT:
      time_start = std::chrono::high_resolution_clock::now();
      status = db->Put(rocksdb::WriteOptions(), key, value);
      time_end = std::chrono::high_resolution_clock::now();
      assert(status.ok());
      break;
    case MERGE:
      time_start = std::chrono::high_resolution_clock::now();
      status = db->Merge(rocksdb::WriteOptions(), key, value);
      time_end = std::chrono::high_resolution_clock::now();
      assert(status.ok());
      stats.update(status, op);
      break;
    case DELETE:
      if(omit_deletes) {return;}
      time_start = std::chrono::high_resolution_clock::now();
      status = db->Delete(rocksdb::WriteOptions(), key);
      time_end = std::chrono::high_resolution_clock::now();
      assert(status.ok());
      deletes.emplace_back(key.ToString());
      break;
    case ERROR:
      cout << "ERROR " + key.ToString() << endl;
      return;
  }
  stats.update(status, op);

  if(op == GET){
    std::chrono::duration<double, std::milli> fetch_execution_time = time_end - time_start;
    fetch_time.emplace_back(fetch_execution_time.count());
  } else {
    std::chrono::duration<double, std::milli> put_delete_execution_time =
        time_end - time_start;
    put_delete_time.emplace_back(put_delete_execution_time.count());
  }
}

vector<rocksdb::Slice> generateKeySlices(const vector<string>& keys){
  vector<rocksdb::Slice> key_slices;
  for(const string& key : keys){
    key_slices.emplace_back(key);
  }
  return key_slices;
}

void insert_deletes(rocksdb::DB* db, vector<string> &deletes, rocksdb::Status &status){
  string value = "0000000000";
  int count = 0;
  for(rocksdb::Slice key : deletes){
    status = db->Put(rocksdb::WriteOptions(), key, value);
    assert(status.ok());
    count++;
  }
  cout << to_string(count) + " keys written" << endl;
  db->Flush(rocksdb::FlushOptions());
}

string get_key(unsigned long i){
  string str = to_string(i);
  size_t n = 10;
  unsigned long precision = n - min(n, str.size());
  str.insert(0, precision, '0');
  return  str;
}

int main(int argc, char** argv) {
  int NUMBER_OF_RUNS = 3;
  double total_time = 0;
  bool omit_deletes = true;
  vector<double> rawdata;
  vector<vector<double>> raw_total_fetch_time;
  vector<vector<double>> raw_total_put_delete_time;
  unsigned long keyspace_size = 1000000000;
  unsigned long key_offset = 0;
  string trace_name;

  if(argc >= 2){
    trace_name = argv[1];
  }
  if(argc >= 3){
    key_offset = stoul(argv[2]);
  }
  if(argc == 4){
    if(strcmp(argv[3], "d") == 0){
      omit_deletes = false;
    }
    else{
      cout << "Could not interpret argument: " << argv[3] << endl;
      return -1;
    }
  }
  else if(argc > 4){
    cout << "Too many arguments" << endl;
    return -1;
  }

  for(int i = 0; i < NUMBER_OF_RUNS; i++) {
    rocksdb::DB *db;
    rocksdb::Options options;
    options.create_if_missing = true;
    options.merge_operator.reset(new MyMergeOperator);
    rocksdb::Status status =
        rocksdb::DB::Open(options, "/home/fredrik/testdb-gadget", &db);
    assert(status.ok());

    fstream input;
    string file_path = "input_data/" + trace_name + "-" + to_string(i) + ".log";
    input.open(file_path, ios::in);
    if (input.is_open()) {
      string line;
      string operation;
      string key;
      string hashed_key;
      string value;
      vector<string> workload;
      vector<string> deletes;
      vector<double> fetch_time;
      vector<double> put_delete_time;
      char delim = ' ';
      Stats stats;
      hash<string> key_hash;

      auto start = std::chrono::high_resolution_clock::now();
      while (getline(input, line)) {
        tokenize(line, delim, workload);
        operation = workload.at(1);
        key = workload.at(2);
        if(key.empty()){
          stats.empty_key();
          continue;
        }
        hashed_key = get_key(key_hash(get_key(stoul(key) + key_offset)) % keyspace_size);
        value = workload.at(3);
        workloadOperation(db, operation, stats, hashed_key, value, status,
                          deletes, omit_deletes, fetch_time, put_delete_time);
      }
      auto end = std::chrono::high_resolution_clock::now();
      input.close();

      raw_total_fetch_time.emplace_back(fetch_time);
      raw_total_put_delete_time.emplace_back(put_delete_time);

      std::chrono::duration<double, std::milli> execution_time = end - start;
      rawdata.emplace_back(execution_time.count());
      total_time += execution_time.count();
      cout << "Execution took: " + to_string(execution_time.count()) + " ms" << endl;
      stats.print();
      cout << endl;

      cout << "Writing back deletes" << endl;
      insert_deletes(db, deletes, status);
      cout << "Write complete" << endl << endl;
    }

    delete db;
//    status = rocksdb::DestroyDB("/home/fredrik/testdb-gadget", options);
//    assert(status.ok());
    key_offset = key_offset + 1000000;
  }

  cout << "No buffer threshold | Omit deletes: " + to_string(omit_deletes) +
              " | Avg execution time: " + to_string(total_time/NUMBER_OF_RUNS) << endl << endl;

  ofstream outfile;
  if(omit_deletes) {
    outfile.open("raw_data/" + trace_name + "_no_buffer_no_deletes.txt");
  }
  else{
    outfile.open("raw_data/"  + trace_name + "_no_buffer_with_deletes.txt");
  }
  if(outfile.is_open()) {
    for(const double &datapoint : rawdata){
      outfile << datapoint << " ";
    }
    outfile << endl;
    for(const vector<double> &fetch_data : raw_total_fetch_time){
      for(const double &datapoint : fetch_data){
        outfile << datapoint << " ";
      }
      outfile << endl;
    }
    for(const vector<double> &put_delete_data : raw_total_put_delete_time){
      for(const double &datapoint : put_delete_data){
        outfile << datapoint << " ";
      }
      outfile << endl;
    }
    outfile.close();
  }
  else{
    cout << "Unable to open output file" << endl;
  }

  return 0;
}