#include "transport_catalogue.h"
#include "geo.h"

#include <string>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <iostream>
#include <iomanip>



void TransportCatalogue::AddStop(const Stop& stop, DistancesToStops& distance_to_stops) {
    stops_.push_back(stop);

    stopname_to_stop_[stops_.back().stopname] = &stops_.back();
    stopname_to_buses_[stops_.back().stopname];

    StopsWithDistances stops_with_distance;

    stops_with_distance.stop = &stops_.back();
    stops_with_distance.distances = distance_to_stops;

    stops_with_distance_.push_back(std::move(stops_with_distance));
}

const Stop* TransportCatalogue::FindStop(std::string_view stopname) const {
    const auto finded_stop = stopname_to_stop_.find(stopname);

    if (finded_stop == stopname_to_stop_.end()) {
        static Stop empty_stop;
        return &empty_stop;
    }
    return finded_stop->second;
}


void TransportCatalogue::AddBus(std::string_view busname, std::vector<std::string>& stopnames, bool is_roundtrip) {
    buses_.push_back(Bus{});
    auto bus = &buses_.back();
    bus->busname = busname;
    bus->is_roundtrip = is_roundtrip;

    for(const auto& stopname : stopnames) {
        const Stop* found_stop = FindStop(stopname);
        bus->stops.push_back(found_stop);

        stopname_to_buses_[found_stop->stopname].insert(bus->busname);
    }

    busname_to_bus_[buses_.back().busname] = bus;
}

void TransportCatalogue::DistanceAdd() {
   for (const auto& stop_distances : stops_with_distance_) {
       if (stop_distances.distances.empty()) {
           continue;
       }
       StopsDistancesAdd(stop_distances.stop, stop_distances.distances);
   }
}

void TransportCatalogue::StopsDistancesAdd(const Stop* stop, const DistancesToStops&  distances) {

    for (const auto& [stopname, distance] : distances) {
        SetDistancesToStops(stop, FindStop(stopname), distance);
    }
}

const Bus* TransportCatalogue::FindBus(std::string_view busname) const {

    const auto finded_bus = busname_to_bus_.find(busname);
    if (finded_bus == busname_to_bus_.end()) {
        static Bus empty_bus;
        return &empty_bus;
    }
    return finded_bus->second;
}

const BusInfo TransportCatalogue::GetBusInfo(std::string_view busname) const {
    const Bus* bus = FindBus(busname);
    if (*bus == Bus{}) {
        BusInfo bus_info;
        bus_info.busname = busname;
        bus_info.is_found = false;
        return bus_info;
    }

    std::unordered_set<const Stop*> uniq_stops = {*bus->stops.begin()};

    size_t route_len = 0;
    double straight_way = 0.0;

    geo::Coordinates coordinate_from = (*bus->stops.begin())->coordinates;
    geo::Coordinates coordinate_to;
    const Stop* stop_from = *bus->stops.begin();
    const Stop* stop_to;

    //Проход по каждой остановки начиная со второй, вычисление растояние и
    //добавление в уникальные остановки
    for (auto iter = std::next(bus->stops.begin()); iter != bus->stops.end(); ++iter) {
        coordinate_to = (*iter)->coordinates;
        straight_way += geo::ComputeDistance(coordinate_from, coordinate_to);
        coordinate_from = coordinate_to;

        stop_to = *iter;
        route_len += distances_to_stops_.at(PairStops{stop_from, stop_to});
        stop_from = stop_to;

        uniq_stops.insert(*iter);
    }

    BusInfo bus_info;

    bus_info.busname = std::string(busname);
    bus_info.stops_count = bus->stops.size();
    bus_info.uniq_stops_count = uniq_stops.size();
    bus_info.route_len = route_len;
    bus_info.curvature = route_len / straight_way;

    return bus_info;
}

const StopInfo TransportCatalogue::GetStopInfo(std::string_view stopname) const {
    auto found_stop = stopname_to_buses_.find(stopname);
    StopInfo stop_info;
    stop_info.stopname = stopname;
    if (found_stop == stopname_to_buses_.end()) {
        stop_info.is_found = false;
        return stop_info;
    }

    stop_info.buses = found_stop->second;

    return stop_info;
}

void TransportCatalogue::SetDistancesToStops(const Stop* stop_from, const Stop* stop_to, size_t distance) {
    distances_to_stops_[{stop_from, stop_to}] = distance;

    PairStops reverse_pair(stop_to, stop_from);
    if (distances_to_stops_.find(reverse_pair) == distances_to_stops_.end()) {
        distances_to_stops_[std::move(reverse_pair)] = distance;
    }
}

const std::unordered_map<std::string_view, const Bus*>* TransportCatalogue::GetBusnameToBus() const {
    return &busname_to_bus_;
}

