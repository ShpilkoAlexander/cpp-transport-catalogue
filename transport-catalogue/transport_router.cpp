#include "transport_router.h"
#include "graph.h"
#include "router.h"
#include "domain.h"

#include <memory>


TransportRouter::TransportRouter(size_t bus_wait_time, size_t bus_velocity, TransportCatalogue& db)
    : bus_wait_time_(bus_wait_time)
    , bus_velocity_((bus_velocity * 1000.0) / 60)
    , db_(db)
    , graph_(db_.CountStops() * 2)
{
    AddWaitEdges();
    AddBusesEdges();
    router_ = std::make_unique<graph::Router<double>>((graph_));
}

std::optional<RouteInfo> TransportRouter::SearchRoute(std::string_view from, std::string_view to) const {
    auto route = router_->BuildRoute((stopname_to_id_.at(from) * 2), (stopname_to_id_.at(to) * 2));
    RouteInfo route_info;
    if (route == std::nullopt) {
        return std::nullopt;
    }
    for(const graph::EdgeId edge_id : route->edges) {
        route_info.edge_info.emplace_back(edge_id_to_info_.at(edge_id));
    }
    route_info.time = route->weight;
    return route_info;
}


EdgeInfo TransportRouter::BuildEdgeInfo(std::string_view name, double time, EdgeType type, size_t span_count) {
    EdgeInfo edge_info;
    edge_info.name = name;
    edge_info.time = time;
    edge_info.type = type;
    edge_info.span_count = span_count;

    return edge_info;
}

void TransportRouter::AddWaitEdges() {
    size_t stop_num = 0;
    for(const Stop& stop : db_.GetStops()) {
        graph::Edge<double> edge;

        edge.from = stop_num * 2;
        edge.to = stop_num * 2 + 1;
        edge.weight = bus_wait_time_;

        edge_id_to_info_[graph_.AddEdge(edge)] = BuildEdgeInfo(stop.stopname, bus_wait_time_, EdgeType::WAIT);
        stopname_to_id_[stop.stopname] = stop_num++;
    }
}

void TransportRouter::AddBusesEdges() {
    for(const Bus& bus : db_.GetBuses()) {
        AddEdgesForBus(bus);
    }
}

void TransportRouter::AddEdgesForBus(const Bus& bus) {
    if (bus.is_roundtrip == true) {
        AddEdges(bus.stops.begin(), bus.stops.end(), bus.busname);
    }
    else {
        AddEdges(bus.stops.begin(), bus.stops.begin() + (bus.stops.size()/2 + 1), bus.busname);
        AddEdges(bus.stops.begin() + (bus.stops.size()/2), bus.stops.end(), bus.busname);
    }
}

//Проход по каждой остановки начиная со второй, вычисление растояние
double TransportRouter::ComputeStopsDistance(const Stop& stop_from, const Stop& stop_to) const {
    return geo::ComputeDistance(stop_from.coordinates, stop_to.coordinates);
}

