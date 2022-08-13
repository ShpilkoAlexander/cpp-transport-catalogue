#include <iostream>

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

#include <iostream>

using namespace std;

int main()
{
    TransportCatalogue transport_catalogue;

    InputReader reader(transport_catalogue);
    reader.ProcessingRequest(std::cin);

    StatReader stat_reader(transport_catalogue, std::cin, std::cout);
    stat_reader.ProcessingStat();

}
