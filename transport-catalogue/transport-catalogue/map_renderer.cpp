#include "map_renderer.h"
#include "geo.h"
#include <algorithm>
#include <unordered_set>
#include "json_reader.h"

using namespace map_renderer;
using namespace transport;
using namespace svg;

MapRenderer::MapRenderer(json_reader::JsonReader& reader, TransportCatalogue& db) 
    : reader_(reader), db_(db) {
    InitProjector();
}

MapDescription MapRenderer::GetMapDescription() const {
    return reader_.GetMapDescription();
}

svg::Point MapRenderer::Convert(geo::Coordinates coordinates) const {
    return (*projector_)(coordinates);
}

std::vector<const transport::Bus*> MapRenderer::GetSortedBuses() const {
    std::vector<const Bus*> buses;
    const auto& all_buses = db_.GetAllBuses();
    buses.reserve(all_buses.size());
    
    for (const auto& bus : all_buses) {
        if (!bus.GetStops().empty()) {
            buses.push_back(&bus);
        }
    }

    std::sort(buses.begin(), buses.end(), BusComparator());
    return buses;
}

void MapRenderer::InitProjector() {
    const auto& all_buses = db_.GetAllBuses();
    std::vector<geo::Coordinates> all_coordinates;
    
    for (const auto& bus : all_buses) {
        const auto& stops = bus.GetStops();
        for (const Stop* stop_ptr : stops) {
            all_coordinates.push_back(stop_ptr->coordinates_);
        }
    }

    const MapDescription& md = GetMapDescription();
    SphereProjector projector(all_coordinates.begin(), all_coordinates.end(), md.width_, md.height_, md.padding_);
    projector_ = projector;
}

svg::Text MapRenderer::CreateBaseText(const std::string& data, svg::Point position, const MapDescription& map_description) const {
        return Text()
        .SetData(data)
        .SetPosition(position)
        .SetOffset(map_description.stop_label_offset_)
        .SetFontSize(map_description.stop_label_font_size_)
        .SetFontFamily("Verdana");
    }

svg::Text MapRenderer::CreateUnderlayer(const Text& base_text, const MapDescription& map_description) const {
    return Text(base_text)
        .SetFillColor(map_description.underlayer_color_)
        .SetStrokeColor(map_description.underlayer_color_)
        .SetStrokeWidth(map_description.underlayer_width_)
        .SetStrokeLineCap(StrokeLineCap::ROUND)
        .SetStrokeLineJoin(StrokeLineJoin::ROUND);
}

svg::Text MapRenderer::CreateBusLabel(const std::string& name, svg::Point position, const Color& color, const MapDescription& map_description) const {
    return CreateBaseText(name, position, map_description)
            .SetFontWeight("bold")
            .SetFillColor(color);
}

void MapRenderer::RenderRoutes(Document& doc, const map_renderer::MapDescription& map_description) const {
    const std::vector<Color>& array_of_colors = map_description.color_palette_;
    auto iterator = array_of_colors.begin();

    const std::vector<const transport::Bus*>& sorted_buses_ptrs = GetSortedBuses();
    for (const Bus* bus_ptr : sorted_buses_ptrs) {
        Polyline polyline;
        polyline.SetStrokeColor(*iterator).SetFillColor(svg::NoneColor).SetStrokeWidth(map_description.line_width_)
            .SetStrokeLineCap(StrokeLineCap::ROUND).SetStrokeLineJoin(StrokeLineJoin::ROUND);

        std::vector<const Stop*> stop_ptrs = bus_ptr->GetStops();

        for (const Stop* stop : stop_ptrs) {
            polyline.AddPoint(Convert(stop->coordinates_));
        }

        doc.Add(polyline);

        if (iterator + 1 == array_of_colors.end()) {
            iterator = array_of_colors.begin();
        }
        else {
            ++iterator;
        }
    }
}

void MapRenderer::RenderBusNames(svg::Document& doc, const map_renderer::MapDescription& map_description) const {
    std::vector<const Bus*> sorted_buses_ptrs = GetSortedBuses();

    auto cyclic_color_it = map_description.color_palette_.begin();

    for (const Bus* bus_ptr : sorted_buses_ptrs) {
        const std::vector<const Stop*>& stops = bus_ptr->GetStops();
        const Stop* first_stop = stops.front();

        Text label;

        label.SetData(std::string(bus_ptr->GetName()))
            .SetPosition(Convert(first_stop->coordinates_))
            .SetOffset(map_description.bus_label_offset_)
            .SetFontSize(map_description.bus_label_font_size_)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold");

        Text underlayer = label;
        underlayer.SetFillColor(map_description.underlayer_color_)
            .SetStrokeColor(map_description.underlayer_color_)
            .SetStrokeWidth(map_description.underlayer_width_)
            .SetStrokeLineCap(StrokeLineCap::ROUND)
            .SetStrokeLineJoin(StrokeLineJoin::ROUND);

        label.SetFillColor(*cyclic_color_it);

        doc.Add(underlayer);
        doc.Add(label);

        if (!(bus_ptr->is_round()) && stops[stops.size() / 2] != stops[0]) {
            size_t last_stop_pos = std::ceil(stops.size() / 2.0);
            const Stop* last_stop = stops[last_stop_pos - 1];

            label.SetPosition(Convert(last_stop->coordinates_));
            underlayer.SetPosition(Convert(last_stop->coordinates_));
            doc.Add(std::move(underlayer));
            doc.Add(std::move(label));
        }
        if (++cyclic_color_it == map_description.color_palette_.end()) {
            cyclic_color_it = map_description.color_palette_.begin();
        }
    }
}

void MapRenderer::RenderStops(svg::Document& doc, const map_renderer::MapDescription& map_description) const {
    std::set<const Stop*, StopComparator> stops_to_draw;
    const auto& all_buses = db_.GetAllBuses();
    
    for (const auto& bus : all_buses) {
        const auto& stops = bus.GetStops();
        stops_to_draw.insert(stops.begin(), stops.end());
    }
    
    for (const Stop* stop_ptr : stops_to_draw) {
        Circle stop;
        stop.SetCenter(Convert(stop_ptr->coordinates_))
           .SetRadius(map_description.stop_radius_)
           .SetFillColor("white");
        doc.Add(std::move(stop));
    }
}

void MapRenderer::RenderStopNames(svg::Document& doc, const map_renderer::MapDescription& map_description) const {
    std::set<const Stop*, StopComparator> stops_to_draw;
    const auto& all_buses = db_.GetAllBuses();
    
    for (const auto& bus : all_buses) {
        const auto& stops = bus.GetStops();
        stops_to_draw.insert(stops.begin(), stops.end());
    }
    
    for (const Stop* stop_ptr : stops_to_draw) {
        Text stop_label = CreateBaseText(stop_ptr->name_, 
                                       Convert(stop_ptr->coordinates_), 
                                       map_description);
        stop_label.SetFillColor("black");
        
        Text underlayer = CreateUnderlayer(stop_label, map_description);
        
        doc.Add(underlayer);
        doc.Add(stop_label);
    }
}

svg::Document MapRenderer::RenderMap() const {
    svg::Document doc;
    const MapDescription& map_description = GetMapDescription();

    RenderRoutes(doc, map_description);
    RenderBusNames(doc, map_description);
    RenderStops(doc, map_description);
    RenderStopNames(doc, map_description);
    return doc;
}
