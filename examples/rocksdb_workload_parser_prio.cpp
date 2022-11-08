//
// Created by fredrik on 2022-05-30.
//

#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <vector>

#include "rocksdb/db.h"

using namespace std;

enum Operation{GET, PUT, DELETE, ERROR};

Operation convert(const string &op){
  if(op == "get") return GET;
  else if (op == "put") return PUT;
  else if (op == "delete") return DELETE;
  return ERROR;
}

class Stats {
 private:
  int getok = 0;
  int getnf = 0;
  int putok = 0;
  int putnf = 0;
  int delok = 0;
  int delnf = 0;
  int emptykey = 0;
  int error = 0;

 public:
  void update(rocksdb::Status &status, const Operation &op) {
    switch (op) {
      case GET:
        if (status.ok())
          getok++;
        else
          getnf++;
        break;
      case PUT:
        if (status.ok())
          putok++;
        else
          putnf++;
        break;
      case DELETE:
        if (status.ok())
          delok++;
        else
          delnf++;
        break;
      case ERROR:
        error++;
        break;
    }
  }

  void empty_key() { emptykey++; }

  void print() const {
    cout << "Operation statistics: " << endl;
    cout << "Get ok: " + to_string(getok) << endl;
    cout << "Get not found: " + to_string(getnf) << endl;
    cout << "Put ok: " + to_string(putok) << endl;
    cout << "Put not found: " + to_string(putnf) << endl;
    cout << "Delete ok: " + to_string(delok) << endl;
    cout << "Delete not found: " + to_string(delnf) << endl;
    cout << "Empty keys: " + to_string(emptykey) << endl;
    cout << "Errors: " + to_string(error) << endl;
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
vector<rocksdb::Slice> generateKeySlices(const vector<string>& keys){
  vector<rocksdb::Slice> key_slices;
  for(const string& key : keys){
    key_slices.emplace_back(key);
  }
  return key_slices;
}

void workloadOperation(rocksdb::DB* db, string const &operation, Stats &stats,
                       rocksdb::Slice const &key, string &value,
                       rocksdb::Status &status, priority_queue<string> &prio_queue,
                       vector<string> &deletes, bool omit_deletes){

  Operation op = convert(operation);
  switch(op){
    case GET:
      prio_queue.emplace(key.ToString());
      break;
    case PUT:
      status = db->Put(rocksdb::WriteOptions(), key, value);
      assert(status.ok());
      stats.update(status, op);
      break;
    case DELETE:
      if(omit_deletes) {return;}
      status = db->Delete(rocksdb::WriteOptions(), key);
      assert(status.ok());
      stats.update(status, op);
      deletes.emplace_back(key.ToString());
      break;
    case ERROR:
      cout << "ERROR " + key.ToString() << endl;
      stats.update(status, op);
      break;
  }
}

void convert_queue(priority_queue<string> &prio_queue, vector<string> &get_buffer){
  while(!prio_queue.empty()){
    get_buffer.emplace_back(prio_queue.top());
    prio_queue.pop();
  }
}

void fetch_get_buffer(rocksdb::DB *db, Stats &stats, vector<string> &get_buffer){
  vector<rocksdb::Slice> buffer_key_slices = generateKeySlices(get_buffer);
  vector<rocksdb::PinnableSlice> values(buffer_key_slices.size());
  vector<rocksdb::Status> statuses(buffer_key_slices.size());
  db->MultiGet(rocksdb::ReadOptions(), db->DefaultColumnFamily(),
               buffer_key_slices.size(), buffer_key_slices.data(), values.data(),
               statuses.data());
  for (rocksdb::Status &status_check : statuses) {
    assert(status_check.ok() || status_check.IsNotFound());
    stats.update(status_check, Operation::GET);
  }
  get_buffer.clear();
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


int main(int argc, char** argv) {
  int NUMBER_OF_RUNS = 10;
  double total_execution_time = 0;
  double total_fetch_time = 0;
  ulong buffer_threshold = 10000;
  bool omit_deletes = true;
  vector<double> rawdata;
  vector<vector<double>> raw_total_fetch_time;

  if(argc == 2){
    buffer_threshold = stoul(argv[1]);
  }
  else if(argc == 3){
    buffer_threshold = stoul(argv[1]);
    if(strcmp(argv[2], "d") == 0){
      omit_deletes = false;
    }
    else{
      cout << "Could not interpret argument: " << argv[2] << endl;
      return -1;
    }
  }
  else if(argc > 3){
    cout << "Too many arguments" << endl;
    return -1;
  }

  for(int i = 0; i < NUMBER_OF_RUNS; i++) {
    rocksdb::DB *db;
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status =
        rocksdb::DB::Open(options, "/home/fredrik/testdb-gadget", &db);
    assert(status.ok());

    fstream input;
    string file_path = "input_data/gadgetzipf1M-" + to_string(i) + ".log";
    input.open(file_path, ios::in);
    if (input.is_open()) {
      string line;
      string operation;
      string key;
      string value;
      vector<string> workload;
      vector<string> deletes;
      vector<string> get_buffer;
      priority_queue<string> prio_queue;
      vector<double> fetch_time;
      char delim = ' ';
      Stats stats;
      ulong window_counter = 0;

      auto start = std::chrono::high_resolution_clock::now();
      while (getline(input, line)) {
        tokenize(line, delim, workload);
        operation = workload.at(1);
        key = workload.at(2);
        value = workload.at(3);
        if(key.empty()){
          stats.empty_key();
          continue;
        }
        workloadOperation(db, operation, stats, key, value, status,
                          prio_queue,deletes, omit_deletes);
        if(++window_counter == buffer_threshold){
          //          cout << "Size of get buffer: " + to_string(get_buffer.size()) << endl;
          convert_queue(prio_queue, get_buffer);
          auto fetch_start = std::chrono::high_resolution_clock::now();
          fetch_get_buffer(db, stats, get_buffer);
          auto fetch_end = std::chrono::high_resolution_clock::now();
          std::chrono::duration<double, std::milli> fetch_execution_time = fetch_end - fetch_start;
          fetch_time.emplace_back(fetch_execution_time.count());
          window_counter = 0;
        }
      }
      if(!prio_queue.empty()){
        //        cout << "Size of get buffer: " + to_string(get_buffer.size()) << endl;
        convert_queue(prio_queue, get_buffer);
        fetch_get_buffer(db, stats, get_buffer);
      }
      auto end = std::chrono::high_resolution_clock::now();
      input.close();

      double total = 0;
      for(double time : fetch_time){
        total += time;
      }
      total_fetch_time += total;
      raw_total_fetch_time.emplace_back(fetch_time);

      std::chrono::duration<double, std::milli> execution_time = end - start;
      rawdata.emplace_back(execution_time.count());
      total_execution_time += execution_time.count();
      cout << "Execution took: " + to_string(execution_time.count()) + " ms" << endl;
      cout << "Total fetch time: " + to_string(total) + " ms" << endl;

      stats.print();
      cout << endl;

      cout << "Writing back deletes" << endl;
      insert_deletes(db, deletes, status);
      cout << "Write complete" << endl << endl;

    }

    delete db;
    //    status = rocksdb::DestroyDB("/home/fredrik/testdb-gadget", options);
    //    assert(status.ok());
  }

  cout << "Buffer threshold: " + to_string(buffer_threshold) +
              " | Omit deletes: " + to_string(omit_deletes) +
              " | Avg execution time: " + to_string(total_execution_time/NUMBER_OF_RUNS) +
              " | Avg fetch time: " + to_string(total_fetch_time/NUMBER_OF_RUNS) << endl << endl;

  ofstream outfile;
  if(omit_deletes) {
    outfile.open("raw_data/gadget_1M_workload_prio" + to_string(buffer_threshold) + "_no_deletes.txt");
  }
  else{
    outfile.open("raw_data/gadget_1M_workload_prio" + to_string(buffer_threshold) + "_with_deletes.txt");
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
    outfile.close();
  }
  else{
    cout << "Unable to open file" << endl;
  }

  return 0;
}