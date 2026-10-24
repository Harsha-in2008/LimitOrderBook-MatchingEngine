CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude

SRC := src/OrderBook.cpp src/main.cpp
HDR := include/Order.h include/OrderBook.h
BIN := matching_engine

$(BIN): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

# Run the sample Phase-4 order stream.
run: $(BIN)
	./$(BIN) sample_orders.txt

clean:
	rm -f $(BIN)

.PHONY: run clean
