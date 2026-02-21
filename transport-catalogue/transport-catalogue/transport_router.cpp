#include "transport_router.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace transport {

namespace { 
    constexpr double MINUTES_IN_HOUR = 60.0;
    constexpr double METERS_IN_KILOMETER = 1000.0;
}

TransportRouter::TransportRouter(const TransportCatalogue& catalogue, const RoutingSettings& settings)
    : catalogue_(catalogue), settings_(settings) {
    BuildGraph();
    router_ = std::make_unique<graph::Router<double>>(*graph_);
}

double TransportRouter::ComputeBusTime(int distance) const {
    return (distance * MINUTES_IN_HOUR) / (settings_.bus_velocity * METERS_IN_KILOMETER);
}

void TransportRouter::BuildGraph() {
    InitializeStopVertices();
    AddWaitEdges();
    AddBusEdges();
}

void TransportRouter::InitializeStopVertices() {
    const auto& stops = catalogue_.GetAllStops();
    const size_t vertex_count = stops.size() * 2;
    graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(vertex_count);

    size_t vertex_id = 0;
    for (const auto& stop : stops) {
        stop_to_vertex_wait_[stop.name_] = vertex_id;
        stop_to_vertex_bus_[stop.name_] = vertex_id + 1;
        vertex_to_stop_[vertex_id] = stop.name_;
        vertex_to_stop_[vertex_id + 1] = stop.name_;
        vertex_id += 2;
    }
}

void TransportRouter::AddWaitEdges() {
    for (const auto& [stop_name, wait_vertex] : stop_to_vertex_wait_) {
        const auto bus_vertex = stop_to_vertex_bus_.at(stop_name);
        
        graph::Edge<double> wait_edge{
            wait_vertex,
            bus_vertex,
            static_cast<double>(settings_.bus_wait_time)
        };
        auto wait_edge_id = graph_->AddEdge(wait_edge);
        edge_to_wait_item_[wait_edge_id] = RouteInfo::WaitItem{
            std::string(stop_name),
            static_cast<double>(settings_.bus_wait_time)
        };
    }
}

void TransportRouter::AddBusEdges() {
    const auto& buses = catalogue_.GetAllBuses();
    
    for (const auto& bus : buses) {
        const auto& bus_stops = bus.GetStops();
        if (bus_stops.empty()) continue;
        
        AddBusEdgesForDirection(bus, bus_stops, true);
        
        if (!bus.is_round()) {
            AddBusEdgesForDirection(bus, bus_stops, false);
        }
    }
}

void TransportRouter::AddBusEdgesForDirection(const Bus& bus, const std::vector<const Stop*>& stops, bool forward) {
    const size_t size = stops.size();
    const size_t start = forward ? 0 : size - 1;
    const size_t end = forward ? size : 0;
    const int step = forward ? 1 : -1;

    for (size_t i = start; forward ? (i < end) : (i > end); i += step) {
        int distance = 0;
        for (size_t j = i + step; forward ? (j < end) : (j > end); j += step) {
            distance += forward 
                ? catalogue_.GetStopDistance(stops[j-1]->name_, stops[j]->name_)
                : catalogue_.GetStopDistance(stops[j+1]->name_, stops[j]->name_);
            AddBusEdge(bus, i, j, distance);
        }
    }
}

void TransportRouter::AddBusEdge(const Bus& bus, size_t from_idx, size_t to_idx, int distance) {
    const Stop* from_stop = bus.GetStops()[from_idx];
    const Stop* to_stop = bus.GetStops()[to_idx];
    
    double time = ComputeBusTime(distance);
    int span_count = abs(static_cast<int>(to_idx) - static_cast<int>(from_idx));
    
    graph::Edge<double> edge{
        stop_to_vertex_bus_.at(from_stop->name_),
        stop_to_vertex_wait_.at(to_stop->name_),
        time
    };
    
    auto edge_id = graph_->AddEdge(edge);
    edge_to_bus_item_[edge_id] = RouteInfo::BusItem{
        bus.GetName(),
        span_count,
        time
    };
}

std::optional<RouteInfo> TransportRouter::FindRoute(const std::string_view from, const std::string_view to) const {
    auto from_it = stop_to_vertex_wait_.find(from);
    auto to_it = stop_to_vertex_wait_.find(to);
    if (from_it == stop_to_vertex_wait_.end() || to_it == stop_to_vertex_wait_.end()) {
        return std::nullopt;
    }

    if (!router_) return std::nullopt;

    auto route_info = router_->BuildRoute(from_it->second, to_it->second);
    if (!route_info) return std::nullopt;

    RouteInfo result;
    result.total_time = route_info->weight;
    result.items.reserve(route_info->edges.size() * 2);

    for (size_t i = 0; i < route_info->edges.size(); ++i) {
        auto edge_id = route_info->edges[i];
        
        if (auto wait_it = edge_to_wait_item_.find(edge_id); wait_it != edge_to_wait_item_.end()) {
            result.items.push_back(wait_it->second);
        }
        else if (auto bus_it = edge_to_bus_item_.find(edge_id); bus_it != edge_to_bus_item_.end()) {
            result.items.push_back(bus_it->second);
        }
    }

    for (size_t i = 1; i < result.items.size(); ++i) {
        if (holds_alternative<RouteInfo::BusItem>(result.items[i-1]) &&
            holds_alternative<RouteInfo::BusItem>(result.items[i])) {
            
            const auto& prev_edge = graph_->GetEdge(route_info->edges[i-1]);
            const std::string& stop_name = vertex_to_stop_.at(prev_edge.to);
            
            result.items.insert(
                result.items.begin() + i,
                RouteInfo::WaitItem{stop_name, static_cast<double>(settings_.bus_wait_time)}
            );
            ++i;
        }
    }

    return result;
}

} // namespace transport
