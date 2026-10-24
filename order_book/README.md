# Limit Order Book — Matching Engine (C++)

A price–time-priority limit order book and matching engine. Accepts a stream of
limit and market orders, matches them against resting orders under strict
price–time priority, and emits trades.

## Build & run

Requires a C++17 compiler.

```bash
make                         # builds ./matching_engine
./matching_engine sample_orders.txt    # run an order stream from a file
./matching_engine                      # or read an order stream from stdin
make run                     # shortcut: runs sample_orders.txt
make clean
```

One-liner without make:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude src/*.cpp -o matching_engine
```

## Project layout

```
include/Order.h       Order and Trade types; Side / OrderType enums
include/OrderBook.h   OrderBook class — the engine interface
src/OrderBook.cpp     matching, cancel, modify, top-of-book, display
src/main.cpp          order-stream parser / driver
sample_orders.txt     a coherent Phase-4 order stream
tests/t1..t10.txt     the discrete test cases below, as runnable inputs
TEST_CASES.md         format spec + every test with expected output
```

## Input format

One instruction per line (`#` starts a comment; blank lines ignored):

```
ADD    <id> <BUY|SELL> <LIMIT|MARKET> <price> <qty>
CANCEL <id>
MODIFY <id> <new_qty>
TOP
PRINT  <N>
```

For a `MARKET` order the price column is present but ignored — use `0`.

## Output format

- **Trade:** `TRADE <price> <qty> <buy_id> <sell_id>`
  The execution price is the **resting (passive) order's** price — the order
  already in the book sets the price; the aggressor takes it, so an aggressive
  order that crosses deep into the book gets price improvement.
  The two ids are always printed buy first, sell second.
- **TOP:** `TOP bid <p> ask <p> spread <v>` (`-` for an empty side/spread).
- **PRINT N:** top N levels per side, `price  aggregate-qty`, best first.

## Data structures & complexity

Three structures work together (`M` = number of distinct price levels on a
side; `K` = number of resting orders a match consumes):

| Structure | Type | Role |
|-----------|------|------|
| `bids_` | `std::map<price, Level, std::greater<>>` | best bid at `begin()` |
| `asks_` | `std::map<price, Level, std::less<>>` | best ask at `begin()` |
| `Level` | `std::list<Order>` | FIFO queue at one price → time priority |
| `index_` | `std::unordered_map<id, Location>` | O(1) order lookup by id |

A `Location` stores the side, the price (the map key), and a **`std::list`
iterator** straight to the order's node. `std::list` is chosen specifically
because its iterators stay valid when other nodes are inserted or erased, and
it supports **O(1) erase from the middle** — that is what makes cancel cheap.

| Operation | Complexity | Why |
|-----------|-----------|-----|
| Add, no match | `O(log M)` | locate/create the price level in the map |
| Add, matching | `O(log M + K)` | walk from the best level, consume K resting orders |
| Cancel | `O(1)` id lookup + `O(log M)` level cleanup | hash to the node, O(1) list erase; the O(log M) is only to drop a level that became empty |
| Modify (shrink) | `O(1)` | edit the quantity in place |
| Modify (grow) | `O(log M)` | cancel + re-add at the back of the level |
| Top of book | `O(1)` | `begin()` on each map |
| Display top N | `O(N + orders in those levels)` | iterate N levels, sum each |

## Design decisions worth defending

- **Why `std::map` (a balanced BST), not a hash map, for price levels?** We
  constantly need *the best price* and *ordered traversal outward* from it.
  A tree keeps levels sorted so `begin()` is the best and `++` walks to the
  next — a hash map gives neither. `O(log M)` is cheap because M is the number
  of *price levels*, not orders.
- **Why `std::list` for the level, not `std::deque`/`std::vector`?** Time
  priority is FIFO, which any queue gives — but cancel must remove an order
  from the *middle* of a level in O(1) using a stored handle. Only `list`
  offers stable iterators plus O(1) middle erase. A `vector`/`deque` erase is
  O(level size) and invalidates the handles in `index_`.
- **Why the id → location index?** Without it, cancel/modify would scan the
  book to find an order. The hash map makes locating any order O(1) average,
  which is exactly the non-functional requirement.
- **Trade price = passive order's price.** Standard exchange convention; the
  resting order was there first and set the terms.

## Modify policy (state this explicitly)

- **Decreasing** quantity edits the order in place and **keeps** its position in
  the FIFO queue — you shouldn't lose your place for reducing size.
- **Increasing** quantity re-inserts the order at the **back** of its level
  (cancel + re-add), so it **loses** time priority — otherwise an order could
  jump the queue by growing. Modify-to-zero is treated as a cancel.

The spec notes a *price* change is modelled as cancel-and-re-add; that's the
right call too, since changing price always forfeits time priority.

## Edge cases handled

- Empty book / empty side (top-of-book reports `-`).
- Partial fill with the incoming order larger (remainder rests / is discarded).
- Partial fill with a resting order larger (it stays, reduced, at the front).
- Market order with no / insufficient liquidity (unfilled part discarded).
- Cancelling a **partially filled** order (removes only the live remainder).
- Price levels are deleted once their last order leaves, so they never
  linger empty.

## Test cases

`TEST_CASES.md` documents ten scenarios (T1–T10) with expected output; each is a
runnable file under `tests/`. All ten currently pass, as does `sample_orders.txt`.

```bash
for t in tests/t*.txt; do echo "== $t =="; ./matching_engine "$t"; done
```

## Study-question answers (quick reference)

- **Complexity of add/cancel/match:** add `O(log M)` (+`K` if it matches),
  cancel `O(1)` average, match `O(log M + K)`. See the table above.
- **How is time priority guaranteed at a price?** Each level is a FIFO
  `std::list`; new orders `push_back`, matches take `front`. Order of arrival =
  order in the list.
- **Partial fill?** Trade `min(incoming, resting)`; reduce both; the one left
  with quantity stays (resting stays at the front of its level; a limit
  incoming remainder rests; a market incoming remainder is discarded).
- **Market order, no opposing liquidity?** No trade is produced and nothing
  rests — the order simply does nothing.
- **How is cancel made ~constant time?** The `index_` hash map maps id → a
  stable `std::list` iterator, so we jump straight to the node and erase it in
  O(1), with no book scan.
- **Latency bottleneck / how to reduce it (Phase 5 territory):** per-order heap
  allocation (every `list`/`map` node) and pointer-chasing cache misses in the
  tree and list. Mitigations: a custom pool allocator / object pool, replacing
  `std::map` with a flat array indexed by price tick when the price range is
  bounded, and intrusive lists to keep a level's nodes contiguous.
