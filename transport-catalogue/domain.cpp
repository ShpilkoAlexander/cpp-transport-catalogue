#include "domain.h"

size_t PairStopsHasher::operator() (const PairStops& pair_stop) const {
    size_t h_first = stop_hasher_(pair_stop.first);
    size_t h_second = stop_hasher_(pair_stop.second);

    return h_first + h_second * 37;
}


