#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <iostream>

// ============================================================
//  TURING MACHINE CORE
//
//  Tape   : vector of symbols (karakter atau token string)
//  Head   : posisi baca/tulis saat ini
//  State  : kondisi mesin saat ini (string label)
//  Delta  : fungsi transisi  (state, symbol) → (new_state, write_symbol, direction)
//
//  Digunakan sebagai fondasi Lexer & Parser.
//  Setiap fase kompilasi adalah satu TM yang memproses tape-nya.
// ============================================================

enum class Direction { LEFT, RIGHT, STAY };

template<typename Symbol>
struct TMRule {
    std::string next_state;
    Symbol      write;
    Direction   dir;
};

template<typename Symbol>
class TuringMachine {
public:
    using State      = std::string;
    using TransKey   = std::pair<State, Symbol>;
    using TransFunc  = std::function<TMRule<Symbol>(const State&, const Symbol&)>;

    static const inline State ACCEPT  = "__ACCEPT__";
    static const inline State REJECT  = "__REJECT__";
    static const inline State HALT    = "__HALT__";

    // Blank symbol (default untuk char: spasi; untuk string: "")
    Symbol blank;

    TuringMachine(Symbol blankSym = Symbol{}) : blank(blankSym) {}

    // Set tape dari vector of symbols
    void loadTape(const std::vector<Symbol>& input) {
        tape = input;
        head = 0;
        currentState = "q0";
        steps = 0;
    }

    // Load dari string (hanya untuk Symbol = char)
    void loadTape(const std::string& input) requires std::is_same_v<Symbol, char> {
        tape = std::vector<char>(input.begin(), input.end());
        head = 0;
        currentState = "q0";
        steps = 0;
    }

    void setInitialState(const State& s) { currentState = s; }

    void setTransition(TransFunc fn) { transFunc = fn; }

    // Jalankan sampai HALT / ACCEPT / REJECT
    bool run(int maxSteps = 1000000) {
        while (currentState != HALT &&
               currentState != ACCEPT &&
               currentState != REJECT) {
            if (steps++ > maxSteps) {
                throw std::runtime_error("TM tidak berhenti (melebihi " +
                    std::to_string(maxSteps) + " langkah)");
            }
            Symbol sym = readHead();
            TMRule<Symbol> rule = transFunc(currentState, sym);
            writeHead(rule.write);
            currentState = rule.next_state;
            move(rule.dir);
        }
        return currentState == ACCEPT;
    }

    // Step satu langkah manual (untuk debug / visualisasi)
    bool step() {
        if (isHalted()) return false;
        Symbol sym = readHead();
        TMRule<Symbol> rule = transFunc(currentState, sym);
        writeHead(rule.write);
        currentState = rule.next_state;
        move(rule.dir);
        steps++;
        return true;
    }

    bool isHalted() const {
        return currentState == HALT ||
               currentState == ACCEPT ||
               currentState == REJECT;
    }
    bool accepted()  const { return currentState == ACCEPT; }
    bool rejected()  const { return currentState == REJECT; }

    const std::vector<Symbol>& getTape()    const { return tape; }
    int                        getHead()    const { return head; }
    const State&               getState()  const { return currentState; }
    long long                  getSteps()  const { return steps; }

    // Ambil konten tape sebagai string (hanya untuk Symbol=char)
    std::string tapeAsString() const requires std::is_same_v<Symbol, char> {
        return std::string(tape.begin(), tape.end());
    }

    // Resize tape ke kanan jika head melewati ujung
    Symbol readHead() {
        if (head < 0) return blank;
        if ((size_t)head >= tape.size()) tape.resize(head + 1, blank);
        return tape[head];
    }

    void writeHead(const Symbol& sym) {
        if (head < 0) return;
        if ((size_t)head >= tape.size()) tape.resize(head + 1, blank);
        tape[head] = sym;
    }

    void move(Direction d) {
        if      (d == Direction::RIGHT) head++;
        else if (d == Direction::LEFT && head > 0) head--;
    }

private:
    std::vector<Symbol> tape;
    int                 head         = 0;
    State               currentState = "q0";
    TransFunc           transFunc;
    long long           steps        = 0;
};
