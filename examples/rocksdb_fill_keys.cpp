#include <iostream>

#include "rocksdb/db.h"

std::string getKey(int i){
  return "key" + std::to_string(i);
}

int main(int argc, char** argv) {
  int keyamount = 1000000000;

  rocksdb::DB* db;
  rocksdb::Options options;
  options.create_if_missing = true;

  rocksdb::Status status =
      rocksdb::DB::Open(options, "/home/fredrik/testdb", &db);
  assert(status.ok());

  int p = 0;
  for(int i = 0; i < keyamount; i++){
    std::string key = getKey(i);
    std::string value = "value" + std::to_string(i);
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
