# EquitiesScreener

A lightweight C++ equity screening engine that filters and ranks stocks based on fundamental valuation and profitability metrics.

## Features

- **Multi-metric filtering** — screen equities by P/E, Forward P/E, P/B, P/S, EV/EBITDA, ROA, ROE, and ROIC
- **Chainable filter API** — compose screens fluently (e.g. `.filterPE(0, 30).filterROE(0.15)`)
- **Sorting** — sort results ascending or descending by any supported metric
- **Extensible universe** — add equities individually or in bulk via `addEquity` / `setUniverse`

## Project Structure

```
EquitiesScreener/
├── data/
│   ├── Equity.h        # Equity data model & Metric enum
│   └── Equity.cpp
├── engine/
│   ├── Engine.h        # Screening engine interface
│   └── Engine.cpp      # Filter, screen, sort logic
├── main.cpp            # Example screens
└── CMakeLists.txt
```

## Build

Requires **CMake ≥ 3.20** and a **C++23**-capable compiler (GCC, Clang, or MSVC).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/EquitiesScreener
```

Or open the project directly in **CLion** — it will pick up `CMakeLists.txt` automatically.

## Example Usage

```cpp
Engine::Engine screener;
screener.addEquity(aapl);
screener.addEquity(msft);

// Screen: P/E <= 30, ROE >= 15%, EV/EBITDA <= 20 — sorted by ROE descending
screener.filterPE(0.0f, 30.0f)
        .filterROE(0.15f)
        .filterEVEBITDA(0.0f, 20.0f);

auto results = screener.runScreenAndSort(Data::Metric::ROE, Engine::SortOrder::Descending);
screener.printResults(results);
```

## Supported Metrics

| Metric | Method |
|--------|--------|
| Spot Price | `filterSpotPrice(min, max)` |
| P/E Ratio | `filterPE(min, max)` |
| Forward P/E | `filterForwardPE(min, max)` |
| P/B Ratio | `filterPB(min, max)` |
| P/S Ratio | `filterPS(min, max)` |
| EV/EBITDA | `filterEVEBITDA(min, max)` |
| ROA | `filterROA(min, max)` |
| ROE | `filterROE(min, max)` |
| ROIC | `filterROIC(min, max)` |

## License

MIT
