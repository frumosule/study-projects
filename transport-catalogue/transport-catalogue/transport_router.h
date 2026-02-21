#ifndef TRANSPORT_ROUTER_H
#define TRANSPORT_ROUTER_H

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace transport {

struct RoutingSettings {
    int bus_wait_time;      
    double bus_velocity;   
};

struct RouteInfo {
    struct WaitItem {
        std::string stop_name;
        double time;
    };

    struct BusItem {
        std::string bus_name;
        int span_count;    
        double time;       
    };

    using Item = std::variant<WaitItem, BusItem>;
    std::vector<Item> items;  
    double total_time;        
};

class TransportRouter {
public:
    TransportRouter(const TransportCatalogue& catalogue, const RoutingSettings& settings);

    TransportRouter(const TransportRouter&) = delete;
    TransportRouter& operator=(const TransportRouter&) = delete;

    std::optional<RouteInfo> FindRoute(std::string_view from, std::string_view to) const;

private:
    void BuildGraph();
    void InitializeStopVertices();
    void AddWaitEdges();
    void AddBusEdges();
    void AddBusEdgesForDirection(const Bus& bus, const std::vector<const Stop*>& stops, bool forward);
    void AddBusEdge(const Bus& bus, size_t from_idx, size_t to_idx, int distance);
    
    double ComputeBusTime(int distance) const;

    const TransportCatalogue& catalogue_;
    const RoutingSettings settings_;

    std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_;
    std::unique_ptr<graph::Router<double>> router_;

    std::unordered_map<std::string_view, graph::VertexId> stop_to_vertex_wait_;  
    std::unordered_map<std::string_view, graph::VertexId> stop_to_vertex_bus_;   
    std::unordered_map<graph::VertexId, std::string> vertex_to_stop_;

    std::unordered_map<graph::EdgeId, RouteInfo::WaitItem> edge_to_wait_item_;
    std::unordered_map<graph::EdgeId, RouteInfo::BusItem> edge_to_bus_item_;
};

} // namespace transport

#endif // TRANSPORT_ROUTER_H
