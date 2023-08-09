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
    JsonReader proccessing_json(db);

    //Чтение Json
    proccessing_json.ReadJson(std::cin);

    //Загрузка данных в транспортный каталог
    proccessing_json.LoadData();

    //Отрисовка карты
    //proccessing_json.RenderMap(std::cout);

    //Обработка запросов
    proccessing_json.ProcessingRequest();

    //Вывод JSON-массива ответов
    proccessing_json.PrintResponseArray(std::cout);
}
