#include <iostream>
#include "data/Equity.h"
#include "engine/Engine.h"

#include <vector>

int main() {
    using namespace Data;

    // Create sample equities universe
    Equity aapl("Apple Inc.", "NASDAQ", "AAPL", 220.50f);
    aapl.setSector("Technology");
    aapl.setPERatio(33.5f);
    aapl.setForwardPE(28.0f);
    aapl.setPBRatio(45.0f);
    aapl.setPSRatio(8.2f);
    aapl.setEVEBITDA(24.5f);
    aapl.setROA(0.22f);
    aapl.setROE(1.50f);
    aapl.setROIC(0.55f);

    Equity msft("Microsoft Corp.", "NASDAQ", "MSFT", 420.00f);
    msft.setSector("Technology");
    msft.setPERatio(35.0f);
    msft.setForwardPE(30.0f);
    msft.setPBRatio(12.0f);
    msft.setPSRatio(13.0f);
    msft.setEVEBITDA(22.0f);
    msft.setROA(0.19f);
    msft.setROE(0.38f);
    msft.setROIC(0.28f);

    Equity nvda("NVIDIA Corp.", "NASDAQ", "NVDA", 125.00f);
    nvda.setSector("Technology");
    nvda.setPERatio(68.0f);
    nvda.setForwardPE(40.0f);
    nvda.setPBRatio(48.0f);
    nvda.setPSRatio(32.0f);
    nvda.setEVEBITDA(55.0f);
    nvda.setROA(0.42f);
    nvda.setROE(1.15f);
    nvda.setROIC(0.70f);

    Equity googl("Alphabet Inc.", "NASDAQ", "GOOGL", 175.00f);
    googl.setSector("Communication Services");
    googl.setPERatio(24.0f);
    googl.setForwardPE(20.0f);
    googl.setPBRatio(6.5f);
    googl.setPSRatio(6.8f);
    googl.setEVEBITDA(16.5f);
    googl.setROA(0.16f);
    googl.setROE(0.30f);
    googl.setROIC(0.25f);

    Equity amzn("Amazon.com Inc.", "NASDAQ", "AMZN", 180.00f);
    amzn.setSector("Consumer Cyclical");
    amzn.setPERatio(42.0f);
    amzn.setForwardPE(32.0f);
    amzn.setPBRatio(8.0f);
    amzn.setPSRatio(3.5f);
    amzn.setEVEBITDA(20.0f);
    amzn.setROA(0.07f);
    amzn.setROE(0.21f);
    amzn.setROIC(0.15f);

    Equity jpm("JPMorgan Chase & Co.", "NYSE", "JPM", 210.00f);
    jpm.setSector("Financial Services");
    jpm.setPERatio(12.0f);
    jpm.setForwardPE(11.5f);
    jpm.setPBRatio(1.7f);
    jpm.setPSRatio(3.2f);
    jpm.setEVEBITDA(10.0f);
    jpm.setROA(0.013f);
    jpm.setROE(0.17f);
    jpm.setROIC(0.14f);

    Equity intc("Intel Corp.", "NASDAQ", "INTC", 30.00f);
    intc.setSector("Technology");
    intc.setPERatio(85.0f);
    intc.setForwardPE(25.0f);
    intc.setPBRatio(1.2f);
    intc.setPSRatio(2.3f);
    intc.setEVEBITDA(14.0f);
    intc.setROA(0.01f);
    intc.setROE(0.04f);
    intc.setROIC(0.03f);

    std::vector<Equity> equities = {aapl, msft, nvda, googl, amzn, jpm, intc};

    for (auto equity : equities) {
        std::cout << equity << std::endl;
    }

    // Initialize Screener Engine
    Engine::Engine screener;
    screener.addEquity(aapl);
    screener.addEquity(msft);
    screener.addEquity(nvda);
    screener.addEquity(googl);
    screener.addEquity(amzn);
    screener.addEquity(jpm);
    screener.addEquity(intc);

    std::cout << "Loaded " << screener.getUniverseSize() << " equities into the screening engine.\n\n";

    // --- Screen 1: Reasonable Valuation + High Profitability ---
    // Criteria: P/E <= 30.0 AND ROE >= 15% (0.15) AND EV/EBITDA <= 20.0
    screener.clearFilters();
    screener.filterPE(0.0f, 30.0f)
            .filterROE(0.15f)
            .filterEVEBITDA(0.0f, 20.0f);

    std::cout << "[Screen 1] Equities with P/E <= 30.0, ROE >= 15%, EV/EBITDA <= 20.0 (Sorted by ROE descending):\n";
    auto screen1_results = screener.runScreenAndSort(Metric::ROE, Engine::SortOrder::Descending);
    screener.printResults(screen1_results);

    // --- Screen 2: High Return on Capital (ROIC >= 25%) ---
    screener.clearFilters();
    screener.filterROIC(0.25f);

    std::cout << "\n[Screen 2] High ROIC Equities (ROIC >= 25%, Sorted by ROIC descending):\n";
    auto screen2_results = screener.runScreenAndSort(Metric::ROIC, Engine::SortOrder::Descending);
    screener.printResults(screen2_results);



    return 0;
}