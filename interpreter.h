#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "common.h"
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

class Interpreter {
public:
    explicit Interpreter(const vector<string>& programLines);

    void run();

private:
    vector<string>             lines;
    unordered_map<string,string> symbolTable;
    unordered_map<string,string> typeTable;

    // ── Statement execution ──────────────────────────────
    void execute(const vector<Statement>& stmts);
    void execDeclare     (const Statement& s);
    void execAssign      (const Statement& s);
    void execPrint       (const Statement& s);
    void execScan        (const Statement& s);
    void execIf          (const Statement& s);
    void execFor         (const Statement& s);
    void execRepeatWhen  (const Statement& s);

    string evaluate(const string& expr, int line = -1);

    double evalArith(const string& expr, int line);

    string evalLogical(const string& expr, int line);

    string substituteVars(const string& expr) const;

    void   checkType(const string& varName,
                     const string& value, int line);
    string defaultValue(const string& type);

    string buildPrintOutput(const string& content, int line);

    double shuntingYard(const string& expr, int line);
    int    precedence(char op);
    double applyOp(double a, double b, char op, int line);

    static string trim(const string& s);
    static bool   isInteger (const string& s);
    static bool   isFloat   (const string& s);
    static bool   isBool    (const string& s);
    static bool   isChar    (const string& s);
};

#endif
