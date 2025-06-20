#include <fstream>
#include <iostream>
#include <stack>
#include <string>

const std::string DFA[8][6] = {
    {"s3", "error", "error", "error", "error", "g2"},
    {"error", "s4", "error", "error", "a", "error"},
    {"r1", "r1", "s6", "r1", "r1", "error"},
    {"s5", "error", "error", "error", "error", "error"},
    {"r3", "r3", "r3", "r3", "r3", "error"},
    {"s3", "error", "error", "error", "error", "g7"},
    {"error", "s4", "error", "s8", "error", "error"},
    {"r2", "r2", "r2", "r2", "r2", "error"}
};

void apply_reduction(std::stack<std::pair<std::string, int>> &stack, int rule, int &state) {
    int pop_count = (rule == 1) ? 1 : (rule == 2 ? 4 : 3);
    for (int i = 0; i < pop_count; ++i) {
        stack.pop();
    }
    state = std::stoi(DFA[stack.top().second - 1][5].substr(1));
    stack.push({"E", state});
}

int parse_file(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not open the file!" << std::endl;
        return -1;
    }

    std::stack<std::pair<std::string, int>> stack;
    stack.push({"S", 1});
    int state = 1, wordCount = 0;
    char c;

    while (file.get(c)) {
        std::string word, op;
        int column = (c == 'i' && file.peek() == 'd') ? (file.get(), word = "id", 0) :
                     (c == '+' ? (word = "+", 1) :
                      (c == '(' ? (word = "(", 2) :
                       (c == ')' ? (word = ")", 3) : -1)));
        
        if (column == -1) {
            file.close();
            return -1;
        }
        
        wordCount++;
        op = DFA[state - 1][column];
        if (op == "error") {
            file.close();
            return -1;
        }
        
        if (op[0] == 's') {
            state = std::stoi(op.substr(1));
            stack.push({word, state});
        } else if (op[0] == 'r') {
            apply_reduction(stack, std::stoi(op.substr(1)), state);
            file.unget();
        }
    }

    std::string final_op = DFA[state - 1][0];
    if (final_op[0] == 'r') {
        apply_reduction(stack, std::stoi(final_op.substr(1)), state);
    }

    file.close();
    return (state == 2) ? wordCount : -1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    int result = parse_file(argv[1]);
    std::cout << (result == -1 ? "reject" : "accept") << std::endl;
    return 0;
}