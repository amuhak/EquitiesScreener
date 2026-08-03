// Minimal unit tests for the Config INI parser (no external framework):
// plain asserts, non-zero exit code on any failure.
#include "Config.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

namespace {

    int failures = 0;

    void check(bool cond, const char* what) {
        if (!cond) {
            std::printf("FAIL: %s\n", what);
            ++failures;
        }
    }

    std::filesystem::path writeTemp(const char* name, const std::string& content) {
        auto path = std::filesystem::temp_directory_path() / name;
        std::ofstream out{path, std::ios::trunc};
        out << content;
        return path;
    }

    std::optional<Config::AppConfig> parseOk(const std::string& content, const char* name) {
        auto path = writeTemp(name, content);
        auto result = Config::parseSettingsFile(path.string());
        std::filesystem::remove(path);
        if (!result) {
            std::printf("  (unexpected parse error: %s)\n", result.error().c_str());
            return std::nullopt;
        }
        return *result;
    }

    bool parseFails(const std::string& content, const char* name) {
        auto path = writeTemp(name, content);
        auto result = Config::parseSettingsFile(path.string());
        std::filesystem::remove(path);
        if (result) {
            std::printf("  (expected error, got %zu screens)\n", result->screens.size());
            return false;
        }
        return true;
    }

} // anonymous namespace

int main() {
    using namespace Config;
    using Data::Metric;
    using Engine::SortOrder;

    // ── happy path: global input + two screens ──
    auto cfg = parseOk(
        "input = stocks.csv\n"
        "[screen:value]\n"
        "filter = PE:0:30\n"
        "filter = ROE:0.15:\n"
        "sort = ROE:desc\n"
        "output = v.csv\n"
        "[screen:growth]\n"
        "filter = ROIC:0.25:\n",
        "cfg_happy.ini");
    check(cfg.has_value(), "happy path parses");
    if (cfg) {
        check(cfg->inputFile == "stocks.csv", "global input parsed");
        check(cfg->screens.size() == 2, "two screens parsed");
        const auto& v = cfg->screens[0];
        check(v.name == "value", "screen name parsed");
        check(v.filters.size() == 2, "two filters parsed");
        check(v.filters[0].metric == Metric::PE
                  && v.filters[0].min_val == 0.0f
                  && v.filters[0].max_val == 30.0f, "PE both bounds");
        check(v.filters[1].metric == Metric::ROE
                  && v.filters[1].min_val == 0.15f
                  && v.filters[1].max_val == std::numeric_limits<float>::infinity(),
              "ROE min-only bound (open max)");
        check(v.sort.has_value() && v.sort->metric == Metric::ROE
                  && v.sort->order == SortOrder::Descending, "explicit desc sort");
        check(v.outputFile.has_value() && *v.outputFile == "v.csv", "explicit output");
        const auto& g = cfg->screens[1];
        check(!g.sort.has_value() && !g.outputFile.has_value(), "growth has no sort/output (defaults)");
    }

    // ── case-insensitivity: header, keys, metrics, orders ──
    auto c2 = parseOk("[SCREEN:value]\nFILTER = pe:0:10\nSORT = Pb:asc\n", "cfg_case.ini");
    check(c2.has_value(), "case-insensitive file parses");
    check(c2 && c2->screens[0].filters[0].metric == Metric::PE, "lowercase metric name");
    check(c2 && c2->screens[0].sort.has_value() && c2->screens[0].sort->order == SortOrder::Ascending,
          "asc order");

    // ── whitespace around bound segments is tolerated ──
    auto c3 = parseOk("[screen:s]\nfilter = PE: 0 :30\n", "cfg_space.ini");
    check(c3.has_value() && c3->screens[0].filters[0].min_val == 0.0f
              && c3->screens[0].filters[0].max_val == 30.0f, "spaced bound segments");

    // ── trailing garbage in a number is rejected (full-consume parse) ──
    check(parseFails("[screen:s]\nfilter = PE:0:30x\n", "cfg_garbage.ini"), "trailing garbage rejected");

    // ── structural errors (fail fast) ──
    check(parseFails("[screen:a]\n[screen:A]\n", "cfg_dupscreen.ini"), "duplicate screen name (case-insensitive)");
    check(parseFails("input = a\ninput = b\n[screen:s]\n", "cfg_dupinput.ini"), "duplicate global input");
    check(parseFails("foo = 1\n[screen:s]\n", "cfg_unkglobal.ini"), "unknown global key");
    check(parseFails("[filters]\n", "cfg_unksection.ini"), "unknown section");
    check(parseFails("[screen:s]\nfiltr = PE:0:30\n", "cfg_unkkey.ini"), "unknown screen key");
    check(parseFails("[screen:s]\nfilter = PE:0\n", "cfg_malformed.ini"), "malformed filter (2 parts)");
    check(parseFails("[screen:s]\nfilter = PE:abc:30\n", "cfg_badmin.ini"), "non-numeric MIN");
    check(parseFails("[screen:s]\nsort = PE:sideways\n", "cfg_badsort.ini"), "invalid sort order");
    check(parseFails("", "cfg_empty.ini"), "empty file");
    check(parseFails("input = a\n", "cfg_noscreens.ini"), "no screens");
    check(parseFails("[screen:s]\ninput = a\n", "cfg_inputinscreen.ini"), "input inside a screen section");
    check(parseFails("[screen:s]\nsort = PE:asc\nsort = PE:desc\n", "cfg_dupsort.ini"), "duplicate sort");

    // ── CRLF endings, comments, blank lines ──
    auto c6 = parseOk("# header comment\r\n; semicolon comment\r\n\r\ninput = stocks.csv\r\n"
                      "[screen:s]\r\nfilter = PE:0:30\r\n", "cfg_crlf.ini");
    check(c6.has_value() && c6->inputFile == "stocks.csv" && c6->screens.size() == 1,
          "CRLF + comments parse");

    // ── metric name helper ──
    check(parseMetric("PE") == Metric::PE && parseMetric("pe") == Metric::PE
              && parseMetric("Roe") == Metric::ROE, "parseMetric case-insensitive");
    check(!parseMetric("EPS").has_value(), "unknown metric rejected");

    // ── output naming ──
    check(defaultOutputName("value") == "value_screened.csv", "auto output name");
    check(defaultOutputName("cheap large caps!") == "cheap_large_caps__screened.csv",
          "output name sanitized");

    if (failures == 0) {
        std::printf("All Config tests passed.\n");
        return 0;
    }
    std::printf("%d Config test(s) FAILED.\n", failures);
    return 1;
}
