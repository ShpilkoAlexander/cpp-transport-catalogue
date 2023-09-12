#include <iostream>

#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"

#include "domain.h"
#include "svg.h"

#include <iostream>

using namespace std;

int main()
{
    TransportCatalogue db;
    JsonReader process_json(db);

    //Чтение Json
    process_json.ReadJson(std::cin);

    //Загрузка данных в транспортный каталог
    process_json.LoadData();

    //Обработка запросов
    process_json.ProcessRequest();

    //Вывод JSON-массива ответов
    process_json.PrintResponseArray(std::cout);

}
