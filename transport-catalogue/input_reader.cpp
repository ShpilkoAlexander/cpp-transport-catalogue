#include "input_reader.h"

#include <string>
#include <string_view>
#include <iostream>
#include <deque>
#include <iostream>
#include <vector>
#include <algorithm>


void RemovePrefixSpace(std::string_view& str) {
    str.remove_prefix(str.find_first_not_of(" "));
}

std::string_view RemoveSpaces(std::string_view str) {
    return str.substr(str.find_first_not_of(' '), str.find_last_not_of(' ') - str.find_first_not_of(' ') + 1);
}

std::string_view GetNextWord(std::string_view& str, size_t end_word) {
    std::string_view word;

    word = str.substr(0, end_word);
    if (end_word == std::string::npos) {
        str = "";
    }
    else {
        str.remove_prefix(end_word + 1);
    }

    return RemoveSpaces(word);
}

InputReader::InputReader(TransportCatalogue& transport_catalogue) :
    transport_catalogue_(transport_catalogue) {
}

void InputReader::ProcessingRequest(std::istream& input) {
    size_t request_count = 0;
    input >> request_count;

    std::string str_requests;
    std::getline(input, str_requests);
    for (size_t counter = 0; counter < request_count; ++counter) {
        std::getline(input, str_requests);
        Request request = DivisionRequest(str_requests);

        if (request.type_ == Type::STOP) {
            requests_.push_front(request);
        }
        else if (request.type_ == Type::BUS) {
            requests_.push_back(request);
        }
    }

    for (auto& request : requests_) {
        if (request.type_ == Type::STOP) {
            StopAddingProcessing(request);
        }

        else if (request.type_ == Type::BUS){
            BusAddingProcessing(request);
        }
    }

    DistanceAddingProcessing();
}

//Обработка запроса на добавления остановки
void InputReader::StopAddingProcessing(Request& request) {
    Stop stop;
    std::string_view stop_info(request.text_);

    stop.stopname = GetNextWord(stop_info, stop_info.find(':'));

    stop.coordinates.lat = std::stod(std::string(GetNextWord(stop_info, stop_info.find(','))));
    stop.coordinates.lng = std::stod(std::string(GetNextWord(stop_info, stop_info.find(','))));

    StopsWithDistances stops_with_distance;
    stops_with_distance.stop = transport_catalogue_.AddStop(std::move(stop));
    stops_with_distance.distances = stop_info;

    stops_with_distance_.push_back(std::move(stops_with_distance));
}

void InputReader::DistanceAddingProcessing() {
   for (auto& stop_distances : stops_with_distance_) {
       if (stop_distances.distances == "") {
           continue;
       }
       StopsDistancesAdding(stop_distances.stop, stop_distances.distances);
   }
}

void InputReader::StopsDistancesAdding(const Stop* stop, std::string_view distance_info) {
    std::pair<const Stop*, size_t> stop_distance;

    for (auto end = distance_info.find(',');
             end != std::string::npos;
             end = distance_info.find(',')) {

        //добавляем к именам остоновок следующее имя и убираем его из оставшейся строки
        stop_distance = ParsingStopAndDistance(GetNextWord(distance_info, end));
        transport_catalogue_.SetDistancesToStops(PairStops{stop, stop_distance.first}, stop_distance.second);
    }
    stop_distance = ParsingStopAndDistance(RemoveSpaces(distance_info));
    transport_catalogue_.SetDistancesToStops({stop, stop_distance.first}, stop_distance.second);
}

std::pair<const Stop*, size_t> InputReader::ParsingStopAndDistance(std::string_view distance_info) {
    size_t distance = std::stoul(std::string(GetNextWord(distance_info, distance_info.find('m'))));
    distance_info.remove_prefix(distance_info.find("to") + 2);

   // std::cout << transport_catalogue_.FindStop(GetNextWord(distance_info, std::string::npos)) << std::endl;
    //std::cout << distance_info << std::endl;

    const Stop* stop = transport_catalogue_.FindStop(GetNextWord(distance_info, std::string::npos));

    return {stop, distance};
}


//Обработка запроса на добавления остановки
void InputReader::BusAddingProcessing(Request& request) {
    std::string_view bus_info(request.text_);

    std::string busname(bus_info.substr(0, bus_info.find(':')));
    bus_info.remove_prefix(bus_info.find(':') + 1);

    char separator;
    if (bus_info.find('>') != std::string::npos) {
        separator = '>';
    }
    else {
        separator = '-';
    }

    std::vector<std::string> stopnames;
    for (auto end_stopname = bus_info.find(separator);
             end_stopname != std::string::npos;
             end_stopname = bus_info.find(separator)) {

        //добавляем к именам остоновок следующее имя и убираем его из оставшейся строки
        stopnames.push_back(std::string(GetNextWord(bus_info, end_stopname)));
    }
    stopnames.push_back(std::string(RemoveSpaces(bus_info)));

    if (separator == '-') {
        std::vector<std::string> reverse_name;
        std::for_each(std::next(stopnames.rbegin()), stopnames.rend(),
                      [&reverse_name] (auto& name) {reverse_name.push_back(name);}
        );
        stopnames.insert(stopnames.end(), reverse_name.begin(), reverse_name.end());
    }

    transport_catalogue_.AddBus(busname, stopnames);
}

//Разбивка строки на тип запрос (Тип запроса и текст запроса)
Request InputReader::DivisionRequest(std::string_view input) {
    RemovePrefixSpace(input);

    Type type;
    if (input.substr(0, 3) == "Bus") {
        type = Type::BUS;
        input.remove_prefix(3);
    }
    else if (input.substr(0, 4) == "Stop") {
        type = Type::STOP;
        input.remove_prefix(4);
    }

    else {
        return Request(Type::ERROR_TYPE_NAME, "");
    }

    RemovePrefixSpace(input);

    return Request(type, std::string(input));
}
