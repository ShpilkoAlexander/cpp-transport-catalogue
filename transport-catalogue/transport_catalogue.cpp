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



Stop* TransportCatalogue::AddStop(Stop stop) {
    stops_.push_back(stop);

    stopname_to_stop_[stops_.back().stopname] = &stops_.back();
    stopname_to_buses_[stops_.back().stopname];
    return &stops_.back();
}

const Stop* TransportCatalogue::FindStop(std::string_view stopname) const {
    const auto finded_stop = stopname_to_stop_.find(stopname);

    if (finded_stop == stopname_to_stop_.end()) {
        static Stop empty_stop;
        return &empty_stop;
    }
    return finded_stop->second;
}


void TransportCatalogue::AddBus(std::string busname, std::vector<std::string> stopnames) {
    buses_.push_back(Bus{});
    auto bus = &buses_.back();
    bus->busname = busname;

    for(const auto& stopname : stopnames) {
        const Stop* found_stop = FindStop(stopname);
        bus->stops.push_back(found_stop);

        stopname_to_buses_[found_stop->stopname].insert(bus->busname);
    }

    busname_to_bus_[buses_.back().busname] = bus;
}

const Bus* TransportCatalogue::FindBus(std::string_view busname) const {

    const auto finded_bus = busname_to_bus_.find(busname);
    if (finded_bus == busname_to_bus_.end()) {
        static Bus empty_bus;
        return &empty_bus;
    }
    return finded_bus->second;
}

BusInfo TransportCatalogue::GetBusInfo(std::string_view busname) const {
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

    Coordinates coordinate_from = (*bus->stops.begin())->coordinates;
    Coordinates coordinate_to;
    const Stop* stop_from = *bus->stops.begin();
    const Stop* stop_to;

    //Проход по каждой остановки начиная со второй, вычисление растояние и
    //добавление в уникальные остановки
    for (auto iter = std::next(bus->stops.begin()); iter != bus->stops.end(); ++iter) {
        coordinate_to = (*iter)->coordinates;
        straight_way += ComputeDistance(coordinate_from, coordinate_to);
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

StopInfo TransportCatalogue::GetStopInfo(std::string_view stopname) const {
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

void TransportCatalogue::SetDistancesToStops(PairStops pair_stops, size_t distance) {
    distances_to_stops_[pair_stops] = distance;

    PairStops reverse_pair(pair_stops.second, pair_stops.first);
    if (distances_to_stops_.find(reverse_pair) == distances_to_stops_.end()) {
        distances_to_stops_[std::move(reverse_pair)] = distance;
    }
}

