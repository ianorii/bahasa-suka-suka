#include "include/turing_machine.h"
#include "include/token.h"
#include "include/lexer.h"
#include "include/ast.h"
#include "include/parser.h"
#include "include/semantic.h"
#include "include/oac_gen.h"
#include "include/cpp_gen.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

// ============================================================
//  TURING MACHINE SIMPLE LEXER
//
//  Lexer asli dari compiler.cpp menggunakan regex.
//  Di sini kita gunakan lexer berbasis TM<char> yang membaca
//  source char-per-char dengan state machine eksplisit.
//  Ini mendemonstrasikan TM sebagai model komputasi.
// ============================================================

void printBanner() {
    std::cout << "\033[36m";
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║   Bahasa Suka Suka Compiler  v2.0 (TM Edition)   ║\n";
    std::cout << "║   Berbasis Turing Machine + One-Address Code     ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n";
    std::cout << "\033[0m";
}

void printHelp(const char* prog) {
    std::cout << "Cara pakai:\n";
    std::cout << "  " << prog << " <file.suka>          → kompilasi ke C++ + jalankan\n";
    std::cout << "  " << prog << " <file.suka> --oac     → hasilkan One-Address Code\n";
    std::cout << "  " << prog << " <file.suka> --tokens  → tampilkan hasil tokenisasi (TM)\n";
    std::cout << "  " << prog << " <file.suka> --ast     → tampilkan struktur AST\n";
    std::cout << "  " << prog << " <file.suka> --cpp     → hanya cetak C++ ke stdout\n";
}

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Gagal membuka file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Cetak token stream
void printTokens(const TokenStream& tokens) {
    std::cout << "\n\033[33m[TOKEN STREAM — TM Lexer]\033[0m\n";
    std::cout << std::string(50, '-') << "\n";
    for (auto& t : tokens) {
        if (t.type == TokenType::END_OF_FILE) break;
        std::cout << "  " << t.line << ":" << t.col
                  << "\t\033[32m" << t.typeName() << "\033[0m"
                  << "\t'" << t.value << "'\n";
    }
    std::cout << std::string(50, '-') << "\n";
}

// Cetak AST
void printAST(ASTNodePtr n, int depth = 0) {
    if (!n) return;
    std::string prefix(depth * 2, ' ');

    auto nodeTypeName = [](NodeType t) -> std::string {
        switch (t) {
            case NodeType::PROGRAM:      return "PROGRAM";
            case NodeType::VAR_DECL:     return "VAR_DECL";
            case NodeType::ASSIGN:       return "ASSIGN";
            case NodeType::EXPR_STMT:    return "EXPR_STMT";
            case NodeType::BLOCK:        return "BLOCK";
            case NodeType::IF_STMT:      return "IF_STMT";
            case NodeType::WHILE_STMT:   return "WHILE_STMT";
            case NodeType::FOR_STMT:     return "FOR_STMT";
            case NodeType::IO_IN:        return "IO_IN";
            case NodeType::IO_OUT:       return "IO_OUT";
            case NodeType::BREAK_STMT:   return "BREAK";
            case NodeType::CONTINUE_STMT:return "CONTINUE";
            case NodeType::EXPR_BINARY:  return "BINARY";
            case NodeType::EXPR_UNARY:   return "UNARY";
            case NodeType::EXPR_LITERAL: return "LITERAL";
            case NodeType::EXPR_VAR:     return "VAR";
            default:                     return "?";
        }
    };

    std::cout << prefix << "\033[35m" << nodeTypeName(n->type) << "\033[0m";
    if (!n->sval.empty()) std::cout << "  \033[33m'" << n->sval << "'\033[0m";
    std::cout << "\n";
    for (auto& c : n->children) printAST(c, depth + 1);
}

int main(int argc, char* argv[]) {
    printBanner();

    if (argc < 2) {
        printHelp(argv[0]);
        return 1;
    }

    std::string filename = argv[1];
    bool modeOAC    = false;
    bool modeTokens = false;
    bool modeAST    = false;
    bool modeCpp    = false;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--oac")    modeOAC    = true;
        if (arg == "--tokens") modeTokens = true;
        if (arg == "--ast")    modeAST    = true;
        if (arg == "--cpp")    modeCpp    = true;
    }

    // ─── Baca source ──────────────────────────────────────────
    std::string source;
    try {
        source = readFile(filename);
    } catch (std::exception& e) {
        std::cerr << "\033[31m[ERROR]\033[0m " << e.what() << "\n";
        return 1;
    }

    // ─── FASE 1: LEXER (TM<char>) ─────────────────────────────
    std::cout << "\033[90m[1/4] Lexing...\033[0m\n";
    Lexer lexer;
    TokenStream tokens;
    try {
        tokens = lexer.tokenize(source);
    } catch (std::exception& e) {
        std::cerr << "\033[31m[LEXER ERROR]\033[0m " << e.what() << "\n";
        return 1;
    }

    if (modeTokens) {
        printTokens(tokens);
        return 0;
    }

    // ─── FASE 2: PARSER (TM<Token>) ───────────────────────────
    std::cout << "\033[90m[2/4] Parsing...\033[0m\n";
    Parser parser;
    ASTNodePtr ast;
    try {
        ast = parser.parse(tokens);
    } catch (ParseError& e) {
        std::cerr << "\033[31m[SYNTAX ERROR]\033[0m "
                  << "baris " << e.line << ": " << e.what() << "\n";
        return 1;
    } catch (std::exception& e) {
        std::cerr << "\033[31m[PARSE ERROR]\033[0m " << e.what() << "\n";
        return 1;
    }

    if (modeAST) {
        std::cout << "\n\033[33m[AST]\033[0m\n";
        printAST(ast);
        return 0;
    }

    // ─── FASE 3: SEMANTIC ANALYSIS ─────────────────────────────
    std::cout << "\033[90m[3/4] Semantic check...\033[0m\n";
    SymbolTable sym;
    SemanticAnalyzer semantic;
    try {
        semantic.analyze(ast, sym);
    } catch (SemanticError& e) {
        std::cerr << "\033[31m[SEMANTIC ERROR]\033[0m "
                  << "baris " << e.line << ": " << e.what() << "\n";
        return 1;
    } catch (std::exception& e) {
        std::cerr << "\033[31m[SEMANTIC ERROR]\033[0m " << e.what() << "\n";
        return 1;
    }

    // ─── FASE 4: CODE GENERATION ──────────────────────────────
    std::cout << "\033[90m[4/4] Code generation...\033[0m\n";

    // ─── MODE OAC ─────────────────────────────────────────────
    if (modeOAC) {
        OACGenerator oacGen;
        auto instrs = oacGen.generate(ast);
        std::string listing = oacGen.listing();

        std::cout << "\n\033[33m[ONE-ADDRESS CODE]\033[0m\n";
        std::cout << listing;

        // Simpan ke file
        std::string outName = filename.substr(0, filename.rfind('.')) + ".oac";
        std::ofstream out(outName);
        out << listing;
        std::cout << "\n\033[32mOAC disimpan ke: " << outName << "\033[0m\n";
        return 0;
    }

    // ─── MODE C++ TRANSPILER (default) ────────────────────────
    CppGenerator cppGen;
    std::string cppSource;
    try {
        cppSource = cppGen.generate(ast);
    } catch (std::exception& e) {
        std::cerr << "\033[31m[CODEGEN ERROR]\033[0m " << e.what() << "\n";
        return 1;
    }

    if (modeCpp) {
        std::cout << "\n" << cppSource;
        return 0;
    }

    // Tulis ke temp file dan kompilasi
    std::string tempFile = "_suka_output.cpp";
    {
        std::ofstream tf(tempFile);
        tf << cppSource;
    }

    std::cout << "\033[32m\nAlhamdulillah sukses dikompilasi!\033[0m\n";
    std::cout << "\033[90mMenjalankan program...\033[0m\n";
    std::cout << std::string(40, '-') << "\n";

    std::cout.flush();
    int ret = system(("g++ -o _suka_bin.exe " + tempFile + " 2>&1 && _suka_bin.exe").c_str());
    system("del _suka_output.cpp _suka_bin.exe >nul 2>&1");

    return (ret == 0) ? 0 : 1;
}
