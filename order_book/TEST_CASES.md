# Order Book — Input Format & Test Cases

## Input format

One instruction per line:

```
ADD    <id> <BUY|SELL> <LIMIT|MARKET> <price> <qty>
CANCEL <id>
MODIFY <id> <new_qty>
TOP
PRINT  <N>
```

- MARKET orders: the `<price>` column is present but ignored. Use `0`.
- All of `id`, `price`, `qty`, `N` are integers; tokens are space-separated.

## Output conventions

- **Trade:** `TRADE <price> <qty> <buy_id> <sell_id>`
- **Trade price** = the **resting (passive) order's** price. The order already
  in the book sets the price; the incoming aggressor takes it.
- **TOP:** `TOP bid <price> ask <price> spread <value>` — use `-` for an empty side.
- **PRINT N:** top N price levels per side, `price qty` (aggregated), best first.

The buy/sell id order in a TRADE line is always *buy first, sell second*,
regardless of which one was the aggressor.

---

## Discrete test cases

Each case starts from an **empty book**. Run them independently.

### T1 — Representation, one side, no matching  (Phase 1)
```
ADD 1 BUY LIMIT 100 10
ADD 2 BUY LIMIT 102 5
ADD 3 BUY LIMIT 101 7
ADD 4 BUY LIMIT 102 3
PRINT 5
```
Expected book (bids, best first):
```
102  8      (orders 2 then 4, FIFO within the level)
101  7
100  10
```
No trades. Verifies price ordering (102 > 101 > 100) and FIFO aggregation at 102.

### T2 — Exact full fill
```
ADD 1 SELL LIMIT 100 10
ADD 2 BUY  LIMIT 100 10
```
Expected:
```
TRADE 100 10 2 1
```
Book empty afterward.

### T3 — Partial fill, incoming larger than resting
```
ADD 1 SELL LIMIT 100 10
ADD 2 BUY  LIMIT 100 15
```
Expected:
```
TRADE 100 10 2 1
```
Remaining 5 of order 2 rests as a bid at 100.

### T4 — Sweep multiple levels + price-time priority
```
ADD 1 SELL LIMIT 101 5
ADD 2 SELL LIMIT 100 5
ADD 3 SELL LIMIT 100 5
ADD 4 BUY  LIMIT 101 12
```
Resting asks: 100 → [2(5), 3(5)], 101 → [1(5)]. Incoming buy 12 @ limit 101.
Expected:
```
TRADE 100 5 4 2
TRADE 100 5 4 3
TRADE 101 2 4 1
```
Order 1 has 3 left, rests as ask at 101. Buy fully filled, nothing rests on bid side.
Verifies best-price-first (100 before 101) and FIFO (order 2 before 3).

### T5 — Market order sweeping levels
```
ADD 1 SELL LIMIT 100 5
ADD 2 SELL LIMIT 101 5
ADD 3 BUY  MARKET 0 8
```
Expected:
```
TRADE 100 5 3 1
TRADE 101 3 3 2
```
Order 2 has 2 left, rests. Market order fully filled.

### T6 — Market order, insufficient liquidity (remainder discarded)
```
ADD 1 SELL LIMIT 100 5
ADD 2 BUY  MARKET 0 10
```
Expected:
```
TRADE 100 5 2 1
```
Remaining 5 is **discarded** (a market order never rests). Book empty.

### T7 — Market order into an empty book
```
ADD 1 BUY MARKET 0 10
```
Expected: **no trades.** Nothing rests. (Edge case: no opposing liquidity.)

### T8 — Cancel changes who gets matched
```
ADD 1 BUY  LIMIT 100 10
ADD 2 BUY  LIMIT 100 5
CANCEL 1
ADD 3 SELL LIMIT 100 8
```
Bids at 100 were [1(10), 2(5)]; cancel removes order 1, leaving [2(5)].
Expected:
```
TRADE 100 5 2 3
```
Remaining 3 of order 3 rests as an ask at 100. Verifies cancel frees the correct node.

### T9 — Cancel a *partially filled* order  (deliverable edge case)
```
ADD 1 SELL LIMIT 100 10
ADD 2 BUY  LIMIT 100 4
CANCEL 1
```
Expected:
```
TRADE 100 4 2 1
```
Order 1 has 6 left resting after the trade; CANCEL 1 removes that remainder.
Book empty afterward.

### T10 — Modify quantity
```
ADD 1 BUY  LIMIT 100 10
MODIFY 1 4
ADD 2 SELL LIMIT 100 10
```
Expected:
```
TRADE 100 4 1 2
```
Remaining 6 of order 2 rests as an ask at 100.
Note: decreasing qty keeps time priority; **increasing** qty should move the order
to the back of its level (lost priority). Be ready to state your modify policy.

---

## Sample order stream  (Phase 4)  — `sample_orders.txt`

A single coherent run. Expected trades and final book below.

```
ADD 1 BUY  LIMIT 99  100
ADD 2 BUY  LIMIT 98  200
ADD 3 SELL LIMIT 101 150
ADD 4 SELL LIMIT 102 100
TOP
ADD 5 SELL LIMIT 99  120
ADD 6 BUY  LIMIT 101 200
CANCEL 2
MODIFY 4 50
TOP
PRINT 5
```

Expected output, in order:
```
TOP bid 99 ask 101 spread 2
TRADE 99 100 1 5
TRADE 99 20 6 5
TRADE 101 150 6 3
TOP bid 101 ask 102 spread 1
```
PRINT 5 then shows:
```
BIDS           ASKS
101  30        102  50
```

Trace of the two aggressive orders:
- **Order 5** (sell 120 @ 99): matches bid 99 (order 1, 100) → `TRADE 99 100 1 5`;
  remaining 20 can't match bid 98 (< 99), so it rests as an ask at 99.
- **Order 6** (buy 200 @ 101): matches ask 99 (order 5's remaining 20) →
  `TRADE 99 20 6 5`; then ask 101 (order 3, 150) → `TRADE 101 150 6 3`;
  remaining 30 can't match ask 102 (> 101), so it rests as a bid at 101.
- **CANCEL 2** removes bid 98. **MODIFY 4 50** shrinks ask 102 from 100 to 50.
