#pragma once

#include "domain.h"

#include <unordered_map>
#include <deque>
#include <set>
#include <functional>

namespace transport {

class TransportCatalogue {
public:
    void AddBus(const Bus& bus);
    void AddStop(const Stop& stop);

    const Bus* GetBus(std::string_view name) const;
    const Stop* GetStop(std::string_view name) const;

    const std::set<const Bus*, BusComparator>& GetBusesByStop(const Stop* stop) const;
    const std::set<const Bus*, BusComparator>& GetBusesByStop(std::string_view stop_name) const;

    const Stop* GetStopPtrByName(std::string_view name) const;

    void SetStopDistance(std::string_view from, std::string_view to, int distance);
    void SetStopDistance(const Stop* from, const Stop* to, int distance);

    int GetStopDistance(std::string_view from, std::string_view to) const;
    int GetStopDistance(const Stop* from, const Stop* to) const;

    const std::deque<Bus>& GetAllBuses() const;
    const std::deque<Stop>& GetAllStops() const;

private:
    std::deque<Bus> buses_;
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, const Bus*> bus_ptrs_;
    std::unordered_map<std::string_view, const Stop*> stop_ptrs_;
    std::unordered_map<const Stop*, std::set<const Bus*, BusComparator>> buses_by_stop_;
    const std::set<const Bus*, BusComparator> empty_set_;
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopDistanceHasher> distances_;
};

} 
