#pragma once

#include "transport_catalogue.h"

#include <string_view>
#include <sstream>

class StatReader {

public:
    StatReader(TransportCatalogue& transport_catalogue, std::istream& input, std::ostream& output);

    void ProcessingStat() const;

private:
    TransportCatalogue& transport_catalogue_;
    std::istream& input_;
    std::ostream& output_;

    //Вывод информации о маршруте
    void OutputBusInfo(std::string_view busname) const;

    //Вывод информации об остановке
    void OutputStopInfo(std::string_view stopname) const;

    //Разбитие запроса на тип и номер маршрута/остановки
    Request SplitRequest(std::string_view input) const;
};
