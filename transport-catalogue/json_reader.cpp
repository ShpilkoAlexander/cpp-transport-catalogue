#include "json_reader.h"

#include "transport_catalogue.h"

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
void JsonReader::ProccessStopRequest(const json::Dict& stop_request) {
    StopInfo stop_info = db_.GetStopInfo(stop_request.at("name").AsString());
    json::Dict response;
    if (stop_info.is_found) {
        json::Array node_buses;

        for (auto bus : stop_info.buses) {
            node_buses.push_back(json::Node(std::string(bus)));
        }
        response["buses"] = node_buses;
    }
    else {
        response["error_message"] = "not found";
    }
    response["request_id"] = stop_request.at("id").AsInt();
    response_array_.push_back(json::Node{response});
}

//Обработка запроса о маршруте
void JsonReader::ProccessBusRequest(const json::Dict& bus_request) {
    BusInfo bus_info = db_.GetBusInfo(bus_request.at("name").AsString());
    json::Dict response;

    if (bus_info.is_found) {
        response["curvature"] = json::Node{bus_info.curvature};
        response["route_length"] = json::Node{bus_info.route_len};
        response["stop_count"] = json::Node{int(bus_info.stops_count)};
        response["unique_stop_count"] = json::Node{int(bus_info.uniq_stops_count)};
    }
    else {
        response["error_message"] = "not found";
    }
    response["request_id"] = bus_request.at("id").AsInt();
    response_array_.push_back(json::Node{response});
}

//Обработка запроса о отрисовки карты
void JsonReader::ProccessRenderMap(int req_id) {
    json::Dict response;
    std::ostringstream output;
    RenderMap(output);

    response["map"] = json::Node{output.str()};
    response["request_id"] = json::Node{req_id};

    response_array_.push_back(json::Node{response});
}

//Обработка запросов
void JsonReader::ProcessingRequest() {
    for (const auto& request : stat_requests_) {
        if (request.AsMap().at("type").AsString() == "Stop") {
            ProccessStopRequest(request.AsMap());
        }
        else if (request.AsMap().at("type").AsString() == "Bus") {
            ProccessBusRequest(request.AsMap());
        }
        else if (request.AsMap().at("type").AsString() == "Map") {
            ProccessRenderMap(request.AsMap().at("id").AsInt());
        }
        else {
            throw std::runtime_error("Proccessing requests error");
        }
    }
}

//Отрисовка карты
void JsonReader::RenderMap(std::ostream& output) {
    renderer::MapRenderer map_renderer(GetRenderSettings());
    RequestHandler req_handler(db_, map_renderer);
    req_handler.RenderMap().Render(output);
}

//Вывод JSON-массива ответов
void JsonReader::PrintResponseArray(std::ostream& output) {
    json::PrintNode(response_array_, output);

}
