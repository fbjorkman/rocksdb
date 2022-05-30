//
// Created by fredrik on 2022-05-30.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

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

void workloadOperation(string const &operation, string const &key, string const &value){
  Operation op = convert(operation);
  switch(op){
    case GET:
      cout << "GET " + key << endl;
      break;
    case PUT:
      cout << "PUT " + key << endl;
      break;
    case DELETE:
      cout << "DELETE " + key << endl;
      break;
    case ERROR:
      cout << "ERROR " + key << endl;
      break;
  }
}

int main() {
  fstream input;
  input.open("input_data/gadget.log",ios::in);
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
      workloadOperation(operation, key, value);
    }
    input.close();
  }
  return 0;
}