#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stack>

std::string DFA[8][6] = {
  {"s3","error","error","error","error","g2"},
  {"error","s4","error","error","a","error"},
  {"r1","r1","s6","r1","r1","error"},
  {"s5","error","error","error","error","error"},
  {"r3","r3","r3","r3","r3","error"},
  {"s3","error","error","error","error","g7"},
  {"error","s4","error","s8","error","error"},
  {"r2","r2","r2","r2","r2","error"}
};



int parse_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Could not open the file!" << std::endl;
    return -1;
  }

  char c;
  std::string word;
  int wordCount = 0;
  std::stack<std::pair<std::string,int>> stack;
  stack.push(std::make_pair("S",1));
  int state = 1; 
  int accept = 0;
  while (file.get(c)) {
    //std::cout<<"state: "<<state<<std::endl;
    if (c == '+'){
      word = "+";
      wordCount++;
      //std::cout<<word<<std::endl;
      
      std::string op = DFA[state-1][1];
      //std::cout<<"op: "<<op<<std::endl;
      if(op == "error"){
        file.close(); 
        return -1;
      }
      else{
        if (op[0] == 's'){
          state = std::stoi(op.substr(1));
          stack.push(std::make_pair(word,state));
        }
        else if (op[0] == 'r'){
          int rule = std::stoi(op.substr(1));
          if(rule == 1){
            stack.pop();
            state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
            stack.push(std::make_pair("E",state));
            file.unget();
          }
          else if(rule == 2){
            stack.pop();
            stack.pop();
            stack.pop();
            stack.pop();
            state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
            stack.push(std::make_pair("E",state));
            file.unget();
          }
          else if(rule == 3){
            stack.pop();
            stack.pop();
            stack.pop();
            state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
            stack.push(std::make_pair("E",state));
            file.unget();
          }
        }
      }
      
    }
    else if (c == '('){
      word = "(";
      wordCount++;
      //std::cout<<word<<std::endl;
      //std::cout<<"op: "<<DFA[state-1][2]<<std::endl;
      std::string op = DFA[state-1][2];
      if(op == "error"){
        file.close(); 
        return -1;
      }
      else{
        if (op[0] == 's'){
          state = std::stoi(op.substr(1));
          stack.push(std::make_pair(word,state));
        }
        else if (op[0] == 'r'){
          int rule = std::stoi(op.substr(1));
          if(rule == 1){
            stack.pop();
            state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
            stack.push(std::make_pair("E",state));
            file.unget();
          }
          else if(rule == 2){
            stack.pop();
            stack.pop();
            stack.pop();
            stack.pop();
            state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
            stack.push(std::make_pair("E",state));
            file.unget();
          }
          else if(rule == 3){
            stack.pop();
            stack.pop();
            stack.pop();
            state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
            stack.push(std::make_pair("E",state));
            file.unget();
          }
        }
      }
    }
    else if (c == ')'){
      word = ")";
      wordCount++;
      //std::cout<<word<<std::endl;
      std::string op = DFA[state-1][3];
      //std::cout<<"op: "<<op<<std::endl;
      if(op == "error"){
        file.close(); 
        return -1;
      }
      else{
        if (op[0] == 's'){
          state = std::stoi(op.substr(1));
          stack.push(std::make_pair(word,state));
        }
        else if (op[0] == 'r'){
          int rule = std::stoi(op.substr(1));
          //std::cout<<"rule:"<<rule<<std::endl;
          if(rule == 1){
            stack.pop();
            state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
            stack.push(std::make_pair("E",state)); 
            file.unget();
          }
          else if(rule == 2){
            stack.pop();
            stack.pop();
            stack.pop();
            stack.pop();
            state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
            stack.push(std::make_pair("E",state));
            file.unget();
          }
          else if(rule == 3){
            stack.pop();
            stack.pop();
            stack.pop();
            state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
            stack.push(std::make_pair("E",state));
            file.unget();
          }
        }
      }
    }
    else if (c == 'i'){
      file.get(c);
      if( c == 'd'){
        wordCount++;
        word = "id";
        //std::cout<<word<<std::endl;
        std::string op = DFA[state-1][0];
        //std::cout<<"op: "<<op<<std::endl;
        if(op == "error"){
          file.close(); 
          return -1;
        }
        else{
          if (op[0] == 's'){
            state = std::stoi(op.substr(1));
            stack.push(std::make_pair(word,state));
          }
          else if (op[0] == 'r'){
            int rule = std::stoi(op.substr(1));
            if(rule == 1){
              stack.pop();
              state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
              stack.push(std::make_pair("E",state));
              file.unget();
            }
            else if(rule == 2){
              stack.pop();
              stack.pop();
              stack.pop();
              stack.pop();
              state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
              stack.push(std::make_pair("E",state));
              file.unget();
            }
            else if(rule == 3){
              stack.pop();
              stack.pop();
              stack.pop();
              state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
              stack.push(std::make_pair("E",state));
              file.unget();
            }
          }
        } 
      }
      else{
        file.close(); 
        return -1;
      }
    }
  }
  
  //std::cout<<"fstate: "<<state<<std::endl;
  std::string op = DFA[state-1][0];
  //std::cout<<"fop: "<<op<<std::endl;
  if (op[0] == 'r'){
    int rule = std::stoi(op.substr(1));
    if(rule == 1){
      stack.pop();
      state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
      stack.push(std::make_pair("E",state));
      file.unget();
    }
    else if(rule == 2){
      stack.pop();
      stack.pop();
      stack.pop();
      stack.pop();
      state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
      stack.push(std::make_pair("E",state));
      file.unget();
    }
    else if(rule == 3){
      stack.pop();
      stack.pop();
      stack.pop();
      state = stoi(DFA[stack.top().second-1][5].substr(1)) ;
      stack.push(std::make_pair("E",state));
      file.unget();
    }
  }
  //std::cout<<"final_state = "<<state<<std::endl;
  if(state != 2) return -1;
  file.close();
  return wordCount;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  std::string filename = argv[1];
  int result = parse_file(filename);

  //std::cout << "The result is: " << result << std::endl;
  if (result == -1) {
    std::cout<<"reject"<<std::endl;
  }
  else{
    std::cout<<"accept"<<std::endl;
  }
  return 0;
}