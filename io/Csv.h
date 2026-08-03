#ifndef EQUITIESSCREENER_CSV_H
#define EQUITIESSCREENER_CSV_H

#include "../data/Equity.h"

#include <expected>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace IO {

    /// Fatal errors for file-level operations.
    enum class CsvError {
        FileNotFound,
        EmptyFile,
        WriteError,
    };

    /// Result of a batch CSV read. Malformed rows are skipped with warnings.
    struct CsvReadResult {
        std::vector<Data::Equity> equities;
        std::vector<std::string>    warnings;
    };

    // ────────────────────────────────────────────────
    //  Public API
    // ────────────────────────────────────────────────

    /// Parse a single CSV line into an Equity. Returns either a valid Equity
    /// or a string describing what went wrong.
    /// Used internally by readCsv() and exposed for callers who need per-line control.
    [[nodiscard]] std::expected<Data::Equity, std::string>
    parseEquity(std::string_view line);

    /// Read a CSV file. The first line is treated as a column header and
    /// skipped.  Malformed rows are dropped and reported in CsvReadResult::warnings.
    /// Returns CsvError if the file cannot be opened or is empty.
    [[nodiscard]] std::expected<CsvReadResult, CsvError>
    readCsv(std::string_view filepath);

    /// Write equities as CSV to a file (includes header row).
    /// Returns CsvError if the file cannot be opened for writing.
    [[nodiscard]] std::expected<void, CsvError>
    writeCsv(std::string_view filepath, std::span<const Data::Equity> equities);

    /// Write equities as CSV to an output stream (includes header row).
    [[nodiscard]] std::expected<void, CsvError>
    writeCsv(std::ostream& os, std::span<const Data::Equity> equities);

} // namespace IO

#endif // EQUITIESSCREENER_CSV_H
