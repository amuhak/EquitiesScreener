#include "Equity.h"
#include <iomanip>
#include <utility>

namespace Data {

    // Constructor using std::move for performance
    Equity::Equity(std::string name, std::string exchange, std::string ticker, float spot_price)
        : name(std::move(name)),
          exchange(std::move(exchange)),
          ticker(std::move(ticker)),
          spot_price(spot_price) {}

    float Equity::getMetricValue(Metric metric) const {
        switch (metric) {
            case Metric::SpotPrice:
                return spot_price;
            case Metric::PE:
                return pe_ratio;
            case Metric::ForwardPE:
                return forward_pe;
            case Metric::PB:
                return pb_ratio;
            case Metric::PS:
                return ps_ratio;
            case Metric::EVEBITDA:
                return ev_ebitda;
            case Metric::ROA:
                return roa;
            case Metric::ROE:
                return roe;
            case Metric::ROIC:
                return roic;
            default:
                return 0.0f;
        }
    }

    bool Equity::passesFilter(Metric metric, float min_val, float max_val) const {
        float val = getMetricValue(metric);
        return (val >= min_val && val <= max_val);
    }

    std::ostream& operator<<(std::ostream& os, const Equity& equity) {
        os << std::left << std::setw(8)  << equity.ticker
           << std::left << std::setw(25) << equity.name
           << std::left << std::setw(10) << equity.exchange
           << "Price: $" << std::fixed << std::setprecision(2) << std::setw(8) << equity.spot_price
           << " | P/E: " << std::setw(6) << equity.pe_ratio
           << " | EV/EBITDA: " << std::setw(6) << equity.ev_ebitda
           << " | ROE: " << std::setprecision(1) << (equity.roe * 100.0f) << "%";

        return os;
    }

} // namespace Data