
#include "OrderBook.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

static void printTrade(const Trade& t) {
    cout << "TRADE " << t.price << ' ' << t.qty << ' '<< t.buyId << ' ' << t.sellId << '\n';
}

static void printTop(const OrderBook& book) {
    int64_t bid = 0, ask = 0;
    bool hb = book.bestBid(bid);
    bool ha = book.bestAsk(ask);

    cout << "TOP bid ";
    if (hb) cout << bid;
    else cout << '-';

    cout << " ask ";
    if (ha) cout << ask;
    else cout << '-';

    cout << " spread ";
    if (hb && ha) cout << (ask - bid);
    else cout << '-';

    cout << '\n';
}

static void printBook(const OrderBook& book, size_t n) {
    auto bids = book.topBids(n);
    auto asks = book.topAsks(n);

    cout << left << setw(16) << "BIDS" << "ASKS\n";

    size_t rows = max(bids.size(), asks.size());

    for (size_t i = 0; i < rows; ++i) {
        ostringstream leftCol, rightCol;

        if (i < bids.size())
            leftCol << bids[i].first << "  " << bids[i].second;

        if (i < asks.size())
            rightCol << asks[i].first << "  " << asks[i].second;

        cout << left << setw(16) << leftCol.str()<< rightCol.str() << '\n';
    }

    if (rows == 0)
        cout << "(empty)\n";
}

int main(int argc, char** argv) {
    istream* in = &cin;
    ifstream file;

    if (argc > 1) {
        file.open(argv[1]);

        if (!file) {
            cerr << "Cannot open file: " << argv[1] << '\n';
            return 1;
        }

        in = &file;
    }

    OrderBook book;
    string line;
    size_t lineNo = 0;

    while (getline(*in, line)) {
        ++lineNo;

        auto hash = line.find('#');
        if (hash != string::npos)
            line.erase(hash);

        istringstream ss(line);
        string cmd;

        if (!(ss >> cmd))
            continue;

        if (cmd == "ADD") {
            uint64_t id = 0, qty = 0;
            int64_t price = 0;
            string sideStr, typeStr;

            if (!(ss >> id >> sideStr >> typeStr >> price >> qty)) {
                cerr << "line " << lineNo << ": malformed ADD\n";
                continue;
            }

            Side side = (sideStr == "BUY")? Side::Buy: Side::Sell;

            OrderType type = (typeStr == "MARKET") ? OrderType::Market: OrderType::Limit;

            for (const auto& t :book.add(id, side, type, price, qty)) {
                printTrade(t);
            }

        } else if (cmd == "CANCEL") {
            uint64_t id = 0;

            if (!(ss >> id)) {
                cerr << "line " << lineNo << ": malformed CANCEL\n";
                continue;
            }

            book.cancel(id);

        } else if (cmd == "MODIFY") {
            uint64_t id = 0, qty = 0;

            if (!(ss >> id >> qty)) {
                cerr << "line " << lineNo<< ": malformed MODIFY\n";
                continue;
            }

            book.modify(id, qty);

        } else if (cmd == "TOP") {
            printTop(book);

        } else if (cmd == "PRINT") {
            size_t n = 5;
            ss >> n;
            printBook(book, n);

        } else {
            cerr << "line " << lineNo<< ": unknown command '" << cmd << "'\n";
        }
    }

    return 0;
}
