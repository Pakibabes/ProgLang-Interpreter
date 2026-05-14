#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>

using namespace std;

// ─────────────────────────────────────────
//  TOKEN TYPES
// ─────────────────────────────────────────
enum TokenType {
    // Structure keywords (multi-word treated as single token)
    TOKEN_SCRIPT_AREA,   // "SCRIPT AREA"
    TOKEN_START_SCRIPT,  // "START SCRIPT"
    TOKEN_END_SCRIPT,    // "END SCRIPT"
    TOKEN_START_IF,      // "START IF"
    TOKEN_END_IF,        // "END IF"
    TOKEN_START_FOR,     // "START FOR"
    TOKEN_END_FOR,       // "END FOR"
    TOKEN_START_REPEAT,  // "START REPEAT"
    TOKEN_END_REPEAT,    // "END REPEAT"

    // Single-word keywords
    TOKEN_DECLARE,
    TOKEN_PRINT,
    TOKEN_SCAN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_ELSE_IF,       // "ELSE IF" as one token
    TOKEN_FOR,
    TOKEN_REPEAT_WHEN,   // "REPEAT WHEN" as one token

    // Data types
    TOKEN_INT_TYPE,
    TOKEN_FLOAT_TYPE,
    TOKEN_BOOL_TYPE,
    TOKEN_CHAR_TYPE,

    // Literals
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_BOOL_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,

    // Identifier
    TOKEN_IDENTIFIER,

    // Arithmetic operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_MODULO,

    // Comparison operators
    TOKEN_GT,    // >
    TOKEN_LT,    // <
    TOKEN_GTE,   // >=
    TOKEN_LTE,   // <=
    TOKEN_EQ,    // ==
    TOKEN_NEQ,   // <>

    // Logical operators
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,

    // Assignment
    TOKEN_ASSIGN,   // =

    // Symbols
    TOKEN_COMMA,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_COLON,
    TOKEN_CONCAT,   // &
    TOKEN_DOLLAR,   // $
    TOKEN_LBRACKET, // [
    TOKEN_RBRACKET, // ]

    TOKEN_NEWLINE,   // end-of-line marker emitted by Lexer
    TOKEN_EOF,
    TOKEN_UNKNOWN
};

// ─────────────────────────────────────────
//  TOKEN STRUCT
// ─────────────────────────────────────────
struct Token {
    TokenType   type;
    string      value;
    int         line;
};

// ─────────────────────────────────────────
//  STATEMENT TYPES
// ─────────────────────────────────────────
enum StatementType {
    STMT_DECLARE,
    STMT_ASSIGN,
    STMT_PRINT,
    STMT_SCAN,
    STMT_IF,
    STMT_FOR,
    STMT_REPEAT_WHEN,

    // Structural (kept so the interpreter can skip them cleanly)
    STMT_SCRIPT_AREA,
    STMT_START_SCRIPT,
    STMT_END_SCRIPT
};

// One variable declaration entry
struct VarDecl {
    string name;
    string initValue;  // raw literal string; empty if no initialiser
    bool   hasInit;
};

// ─────────────────────────────────────────
//  STATEMENT STRUCT
// ─────────────────────────────────────────
struct Statement {
    StatementType       type;
    int                 line = -1;

    // ── DECLARE ──────────────────────────
    string              dataType;       // "INT" | "FLOAT" | "BOOL" | "CHAR"
    vector<VarDecl>     declarations;

    // ── ASSIGN ───────────────────────────
    // Supports chained assignment: a = b = <expr>
    vector<string>      targets;        // left-hand side variables (in order)
    string              expr;           // raw RHS expression string

    // ── PRINT ────────────────────────────
    string              printContent;   // everything after "PRINT:"

    // ── SCAN ─────────────────────────────
    vector<string>      scanVars;

    // ── IF / ELSE IF / REPEAT WHEN ───────
    string              condition;      // raw boolean expression string

    // ── FOR ──────────────────────────────
    string              forInit;        // raw init  expression  (e.g. "i=0")
    string              forCond;        // raw loop  condition
    string              forUpdate;      // raw update expression (e.g. "i=i+1")

    // ── Block bodies ─────────────────────
    vector<Statement>   body;           // main / true branch
    vector<Statement>   elseBody;       // else / false branch
    //   ELSE-IF chains are stored as a nested STMT_IF inside elseBody
};

// ─────────────────────────────────────────
//  GLOBAL ERROR HELPER  (defined in common.cpp)
// ─────────────────────────────────────────
void error(const string& code, const string& message, int line = -1);

#endif // COMMON_H
