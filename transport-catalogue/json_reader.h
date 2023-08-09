#pragma once

#include "json_reader.h"
#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"
#include "svg.h"
#include "request_handler.h"

#include <iostream>


struct StopsWithDistances {
    Stop* stop;
    json::Dict distances;
};

class JsonReader {
public:
    JsonReader(TransportCatalogue db);

    //Чтение Json и определение base_requests_ и stat_requests_
    void ReadJson(std::istream& input);

    //Загрузка данных в транспортный каталог
    void LoadData();

    //Обработка запросов
    void ProcessingRequest();

    //Вывод JSON-массива ответов
    void PrintResponseArray(std::ostream& output);

private:
    TransportCatalogue db_;
    json::Array base_requests_;
    json::Array stat_requests_;
    json::Dict render_settings_;
    json::Array response_array_;
    std::vector<StopsWithDistances> stops_with_distance_;

    //Обработка запроса на добавления остановки
    void StopAdding(const json::Dict& stop_info);

    //Обработка запроса на добавления остановки
    void BusAdding(const json::Dict& bus_info);

    //Обработка запросов на добавление дистанции между остоновками
    void DistanceAdding();

    //Добавление дистанции между остановками
    void StopsDistancesAdding(const Stop* stop, const json::Dict& distances);

    //Обработка запроса об остановке
    void ProccessingStopRequest(const json::Dict& stop_request);

    //Обработка запроса о маршруте
    void ProccessingBusRequest(const json::Dict& bus_request);

    //Обработка запроса о отрисовки карты
    void ProccessingRenderMap(int req_id);

    //Отрисовка карты
    void RenderMap(std::ostream& output);

    //Получение данных для вывод карты
    renderer::RenderSettings GetRenderSettings();

    //Чтение цвета из Node
    svg::Color GetColorFromNode(json::Node& color);


};
