#pragma once
#include "svg.h"
#include <algorithm>
#include "geo.h"
#include "transport_catalogue.h"


namespace json_reader {
class JsonReader;
}

namespace map_renderer {

struct MapDescription {
    double width_{};
    double height_{};

    double padding_{};

    double line_width_{};
    double stop_radius_{};

    int bus_label_font_size_{};
    svg::Point bus_label_offset_{}; 

    int stop_label_font_size_{};
    svg::Point stop_label_offset_{};  

    svg::Color underlayer_color_{};
    double underlayer_width_{};

    std::vector<svg::Color> color_palette_{};
};

inline const double EPSILON = 1e-6;
inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding)
        : padding_(padding) 
    {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public: 

    MapRenderer(json_reader::JsonReader& reader, transport::TransportCatalogue& db);

    MapDescription GetMapDescription() const;
    svg::Point Convert(geo::Coordinates coordinates) const;

    svg::Document RenderMap() const;
    svg::Text CreateBaseText(const std::string& data, svg::Point position, const map_renderer::MapDescription& map_description) const;
    svg::Text CreateUnderlayer(const svg::Text& base_text, const map_renderer::MapDescription& map_description) const;
    svg::Text CreateBusLabel(const std::string& name, svg::Point position, const svg::Color& color, const map_renderer::MapDescription& map_description) const;

private:

    void InitProjector();

    std::vector<const transport::Bus*> GetSortedBuses() const;
    void RenderRoutes(svg::Document& doc, const map_renderer::MapDescription& map_description) const; // вспомогательная фнукия
    void RenderBusNames(svg::Document& doc, const map_renderer::MapDescription& map_description) const;
    void RenderStops(svg::Document& doc, const map_renderer::MapDescription& map_description) const;
    void RenderStopNames(svg::Document& doc, const map_renderer::MapDescription& map_description) const;

    std::optional<SphereProjector> projector_;   
    json_reader::JsonReader& reader_;
    transport::TransportCatalogue& db_;    
};

} 
