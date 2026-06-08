#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// ============================================================
//  TOKEN DEFINITIONS
//  Setiap token yang dihasilkan Lexer punya tipe dan nilai.
// ============================================================

enum class TokenType {
    // Tipe data
    KW_ANGKA, KW_NGAMBANG, KW_HURUF,

    // Kontrol alur
    KW_KALAU, KW_KALAU_TIDAK, KW_LAIN,
    KW_ULANG, KW_ULANG_KALAU, KW_UNTUK,
    KW_ITU, KW_TRS, KW_NAH,
    KW_BERHENTI, KW_LANJUT,
    KW_TIDAK,

    // I/O
    KW_MASUKIN, KW_KELUARIN,
    KW_ENDL, KW_DAN, KW_ATAU,

    // Literal
    LIT_INT, LIT_FLOAT, LIT_STRING,

    // Identifier
    IDENTIFIER,

    // Operator
    OP_ASSIGN,        // =
    OP_EQ,  OP_NEQ,
    OP_LT,  OP_GT,  OP_LTE,  OP_GTE,
    OP_PLUS, OP_MINUS, OP_MUL, OP_DIV, OP_MOD,
    OP_INC,           // ++
    OP_DEC,           // --
    OP_PLUS_ASSIGN,   // +=
    OP_MINUS_ASSIGN,  // -=
    OP_STREAM_IN,     // >>
    OP_STREAM_OUT,    // <<

    // Tanda baca
    SEMICOLON,
    LBRACE, RBRACE,
    LPAREN, RPAREN,
    COMMA,

    // Spesial
    END_OF_LINE,
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType   type;
    std::string value;
    int         line;
    int         col;

    std::string typeName() const {
        static const std::unordered_map<int, std::string> names = {
            {(int)TokenType::KW_ANGKA,       "angka"},
            {(int)TokenType::KW_NGAMBANG,    "ngambang"},
            {(int)TokenType::KW_HURUF,       "huruf"},
            {(int)TokenType::KW_KALAU,       "kalau"},
            {(int)TokenType::KW_KALAU_TIDAK, "kalau tidak"},
            {(int)TokenType::KW_LAIN,        "lain"},
            {(int)TokenType::KW_ULANG,       "ulang"},
            {(int)TokenType::KW_ULANG_KALAU, "ulang kalau"},
            {(int)TokenType::KW_UNTUK,       "untuk"},
            {(int)TokenType::KW_ITU,         "itu"},
            {(int)TokenType::KW_TRS,         "trs"},
            {(int)TokenType::KW_NAH,         "nah"},
            {(int)TokenType::KW_TIDAK,       "tidak"},
            {(int)TokenType::KW_BERHENTI,    "berhenti"},
            {(int)TokenType::KW_LANJUT,      "lanjut"},
            {(int)TokenType::KW_MASUKIN,     "masukin"},
            {(int)TokenType::KW_KELUARIN,    "keluarin"},
            {(int)TokenType::KW_ENDL,        "endl"},
            {(int)TokenType::KW_DAN,         "dan"},
            {(int)TokenType::KW_ATAU,        "atau"},
            {(int)TokenType::LIT_INT,        "integer literal"},
            {(int)TokenType::LIT_FLOAT,      "float literal"},
            {(int)TokenType::LIT_STRING,     "string literal"},
            {(int)TokenType::IDENTIFIER,     "identifier"},
            {(int)TokenType::OP_ASSIGN,      "="},
            {(int)TokenType::OP_EQ,          "=="},
            {(int)TokenType::OP_NEQ,         "!="},
            {(int)TokenType::OP_LT,          "<"},
            {(int)TokenType::OP_GT,          ">"},
            {(int)TokenType::OP_LTE,         "<="},
            {(int)TokenType::OP_GTE,         ">="},
            {(int)TokenType::OP_PLUS,        "+"},
            {(int)TokenType::OP_INC,          "++"},
            {(int)TokenType::OP_DEC,          "--"},
            {(int)TokenType::OP_PLUS_ASSIGN,  "+="},
            {(int)TokenType::OP_MINUS_ASSIGN, "-="},
            {(int)TokenType::OP_MINUS,       "-"},
            {(int)TokenType::OP_MUL,         "*"},
            {(int)TokenType::OP_DIV,         "/"},
            {(int)TokenType::OP_MOD,         "%"},
            {(int)TokenType::OP_STREAM_IN,   ">>"},
            {(int)TokenType::OP_STREAM_OUT,  "<<"},
            {(int)TokenType::SEMICOLON,      ";"},
            {(int)TokenType::LBRACE,         "{"},
            {(int)TokenType::RBRACE,         "}"},
            {(int)TokenType::LPAREN,         "("},
            {(int)TokenType::RPAREN,         ")"},
            {(int)TokenType::COMMA,          ","},
            {(int)TokenType::END_OF_FILE,    "EOF"},
            {(int)TokenType::UNKNOWN,        "?"},
        };
        auto it = names.find((int)type);
        if (it != names.end()) return it->second;
        return "?";
    }
};

using TokenStream = std::vector<Token>;
