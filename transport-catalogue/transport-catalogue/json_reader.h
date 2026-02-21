// json_reader.h
#pragma once
#include "transport_catalogue.h"
#include "request_handler.h"
#include "json.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "transport_router.h" 

namespace json_reader {

enum class ObjectType
{
    Bus, Stop, Map, Route
};

struct Request {
    Request(int id, ObjectType type, std::string name) : id_(id), type_(type), name_(name) {}
    Request(ObjectType type, int id) : id_(id), type_(type) {}
    Request(ObjectType type, int id, std::string from, std::string to)
        : id_(id), type_(type), from_(std::move(from)), to_(std::move(to)) {}
    int id_;
    ObjectType type_;
    std::string name_;
    std::string from_;  
    std::string to_;
};

class JsonReader {
public:
    JsonReader(std::istream& input, std::ostream& output, transport::TransportCatalogue& catalogue)
        : input_(input), output_(output), catalogue_(catalogue) {}

    void Read();
    void AnswerToRequests() const;
    std::string RenderMap() const;

    map_renderer::MapDescription GetMapDescription() const {
        return map_description_;
    }

    void LoadMap(const std::string& map_str);    

private:
    void GetDescription(const json::Array& discription);
    void GetRequest(const json::Array& requests);
    void ProcessBus(const json::Dict& bus_map);
    void ProcessStopDistances(const json::Dict& stop_map);
    void ProcessStop(const json::Dict& stop_map);

    json::Dict CreateBusInfoDict(const Request& req) const;    
    json::Dict CreateStopInfoDict(const Request& req) const;    
    json::Dict CreateMapDict(const Request& req) const;
    svg::Color ParseColor(const json::Node& color_node) const;
    json::Dict CreateRouteDict(const Request& req) const;

    void GetRenderSettings(const json::Dict& dict);
    void GetRoutingSettings(const json::Dict& dict);

    std::vector<Request> requests_;

    std::istream& input_;
    std::ostream& output_;    
    transport::TransportCatalogue& catalogue_;    

    map_renderer::MapDescription map_description_;
    std::string map_str_;

    transport::RoutingSettings router_settings_;
    std::unique_ptr<transport::TransportRouter> router_;
};

} // namespace json_reader
