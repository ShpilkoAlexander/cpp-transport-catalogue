#include "map_renderer.h"


namespace renderer {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

// Проецирует широту и долготу в координаты внутри SVG-изображения
svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

void MapRenderer::SetBuses(std::vector<const Bus*> buses) {
    buses_ = buses;
}

void MapRenderer::SetStops(std::vector<const Stop*> stops) {
    stops_ = stops;
}

void MapRenderer::SetCoordinates(std::vector<geo::Coordinates> coordinates) {
    coordinates_ = coordinates;
}

void MapRenderer::RenderRoute() {

    // Создаём проектор сферических координат на карту
   const SphereProjector proj{
        coordinates_.begin(), coordinates_.end()
                , render_settings_.width
                , render_settings_.height
                , render_settings_.padding
    };

    size_t colar_num = 0;
    // Отрисовка маршрутов
    for (const auto bus : buses_) {
        std::vector<svg::Point> result_coords;
        for (auto stop : bus->stops) {
            result_coords.emplace_back(proj(stop->coordinates));
        }
        map_.emplace_back(std::make_unique<Route>(result_coords, render_settings_, colar_num++));
    }

    colar_num = 0;
    // Отрисовка названий маршрутов
    for (const auto bus : buses_) {
        map_.emplace_back(std::make_unique<RouteName>(bus, proj(bus->stops.back()->coordinates), render_settings_, colar_num++));
        if (!bus->is_roundtrip) {
            auto stop = bus->stops.at(bus->stops.size() / 2);
            if (stop->stopname != bus->stops.back()->stopname) {
            map_.emplace_back(std::make_unique<RouteName>(bus, proj(stop->coordinates), render_settings_, colar_num - 1));
            }
        }
    }

    // Отрисовка сиволов остановок
    for (const auto stop : stops_) {
        map_.emplace_back(std::make_unique<StopSymbol>(proj(stop->coordinates), render_settings_));
    }

    // Отрисовка названия остановок
    for (const auto stop : stops_) {
        map_.emplace_back(std::make_unique<StopName>(stop->stopname, proj(stop->coordinates), render_settings_));
    }
}

svg::Document MapRenderer::RenderMap() {

    svg::Document doc;
    DrawPicture(map_, doc);

    return doc;
}

void Route::Draw(svg::ObjectContainer& container) const {
    svg::Polyline route;
    route.SetFillColor(svg::NoneColor);
    route.SetStrokeColor(render_settings_.color_palette.at(colar_num % render_settings_.color_palette.size()));
    route.SetStrokeWidth(render_settings_.line_width);
    route.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    route.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    for (const auto& stop : stops_) {
        route.AddPoint(stop);
    }
    container.Add(route);
}

void RouteName::Draw(svg::ObjectContainer& container) const {
    svg::Text substrate;
    svg::Text inscript;
    svg::Point offset(render_settings_.bus_label_offset.first, render_settings_.bus_label_offset.second);

    substrate.SetFillColor(render_settings_.underlayer_color);
    substrate.SetStrokeColor(render_settings_.underlayer_color);
    substrate.SetStrokeWidth(render_settings_.underlayer_width);
    substrate.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    substrate.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    substrate.SetPosition(stop_);
    substrate.SetOffset(offset);
    substrate.SetFontSize(render_settings_.bus_label_font_size);
    substrate.SetFontFamily("Verdana");
    substrate.SetFontWeight("bold");
    substrate.SetData(bus_->busname);

    inscript.SetFillColor(render_settings_.color_palette.at(colar_num_ % render_settings_.color_palette.size()));
    inscript.SetPosition(stop_);
    inscript.SetOffset(offset);
    inscript.SetFontSize(render_settings_.bus_label_font_size);
    inscript.SetFontFamily("Verdana");
    inscript.SetFontWeight("bold");
    inscript.SetData(bus_->busname);

    container.Add(substrate);
    container.Add(inscript);
}

void StopSymbol::Draw(svg::ObjectContainer& container) const {
    svg::Circle stop_symbol;

    stop_symbol.SetCenter(stop_);
    stop_symbol.SetRadius(render_settings_.stop_radius);
    stop_symbol.SetFillColor(svg::Color{"white"});

    container.Add(stop_symbol);
}

void StopName::Draw(svg::ObjectContainer& container) const {
    svg::Text substrate;
    svg::Text inscript;
    svg::Point offset(render_settings_.stop_label_offset.first, render_settings_.stop_label_offset.second);

    substrate.SetPosition(position_);
    substrate.SetOffset(offset);
    substrate.SetFontSize(render_settings_.stop_label_font_size);
    substrate.SetFontFamily("Verdana");
    substrate.SetData(stopname_);
    substrate.SetFillColor(render_settings_.underlayer_color);
    substrate.SetStrokeColor(render_settings_.underlayer_color);
    substrate.SetStrokeWidth(render_settings_.underlayer_width);
    substrate.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    substrate.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    inscript.SetPosition(position_);
    inscript.SetOffset(offset);
    inscript.SetFontSize(render_settings_.stop_label_font_size);
    inscript.SetFontFamily("Verdana");
    inscript.SetData(stopname_);
    inscript.SetFillColor(svg::Color{"black"});

    container.Add(substrate);
    container.Add(inscript);
}

} //rendere


