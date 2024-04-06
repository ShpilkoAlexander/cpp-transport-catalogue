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
    svg::Document RenderMap() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const TransportCatalogue& db_;
    renderer::MapRenderer& renderer_;

    void LoadBusesAndCoordinates() const;
};

