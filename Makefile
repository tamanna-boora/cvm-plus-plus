# On Windows with MSYS2, use the full path or add C:\msys64\ucrt64\bin to PATH
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -I src
TARGET   := cvm
SRC      := src/main.cpp

.PHONY: all clean run-hello run-fibonacci run-factorial

all: $(TARGET)

$(TARGET): $(SRC) src/lexer.hpp src/ast.hpp src/parser.hpp src/compiler.hpp src/vm.hpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run-hello: $(TARGET)
	./$(TARGET) examples/hello.cvm

run-fibonacci: $(TARGET)
	./$(TARGET) examples/fibonacci.cvm

run-factorial: $(TARGET)
	./$(TARGET) examples/factorial.cvm

clean:
	rm -f $(TARGET)
