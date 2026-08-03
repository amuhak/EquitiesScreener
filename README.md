# EquitiesScreener

A lightweight C++ equity screening engine that filters and ranks stocks based on fundamental valuation and profitability metrics.

## Features

- **CSV I/O** — load equity universes from CSV files and export screened results back to CSV
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
├── io/
│   ├── Csv.h           # CSV reader & writer API
│   └── Csv.cpp         # Parsing & formatting with C++23 ranges
├── main.cpp            # CLI — CSV in, filtered/sorted CSV out
├── sample.csv          # 7-ticker sample universe
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

## CLI Usage

```bash
# Pipe screened stocks to stdout (Unix filter style)
EquitiesScreener stocks.csv -f PE:0:30 -f ROE:0.15 -s ROE > value.csv

# Write to file
EquitiesScreener stocks.csv -f ROIC:0.25: -o high_roic.csv

# Pretty-print to terminal
EquitiesScreener stocks.csv -f PE:0:30 -f ROE:0.15 -p

# Show help
EquitiesScreener -h
```

### C++ Library API

```cpp
#include "data/Equity.h"
#include "engine/Engine.h"
#include "io/Csv.h"

// Read CSV into the engine
auto csv = IO::readCsv("stocks.csv");
if (!csv) { /* handle error */ }

Engine::Engine screener(std::move(csv->equities));
screener.filterPE(0.0f, 30.0f)
        .filterROE(0.15f)
        .filterEVEBITDA(0.0f, 20.0f);

auto results = screener.runScreenAndSort(Data::Metric::ROE, Engine::SortOrder::Descending);

// Write results back to CSV
IO::writeCsv("screened.csv", results);
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
