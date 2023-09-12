#pragma once

#include "transport_catalogue.h"

#include <string>
#include <string_view>
#include <deque>
#include <sstream>

struct StopsWithDistances {
    Stop* stop;
    std::string_view distances;
};

void RemovePrefixSpace(std::string_view& str);

std::string_view RemoveSpaces(std::string_view str);

std::string_view GetNextWord(std::string_view& str, size_t end_word);

class InputReader {

public:
    InputReader(TransportCatalogue& transport_catalogue);

    // Чтение и обработка запросов на добавление в базу данных
    void ProcessingRequest(std::istream& input);


private:
    std::deque<Request> requests_;
    TransportCatalogue& transport_catalogue_;
    std::vector<StopsWithDistances> stops_with_distance_;

    //Разбивка строки на тип запрос (Тип запроса и текст запроса)
    Request DivisionRequest(std::string_view input);

    //Обработка запроса на добавления остановки
    void StopAddingProcessing(Request& request);

    //Обработка запроса на добавления остановки
    void BusAddingProcessing(Request& request);

    //Обработка запросов на добавление дистанции между остоновками
    void DistanceAddingProcessing();

    //Добавление дистанции между остановками
    void StopsDistancesAdding(const Stop* stop, std::string_view distance_info);

    //Парсинг запроса на остановку и дистанцию
    std::pair<const Stop*, size_t> ParsingStopAndDistance(std::string_view);


};
