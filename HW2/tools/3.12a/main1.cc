#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stack>
#include <vector>
#include <map>

using namespace std;

struct Action {
  string type;
  int state;
};

map<pair<int, string>, Action> actionTable;
map<pair<int, string>, int> gotoTable;

vector<pair<string, vector<string>>> grammar = {
  {"S", {"E", "$"}},  // S → E $
  {"E", {"id"}},      // E → id
  {"E", {"id", "(", "E", ")"}}, // E → id ( E )
  {"E", {"E", "+", "id"}}       // E → E + id
};


void initParser() {
  actionTable[{0, "id"}] = {"shift", 1};
  actionTable[{0, "E"}] = {"shift", 2};
  actionTable[{1, "("}] = {"shift", 3};
  actionTable[{2, "$"}] = {"accept", -1};
  actionTable[{2, "+"}] = {"shift", 4};
  actionTable[{3, "E"}] = {"shift", 5};
  actionTable[{3, "id"}] = {"shift", 6};
  actionTable[{4, "id"}] = {"shift", 6};
  actionTable[{5, ")"}] = {"shift", 7};
  
  actionTable[{1, "$"}] = {"reduce", 1};
  actionTable[{1, "+", }] = {"reduce", 1};
  actionTable[{6, "$"}] = {"reduce", 1};
  actionTable[{6, "+"}] = {"reduce", 1};
  actionTable[{7, "$"}] = {"reduce", 2};
  actionTable[{7, "+"}] = {"reduce", 2};

  gotoTable[{0, "E"}] = 2;
  gotoTable[{3, "E"}] = 5;
}

bool parse(vector<string> input) {
  stack<int> stateStack;
  stack<string> symbolStack;
  stateStack.push(0);
  int index = 0;

  while (true) {
      int currentState = stateStack.top();
      string currentToken = input[index];

      if (actionTable.find({currentState, currentToken}) == actionTable.end()) {
          cout << "解析失败！错误发生在：" << currentToken << endl;
          return false;
      }

      Action action = actionTable[{currentState, currentToken}];

      if (action.type == "shift") {
          stateStack.push(action.state);
          symbolStack.push(currentToken);
          index++;
      } 
      else if (action.type == "reduce") {
          auto rule = grammar[action.state];
          for (int i = 0; i < rule.second.size(); i++) {
              stateStack.pop();
              symbolStack.pop();
          }
          symbolStack.push(rule.first);
          int gotoState = gotoTable[{stateStack.top(), rule.first}];
          stateStack.push(gotoState);
      } 
      else if (action.type == "accept") {
          cout << "解析成功！" << endl;
          return true;
      }
  }
}

int parse_file(const string &filename) {
  ifstream file(filename);
  if (!file.is_open()) {
      cerr << "Could not open the file!" << endl;
      return -1;
  }

  string word;
  vector<string> tokens;
  while (file >> word) {
      cout << word << " ";
      tokens.push_back(word);
  }
  cout << endl;

  file.close();
  
  tokens.push_back("$");

  if (parse(tokens)) {
      cout << "The input was successfully parsed!" << endl;
      return 0;
  } else {
      cout << "Parsing failed!" << endl;
      return 1;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
      cerr << "Usage: " << argv[0] << " <filename>" << endl;
      return 1;
  }

  string filename = argv[1];
  initParser();
  int result = parse_file(filename);
  cout << "Parsing result: " << result << endl;

  return 0;
}