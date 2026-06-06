#pragma once
#include "turing_machine.h"
#include "token.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <unordered_map>

// ============================================================
//  LEXER — berbasis Turing Machine (TM<char>)
//
//  Tape   : karakter source code (+ sentinel '\0')
//  Head   : posisi karakter yang sedang dibaca
//  State  : kondisi lexer (START, IDENT, NUM, STR_DQ, dsb.)
//
//  Setiap transisi state TM memproses satu karakter dan
//  memutuskan: akumulasi ke buffer, atau emit token.
// ============================================================

class Lexer {
public:
    TokenStream tokenize(const std::string& source) {
        tokens.clear();
        pos    = 0;
        line   = 1;
        col    = 1;
        input  = source + '\0';
        state  = "START";

        // Jalankan TM sampai EOF
        while (state != "HALT") {
            char ch = input[pos];
            transition(ch);
        }
        tokens.push_back({TokenType::END_OF_FILE, "", line, col});
        return tokens;
    }

private:
    std::string  input;
    size_t       pos   = 0;
    int          line  = 1, col = 1;
    std::string  state = "START";
    std::string  buf;
    int          tokLine = 1, tokCol = 1;
    TokenStream  tokens;

    // ── Advance head ──────────────────────────────────────────
    char advance() {
        char c = input[pos++];
        if (c == '\n') { line++; col = 1; } else col++;
        return c;
    }
    char peek(int offset = 0) {
        size_t i = pos + offset;
        return (i < input.size()) ? input[i] : '\0';
    }

    // ── Emit token dari buffer ─────────────────────────────────
    void emit(TokenType t, const std::string& val = "") {
        std::string v = val.empty() ? buf : val;
        tokens.push_back({t, v, tokLine, tokCol});
        buf.clear();
    }

    void markTokStart() { tokLine = line; tokCol = col; }

    // ── Keyword map ───────────────────────────────────────────
    TokenType classifyIdent(const std::string& s) {
        static const std::unordered_map<std::string, TokenType> kw = {
            {"angka",    TokenType::KW_ANGKA},
            {"ngambang", TokenType::KW_NGAMBANG},
            {"huruf",    TokenType::KW_HURUF},
            {"kalau",    TokenType::KW_KALAU},
            {"lain",     TokenType::KW_LAIN},
            {"ulang",    TokenType::KW_ULANG},
            {"untuk",    TokenType::KW_UNTUK},
            {"itu",      TokenType::KW_ITU},
            {"trs",      TokenType::KW_TRS},
            {"nah",      TokenType::KW_NAH},
            {"tidak",    TokenType::KW_TIDAK},
            {"berhenti", TokenType::KW_BERHENTI},
            {"lanjut",   TokenType::KW_LANJUT},
            {"masukin",  TokenType::KW_MASUKIN},
            {"keluarin", TokenType::KW_KELUARIN},
            {"endl",     TokenType::KW_ENDL},
            {"dan",      TokenType::KW_DAN},
            {"atau",     TokenType::KW_ATAU},
        };
        auto it = kw.find(s);
        if (it != kw.end()) return it->second;
        return TokenType::IDENTIFIER;
    }

    // ── Fungsi transisi utama TM ──────────────────────────────
    void transition(char ch) {

        // ─── HALT ─────────────────────────────────────────────
        if (state == "HALT") return;

        // ─── START ───────────────────────────────────────────
        if (state == "START") {
            if (ch == '\0') { state = "HALT"; return; }

            // Whitespace
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                advance(); return;
            }
            // Komentar
            if (ch == '/' && peek(1) == '/') {
                advance(); advance(); state = "COMMENT"; return;
            }
            // String literal
            if (ch == '"')  { markTokStart(); buf += advance(); state = "STR_DQ"; return; }
            if (ch == '\'') { markTokStart(); buf += advance(); state = "STR_SQ"; return; }
            // Angka
            if (isdigit(ch)) { markTokStart(); buf += advance(); state = "NUM"; return; }
            // Identifier / keyword
            if (isalpha(ch) || ch == '_') { markTokStart(); buf += advance(); state = "IDENT"; return; }

            // Operator & tanda baca
            markTokStart();
            lexSymbol();
            return;
        }

        // ─── COMMENT ─────────────────────────────────────────
        if (state == "COMMENT") {
            if (ch == '\n' || ch == '\0') { state = "START"; }
            else advance();
            return;
        }

        // ─── IDENTIFIER ──────────────────────────────────────
        if (state == "IDENT") {
            if (isalnum(ch) || ch == '_') { buf += advance(); return; }

            // Cek kata ganda: "kalau tidak", "ulang kalau"
            // buf berisi kata pertama, kita lihat apakah ada spasi + kata kedua
            if (buf == "kalau" && ch == ' ') {
                // peek kata setelah spasi
                size_t j = pos + 1;
                std::string word2;
                while (j < input.size() && (isalpha(input[j]) || input[j] == '_'))
                    word2 += input[j++];
                if (word2 == "tidak") {
                    // consume spasi + "tidak"
                    advance(); // spasi
                    for (size_t k = 0; k < word2.size(); k++) advance();
                    buf = "kalau tidak";
                    emit(TokenType::KW_KALAU_TIDAK);
                    state = "START";
                    return;
                }
            }
            if (buf == "ulang" && ch == ' ') {
                size_t j = pos + 1;
                std::string word2;
                while (j < input.size() && (isalpha(input[j]) || input[j] == '_'))
                    word2 += input[j++];
                if (word2 == "kalau") {
                    advance(); // spasi
                    for (size_t k = 0; k < word2.size(); k++) advance();
                    buf = "ulang kalau";
                    emit(TokenType::KW_ULANG_KALAU);
                    state = "START";
                    return;
                }
            }

            // Emit identifier / keyword biasa
            emit(classifyIdent(buf));
            state = "START";
            // Jangan advance — karakter saat ini diproses ulang
            return;
        }

        // ─── NUMBER ──────────────────────────────────────────
        if (state == "NUM") {
            if (isdigit(ch)) { buf += advance(); return; }
            if (ch == '.') { buf += advance(); state = "NUM_FLOAT"; return; }
            emit(TokenType::LIT_INT);
            state = "START";
            return;
        }
        if (state == "NUM_FLOAT") {
            if (isdigit(ch)) { buf += advance(); return; }
            emit(TokenType::LIT_FLOAT);
            state = "START";
            return;
        }

        // ─── STRING LITERAL ───────────────────────────────────
        if (state == "STR_DQ") {
            if (ch == '\0') { emit(TokenType::LIT_STRING); state = "HALT"; return; }
            if (ch == '\\') { buf += advance(); buf += advance(); return; }
            if (ch == '"')  { buf += advance(); emit(TokenType::LIT_STRING); state = "START"; return; }
            buf += advance(); return;
        }
        if (state == "STR_SQ") {
            if (ch == '\0') { emit(TokenType::LIT_STRING); state = "HALT"; return; }
            if (ch == '\\') { buf += advance(); buf += advance(); return; }
            if (ch == '\'') { buf += advance(); emit(TokenType::LIT_STRING); state = "START"; return; }
            buf += advance(); return;
        }

        // ─── OPERATOR MULTI-CHAR ──────────────────────────────
        if (state == "OP_GT") {
            if (ch == '>') { advance(); emit(TokenType::OP_STREAM_IN, ">>"); state = "START"; return; }
            if (ch == '=') { advance(); emit(TokenType::OP_GTE, ">=");       state = "START"; return; }
            emit(TokenType::OP_GT, ">"); state = "START"; return;
        }
        if (state == "OP_LT") {
            if (ch == '<') { advance(); emit(TokenType::OP_STREAM_OUT, "<<"); state = "START"; return; }
            if (ch == '=') { advance(); emit(TokenType::OP_LTE, "<=");        state = "START"; return; }
            emit(TokenType::OP_LT, "<"); state = "START"; return;
        }
        if (state == "OP_BANG") {
            if (ch == '=') { advance(); emit(TokenType::OP_NEQ, "!="); state = "START"; return; }
            emit(TokenType::UNKNOWN, "!"); state = "START"; return;
        }

        // Fallback
        advance();
    }

    // ── Simbol tunggal / transisi ke OP_GT dsb. ───────────────
    void lexSymbol() {
        char ch = input[pos];
        switch (ch) {
            case ';': advance(); emit(TokenType::SEMICOLON,    ";"); return;
            case '{': advance(); emit(TokenType::LBRACE,       "{"); return;
            case '}': advance(); emit(TokenType::RBRACE,       "}"); return;
            case '(': advance(); emit(TokenType::LPAREN,       "("); return;
            case ')': advance(); emit(TokenType::RPAREN,       ")"); return;
            case ',': advance(); emit(TokenType::COMMA,        ","); return;
            case '+': advance(); emit(TokenType::OP_PLUS,      "+"); return;
            case '-': advance(); emit(TokenType::OP_MINUS,     "-"); return;
            case '*': advance(); emit(TokenType::OP_MUL,       "*"); return;
            case '/': advance(); emit(TokenType::OP_DIV,       "/"); return;
            case '%': advance(); emit(TokenType::OP_MOD,       "%"); return;
            case '>': advance(); buf = ">"; state = "OP_GT";   return;
            case '<': advance(); buf = "<"; state = "OP_LT";   return;
            case '=':
                if (peek(1) == '=') { advance(); advance(); emit(TokenType::OP_EQ, "=="); return; }
                advance(); emit(TokenType::OP_ASSIGN, "="); return;
            case '!': advance(); buf = "!"; state = "OP_BANG"; return;
            default:
                advance();
                emit(TokenType::UNKNOWN, std::string(1, ch));
                return;
        }
    }
};
