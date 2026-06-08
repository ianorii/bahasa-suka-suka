# Tugas Teori Bahasa dan Kompilasi
| Mohammad Arya Dhinata | 2504992 |
# Bahasa Suka Suka Compiler v2.0 — TM Edition

Compiler untuk Bahasa Suka Suka, dibangun di atas fondasi **Turing Machine** dan dilengkapi mode **One-Address Code (OAC)**.

## Sintaks Bahasa Suka Suka

### Tipe Data
| Tipe | Setara C++ | Contoh |
|------|-----------|--------|
| `angka` | `long long` | `angka x = 10;` |
| `ngambang` | `float` | `ngambang pi = 3.14;` |
| `huruf` | `string` | `huruf nama = "Rian";` |

### Deklarasi & Assignment
| Sintaks | Contoh |
|---------|--------|
| Deklarasi | `angka x;` |
| Deklarasi + init | `angka x = 10;` |
| Assignment | `x = 20;` |
| Multi deklarasi | `angka x, y, z;` |

### Operator
| Kategori | Operator |
|----------|----------|
| Aritmatika | `+` `-` `*` `/` `%` |
| Perbandingan | `==` `!=` `<` `>` `<=` `>=` |
| Logika | `dan` `atau` |

### Input & Output
| Sintaks | Contoh |
|---------|--------|
| Output | `keluarin << x;` |
| Output banyak | `keluarin << "nilai: " << x << endl;` |
| Input | `masukin >> x;` |
| Input banyak | `masukin >> x >> y;` |

### Percabangan
| Sintaks | Contoh |
|---------|--------|
| If | `kalau x > 0 { ... }` |
| Else if | `kalau tidak x == 0 { ... }` |
| Else | `lain { ... }` |

### Perulangan
| Sintaks | Contoh |
|---------|--------|
| While | `ulang kalau x < 10 { ... }` |
| For | `untuk i itu 0 trs i < 5 nah i++ { ... }` |
| Break | `berhenti;` |
| Continue | `lanjut;` |

### Komentar
| Sintaks | Contoh |
|---------|--------|
| Satu baris | `// ini komentar` |

## Struktur File

```
suka_compiler/
├── main.cpp                   # Entry point & orkestrasi
├── Makefile
├── test.suka                  # Contoh program
└── include/
    ├── turing_machine.h       # TM<Symbol> generik — state machine inti
    ├── token.h                # Definisi TokenType & Token
    ├── lexer.h                # Lexer berbasis TM<char>
    ├── ast.h                  # SymbolTable + ASTNode
    ├── parser.h               # Parser (TM<Token>) → AST
    ├── semantic.h             # Semantic analyzer + type checker
    ├── oac_gen.h              # One-Address Code generator
    └── cpp_gen.h              # C++ transpiler (code generator)
```

## Cara Kompilasi

```bash
make          # Kompilasi compiler
```

## Cara Pakai

```bash
compiler <file.suka>           # Kompilasi + jalankan langsung
compiler <file.suka> --oac     # Hasilkan One-Address Code (.oac)
compiler <file.suka> --tokens  # Tampilkan hasil tokenisasi TM
compiler <file.suka> --ast     # Tampilkan struktur AST
compiler <file.suka> --cpp     # Cetak C++ yang dihasilkan
```

---

## Konsep Turing Machine

### Komponen TM

| Komponen | Peran di Compiler |
|----------|------------------|
| **Tape** | Input yang diproses (karakter / token) |
| **Head** | Posisi baca saat ini |
| **State** | Kondisi mesin (START, IDENT, NUM, dsb.) |
| **Delta (δ)** | Fungsi transisi: (state, simbol) → (state baru, tulis, arah) |

### TM di Lexer

Tape berisi karakter source code. TM membaca satu karakter per langkah dan bertransisi antar state:

```
START --[a-z]--> IDENT --[a-z0-9]--> IDENT --[spasi]--> emit(KEYWORD/IDENT), START
START --[0-9]--> NUM   --[0-9]-----> NUM   --[.] ------> NUM_FLOAT
START --["]--->  STR_DQ --[^"]--->   STR_DQ --["] ------> emit(LIT_STRING), START
```

State `HALT` tercapai saat karakter `'\0'` (sentinel akhir input).

### TM di Parser

Token stream adalah tape baru. Parser membaca token satu per satu (head maju ke kanan) dan state diemulasikan via recursive descent — setiap fungsi (`parseStatement`, `parseExpr`, dsb.) adalah satu "state" TM.

---

## One-Address Code (OAC)

OAC adalah arsitektur **akumulator implisit** — hanya satu operand eksplisit per instruksi.

### Register

- **ACC** — Accumulator (satu-satunya register eksplisit)
- **MEM[addr]** — Memori bernama (variabel program)
- **FLAG** — Hasil perbandingan (EQ, NE, LT, GT, LE, GE)

### Instruksi

| Instruksi | Semantik |
|-----------|----------|
| `LOADI val` | ACC ← val |
| `LOAD addr` | ACC ← MEM[addr] |
| `STORE addr` | MEM[addr] ← ACC |
| `ADD addr` | ACC ← ACC + MEM[addr] |
| `SUB addr` | ACC ← ACC − MEM[addr] |
| `MUL addr` | ACC ← ACC × MEM[addr] |
| `DIV addr` | ACC ← ACC ÷ MEM[addr] |
| `MOD addr` | ACC ← ACC mod MEM[addr] |
| `CMP addr` | FLAG ← compare(ACC, MEM[addr]) |
| `CMPI val` | FLAG ← compare(ACC, val) |
| `JMP label` | PC ← label |
| `JEQ/JNE/JLT/JGT/JLE/JGE label` | Jump bersyarat |
| `IN addr` | MEM[addr] ← stdin |
| `OUT addr` | stdout ← MEM[addr] |
| `OUTI val` | stdout ← val |
| `OUTNL` | stdout ← newline |
| `HALT` | Selesai |

### Contoh OAC

Suka Suka:
```
hasil = hasil + i;
```

OAC yang dihasilkan:
```asm
    LOAD  hasil    ; load hasil ke ACC
    STORE _t0      ; simpan ke temp
    LOAD  i        ; load i ke ACC
    STORE _t1      ; simpan ke temp
    LOAD  _t0      ; restore kiri ke ACC
    ADD   _t1      ; ACC = ACC + _t1
    STORE hasil    ; simpan kembali
```

---

## Pipeline Kompilasi

```
Source (.suka)
    │
    ▼ [LEXER — TM<char>]
TokenStream
    │
    ▼ [PARSER — TM<Token>]
AST (Abstract Syntax Tree)
    │
    ▼ [SEMANTIC ANALYZER]
AST + SymbolTable terisi
    │
    ├──(--oac)──▶ [OAC GENERATOR] ──▶ .oac file
    └──(default)▶ [C++ GENERATOR] ──▶ kompilasi g++ ──▶ run
```
