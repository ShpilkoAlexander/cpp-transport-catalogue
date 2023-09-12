#include "json_reader.h"
#include "transport_catalogue.h"

#include <sstream>

using namespace std::literals;

JsonReader::JsonReader(TransportCatalogue db)
    : db_(db) {
}

//Загрузка данных в транспортный каталог
void JsonReader::LoadData() {
    for (const auto& request : base_requests_) {
        if (request.AsMap().at("type").AsString() == "Stop") {
            ParsingStop(request.AsMap());
        }
    }
    for (const auto& request : base_requests_) {
        if (request.AsMap().at("type").AsString() == "Bus") {
                    ParsingBus(request.AsMap());
        }
    }
    db_.DistanceAdd();
    trans_router_ = std::make_unique<TransportRouter>(routing_settings_.at("bus_wait_time").AsInt(), routing_settings_.at("bus_velocity").AsInt(), db_);


}

//Чтение цвета из Node
svg::Color JsonReader::GetColorFromNode(json::Node& color) {
    bool color_is_string = color.IsString();
    if (color_is_string) {
        return color.AsString();
    }
    if (!color.IsArray()) {
        throw json::ParsingError("Failed to read color");
    }
    const json::Array& color_array = color.AsArray();

    if (color_array.size() == 3) {
        svg::Rgb rgb;
        rgb.red = color_array.at(0).AsInt();
        rgb.green = color_array.at(1).AsInt();
        rgb.blue = color_array.at(2).AsInt();
        return rgb;
    }

    if (color_array.size() == 4) {
        svg::Rgba rgba;
        rgba.red = color_array.at(0).AsInt();
        rgba.green = color_array.at(1).AsInt();
        rgba.blue = color_array.at(2).AsInt();
        rgba.opacity = color_array.at(3).AsDouble();
        return rgba;
    }
    throw json::ParsingError("Failed to read color");
}

//Получение данных для вывод карты
renderer::RenderSettings JsonReader::GetRenderSettings() {
    renderer::RenderSettings render_settings;
    render_settings.width = render_settings_.at("width").AsDouble();
    render_settings.height = render_settings_.at("height").AsDouble();
    render_settings.padding = render_settings_.at("padding").AsDouble();
    render_settings.stop_radius = render_settings_.at("stop_radius").AsDouble();
    render_settings.line_width = render_settings_.at("line_width").AsDouble();
    render_settings.bus_label_font_size = render_settings_.at("bus_label_font_size").AsInt();
    render_settings.bus_label_offset = {render_settings_.at("bus_label_offset").AsArray().at(0).AsDouble()
                                        ,render_settings_.at("bus_label_offset").AsArray().at(1).AsDouble()};
    render_settings.stop_label_font_size = render_settings_.at("stop_label_font_size").AsInt();
    render_settings.stop_label_offset = {render_settings_.at("stop_label_offset").AsArray().at(0).AsDouble()
                                        ,render_settings_.at("stop_label_offset").AsArray().at(1).AsDouble()};

    render_settings.underlayer_color = GetColorFromNode(render_settings_.at("underlayer_color"));
    render_settings.underlayer_width = render_settings_.at("underlayer_width").AsDouble();

    std::vector<svg::Color> color_palette;
    for (auto color : render_settings_.at("color_palette").AsArray()) {
        color_palette.emplace_back(GetColorFromNode(color));
    }
    render_settings.color_palette = color_palette;

    return render_settings;
}

//Чтение Json и определение base_requests_ и stat_requests_
void JsonReader::ReadJson(std::istream& input) {
    json::Document read_data = json::Load(input);

    base_requests_ = read_data.GetRoot().AsMap().at("base_requests").AsArray();
    stat_requests_ = read_data.GetRoot().AsMap().at("stat_requests").AsArray();
    render_settings_ = read_data.GetRoot().AsMap().at("render_settings").AsMap();
    routing_settings_ = read_data.GetRoot().AsMap().at("routing_settings").AsMap();
}



//Обработка запроса на добавления остановки
void JsonReader::ParsingStop(const json::Dict& stop_info) {
    Stop stop;

    stop.stopname = stop_info.at("name").AsString();

    stop.coordinates.lat = stop_info.at("latitude").AsDouble();
    stop.coordinates.lng = stop_info.at("longitude").AsDouble();

    StopsWithDistances stops_with_distance;
    DistancesToStops distances_to_stops = DictStrNodeToStrInt(stop_info.at("road_distances").AsMap());
    db_.AddStop(std::move(stop), distances_to_stops);
}

//Обработка запроса на добавления остановки
void JsonReader::ParsingBus(const json::Dict& bus_info) {
    std::vector<std::string> stopnames;

    for (auto stopname : bus_info.at("stops").AsArray()) {
        stopnames.push_back(stopname.AsString());
    }

    if (!bus_info.at("is_roundtrip").AsBool()) {
        std::vector<std::string> reverse_name;
        std::for_each(std::next(stopnames.rbegin()), stopnames.rend(),
                      [&reverse_name] (auto& name) {reverse_name.push_back(name);}
        );
        stopnames.insert(stopnames.end(), reverse_name.begin(), reverse_name.end());
    }

    db_.AddBus(bus_info.at("name").AsString(), stopnames, bus_info.at("is_roundtrip").AsBool());
}

const DistancesToStops JsonReader::DictStrNodeToStrInt(const json::Dict& distances_node) {
    DistancesToStops distances_to_stops;
    for (const auto& [stop, distances] : distances_node) {
        distances_to_stops[stop] = distances.AsInt();
    }
    return distances_to_stops;
}

//Обработка запроса об остановке
void JsonReader::ProcessStopRequest(const json::Dict& stop_request) {
    StopInfo stop_info = db_.GetStopInfo(stop_request.at("name").AsString());
    json::Builder response;
    response.StartDict();

    response.Key("request_id").Value(stop_request.at("id").AsInt());

    if (stop_info.is_found) {
        response.Key("buses").StartArray();
        for (auto bus : stop_info.buses) {
            response.Value(json::Node{static_cast<std::string>(bus)});
        }
        response.EndArray();
    }
    else {
        response.Key("error_message").Value("not found");
    }
    response.EndDict();
    response_array_.Value(response.Build());
}

//Обработка запроса о маршруте
void JsonReader::ProcessBusRequest(const json::Dict& bus_request) {
    BusInfo bus_info = db_.GetBusInfo(bus_request.at("name").AsString());
    json::Builder response;
    response.StartDict();
    response.Key("request_id").Value(bus_request.at("id").AsInt());

    if (bus_info.is_found) {
        response.Key("curvature").Value(json::Node{bus_info.curvature});
        response.Key("route_length").Value(json::Node{bus_info.route_len});
        response.Key("stop_count").Value(json::Node{int(bus_info.stops_count)});
        response.Key("unique_stop_count").Value(json::Node{int(bus_info.uniq_stops_count)});
    }
    else {
        response.Key("error_message").Value("not found");
    }

    response.EndDict();
    response_array_.Value(response.Build());
}

json::Node JsonReader::RouteInfoToJson(const std::vector<EdgeInfo>& edge_info) const {
    json::Builder json_info;
    json_info.StartArray();

    for(const auto& info : edge_info) {
        switch (info.type) {
        case EdgeType::WAIT:
            json_info.StartDict();
            json_info.Key("type").Value("Wait"s);
            json_info.Key("stop_name").Value(std::string(info.name));
            json_info.Key("time").Value(info.time);
            json_info.EndDict();
            break;
        case EdgeType::BUS_T:
            json_info.StartDict();
            json_info.Key("type").Value("Bus"s);
            json_info.Key("bus").Value(std::string(info.name));
            json_info.Key("span_count").Value(info.span_count);
            json_info.Key("time").Value(info.time);
            json_info.EndDict();
            break;
        default:
            throw std::runtime_error("Error edge type");
            break;
        }
    }

    json_info.EndArray();
    return json_info.Build();
}

//Обработка запроса о построении маршрута
void JsonReader::ProcessRoute(const json::Dict& route_request) {

    auto info = trans_router_->SearchRoute(route_request.at("from").AsString(), route_request.at("to").AsString());

    json::Builder response;
    response.StartDict();
    response.Key("request_id").Value(route_request.at("id").AsInt());

    if (info == std::nullopt) {
        response.Key("error_message").Value("not found"s);
    }
    else {
        response.Key("items").Value(RouteInfoToJson(info->edge_info));
        response.Key("total_time").Value(info->time);
    }

    response.EndDict();
    response_array_.Value(response.Build());
}

//Обработка запроса о отрисовки карты
void JsonReader::ProcessRenderMap(int req_id) {
    json::Builder response;
    response.StartDict();

    std::ostringstream output;
    RenderMap(output);

    response.Key("request_id").Value(json::Node{req_id});;
    response.Key("map").Value(output.str());

    response.EndDict();
    response_array_.Value(response.Build());
}

//Обработка запросов
void JsonReader::ProcessRequest() {
    response_array_.StartArray();
    for (const auto& request : stat_requests_) {
        if (request.AsMap().at("type").AsString() == "Stop") {
            ProcessStopRequest(request.AsMap());
        }
        else if (request.AsMap().at("type").AsString() == "Bus") {
            ProcessBusRequest(request.AsMap());
        }
        else if (request.AsMap().at("type").AsString() == "Map") {
            ProcessRenderMap(request.AsMap().at("id").AsInt());
        }
        else if (request.AsMap().at("type").AsString() == "Route") {
            ProcessRoute(request.AsMap());
        }
        else {
            throw std::runtime_error("Processing requests error");
        }
    }
    response_array_.EndArray();
}

//Отрисовка карты
void JsonReader::RenderMap(std::ostream& output) {
    renderer::MapRenderer map_renderer(GetRenderSettings());
    RequestHandler req_handler(db_, map_renderer);
    req_handler.RenderMap().Render(output);
}

//Вывод JSON-массива ответов
void JsonReader::PrintResponseArray(std::ostream& output) {
    json::Print(response_array_.Build(), output);

}
