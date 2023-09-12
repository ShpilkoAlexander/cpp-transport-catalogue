#pragma once

#include "domain.h"

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <set>


class TransportCatalogue {

public:

    void AddStop(const Stop& stop, DistancesToStops& distance_to_stops);

    const Stop* FindStop(std::string_view stopname) const;

    void AddBus(std::string_view busname, std::vector<std::string>& stopnames, bool is_roundtrip = false);

    const Bus* FindBus(std::string_view busname) const;

    const BusInfo GetBusInfo(std::string_view busname) const;

    const StopInfo GetStopInfo(std::string_view stopname) const;

    void SetDistancesToStops(const Stop* stop_from, const Stop* stop_to, size_t distance);

    const std::unordered_map<PairStops, size_t, PairStopsHasher>& GetDistancesToStops() const;

    const std::unordered_map<std::string_view, const Bus*>* GetBusnameToBus() const;

    //Обработка запросов на добавление дистанции между остоновками
    void DistanceAdd();

    const std::deque<Stop>& GetStops() const;
    size_t CountStops() const;

    const std::deque<Bus>& GetBuses() const;

private:
    std::deque<Stop> stops_;    //остановки
    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;  //индексы остановок
    std::deque<Bus> buses_; //маршруты
    std::unordered_map<std::string_view, const Bus*> busname_to_bus_;   //индексы маршрутов
    std::unordered_map<std::string_view, std::set<std::string_view>> stopname_to_buses_ ;   //индексы остановок для маршрутов
    std::unordered_map<PairStops, size_t, PairStopsHasher> distances_to_stops_;   //пары индексов остановок для расстояний между ними
    std::vector<StopsWithDistances> stops_with_distance_; //остановки с расстояниями до других остановок

    //Добавление дистанции между остановками
    void StopsDistancesAdd(const Stop* stop, const DistancesToStops& distances);

}; 
