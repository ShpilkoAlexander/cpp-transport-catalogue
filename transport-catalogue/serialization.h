#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "graph.h"

#include <filesystem>
#include <fstream>

#include <transport_catalogue.pb.h>
#include <map_renderer.pb.h>
#include <svg.pb.h>
#include <transport_router.pb.h>
#include <graph.pb.h>


namespace proto_info {
class ProtoInfo {
public:
    ProtoInfo() = default;
    ProtoInfo(TransportCatalogue& db,
              renderer::MapRenderer& renderer, TransportRouter& route);

    void Serialization(const std::filesystem::path& path);
    void Deserialization(const std::filesystem::path& path);

    std::vector<Stop> ParseProtoStops();

    using StopsDistances = std::unordered_map<PairStops, size_t, PairStopsHasher>;

    StopsDistances ParseProtoDistance(TransportCatalogue& db);
    std::vector<Bus> ParseProtoBuses(TransportCatalogue& db);
    void ParseProtoMap(renderer::MapRenderer& renderer);
    void ParseProtoTransportRouter(TransportRouter& route, TransportCatalogue& db);

private:
    t_catalogue_proto::TransportCatalogue t_catalogue_;

    void AddBuses(TransportCatalogue& db);
    void AddDistance(TransportCatalogue& db);
    void AddStops(TransportCatalogue& db);
    void AddMap(renderer::MapRenderer& renderer);
    void AddRoute(TransportRouter& route);
    void AddColorInProto(renderer::MapRenderer& renderer);
    void AddColorPaletteInProto(renderer::MapRenderer& renderer);
    void AddColorOutProto(renderer::MapRenderer& renderer);
    void AddColorPaletteOutProto(renderer::MapRenderer& renderer);
    void HelperAddGraphInProto(TransportRouter& route);
    void HelperAddEdgesInProto(TransportRouter& route);
    void HelperAddStopnamesToIdInProto(TransportRouter& route);
    void HelperAddRouterInProto(TransportRouter& route);
    void ParseProtoGraph(TransportRouter& route);
    void ParseProtoEdgeInfo(TransportRouter& route, TransportCatalogue& db);
    void ParseProtoStopnamesToId(TransportRouter& route);
    void ParseProtoRouter(TransportRouter& route);
};
}
