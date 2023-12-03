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

void TransportCatalogue::AddStop(const Stop& stop) {
    stops_.push_back(stop);

    stopname_to_stop_[stops_.back().stopname] = &stops_.back();
    stopname_to_buses_[stops_.back().stopname];
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

void TransportCatalogue::AddBus(Bus add_bus) {
    buses_.push_back(add_bus);
    auto bus = &buses_.back();

    for(const auto& stop : bus->stops) {
        stopname_to_buses_[stop->stopname].insert(bus->busname);
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

const std::deque<Stop>& TransportCatalogue::GetStops() const {
    return stops_;
}

size_t TransportCatalogue::CountStops() const {
    return stops_.size();
}

const std::deque<Bus>& TransportCatalogue::GetBuses() const {
    return buses_;
}

void TransportCatalogue::SetDistancesToStops(std::unordered_map<PairStops, size_t, PairStopsHasher> distances_to_stops) {
    distances_to_stops_ = distances_to_stops;
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

const std::unordered_map<PairStops, size_t, PairStopsHasher>& TransportCatalogue::GetDistancesToStops() const{
    return distances_to_stops_;
}


const std::unordered_map<std::string_view, const Bus*>* TransportCatalogue::GetBusnameToBus() const {
    return &busname_to_bus_;
}


std::vector<const Stop*> TransportCatalogue::SortStops() const {
    std::vector<const Stop*> sorted_stops;
    sorted_stops.reserve(stops_.size());
    for (const Stop& stop : stops_) {
        sorted_stops.push_back(&stop);
    }
    std::sort(sorted_stops.begin(), sorted_stops.end(),
        [](const Stop* lhs, const Stop* rhs) {return lhs->stopname < rhs->stopname; });
    return sorted_stops;
}

//transport_catalogue_serialization::Catalogue TransportCatalogue::Serialize() const
//{
//    std::vector<const Stop*> sorted_stops = SortStops();
//    //buses
//    transport_catalogue_serialization::BusList bus_list;
//    for (const Bus& bus : buses_) {
//        transport_catalogue_serialization::Bus bus_to_out;
//        bus_to_out.set_name(bus.busname);
//        bus_to_out.set_route_type(bus.is_roundtrip);
//        if (!bus.stops.empty()) {
//           int stops_count = bus.stops.size(); /*bus.is_roundtrip ? bus.stops.size() : bus.stops.size() / 2 + 1;*/
//           for (int i = 0; i < stops_count; ++i) {
//               const Stop* stop = bus.stops[i];
//               int pos = std::lower_bound(sorted_stops.begin(), sorted_stops.end(),
//                                          stop, [](const Stop* lhs, const Stop* rhs) {
//                                              return lhs->stopname < rhs->stopname; }) - sorted_stops.begin();
//               bus_to_out.add_stop(pos);
//           }
//        }
//        bus_list.add_bus();
//        *bus_list.mutable_bus(bus_list.bus_size() - 1) = bus_to_out;
//    }
//    //stops
//    transport_catalogue_serialization::StopList stop_list;
//    for (const Stop& stop : stops_) {
//        transport_catalogue_serialization::Stop stop_to_out;
//        stop_to_out.set_name(stop.stopname);
//        stop_to_out.set_latitude(stop.coordinates.lat);
//        stop_to_out.set_longitude(stop.coordinates.lng);
//        stop_list.add_stop();
//        *stop_list.mutable_stop(stop_list.stop_size() - 1) = stop_to_out;
//    }
//    //distances
//    transport_catalogue_serialization::DistanceList distance_list;
//    for (const auto& [key, value] : distances_to_stops_) {
//        transport_catalogue_serialization::Distance distance_to_out;
//        int pos = std::lower_bound(sorted_stops.begin(), sorted_stops.end(),
//                                   key.first, [](const Stop* lhs, const Stop* rhs) {
//                                       return lhs->stopname < rhs->stopname; }) - sorted_stops.begin();
//        distance_to_out.set_index_from(pos);
//        pos = std::lower_bound(sorted_stops.begin(), sorted_stops.end(),
//                               key.second, [](const Stop* lhs, const Stop* rhs) {
//                                   return lhs->stopname < rhs->stopname; }) - sorted_stops.begin();
//        distance_to_out.set_index_to(pos);
//        distance_to_out.set_distance(value);
//        distance_list.add_distance();
//        *distance_list.mutable_distance(distance_list.distance_size() - 1) = distance_to_out;
//    }
//    transport_catalogue_serialization::Catalogue catalogue;
//    *catalogue.mutable_bus_list() = bus_list;
//    *catalogue.mutable_stop_list() = stop_list;
//    *catalogue.mutable_distance_list() = distance_list;
//    return catalogue;
//}
//bool TransportCatalogue::Deserialize(transport_catalogue_serialization::Catalogue& catalogue)
//{
//    //stops
//    transport_catalogue_serialization::StopList stop_list = catalogue.stop_list();
//    for (int i = 0; i < stop_list.stop_size(); ++i) {
//        const transport_catalogue_serialization::Stop& stop = stop_list.stop(i);
//        Stop new_stop;
//        new_stop.stopname = stop.name();
//        new_stop.coordinates.lat = stop.latitude();
//        new_stop.coordinates.lng = stop.longitude();

//        AddStop(std::move(new_stop));
//    }
//    std::vector<const Stop*> sorted_stops = SortStops();
//    //distances
//    transport_catalogue_serialization::DistanceList distance_list = catalogue.distance_list();
//    for (int i = 0; i < distance_list.distance_size(); ++i) {
//        const transport_catalogue_serialization::Distance distance = distance_list.distance(i);
//        distances_to_stops_[{sorted_stops[distance.index_from()], sorted_stops[distance.index_to()]}] = distance.distance();
//    }
//    //buses
//    transport_catalogue_serialization::BusList bus_list = catalogue.bus_list();
//    for (int i = 0; i < bus_list.bus_size(); ++i) {
//        const transport_catalogue_serialization::Bus& bus_from_input = bus_list.bus(i);
//        std::vector<std::string> stops_in_bus;
//        stops_in_bus.reserve(bus_from_input.stop_size());
//        for (int i = 0; i < bus_from_input.stop_size(); ++i) {
//           stops_in_bus.push_back(sorted_stops[bus_from_input.stop(i)]->stopname);
//        }
////        if (!bus_from_input.route_type()) {
////           for (int i = bus_from_input.stop_size() - 2; i > 0; --i) {
////               stops_in_bus.push_back(sorted_stops[bus_from_input.stop(i)]->stopname);
////           }
////        }
//        AddBus(bus_from_input.name(), stops_in_bus, bus_from_input.route_type());
//    }
//    return true;
//}

