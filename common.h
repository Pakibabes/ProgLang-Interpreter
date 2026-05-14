#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>

using namespace std;


enum TokenType {
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

    TOKEN_NEWLINE, 
    TOKEN_EOF,
    TOKEN_UNKNOWN
};


struct Token {
    TokenType   type;
    string      value;
    int         line;
};

enum StatementType {
    STMT_DECLARE,
    STMT_ASSIGN,
    STMT_PRINT,
    STMT_SCAN,
    STMT_IF,
    STMT_FOR,
    STMT_REPEAT_WHEN,

    STMT_SCRIPT_AREA,
    STMT_START_SCRIPT,
    STMT_END_SCRIPT
};

struct VarDecl {
    string name;
    string initValue;
    bool   hasInit;
};

struct Statement {
    StatementType       type;
    int                 line = -1;

    string              dataType;
    vector<VarDecl>     declarations;

    vector<string>      targets;
    string              expr; 

    string              printContent;

    // ── SCAN ─────────────────────────────
    vector<string>      scanVars;

    string              condition;
    string              forInit;
    string              forCond;
    string              forUpdate;

    vector<Statement>   body;
    vector<Statement>   elseBody;
};

void error(const string& code, const string& message, int line = -1);

#endif 
