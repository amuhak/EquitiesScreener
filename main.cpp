#include "config/Config.h"
#include "data/Equity.h"
#include "engine/Engine.h"
#include "io/Csv.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

// ============================================================
//  CLI argument representation
// ============================================================

struct CliArgs {
    std::string                                             inputFile;   // positional or --input
    std::optional<std::string>                              configFile;  // --config
    std::vector<Engine::FilterRule>                         filters;     // -f (CLI extras)
    std::optional<std::pair<Data::Metric, Engine::SortOrder>> sort;      // -s
    std::string                                             outputFile;  // -o
    bool pretty = false;
    bool help   = false;
    bool valid  = true;
};

// ============================================================
//  CLI helpers
// ============================================================

namespace {

    void printUsage() {
        std::println(stderr,
            "EquitiesScreener - filter and rank stocks from a CSV file.\n"
            "\n"
            "Usage:\n"
            "  EquitiesScreener <input.csv> [options]          flag-based mode\n"
            "  EquitiesScreener [--config <settings.ini>] [options]\n"
            "                                                  settings-file mode\n"
            "                                                  (auto-loads ./screener.ini\n"
            "                                                   when no input/config given)\n"
            "\n"
            "Settings-file mode:\n"
            "  -c, --config <FILE>        Use FILE as the settings file.\n"
            "  -i, --input <FILE>         Override the universe CSV declared in the file.\n"
            "\n"
            "Options (flag-based mode; in settings-file mode they extend every screen):\n"
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
            "                                  In settings-file mode only valid with ONE screen.\n"
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
        auto metric = Config::parseMetric(metricName);
        if (!metric) {
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

        return Engine::FilterRule{*metric, minVal, maxVal};
    }

    std::optional<std::pair<Data::Metric, Engine::SortOrder>>
    parseSortArg(std::string_view arg) {
        auto colon = arg.find(':');
        std::string_view metricName = arg.substr(0, colon);
        auto metric = Config::parseMetric(metricName);
        if (!metric) {
            std::println(stderr, "Unknown sort metric: \"{}\"", metricName);
            return std::nullopt;
        }
        Engine::SortOrder order = Engine::SortOrder::Descending;
        if (colon != std::string_view::npos) {
            auto orderStr = arg.substr(colon + 1);
            if (orderStr == "ASC" || orderStr == "asc")
                order = Engine::SortOrder::Ascending;
        }
        return std::pair{*metric, order};
    }

    // shared error reporting for CSV reads
    int reportCsvError(IO::CsvError error, std::string_view path) {
        switch (error) {
        case IO::CsvError::FileNotFound:
            std::println(stderr, "Error: file not found - \"{}\"", path);
            break;
        case IO::CsvError::EmptyFile:
            std::println(stderr, "Error: file is empty - \"{}\"", path);
            break;
        default:
            std::println(stderr, "Error: could not read \"{}\"", path);
            break;
        }
        return 1;
    }

    // ── legacy mode: the original flag-based CLI behavior ──
    int runLegacyMode(const CliArgs& opts) {
        auto csvResult = IO::readCsv(opts.inputFile);
        if (!csvResult) return reportCsvError(csvResult.error(), opts.inputFile);

        for (const auto& w : csvResult->warnings)
            std::println(stderr, "Warning: {}", w);

        Engine::Engine screener(std::move(csvResult->equities));
        for (const auto& rule : opts.filters)
            screener.addFilter(rule);

        auto results = screener.runScreen();
        if (opts.sort)
            Engine::Engine::sortEquities(results, opts.sort->first, opts.sort->second);

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
            if (!IO::writeCsv(std::cout, results)) {
                std::println(stderr, "Error: could not write to stdout");
                return 1;
            }
        }
        return 0;
    }

    // ── settings-file mode ──
    int runConfigMode(const std::string& settingsPath, const CliArgs& opts) {
        auto parsed = Config::parseSettingsFile(settingsPath);
        if (!parsed) {
            std::println(stderr, "Error: {}", parsed.error());
            return 1;
        }
        Config::AppConfig config = std::move(*parsed);

        // resolve input CSV: CLI override (positional or --input) > settings input
        std::string inputPath = opts.inputFile.empty() ? config.inputFile : opts.inputFile;
        if (inputPath.empty()) {
            std::println(stderr,
                "Error: no input CSV - set \"input =\" in \"{}\" or pass one on the command line",
                settingsPath);
            return 1;
        }

        if (!opts.outputFile.empty() && config.screens.size() != 1) {
            std::println(stderr,
                "Error: --output is only valid when the settings file defines exactly ONE screen "
                "(use per-screen \"output =\" for multiple screens)");
            return 2;
        }

        auto csvResult = IO::readCsv(inputPath);
        if (!csvResult) return reportCsvError(csvResult.error(), inputPath);

        for (const auto& w : csvResult->warnings)
            std::println(stderr, "Warning: {}", w);

        // the universe is read once and shared by every screen
        Engine::Engine screener(std::move(csvResult->equities));

        for (const auto& screen : config.screens) {
            // 1) filters: settings first, CLI -f appended (AND-combined)
            std::vector<Engine::FilterRule> rules = screen.filters;
            rules.insert(rules.end(), opts.filters.begin(), opts.filters.end());

            // 2) presence-aware screening: exclude + warn on missing metric data
            std::vector<Data::Equity> results;
            for (const auto& eq : screener.getUniverse()) {
                bool missing = false;
                for (const auto& rule : rules) {
                    if (!eq.hasMetric(rule.metric)) {
                        std::println(stderr, "Warning: Screen '{}': excluded {} - missing {} data",
                                     screen.name, eq.getTicker(), Config::metricName(rule.metric));
                        missing = true;   // report every missing metric (no break)
                    }
                }
                if (missing) continue;
                if (screener.matchesFilters(eq, rules)) results.push_back(eq);
            }

            // 3) sort: CLI -s overrides the screen's own sort
            std::optional<Config::SortSpec> sortSpec;
            if (opts.sort) {
                sortSpec = Config::SortSpec{opts.sort->first, opts.sort->second};
            } else if (screen.sort) {
                sortSpec = screen.sort;
            }
            if (sortSpec)
                Engine::Engine::sortEquities(results, sortSpec->metric, sortSpec->order);

            // 4) output path: -o > output= > {name}_screened.csv
            std::string outPath = opts.outputFile.empty()
                ? (screen.outputFile ? *screen.outputFile : Config::defaultOutputName(screen.name))
                : opts.outputFile;

            auto written = IO::writeCsv(outPath, results);
            if (!written) {
                std::println(stderr, "Error: could not write to \"{}\"", outPath);
                return 1;
            }
            std::println(stderr, "Screen '{}': wrote {} equities to \"{}\"",
                         screen.name, results.size(), outPath);

            if (opts.pretty) screener.printResults(results);
        }
        return 0;
    }

} // anonymous namespace

// ============================================================
//  CLI argument parser
// ============================================================

CliArgs parseArgs(int argc, char** argv) {
    CliArgs opts;
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
        if (arg == "-c" || arg == "--config") {
            if (i + 1 >= argc) { opts.valid = false; return opts; }
            opts.configFile = argv[++i];
            continue;
        }
        if (arg == "-i" || arg == "--input") {
            if (i + 1 >= argc) { opts.valid = false; return opts; }
            opts.inputFile = argv[++i];
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

        if (!arg.starts_with('-')) {
            if (opts.inputFile.empty()) {
                opts.inputFile = arg;
            } else {
                opts.valid = false;   // multiple positional arguments
                return opts;
            }
        } else {
            opts.valid = false;       // unknown flag
            return opts;
        }
    }
    return opts;
}

// ============================================================
//  Main — mode dispatch
// ============================================================

int main(int argc, char* argv[]) {
    auto opts = parseArgs(argc, argv);

    if (opts.help) {
        printUsage();
        return 0;
    }
    if (!opts.valid) {
        printUsage();
        return 2;
    }

    if (opts.configFile) return runConfigMode(*opts.configFile, opts);
    if (!opts.inputFile.empty()) return runLegacyMode(opts);

    // no --config and no input: auto-load ./screener.ini if present
    if (std::filesystem::exists("screener.ini")) return runConfigMode("screener.ini", opts);

    printUsage();
    return 2;
}
