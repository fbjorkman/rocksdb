//
// Created by fredrik on 2022-05-30.
//

#include <fstream>
#include <iostream>
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

void tokenize(string const &str, const char delim, vector<string> &out){
  out.clear();
  stringstream ss(str);
  string token;
  while (getline(ss, token, delim)) {
    out.push_back(token);
  }
}

void workloadOperation(rocksdb::DB* db, string const &operation, rocksdb::Slice const &key, string &value, rocksdb::Status &status){
  Operation op = convert(operation);
  switch(op){
    case GET:
      status = db->Get(rocksdb::ReadOptions(), key, &value);
      assert(status.ok() || status.IsNotFound());
      break;
    case PUT:
      status = db->Put(rocksdb::WriteOptions(), key, value);
      assert(status.ok() || status.IsNotFound());
      break;
    case DELETE:
      status = db->Delete(rocksdb::WriteOptions(), key);
      assert(status.ok() || status.IsNotFound());
      break;
    case ERROR:
      cout << "ERROR " + key.ToString() << endl;
      break;
  }
}

int main() {
  rocksdb::DB* db;
  rocksdb::Options options;
  options.create_if_missing = true;
  rocksdb::Status status =
      rocksdb::DB::Open(options, "/home/fredrik/testdb-gadget", &db);
  assert(status.ok());

  fstream input;
  input.open("input_data/gadgetzipf1M.log",ios::in);
  if (input.is_open()){
    string line;
    string operation;
    string key;
    string value;
    vector<string> workload;
    char delim = ' ';

    while(getline(input, line)){
      tokenize(line, delim, workload);
      operation = workload.at(1);
      key = workload.at(2);
      value = workload.at(3);
      workloadOperation(db, operation, key, value, status);
    }
    input.close();
  }

  delete db;
  status = rocksdb::DestroyDB("/home/fredrik/testdb-gadget", options);
  assert(status.ok());

  return 0;
}