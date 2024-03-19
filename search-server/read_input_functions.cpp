#include "read_input_functions.h"

std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int number;
    std::cin >> number;
    std::cin >> std::ws;
    return number;
}