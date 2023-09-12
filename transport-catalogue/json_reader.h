#pragma once

#include "json_reader.h"
#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"
#include "svg.h"
#include "request_handler.h"
#include "json_builder.h"
#include "transport_router.h"

#include <iostream>
#include <memory>

//struct RouteInfo {
//    std::vector<
//};


class JsonReader {
public:
    JsonReader(TransportCatalogue db);

    //Чтение Json и определение base_requests_ и stat_requests_
    void ReadJson(std::istream& input);

    //Загрузка данных в транспортный каталог
    void LoadData();

    //Обработка запросов
    void ProcessRequest();

    //Вывод JSON-массива ответов
    void PrintResponseArray(std::ostream& output);

private:
    TransportCatalogue db_;
    std::unique_ptr<TransportRouter> trans_router_;
    json::Array base_requests_;
    json::Array stat_requests_;
    json::Dict render_settings_;
    json::Dict routing_settings_;
    json::Builder response_array_;

    //Обработка запроса на добавления остановки
    void ParsingStop(const json::Dict& stop_info);

    //Обработка запроса на добавления остановки
    void ParsingBus(const json::Dict& bus_info);

    const DistancesToStops DictStrNodeToStrInt(const json::Dict& distances_node);

    //Обработка запроса об остановке
    void ProcessStopRequest(const json::Dict& stop_request);

    //Обработка запроса о маршруте
    void ProcessBusRequest(const json::Dict& bus_request);

    //Обработка запроса о построении маршрута
    void ProcessRoute(const json::Dict& route_request);

    json::Node RouteInfoToJson(const std::vector<EdgeInfo>& edge_info) const;

    //Обработка запроса о отрисовки карты
    void ProcessRenderMap(int req_id);

    //Отрисовка карты
    void RenderMap(std::ostream& output);

    //Получение данных для вывод карты
    renderer::RenderSettings GetRenderSettings();

    //Отрисовка карты
    void BuildRoute(std::ostream& output);

    //Чтение цвета из Node
    svg::Color GetColorFromNode(json::Node& color);


};
