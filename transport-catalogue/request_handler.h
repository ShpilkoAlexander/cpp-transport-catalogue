#pragma once

#include "transport_catalogue.h"
#include "svg.h"
#include "map_renderer.h"

#include <string>
#include <set>
#include <optional>
#include <algorithm>
#include <iostream>

class RequestHandler {
public:
    RequestHandler(const TransportCatalogue& db, renderer::MapRenderer& renderer);

    // Возвращает информацию о маршруте (запрос Bus)
//    std::optional<BusInfo> GetBusStat(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через
//    const std::set<std::string_view> GetBusesByStop(const std::string_view& stop_name) const;

    svg::Document RenderMap() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const TransportCatalogue& db_;
    renderer::MapRenderer& renderer_;

    void LoadBusesAndCoordinates() const;
};

