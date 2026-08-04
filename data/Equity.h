#ifndef EQUITIESSCREENER_EQUITY_H
#define EQUITIESSCREENER_EQUITY_H

#include <bitset>
#include <cstddef>
#include <iostream>
#include <string>

namespace Data {

    enum class Metric {
        SpotPrice,
        PE,
        ForwardPE,
        PB,
        PS,
        EVEBITDA,
        ROA,
        ROE,
        ROIC
    };

    class Equity {
    private:
        // Basic Metadata
        std::string name;
        std::string exchange;
        std::string ticker;
        std::string sector;

        float spot_price{0.0f};

        // Valuation Metrics
        float pe_ratio{0.0f};
        float forward_pe{0.0f};
        float pb_ratio{0.0f};
        float ps_ratio{0.0f};
        float ev_ebitda{0.0f};

        // Profitability / Return Metrics (stored as decimals, e.g., 0.15 for 15%)
        float roa{0.0f};
        float roe{0.0f};
        float roic{0.0f};

        // Presence tracking: which optional metrics were explicitly set.
        // Bit index = static_cast<size_t>(Metric). Used by the settings-file
        // pipeline to exclude stocks that are missing data a filter asks about.
        std::bitset<9> present_{};

    public:
        // Default and Parameterized Constructors
        Equity() = default;
        Equity(std::string name, std::string exchange, std::string ticker, float spot_price);

        // Basic Getters & Setters
        std::string getName() const { return name; }
        std::string getExchange() const { return exchange; }
        std::string getTicker() const { return ticker; }
        std::string getSector() const { return sector; }
        float getSpotPrice() const { return spot_price; }

        void setName(const std::string& t_name) { name = t_name; }
        void setExchange(const std::string& t_exchange) { exchange = t_exchange; }
        void setTicker(const std::string& t_ticker) { ticker = t_ticker; }
        void setSector(const std::string& t_sector) { sector = t_sector; }
        void setSpotPrice(float t_spot_price) {
            spot_price = t_spot_price;
            present_.set(static_cast<std::size_t>(Metric::SpotPrice));
        }

        // Valuation Getters & Setters
        float getPERatio() const { return pe_ratio; }
        float getForwardPE() const { return forward_pe; }
        float getPBRatio() const { return pb_ratio; }
        float getPSRatio() const { return ps_ratio; }
        float getEVEBITDA() const { return ev_ebitda; }

        void setPERatio(float val) {
            pe_ratio = val;
            present_.set(static_cast<std::size_t>(Metric::PE));
        }
        void setForwardPE(float val) {
            forward_pe = val;
            present_.set(static_cast<std::size_t>(Metric::ForwardPE));
        }
        void setPBRatio(float val) {
            pb_ratio = val;
            present_.set(static_cast<std::size_t>(Metric::PB));
        }
        void setPSRatio(float val) {
            ps_ratio = val;
            present_.set(static_cast<std::size_t>(Metric::PS));
        }
        void setEVEBITDA(float val) {
            ev_ebitda = val;
            present_.set(static_cast<std::size_t>(Metric::EVEBITDA));
        }

        // Profitability Getters & Setters
        float getROA() const { return roa; }
        float getROE() const { return roe; }
        float getROIC() const { return roic; }

        void setROA(float val) {
            roa = val;
            present_.set(static_cast<std::size_t>(Metric::ROA));
        }
        void setROE(float val) {
            roe = val;
            present_.set(static_cast<std::size_t>(Metric::ROE));
        }
        void setROIC(float val) {
            roic = val;
            present_.set(static_cast<std::size_t>(Metric::ROIC));
        }

        // --- Screener Specific Methods ---

        // Generic Metric Accessor for dynamic filtering/sorting
        float getMetricValue(Metric metric) const;

        // True if this equity has explicit data for the given metric.
        // SpotPrice is always considered present (required by the constructor).
        bool hasMetric(Metric metric) const noexcept {
            return metric == Metric::SpotPrice
                || present_.test(static_cast<std::size_t>(metric));
        }

        // Check if the metric passes a min/max bound
        bool passesFilter(Metric metric, float min_val, float max_val) const;

        // Output helper for printing screener results
        friend std::ostream& operator<<(std::ostream& os, const Equity& equity);
    };

} // namespace Data

#endif // EQUITIESSCREENER_EQUITY_H