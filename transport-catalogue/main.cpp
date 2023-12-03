#include <iostream>

#include "transport_catalogue.h"
#include "json_reader.h"

#include <iostream>

using namespace std;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);
    TransportCatalogue db;
    JsonReader process_json(db);

    if (mode == "make_base"sv) {
        //Чтение Json
        process_json.ReadJson(std::cin, mode);

        //Загрузка данных в транспортный каталог
        process_json.LoadData();

    }
    else if (mode == "process_requests"sv) {
        //Чтение Json
        process_json.ReadJson(std::cin, mode);

        //Обработка запросов
        process_json.ProcessRequest();

        //Вывод JSON-массива ответов
        process_json.PrintResponseArray(std::cout);

    }
    else {
        PrintUsage();
        return 1;
    }
}
