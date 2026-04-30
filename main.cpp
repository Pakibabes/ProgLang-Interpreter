#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "interpreter.h"
using namespace std;


vector<string> readFile(string fName){
    vector<string> lines;
    ifstream file(fName);
    string line;

    while(getline(file, line)){
        lines.push_back(line);
    }

    return lines;
}
int main(){
    // vector<string> program = readFile("test_ni_Pakibabes.lexor");
    vector<string> program = readFile("sampleProgram2.lexor");
    Interpreter lexor(program);
    lexor.run();   
    return 0;
}
// Running the program
// g++ is the compiler main  and interpreter needs to be compiled
// g++ main.cpp interpreter.cpp -o lexor
// ./lexor