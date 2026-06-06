CXX      = g++
CXXFLAGS = -std=c++20 -Wall -Wno-unused-variable -Wno-unused-parameter
TARGET   = compiler
SRC      = main.cpp

all: $(TARGET)

$(TARGET): $(SRC) include/*.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) _suka_output.cpp _suka_bin *.oac

run:
	./$(TARGET) test.suka

oac:
	./$(TARGET) test.suka --oac

tokens:
	./$(TARGET) test.suka --tokens

ast:
	./$(TARGET) test.suka --ast

cpp:
	./$(TARGET) test.suka --cpp

.PHONY: all clean run oac tokens ast cpp
