#include "stat_reader.h"
#include "transport_catalogue.h"
#include "input_reader.h"

#include <string_view>
#include <string>
#include <iostream>
#include <iomanip>



StatReader::StatReader(TransportCatalogue& transport_catalogue, std::istream& input, std::ostream& output)
    : transport_catalogue_(transport_catalogue), input_(input), output_(output) {

}

void StatReader::ProcessingStat() const {
    size_t request_count = 0;
    input_ >> request_count;

    std::vector<Request> requests;
    std::string str_requests;

    std::getline(input_, str_requests);
    for (size_t counter = 0; counter < request_count; ++counter) {
        std::getline(input_, str_requests);
        requests.push_back(SplitRequest(str_requests));
    }
    for (const Request& request : requests) {
        if (request.type_ == Type::BUS) {
            OutputBusInfo(request.text_);
        }
        else if (request.type_ == Type::STOP){
            OutputStopInfo(request.text_);
        }
    }
}

void StatReader::OutputBusInfo(std::string_view busname) const {
    BusInfo bus_info = transport_catalogue_.GetBusInfo(busname);

    if (!bus_info.is_found) {
        output_ << "Bus " << bus_info.busname << ": not found\n";
        return;
    }

    output_ << "Bus " << bus_info.busname << ": "
              << bus_info.stops_count << " stops on route, "
              << bus_info.uniq_stops_count << " unique stops, "
              << bus_info.route_len << " route length, "
              << std::setprecision(6)
              << bus_info.curvature << " curvature\n";
}

void StatReader::OutputStopInfo(std::string_view stopname) const {
    StopInfo stop_info = transport_catalogue_.GetStopInfo(stopname);

    output_ << "Stop " << stop_info.stopname << ": ";
    if (!stop_info.is_found) {
        output_ << "not found\n";
    }

    else if (stop_info.buses.size() == 0){
        output_ << "no buses\n";
    }

    else {
        output_ << "buses";
        for (std::string_view busname : stop_info.buses) {
            output_ << " " << busname;
        }
        output_ << "\n";
    }
}

Request StatReader::SplitRequest(std::string_view input) const {
    Type type;
    if (input.substr(0, 3) == "Bus") {
        type = Type::BUS;
        input.remove_prefix(4);
    }
    else if (input.substr(0, 4) == "Stop") {
        type = Type::STOP;
        input.remove_prefix(5);
    }

    else {
        return Request(Type::ERROR_TYPE_NAME, "");
    }

    return Request(type, std::string(input));
}


