#pragma once
#include "token.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>

// ============================================================
//  SYMBOL TABLE
// ============================================================

struct VarInfo {
    std::string type;   // "angka", "ngambang", "huruf"
    int         declLine;
};

class SymbolTable {
public:
    bool has(const std::string& name) const { return table.count(name) > 0; }

    void declare(const std::string& name, const std::string& type, int line) {
        if (has(name))
            throw std::runtime_error("Variabel '" + name + "' udah dideklarasi bang! (baris " + std::to_string(line) + ")");
        table[name] = {type, line};
    }

    const VarInfo& get(const std::string& name) const {
        auto it = table.find(name);
        if (it == table.end())
            throw std::runtime_error("Variabel '" + name + "' belum dideklarasikan!");
        return it->second;
    }

    std::string typeOf(const std::string& name) const {
        auto it = table.find(name);
        if (it == table.end()) return "";
        return it->second.type;
    }

    void clear() { table.clear(); }

private:
    std::unordered_map<std::string, VarInfo> table;
};

// ============================================================
//  AST NODE TYPES
// ============================================================

enum class NodeType {
    PROGRAM,
    VAR_DECL,       // tipe nama [= expr]
    ASSIGN,         // nama itu expr
    EXPR_STMT,      // expr;
    BLOCK,          // { stmts }
    IF_STMT,        // kalau / kalau tidak / lain
    WHILE_STMT,     // ulang kalau
    FOR_STMT,       // untuk
    IO_IN,          // masukin >> var
    IO_OUT,         // keluarin << expr [<< expr ...]
    BREAK_STMT,
    CONTINUE_STMT,

    // Expressions
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_LITERAL,
    EXPR_VAR,
    EXPR_ASSIGN,
};

struct ASTNode {
    NodeType                         type;
    Token                            tok;      // token representatif
    std::string                      sval;     // nilai string tambahan
    std::vector<std::shared_ptr<ASTNode>> children;

    ASTNode(NodeType t, Token tk = {}) : type(t), tok(tk) {}

    void add(std::shared_ptr<ASTNode> child) {
        children.push_back(std::move(child));
    }
};

using ASTNodePtr = std::shared_ptr<ASTNode>;
