#include "common.h"
#include <iostream>
#include <cstdlib>

using namespace std;

void error(const string& code, const string& message, int line) {
    cerr << "LEXOR Error [" << code << "]";
    if (line != -1)
        cerr << " at line " << line;
    cerr << ": " << message << endl;
    exit(1);
}
