#include "transport_catalogue.h"
#include <algorithm>
#include <unordered_set>
#include <stdexcept>

using namespace transport;

void TransportCatalogue::AddBus(const Bus& bus) {
    buses_.push_back(bus);
    auto [it, inserted] = bus_ptrs_.emplace(buses_.back().name_, &buses_.back());
    const Bus* added_bus = it->second;
    for (const Stop* stop : added_bus->GetStops()) {
        buses_by_stop_[stop].insert(added_bus);
    }
}

void TransportCatalogue::AddStop(const Stop& stop) {
    stops_.push_back(stop);
    stop_ptrs_.emplace(stops_.back().name_, &stops_.back());
}

const Bus* TransportCatalogue::GetBus(std::string_view name) const {
    auto pos = bus_ptrs_.find(name);
    return pos == bus_ptrs_.end() ? nullptr : pos->second;
}

const Stop* TransportCatalogue::GetStop(std::string_view name) const {
    auto pos = stop_ptrs_.find(name);
    return pos == stop_ptrs_.end() ? nullptr : pos->second;
}

const std::set<const Bus*, BusComparator>& TransportCatalogue::GetBusesByStop(const Stop* stop) const {
    if (!stop) {
        return empty_set_;
    }
    auto pos = buses_by_stop_.find(stop);
    return pos != buses_by_stop_.end() ? pos->second : empty_set_;
}

const Stop* TransportCatalogue::GetStopPtrByName(std::string_view name) const {
    return GetStop(name);
}

void TransportCatalogue::SetStopDistance(const Stop* from, const Stop* to, int distance) {
    distances_[{from, to}] = distance;
}

void TransportCatalogue::SetStopDistance(std::string_view from, std::string_view to, int distance) {
    SetStopDistance(GetStop(from), GetStop(to), distance);
}

int TransportCatalogue::GetStopDistance(const Stop* from, const Stop* to) const {
    auto pos = distances_.find({from, to});
    if (pos == distances_.end()) {
        pos = distances_.find({to, from});
        if (pos == distances_.end()) {
            return 0;
        }
    }
    return pos->second;
}

int TransportCatalogue::GetStopDistance(std::string_view from, std::string_view to) const {
    return GetStopDistance(GetStop(from), GetStop(to));
}

const std::deque<Bus>& TransportCatalogue::GetAllBuses() const {
    return buses_;
}

const std::deque<Stop>& TransportCatalogue::GetAllStops() const {
    return stops_;
}
