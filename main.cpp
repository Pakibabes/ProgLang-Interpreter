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
    vector<string> program = readFile("test_ni_Pakibabes.lexor");
    Interpreter lexor(program);
    lexor.run();   
    return 0;
}