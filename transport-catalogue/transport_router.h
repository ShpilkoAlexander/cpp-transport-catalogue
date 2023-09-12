#pragma once

#include "transport_catalogue.h"
#include "graph.h"
#include "transport_router.h"
#include "router.h"

#include <string_view>
#include <map>

enum EdgeType {
    WAIT,
    BUS_T
};

struct EdgeInfo {
    std::string_view name;
    int span_count = 0;
    double time = 0.0;
    EdgeType type;
};

struct RouteInfo {
    std::vector<EdgeInfo> edge_info;
    double time;
};



class TransportRouter {
public:
    TransportRouter(size_t bus_wait_time, size_t bus_velocity, TransportCatalogue& db);

    std::optional<RouteInfo> SearchRoute(std::string_view from, std::string_view to) const;

private:
    size_t bus_wait_time_;
    double bus_velocity_;
    TransportCatalogue& db_;
    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    std::map<std::string_view, size_t> stopname_to_id_;
    std::map<size_t, EdgeInfo> edge_id_to_info_;

    EdgeInfo BuildEdgeInfo(std::string_view name, double time, EdgeType type, size_t span_count = 0);
    double ComputeStopsDistance(const Stop& stop_from, const Stop& stop_to) const;
    void AddWaitEdges();
    void AddBusesEdges();
    void AddEdgesForBus(const Bus& bus);
    template <typename It>
    void AddEdges(const It begin, const It end, std::string_view busname);
};



template <typename It>
void TransportRouter::AddEdges(const It begin, const It end, std::string_view busname) {

    auto distances = db_.GetDistancesToStops();
    auto iter_last_stop = std::prev(end);
    for(auto iter_stops = begin; iter_stops != iter_last_stop; ++iter_stops) {
        graph::Edge<double> edge;
        edge.from = stopname_to_id_.at((*iter_stops)->stopname) * 2 + 1;
        double time = 0.0;
        size_t span_count = 0;
        for(auto iter_sub_stops = next(iter_stops); iter_sub_stops != std::next(iter_last_stop); ++iter_sub_stops) {
            double distance = distances.at({*std::prev(iter_sub_stops), *iter_sub_stops});
            time += (distance / bus_velocity_);
            edge.to = stopname_to_id_.at((*iter_sub_stops)->stopname) * 2;
            edge.weight = time;

            edge_id_to_info_[graph_.AddEdge(edge)] = BuildEdgeInfo(busname, time, EdgeType::BUS_T, ++span_count);
        }
    }
}
