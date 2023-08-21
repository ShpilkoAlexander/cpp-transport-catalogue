#include "request_handler.h"

RequestHandler::RequestHandler(const TransportCatalogue& db, renderer::MapRenderer& renderer)
    : db_(db), renderer_(renderer) {
}

svg::Document RequestHandler::RenderMap() const {
    LoadBusesAndCoordinates();
    renderer_.RenderRoute();
    return renderer_.RenderMap();
}

void RequestHandler::LoadBusesAndCoordinates() const {
    std::vector<const Bus*> buses;
    std::set<const Stop*> stops_set;
    std::vector<geo::Coordinates> coordinates;

    for (const auto& [key, value] : *db_.GetBusnameToBus()) {
        buses.emplace_back(value);
        for (auto const stop : value->stops) {
            stops_set.insert(stop);
            coordinates.emplace_back(stop->coordinates);   
        }
    }
    std::sort(buses.begin(), buses.end(), []
            (const auto lhs, const auto rhs) {
                return lhs->busname < rhs->busname;
    });
    std::vector<const Stop*> stops(stops_set.begin(), stops_set.end());
    std::sort(stops.begin(), stops.end(), []
            (const auto lhs, const auto rhs) {
                return lhs->stopname < rhs->stopname;
    });

    renderer_.SetBuses(buses);
    renderer_.SetStops(stops);
    renderer_.SetCoordinates(coordinates);
}
