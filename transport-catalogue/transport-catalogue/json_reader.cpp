#include "json_reader.h"
#include "transport_router.h" 
#include <sstream> 

using namespace transport;
using namespace json;
using namespace json_reader;
using namespace std::literals;

json::Dict JsonReader::CreateBusInfoDict(const Request& req) const {
    using namespace json;
    
    const Bus* bus = catalogue_.GetBus(req.name_);
    
    Builder builder;
    builder.StartDict()
        .Key("request_id").Value(req.id_);
    
    if (bus != nullptr) {
        BusInfo info = bus->GetInfo(catalogue_);
        builder.Key("curvature").Value(info.curvature_)
               .Key("route_length").Value(info.length_)
               .Key("stop_count").Value(static_cast<int>(info.total_stops_))
               .Key("unique_stop_count").Value(static_cast<int>(info.unique_));
    } else {
        builder.Key("error_message").Value("not found");
    }
    
    builder.EndDict();
    return builder.Build().AsDict();
}

json::Dict JsonReader::CreateStopInfoDict(const Request& req) const {
    using namespace json;
    
    const Stop* stop = catalogue_.GetStop(req.name_);
    
    Builder builder;
    builder.StartDict()
        .Key("request_id").Value(req.id_);
    
    if (stop != nullptr) {
        const auto& buses = catalogue_.GetBusesByStop(stop);
        builder.Key("buses");
        
        if (buses.empty()) {
            builder.StartArray().EndArray();
        } else {
            builder.StartArray();
            for (const Bus* bus : buses) {
                builder.Value(std::string(bus->GetName()));
            }
            builder.EndArray();
        }
    } else {
        builder.Key("error_message").Value("not found");
    }
    
    builder.EndDict();
    return builder.Build().AsDict();
}


json::Dict JsonReader::CreateMapDict(const Request& req) const {
    using namespace json;
    
    Builder builder;
    builder.StartDict()
        .Key("request_id").Value(req.id_)
        .Key("map").Value(RenderMap())
        .EndDict();
    
    return builder.Build().AsDict();
}

void JsonReader::LoadMap(const std::string& map_str) {
    map_str_ = map_str;
}

void JsonReader::ProcessStop(const json::Dict& stop_map) {
    std::string name = stop_map.at("name").AsString();
    geo::Coordinates coordinates{
        .lat = stop_map.at("latitude").AsDouble(),
        .lng = stop_map.at("longitude").AsDouble()
    };
    catalogue_.AddStop(Stop(std::move(name), coordinates));
}

void JsonReader::ProcessStopDistances(const json::Dict& stop_map) {
    std::string_view main_stop = stop_map.at("name").AsString();
    Dict distances = stop_map.at("road_distances").AsDict();
    for (const auto& [stop, distance] : distances) {
        catalogue_.SetStopDistance(main_stop, stop, distance.AsInt());
    }
}

void JsonReader::ProcessBus(const json::Dict& bus_map) {
    std::string name = bus_map.at("name").AsString();
    std::vector<const Stop*> stop_ptrs;
    Array node_stops = bus_map.at("stops").AsArray();
    
    for (const Node& node : node_stops) {
        const Stop* stop_ptr = catalogue_.GetStop(node.AsString());
        stop_ptrs.push_back(stop_ptr);
    }
    
    Type type = bus_map.at("is_roundtrip").AsBool() ? Type::RING : Type::NONRING;
    
    if (type == Type::NONRING && !stop_ptrs.empty()) {
        for (int i = stop_ptrs.size() - 2; i >= 0; --i) {
            stop_ptrs.push_back(stop_ptrs[i]);
        }
    }
    
    catalogue_.AddBus(Bus(std::move(name), stop_ptrs, type));
}

void JsonReader::GetDescription(const Array& description) {
    std::vector<Dict> stops_buffer;
    std::vector<Dict> bus_buffer;
    
    for (const Node& node : description) {
        Dict map = node.AsDict();
        if (map.at("type").AsString() == "Stop") {
            stops_buffer.push_back(std::move(map));
        } else {
            bus_buffer.push_back(std::move(map));
        }
    }
    
    for (const Dict& stop_map : stops_buffer) {
        ProcessStop(stop_map);
    }
    
    for (const Dict& stop_map : stops_buffer) {
        ProcessStopDistances(stop_map);
    }
    
    for (const Dict& bus_map : bus_buffer) {
        ProcessBus(bus_map);
    }
}

void JsonReader::GetRequest(const Array& requests) {    
    for (const Node& node : requests) {
        Dict map = node.AsDict();
        int id = map.at("id").AsInt();        
        std::string_view type_str = map.at("type").AsString();
        
        if (type_str == "Map"sv) {
            requests_.emplace_back(ObjectType::Map, id);
        } else if (type_str == "Route"sv) {
            requests_.emplace_back(
                ObjectType::Route, 
                id,
                map.at("from").AsString(),
                map.at("to").AsString()
            );
        } else {            
            ObjectType type = (type_str == "Stop"sv) ? ObjectType::Stop : ObjectType::Bus;
            std::string name = map.at("name").AsString();
            requests_.emplace_back(id, type, std::move(name));
        }
    }
}

void JsonReader::AnswerToRequests() const {
    using namespace json;
    
    Builder builder;
    auto array_context = builder.StartArray();
    
    for (const auto& request : requests_) {
        switch (request.type_) {
            case ObjectType::Bus:
                array_context.Value(CreateBusInfoDict(request));
                break;
            case ObjectType::Stop:
                array_context.Value(CreateStopInfoDict(request));
                break;
            case ObjectType::Map:
                array_context.Value(CreateMapDict(request));
                break;
            case ObjectType::Route:
                array_context.Value(CreateRouteDict(request));
                break;
        }
    }
    
    array_context.EndArray();
    
    json::Print(json::Document(builder.Build()), output_);
}

svg::Color JsonReader::ParseColor(const json::Node& color_node) const {
    if (color_node.IsArray()) {
        auto color_arr = color_node.AsArray();
        int red = color_arr[0].AsInt();
        int green = color_arr[1].AsInt();
        int blue = color_arr[2].AsInt();
        
        if (color_arr.size() == 3) {
            return svg::Rgb(red, green, blue);
        } else {
            double opacity = color_arr[3].AsDouble();
            return svg::Rgba(red, green, blue, opacity);
        }
    }
    return color_node.AsString();
}

void JsonReader::GetRenderSettings(const Dict& dict) {
    double width = dict.at("width").AsDouble();
    double height = dict.at("height").AsDouble();
    double padding = dict.at("padding").AsDouble();
    double line_width = dict.at("line_width").AsDouble();
    double stop_radius = dict.at("stop_radius").AsDouble();
    int bus_label_font_size = dict.at("bus_label_font_size").AsInt();
    
    Array bus_label_offset_arr = dict.at("bus_label_offset").AsArray();
    svg::Point bus_label_offset(bus_label_offset_arr[0].AsDouble(), bus_label_offset_arr[1].AsDouble());

    int stop_label_font_size = dict.at("stop_label_font_size").AsInt();

    Array stop_label_offset_arr = dict.at("stop_label_offset").AsArray();
    svg::Point stop_label_offset(stop_label_offset_arr[0].AsDouble(), stop_label_offset_arr[1].AsDouble());

    svg::Color underlayer_color = ParseColor(dict.at("underlayer_color"));
    double underlayer_width = dict.at("underlayer_width").AsDouble();

    Array color_palette_arr = dict.at("color_palette").AsArray();    
    std::vector<svg::Color> color_palette;
    for (const Node& node : color_palette_arr) {
        color_palette.push_back(ParseColor(node));
    }

    map_renderer::MapDescription result{
        .width_ = width,
        .height_ = height,
        .padding_ = padding,
        .line_width_ = line_width,
        .stop_radius_ = stop_radius,
        .bus_label_font_size_ = bus_label_font_size,
        .bus_label_offset_ = bus_label_offset,
        .stop_label_font_size_ = stop_label_font_size,
        .stop_label_offset_ = stop_label_offset,
        .underlayer_color_ = underlayer_color,
        .underlayer_width_ = underlayer_width,
        .color_palette_ = color_palette
    };
    map_description_ = std::move(result);
}

std::string JsonReader::RenderMap() const {
    map_renderer::MapRenderer renderer(*this, catalogue_);
    svg::Document doc = renderer.RenderMap();
    std::ostringstream svg_stream;
    doc.Render(svg_stream);
    return svg_stream.str();
}

void JsonReader::GetRoutingSettings(const json::Dict& dict) {
    router_settings_.bus_wait_time = dict.at("bus_wait_time").AsInt();
    router_settings_.bus_velocity = dict.at("bus_velocity").AsDouble();
}

json::Dict JsonReader::CreateRouteDict(const Request& req) const {
    using namespace json;
    using transport::RouteInfo;
    
    if (!router_) {
        return Builder{}
            .StartDict()
                .Key("request_id").Value(req.id_)
                .Key("error_message").Value("not found")
            .EndDict()
            .Build().AsDict();
    }
    
    auto route_info = router_->FindRoute(req.from_, req.to_);
    
    Builder builder;
    builder.StartDict()
        .Key("request_id").Value(req.id_);
    
    if (route_info) {
        builder.Key("total_time").Value(route_info->total_time)
               .Key("items").StartArray();
        
        for (const auto& item : route_info->items) {
            if (std::holds_alternative<RouteInfo::WaitItem>(item)) {
                const auto& wait_item = std::get<RouteInfo::WaitItem>(item);
                builder.StartDict()
                    .Key("type").Value("Wait")
                    .Key("stop_name").Value(wait_item.stop_name)
                    .Key("time").Value(wait_item.time)
                    .EndDict();
            } else {
                const auto& bus_item = std::get<RouteInfo::BusItem>(item);
                builder.StartDict()
                    .Key("type").Value("Bus")
                    .Key("bus").Value(bus_item.bus_name)
                    .Key("span_count").Value(bus_item.span_count)
                    .Key("time").Value(bus_item.time)
                    .EndDict();
            }
        }
        
        builder.EndArray();
    } else {
        std::cerr << "Something wrong here" << std::endl;
        builder.Key("error_message").Value("not found");
    }
    
    builder.EndDict();
    return builder.Build().AsDict();
}

void JsonReader::Read() {
    Document doc = Load(input_);
    Dict map = doc.GetRoot().AsDict();

    GetDescription(map.at("base_requests").AsArray());
    GetRenderSettings(map.at("render_settings").AsDict());
    
    if (map.count("routing_settings")) {
        GetRoutingSettings(map.at("routing_settings").AsDict());
        router_ = std::make_unique<transport::TransportRouter>(catalogue_, router_settings_);
    }    
    GetRequest(map.at("stat_requests").AsArray());
}
