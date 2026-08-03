#include "data/Equity.h"
#include "engine/Engine.h"
#include "io/Csv.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// ============================================================
//  CLI argument representation
// ============================================================

struct CliArgs {
    std::string                                       inputFile;
    std::vector<Engine::FilterRule>                   filters;
    std::optional<std::pair<Data::Metric, Engine::SortOrder>> sort;
    std::string                                       outputFile;
    bool pretty = false;
    bool help   = false;
    bool valid  = true;
};

// ============================================================
//  Metric name helpers
// ============================================================

namespace {
    std::string toUpper(std::string_view sv) {
        std::string out;
        for (char c : sv)
            out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return out;
    }

    Data::Metric metricFromString(std::string_view name) {
        static const std::map<std::string_view, Data::Metric> map = {
            {"SPOTPRICE", Data::Metric::SpotPrice},
            {"PE",        Data::Metric::PE},
            {"FORWARDPE", Data::Metric::ForwardPE},
            {"PB",        Data::Metric::PB},
            {"PS",        Data::Metric::PS},
            {"EVEBITDA",  Data::Metric::EVEBITDA},
            {"ROA",       Data::Metric::ROA},
            {"ROE",       Data::Metric::ROE},
            {"ROIC",      Data::Metric::ROIC},
        };
        auto it = map.find(toUpper(name));
        return (it != map.end()) ? it->second : Data::Metric::SpotPrice;
    }

    bool isValidMetric(std::string_view name) {
        static const auto sentinel = Data::Metric::SpotPrice;
        // relies on metricFromString returning SpotPrice as sentinel for unknowns
        return metricFromString(name) != sentinel
            || toUpper(name) == "SPOTPRICE";
    }

    void printUsage() {
        std::println(stderr,
            "EquitiesScreener - filter and rank stocks from a CSV file.\n"
            "\n"
            "Usage:\n"
            "  EquitiesScreener <input.csv> [options]\n"
            "\n"
            "Options:\n"
            "  -f, --filter <METRIC:MIN:MAX>   Apply a filter. MAX is optional.\n"
            "                                  Repeatable. Examples:\n"
            "                                    -f PE:0:30       (0 <= PE <= 30)\n"
            "                                    -f ROE:0.15:     (ROE >= 15%)\n"
            "                                    -f EVEBITDA::20  (EV/EBITDA <= 20)\n"
            "\n"
            "  -s, --sort   <METRIC[:ASC]>     Sort by metric (default: descending).\n"
            "                                  Example: -s ROE  or  -s PE:ASC\n"
            "\n"
            "  -o, --output <FILE>             Write CSV to file. Default: stdout.\n"
            "\n"
            "  -p, --pretty                    Pretty-print results to terminal.\n"
            "\n"
            "  -h, --help                      Show this message.\n"
        );
    }

    std::optional<Engine::FilterRule> parseFilterArg(std::string_view arg) {
        auto colon1 = arg.find(':');
        if (colon1 == std::string_view::npos) return std::nullopt;

        std::string_view metricName = arg.substr(0, colon1);
        if (!isValidMetric(metricName)) {
            std::println(stderr, "Unknown metric: \"{}\"", metricName);
            return std::nullopt;
        }

        auto rest   = arg.substr(colon1 + 1);
        auto colon2 = rest.find(':');
        auto minStr = rest.substr(0, colon2);
        auto maxStr = (colon2 == std::string_view::npos)
                          ? std::string_view{}
                          : rest.substr(colon2 + 1);

        // trim whitespace before numeric parsing
        auto trimSv = [](std::string_view s) {
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))  s.remove_suffix(1);
            return s;
        };

        float minVal = -std::numeric_limits<float>::infinity();
        float maxVal =  std::numeric_limits<float>::infinity();

        if (!minStr.empty()) {
            auto trimmed = trimSv(minStr);
            auto [_, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), minVal);
            if (ec != std::errc{}) {
                std::println(stderr, "Invalid number in filter: \"{}\"", trimmed);
                return std::nullopt;
            }
        }
        if (!maxStr.empty()) {
            auto trimmed = trimSv(maxStr);
            auto [_, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), maxVal);
            if (ec != std::errc{}) {
                std::println(stderr, "Invalid number in filter: \"{}\"", trimmed);
                return std::nullopt;
            }
        }

        return Engine::FilterRule{metricFromString(metricName), minVal, maxVal};
    }

    std::optional<std::pair<Data::Metric, Engine::SortOrder>>
    parseSortArg(std::string_view arg) {
        auto colon = arg.find(':');
        std::string_view metricName = arg.substr(0, colon);
        if (!isValidMetric(metricName)) {
            std::println(stderr, "Unknown sort metric: \"{}\"", metricName);
            return std::nullopt;
        }
        Engine::SortOrder order = Engine::SortOrder::Descending;
        if (colon != std::string_view::npos) {
            auto orderStr = arg.substr(colon + 1);
            if (orderStr == "ASC" || orderStr == "asc")
                order = Engine::SortOrder::Ascending;
        }
        return std::pair{metricFromString(metricName), order};
    }
} // anonymous namespace

// ============================================================
//  CLI argument parser
// ============================================================

CliArgs parseArgs(int argc, char** argv) {
    CliArgs opts;
    if (argc < 2) { opts.valid = false; return opts; }

    for (int i = 1; i < argc; ++i) {
        std::string_view arg{argv[i]};

        if (arg == "-h" || arg == "--help") {
            opts.help = true;
            return opts;
        }
        if (arg == "-p" || arg == "--pretty") {
            opts.pretty = true;
            continue;
        }
        if (arg == "-f" || arg == "--filter") {
            if (i + 1 >= argc) { opts.valid = false; return opts; }
            auto rule = parseFilterArg(argv[++i]);
            if (!rule) { opts.valid = false; return opts; }
            opts.filters.push_back(std::move(*rule));
            continue;
        }
        if (arg == "-s" || arg == "--sort") {
            if (i + 1 >= argc) { opts.valid = false; return opts; }
            auto s = parseSortArg(argv[++i]);
            if (!s) { opts.valid = false; return opts; }
            opts.sort = std::move(*s);
            continue;
        }
        if (arg == "-o" || arg == "--output") {
            if (i + 1 >= argc) { opts.valid = false; return opts; }
            opts.outputFile = argv[++i];
            continue;
        }

        if (opts.inputFile.empty() && !arg.starts_with('-')) {
            opts.inputFile = arg;
        } else if (!arg.starts_with('-')) {
            opts.valid = false;
            return opts;
        } else {
            opts.valid = false;
            return opts;
        }
    }

    if (opts.inputFile.empty()) opts.valid = false;
    return opts;
}

// ============================================================
//  Main pipeline
// ============================================================

int main(int argc, char* argv[]) {
    auto opts = parseArgs(argc, argv);

    if (opts.help) {
        printUsage();
        return 0;
    }
    if (!opts.valid || opts.inputFile.empty()) {
        printUsage();
        return 2;
    }

    // --- read CSV ---
    auto csvResult = IO::readCsv(opts.inputFile);
    if (!csvResult) {
        switch (csvResult.error()) {
        case IO::CsvError::FileNotFound:
            std::println(stderr, "Error: file not found - \"{}\"", opts.inputFile);
            break;
        case IO::CsvError::EmptyFile:
            std::println(stderr, "Error: file is empty - \"{}\"", opts.inputFile);
            break;
        default:
            std::println(stderr, "Error: could not read \"{}\"", opts.inputFile);
            break;
        }
        return 1;
    }

    for (const auto& w : csvResult->warnings)
        std::println(stderr, "Warning: {}", w);

    // --- engine ---
    Engine::Engine screener(std::move(csvResult->equities));

    for (const auto& rule : opts.filters)
        screener.addFilter(rule);

    auto results = screener.runScreen();

    if (opts.sort)
        Engine::Engine::sortEquities(results, opts.sort->first, opts.sort->second);

    // --- output ---
    if (opts.pretty) {
        screener.printResults(results);
    } else if (!opts.outputFile.empty()) {
        auto written = IO::writeCsv(opts.outputFile, results);
        if (!written) {
            std::println(stderr, "Error: could not write to \"{}\"", opts.outputFile);
            return 1;
        }
        std::println(stderr, "Wrote {} equities to \"{}\"", results.size(), opts.outputFile);
    } else {
        IO::writeCsv(std::cout, results);
    }

    return 0;
}
