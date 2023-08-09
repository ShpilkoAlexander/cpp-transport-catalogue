#pragma once

#include "geo.h"
#include "svg.h"
#include "domain.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <memory>

namespace renderer {

struct RenderSettings {
    double width = 0.0; // ширина изображения (в пиксилях)
    double height = 0.0; // высота изображения (в пиксилях)

    double padding = 0.0; // отступ краёв карты от границ SVG-документа

    double line_width = 0.0; // толщина линий, которыми рисуются автобусные маршруты
    double stop_radius = 0.0; // радиус окружностей, которыми обозначаются остановки

    size_t bus_label_font_size = 0; // размер текста, которым написаны названия автобусных маршрутов
    std::pair<double, double> bus_label_offset; // смещение надписи с названием маршрута относительно координат конечной остановки на карте

    size_t stop_label_font_size = 0; // размер текста, которым отображаются названия остановок
    std::pair<double, double> stop_label_offset; // смещение названия остановки относительно её координат на карте

    svg::Color underlayer_color; // цвет подложки под названиями остановок и маршрутов
    double underlayer_width = 0.0; // толщина подложки под названиями остановок и маршрутов

    std::vector<svg::Color> color_palette; // цветовая палитра
};

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding);

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class Route : public svg::Drawable {
public:
    Route(const std::vector<svg::Point> stops, RenderSettings& render_settings, size_t colar_num)
        : stops_(stops)
        , render_settings_(render_settings)
        , colar_num(colar_num) {
    }

    // Реализует метод Draw интерфейса svg::Drawable
    void Draw(svg::ObjectContainer& container) const override;

private:
    const std::vector<svg::Point> stops_;
    RenderSettings& render_settings_;
    size_t colar_num;
};

class RouteName : public svg::Drawable {
public:
    RouteName(const Bus* bus, svg::Point stop, RenderSettings& render_settings, size_t colar_num)
        : bus_(bus)
        , stop_(stop)
        , render_settings_(render_settings)
        , colar_num_(colar_num) {
    }

    // Реализует метод Draw интерфейса svg::Drawable
    void Draw(svg::ObjectContainer& container) const override;

private:
    const Bus* bus_;
    svg::Point stop_;
    RenderSettings& render_settings_;
    size_t colar_num_;
};

class StopSymbol : public svg::Drawable {
public:
    StopSymbol(svg::Point stop, RenderSettings& render_settings)
        : stop_(stop)
        , render_settings_(render_settings) {
    }

    // Реализует метод Draw интерфейса svg::Drawable
    void Draw(svg::ObjectContainer& container) const override;

private:
    svg::Point stop_;
    RenderSettings& render_settings_;
};

class StopName : public svg::Drawable {
public:
    StopName(const std::string& stopname, svg::Point position, RenderSettings& render_settings)
        : stopname_(stopname)
        , position_(position)
        , render_settings_(render_settings) {
    }

    // Реализует метод Draw интерфейса svg::Drawable
    void Draw(svg::ObjectContainer& container) const override;

private:
    const std::string& stopname_;
    svg::Point position_;
    RenderSettings& render_settings_;
};


template <typename DrawableIterator>
void DrawPicture(DrawableIterator begin, DrawableIterator end, svg::ObjectContainer& target) {
    for (auto it = begin; it != end; ++it) {
        (*it)->Draw(target);
    }
}

template <typename Container>
void DrawPicture(const Container& container, svg::ObjectContainer& target) {
    using namespace std;
    DrawPicture(begin(container), end(container), target);
}

class MapRenderer {

public:
    MapRenderer(RenderSettings render_settings)
        : render_settings_(render_settings){
    }

    void SetBuses(std::vector<const Bus*> buses);
    void SetStops(std::vector<const Stop*> stops);
    void SetCoordinates(std::vector<geo::Coordinates> coords);
    void RenderRoute();

    svg::Document RenderMap();

private:
    RenderSettings render_settings_;
    std::vector<std::unique_ptr<svg::Drawable>> map_;
    std::vector<const Bus*> buses_;
    std::vector<const Stop*> stops_;
    std::vector<geo::Coordinates> coordinates_;

};






template <typename PointInputIt>
SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                double max_width, double max_height, double padding)
    : padding_(padding) //
{
    // Если точки поверхности сферы не заданы, вычислять нечего
    if (points_begin == points_end) {
        return;
    }

    // Находим точки с минимальной и максимальной долготой
    const auto [left_it, right_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    // Находим точки с минимальной и максимальной широтой
    const auto [bottom_it, top_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    // Вычисляем коэффициент масштабирования вдоль координаты x
    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    // Вычисляем коэффициент масштабирования вдоль координаты y
    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        // Коэффициенты масштабирования по ширине и высоте ненулевые,
        // берём минимальный из них
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
        // Коэффициент масштабирования по ширине ненулевой, используем его
        zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
        // Коэффициент масштабирования по высоте ненулевой, используем его
        zoom_coeff_ = *height_zoom;
    }
}


} //renderer
