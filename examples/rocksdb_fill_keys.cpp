#include <iostream>
#include <string>
#include <algorithm>

#include "rocksdb/db.h"

std::string getKey(int i){
  std::string str = std::to_string(i);
  size_t n = 10;
  int precision = n - std::min(n, str.size());
  str.insert(0, precision, '0');
  return  str;
}

int main(int argc, char** argv) {
  int keyamount = 1000000000;

  rocksdb::DB* db;
  rocksdb::Options options;
  options.create_if_missing = true;

  rocksdb::Status status =
      rocksdb::DB::Open(options, "/home/fredrik/testdb-gadget", &db);
  assert(status.ok());

  int p = 0;
  for(int i = 0; i < keyamount; i++){
    std::string key = getKey(i);
    std::string value = "0000000000";
    status = db->Put(rocksdb::WriteOptions(), key, value);
    assert(status.ok());
    if(i%(keyamount/100)==0){
      std::cout << "[" + std::to_string(p++) + "%]" << std::endl;
    }
  }
  db->Flush(rocksdb::FlushOptions());

  delete db;
  return 0;
}
