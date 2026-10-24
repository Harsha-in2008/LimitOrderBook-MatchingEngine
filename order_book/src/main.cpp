// Driver: reads an order stream (from a file argument or stdin) and runs it
// through the matching engine, printing trades, top-of-book, and book snapshots.
//
// Input grammar (one instruction per line; '#' starts a comment):
//   ADD    <id> <BUY|SELL> <LIMIT|MARKET> <price> <qty>
//   CANCEL <id>
//   MODIFY <id> <new_qty>
//   TOP
//   PRINT  <N>

#include "OrderBook.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

static void printTrade(const Trade& t) {
    std::cout << "TRADE " << t.price << ' ' << t.qty << ' '
              << t.buyId << ' ' << t.sellId << '\n';
}

static void printTop(const OrderBook& book) {
    int64_t bid = 0, ask = 0;
    bool hb = book.bestBid(bid);
    bool ha = book.bestAsk(ask);

    std::cout << "TOP bid ";
    if (hb) std::cout << bid; else std::cout << '-';
    std::cout << " ask ";
    if (ha) std::cout << ask; else std::cout << '-';
    std::cout << " spread ";
    if (hb && ha) std::cout << (ask - bid); else std::cout << '-';
    std::cout << '\n';
}

static void printBook(const OrderBook& book, std::size_t n) {
    auto bids = book.topBids(n);
    auto asks = book.topAsks(n);

    std::ostringstream head;
    head << "BIDS";
    std::cout << std::left << std::setw(16) << head.str() << "ASKS\n";

    std::size_t rows = std::max(bids.size(), asks.size());
    for (std::size_t i = 0; i < rows; ++i) {
        std::ostringstream left, right;
        if (i < bids.size()) left  << bids[i].first << "  " << bids[i].second;
        if (i < asks.size()) right << asks[i].first << "  " << asks[i].second;
        std::cout << std::left << std::setw(16) << left.str() << right.str() << '\n';
    }
    if (rows == 0)
        std::cout << "(empty)\n";
}

int main(int argc, char** argv) {
    std::istream* in = &std::cin;
    std::ifstream file;
    if (argc > 1) {
        file.open(argv[1]);
        if (!file) {
            std::cerr << "Cannot open file: " << argv[1] << '\n';
            return 1;
        }
        in = &file;
    }

    OrderBook   book;
    std::string line;
    std::size_t lineNo = 0;

    while (std::getline(*in, line)) {
        ++lineNo;

        auto hash = line.find('#');            // strip inline comments
        if (hash != std::string::npos)
            line.erase(hash);

        std::istringstream ss(line);
        std::string        cmd;
        if (!(ss >> cmd))                      // blank / comment-only line
            continue;

        if (cmd == "ADD") {
            uint64_t    id = 0, qty = 0;
            int64_t     price = 0;
            std::string sideStr, typeStr;
            if (!(ss >> id >> sideStr >> typeStr >> price >> qty)) {
                std::cerr << "line " << lineNo << ": malformed ADD\n";
                continue;
            }
            Side      side = (sideStr == "BUY") ? Side::Buy : Side::Sell;
            OrderType type = (typeStr == "MARKET") ? OrderType::Market
                                                   : OrderType::Limit;
            for (const auto& t : book.add(id, side, type, price, qty))
                printTrade(t);

        } else if (cmd == "CANCEL") {
            uint64_t id = 0;
            if (!(ss >> id)) { std::cerr << "line " << lineNo << ": malformed CANCEL\n"; continue; }
            book.cancel(id);

        } else if (cmd == "MODIFY") {
            uint64_t id = 0, qty = 0;
            if (!(ss >> id >> qty)) { std::cerr << "line " << lineNo << ": malformed MODIFY\n"; continue; }
            book.modify(id, qty);

        } else if (cmd == "TOP") {
            printTop(book);

        } else if (cmd == "PRINT") {
            std::size_t n = 5;
            ss >> n;                           // optional; defaults to 5
            printBook(book, n);

        } else {
            std::cerr << "line " << lineNo << ": unknown command '" << cmd << "'\n";
        }
    }
    return 0;
}
