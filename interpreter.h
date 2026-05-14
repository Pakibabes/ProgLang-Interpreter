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
    unordered_map<string,string> symbolTable;  // varName -> value string
    unordered_map<string,string> typeTable;    // varName -> "INT"|"FLOAT"|"BOOL"|"CHAR"

    // ── Statement execution ──────────────────────────────
    void execute(const vector<Statement>& stmts);
    void execDeclare     (const Statement& s);
    void execAssign      (const Statement& s);
    void execPrint       (const Statement& s);
    void execScan        (const Statement& s);
    void execIf          (const Statement& s);
    void execFor         (const Statement& s);
    void execRepeatWhen  (const Statement& s);

    // ── Expression evaluation ────────────────────────────
    // Evaluates an arithmetic / logical / mixed expression.
    // Returns the result as a string ("42", "3.14", "TRUE", "c").
    string evaluate(const string& expr, int line = -1);

    // Arithmetic sub-evaluator (returns double)
    double evalArith(const string& expr, int line);

    // Logical sub-evaluator (returns "TRUE" or "FALSE")
    string evalLogical(const string& expr, int line);

    // Replace variable names in an expression with their values
    string substituteVars(const string& expr) const;

    // ── Type helpers ─────────────────────────────────────
    void   checkType(const string& varName,
                     const string& value, int line);
    string defaultValue(const string& type);

    // ── PRINT helpers ────────────────────────────────────
    string buildPrintOutput(const string& content, int line);

    // ── Shunting-yard arithmetic ─────────────────────────
    double shuntingYard(const string& expr, int line);
    int    precedence(char op);
    double applyOp(double a, double b, char op, int line);

    // ── Utility ──────────────────────────────────────────
    static string trim(const string& s);
    static bool   isInteger (const string& s);
    static bool   isFloat   (const string& s);
    static bool   isBool    (const string& s);
    static bool   isChar    (const string& s);
};

#endif // INTERPRETER_H
