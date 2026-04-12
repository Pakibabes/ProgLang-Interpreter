// executes statements
#include "interpreter.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>

using namespace std;
// remove spacing
string trim (string str){
    int start = str.find_first_not_of(" \t");
    int end = str.find_last_not_of(" \t");

    if (start == -1) return "";

    return str.substr(start, end-start + 1);
}

Interpreter::Interpreter(vector<string> programLines) {

    lines = programLines;

}

void Interpreter::run() {
    removeComments();
    validateStructure();

    
    cout << "LEXOR program structure is valid." << endl;

    processDeclarations();
    processAssignments();
    processPrint();
}

void Interpreter::removeComments() {

    vector<string> cleaned;

    for (string line : lines) {
        line = trim(line);
        if (line.empty()) continue;
        if (line.find("%%") == 0) {
            continue;
        }
        cleaned.push_back(line);
    }
    lines = cleaned;
}


void Interpreter::validateStructure() {

    if (lines.size() < 3) {
        cout << "Invalid program." << endl;
        exit(1);
    }

    if (lines[0] != "SCRIPT AREA") {
        cout << "Error: Missing SCRIPT AREA." << endl;
        exit(1);
    }

    if (lines[1] != "START SCRIPT") {
        cout << "Error: Missing START SCRIPT." << endl;
        exit(1);
    }

    if (lines.back() != "END SCRIPT") {
        cout << "Error: Missing END SCRIPT." << endl;
        exit(1);
    }
}

void Interpreter::processDeclarations(){
    for (int i = 2; i < lines.size(); i++){
        string line = lines[i];

        // when declaration ends
        if(line.find("DECLARE") != 0) break;

        string rest = line.substr(8); // removes the word declare

        int spacePos = rest.find(" ");
        string type = rest.substr(0, spacePos);
        string vars = rest.substr(spacePos + 1);

        string var = "";
        for (int j = 0; j <= vars.length(); j++) {
            if (j == vars.length() || vars[j] == ',') {
                int equalPos = var.find('=');
                if (equalPos != string::npos) {
                    string name = trim(var.substr(0, equalPos));
                    string value = trim(var.substr(equalPos + 1));
                    if ((value.front() == '"' && value.back() == '"') ||
                        (value.front() == '\'' && value.back() == '\'')) {

                        value = value.substr(1, value.size() - 2);
                    }
                    symbolTable[name] = value;
                    typeTable[name] = type;
                }
                else {
                    string name = trim(var);
                    if (type == "INT" || type == "FLOAT")
                        symbolTable[name] = "0";
                    else if (type == "BOOL")
                        symbolTable[name] = "FALSE";
                    else if (type == "CHAR")
                        symbolTable[name] = "";

                    typeTable[name] = type;
                }
                var = "";
            }
            else {
                var += vars[j];
            }
        }
    }
}

void Interpreter::processAssignments(){
    for (int i = 2; i < lines.size(); i++){
        string line = lines[i];

        if(line.find("DECLARE") == 0) continue;
        if(line.find("PRINT") == 0) continue;
        if(line.find("IF") == 0) continue;
        if(line.find("=") != string::npos){
            vector<string>parts;
            string temp ="";
            for (char c : line){
                if(c == '='){
                    parts.push_back(trim(temp));
                    temp = "";
                }else temp += c;
            }
            parts.push_back(trim(temp));
            string value = parts.back();
            if ((value.front() == '"' && value.back() == '"') ||
                (value.front() == '\'' && value.back() == '\'')) {

                value = value.substr(1, value.size() - 2);
            }
            for (int j = parts.size() - 2; j >= 0; j--) {
                string var = parts[j];
                // if value is another variable
                if (symbolTable.find(value) != symbolTable.end()) {
                    value = symbolTable[value];
                }
                symbolTable[var] = value;
            }
        }
        
    }
}

void Interpreter::processPrint(){
    for(string line : lines){
        if(line.find("PRINT:")==0){
            string content = trim(line.substr(6));
            string part="";
            for ( int i = 0; i <= content.length(); i++ ){
                if ( i == content.length() || content[i] =='&'){
                    string token = trim(part);
                    // newline
                    if (token == "$") cout << endl;
                    // special characters
                    else if (token.size() >= 2 && token.front() == '[' && token.back() == ']')cout << token.substr(1, token.size() - 2);
                    // variables
                    else if(symbolTable.find(token) != symbolTable.end()) cout << symbolTable[token];
                    // just pure string
                    else{
                        if((token.front() == '"' && token.back() == '"') ||
                           (token.front() == '\'' && token.back() == '\''))cout << token.substr(1, token.size() - 2);
                        else if((token.front() == '"' && token.back() != '"')) throw runtime_error("Kuwang ug uwahi na QUOTA : At line ");
                        else if((token.front() == '\'' && token.back() != '\'')) throw runtime_error("Kuwang ug usa ka uwahi na QUOTA : At line ");
                        else if((token.front() != '"' && token.back() == '"')) throw runtime_error("Kuwang ug kinaunhan na QUOTA : At line ");
                        else if((token.front() != '\'' && token.back() == '\'')) throw runtime_error("Kuwang ug usa ka kinaunhan na QUOTA : At line ");
                        else cout << token;
                    }
                    part = "";
                }else part+= content[i];
            }
            cout << endl;
        }
    }
}