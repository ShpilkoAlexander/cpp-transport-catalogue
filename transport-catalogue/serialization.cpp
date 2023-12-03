#include "serialization.h"

proto_info::ProtoInfo::ProtoInfo(TransportCatalogue& db,
                                 renderer::MapRenderer& renderer, TransportRouter& route) {
    AddStops(db);
    AddDistance(db);
    AddBuses(db);
    AddMap(renderer);
    AddRoute(route);
}

void proto_info::ProtoInfo::Serialization(const std::filesystem::path& path) {
    std::ofstream out_file(path, std::ios::binary);
    t_catalogue_.SerializeToOstream(&out_file);
}

void proto_info::ProtoInfo::Deserialization(const std::filesystem::path& path) {
    std::ifstream in_file(path, std::ios::binary);
    t_catalogue_.ParseFromIstream(&in_file);
}

std::vector<Bus> proto_info::ProtoInfo::ParseProtoBuses(TransportCatalogue& db) {
    std::vector<Bus> result_bus;
    result_bus.reserve(t_catalogue_.buses_size());

    for (size_t i = 0; i < t_catalogue_.buses_size(); ++i) {
        Bus bus;
        bus.is_roundtrip = t_catalogue_.mutable_buses(i)->is_roundtrip();
        bus.busname = t_catalogue_.mutable_buses(i)->bus_name();
        for (size_t j = 0; j < t_catalogue_.mutable_buses(i)->route_size(); ++j) {
            bus.stops.push_back(db.FindStop(t_catalogue_.mutable_buses(i)->mutable_route(j)->stop_name()));
        }
        result_bus.push_back(bus);
    }

    return result_bus;
}

void proto_info::ProtoInfo::ParseProtoMap(renderer::MapRenderer& renderer) {
    renderer.render_settings_.width = t_catalogue_.mutable_map()->width();
    renderer.render_settings_.height = t_catalogue_.mutable_map()->height();
    renderer.render_settings_.padding = t_catalogue_.mutable_map()->padding();
    renderer.render_settings_.stop_radius = t_catalogue_.mutable_map()->stop_radius();
    renderer.render_settings_.line_width = t_catalogue_.mutable_map()->line_width();
    renderer.render_settings_.bus_label_font_size = t_catalogue_.mutable_map()->bus_label_font_size();

    renderer.render_settings_.bus_label_offset.first = t_catalogue_.mutable_map()->bus_label_offset(0);
    renderer.render_settings_.bus_label_offset.second = t_catalogue_.mutable_map()->bus_label_offset(1);

    renderer.render_settings_.stop_label_font_size = t_catalogue_.mutable_map()->stop_label_font_size();

    renderer.render_settings_.stop_label_offset.first = t_catalogue_.mutable_map()->stop_label_offset(0);
    renderer.render_settings_.stop_label_offset.second = t_catalogue_.mutable_map()->stop_label_offset(1);

    renderer.render_settings_.underlayer_width = t_catalogue_.mutable_map()->underlayer_width();
    AddColorOutProto(renderer);
    AddColorPaletteOutProto(renderer);
}

void proto_info::ProtoInfo::ParseProtoTransportRouter(TransportRouter& route, TransportCatalogue& db) {
    ParseProtoGraph(route);
    ParseProtoEdgeInfo(route, db);
    ParseProtoStopnamesToId(route);
    ParseProtoRouter(route);
}

void proto_info::ProtoInfo::AddBuses(TransportCatalogue& db) {
    for (auto& bus : db.GetBuses()) {
        t_catalogue_proto::Bus proto_bus;
        proto_bus.set_is_roundtrip(bus.is_roundtrip);
        proto_bus.set_bus_name(bus.busname);
        for (auto& stop : bus.stops) {
            t_catalogue_proto::Stop proto_stop;
            //proto_stop.set_vertex_id(stop->vertex_id);
            proto_stop.set_stop_name(stop->stopname);
            proto_stop.mutable_coordinates_()->set_lat(stop->coordinates.lat);
            proto_stop.mutable_coordinates_()->set_lng(stop->coordinates.lng);
            *proto_bus.mutable_route()->Add() = proto_stop;
        }
        *t_catalogue_.mutable_buses()->Add() = proto_bus;
    }
}

void proto_info::ProtoInfo::AddDistance(TransportCatalogue& db) {
    for (auto& stop : db.GetStops()) {
        t_catalogue_proto::Stop proto_stop;
        proto_stop.set_stop_name(stop.stopname);
        proto_stop.mutable_coordinates_()->set_lat(stop.coordinates.lat);
        proto_stop.mutable_coordinates_()->set_lng(stop.coordinates.lng);
        //proto_stop.set_vertex_id(stop.vertex_id);
        *t_catalogue_.mutable_stops()->Add() = proto_stop;
    }
}

void proto_info::ProtoInfo::AddStops(TransportCatalogue& db) {
    for (auto& [stops, distance] : db.GetDistancesToStops()) {
        t_catalogue_proto::Dist proto_dist;
        proto_dist.set_stop_one(stops.first->stopname);
        proto_dist.set_stop_two(stops.second->stopname);
        proto_dist.set_distance(distance);
        *t_catalogue_.mutable_stops_distance()->Add() = proto_dist;
    }
}

void proto_info::ProtoInfo::AddMap(renderer::MapRenderer& renderer) {
    t_catalogue_.mutable_map()->set_width(renderer.render_settings_.width);
    t_catalogue_.mutable_map()->set_height(renderer.render_settings_.height);
    t_catalogue_.mutable_map()->set_padding(renderer.render_settings_.padding);
    t_catalogue_.mutable_map()->set_stop_radius(renderer.render_settings_.stop_radius);
    t_catalogue_.mutable_map()->set_line_width(renderer.render_settings_.line_width);
    t_catalogue_.mutable_map()->set_bus_label_font_size(renderer.render_settings_.bus_label_font_size);

    t_catalogue_.mutable_map()->add_bus_label_offset(renderer.render_settings_.bus_label_offset.first);
    t_catalogue_.mutable_map()->add_bus_label_offset(renderer.render_settings_.bus_label_offset.second);

    t_catalogue_.mutable_map()->set_stop_label_font_size(renderer.render_settings_.stop_label_font_size);

    t_catalogue_.mutable_map()->add_stop_label_offset(renderer.render_settings_.stop_label_offset.first);
    t_catalogue_.mutable_map()->add_stop_label_offset(renderer.render_settings_.stop_label_offset.second);

    t_catalogue_.mutable_map()->set_underlayer_width(renderer.render_settings_.underlayer_width);
    AddColorInProto(renderer);
    AddColorPaletteInProto(renderer);

}

void proto_info::ProtoInfo::AddRoute(TransportRouter& route) {
    HelperAddGraphInProto(route);
    HelperAddEdgesInProto(route);
    HelperAddStopnamesToIdInProto(route);
    HelperAddRouterInProto(route);
    t_catalogue_.mutable_transport_router()->set_wait(route.GetWaitTime());
    t_catalogue_.mutable_transport_router()->set_speed(route.GetVelocity());
}

void proto_info::ProtoInfo::AddColorInProto(renderer::MapRenderer& renderer) {
    switch (renderer.render_settings_.underlayer_color.index()) {
    case 1: {
        t_catalogue_.mutable_map()->mutable_underlayer_color()->
            set_str_color(std::get<std::string>(renderer.render_settings_.underlayer_color));
        break;
    }
    case 2: {
        svg::Rgb rgb;
        rgb = std::get<svg::Rgb>(renderer.render_settings_.underlayer_color);
        t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgb_color()->set_r(rgb.red);
        t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgb_color()->set_g(rgb.green);
        t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgb_color()->set_b(rgb.blue);
        break;
    }
    case 3: {
        svg::Rgba rgba;
        rgba = std::get<svg::Rgba>(renderer.render_settings_.underlayer_color);
        t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgba_color()->set_r(rgba.red);
        t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgba_color()->set_g(rgba.green);
        t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgba_color()->set_b(rgba.blue);
        t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgba_color()->set_opacity(rgba.opacity);
        break;
    }
    default: { break; }
    }
}

void proto_info::ProtoInfo::AddColorPaletteInProto(renderer::MapRenderer& renderer) {
    for (auto& elem : renderer.render_settings_.color_palette) {
        switch (elem.index()) {
        case 1: {
            t_catalogue_proto::Color color;
            color.set_str_color(std::get<std::string>(elem));
            *t_catalogue_.mutable_map()->mutable_color_palette_()->Add() = color;
            break;
        }
        case 2: {
            t_catalogue_proto::Color color;
            svg::Rgb rgb;
            rgb = std::get<svg::Rgb>(elem);
            color.mutable_rgb_color()->set_r(rgb.red);
            color.mutable_rgb_color()->set_g(rgb.green);
            color.mutable_rgb_color()->set_b(rgb.blue);
            *t_catalogue_.mutable_map()->mutable_color_palette_()->Add() = color;
            break;
        }
        case 3: {
            t_catalogue_proto::Color color;
            svg::Rgba rgba;
            rgba = std::get<svg::Rgba>(elem);
            color.mutable_rgba_color()->set_r(rgba.red);
            color.mutable_rgba_color()->set_g(rgba.green);
            color.mutable_rgba_color()->set_b(rgba.blue);
            color.mutable_rgba_color()->set_opacity(rgba.opacity);
            *t_catalogue_.mutable_map()->mutable_color_palette_()->Add() = color;
            break;
        }
        default: { break; }
        }
    }
}

void proto_info::ProtoInfo::AddColorOutProto(renderer::MapRenderer& renderer) {
    if (t_catalogue_.mutable_map()->mutable_underlayer_color()->col_case() == 1) {
        renderer.render_settings_.underlayer_color = t_catalogue_.mutable_map()->mutable_underlayer_color()->str_color();
    }
    else if (t_catalogue_.mutable_map()->mutable_underlayer_color()->col_case() == 2) {
        svg::Rgb rgb;
        rgb.red = t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgb_color()->r();
        rgb.green = t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgb_color()->g();
        rgb.blue = t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgb_color()->b();
        renderer.render_settings_.underlayer_color = rgb;
    }
    else if (t_catalogue_.mutable_map()->mutable_underlayer_color()->col_case() == 3) {
        svg::Rgba rgba;
        rgba.red = t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgba_color()->r();
        rgba.green = t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgba_color()->g();
        rgba.blue = t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgba_color()->b();
        rgba.opacity = t_catalogue_.mutable_map()->mutable_underlayer_color()->mutable_rgba_color()->opacity();
        renderer.render_settings_.underlayer_color = rgba;
    }
    else {
        renderer.render_settings_.underlayer_color = std::monostate{};
    }
}

void proto_info::ProtoInfo::AddColorPaletteOutProto(renderer::MapRenderer& renderer) {
    for (size_t i = 0; i < t_catalogue_.mutable_map()->color_palette__size(); ++i) {
        if (t_catalogue_.mutable_map()->color_palette_(i).col_case() == 1) {
            renderer.render_settings_.color_palette.push_back(t_catalogue_.mutable_map()->mutable_color_palette_(i)->str_color());
        }
        else if (t_catalogue_.mutable_map()->mutable_color_palette_(i)->col_case() == 2) {
            svg::Rgb rgb;
            rgb.red = t_catalogue_.mutable_map()->mutable_color_palette_(i)->mutable_rgb_color()->r();
            rgb.green = t_catalogue_.mutable_map()->mutable_color_palette_(i)->mutable_rgb_color()->g();
            rgb.blue = t_catalogue_.mutable_map()->mutable_color_palette_(i)->mutable_rgb_color()->b();
            renderer.render_settings_.color_palette.push_back(rgb);
        }
        else if (t_catalogue_.mutable_map()->mutable_color_palette_(i)->col_case() == 3) {
            svg::Rgba rgba;
            rgba.red = t_catalogue_.mutable_map()->mutable_color_palette_(i)->mutable_rgba_color()->r();
            rgba.green = t_catalogue_.mutable_map()->mutable_color_palette_(i)->mutable_rgba_color()->g();
            rgba.blue = t_catalogue_.mutable_map()->mutable_color_palette_(i)->mutable_rgba_color()->b();
            rgba.opacity = t_catalogue_.mutable_map()->mutable_color_palette_(i)->mutable_rgba_color()->opacity();
            renderer.render_settings_.color_palette.push_back(rgba);
        }
        else {
            renderer.render_settings_.color_palette.push_back(std::monostate{});
        }
    }
}

void proto_info::ProtoInfo::HelperAddGraphInProto(TransportRouter& route) {
    for (size_t i = 0; i < route.GetGraph().GetEdges().size(); ++i) {
        t_catalogue_proto::Edge edge;
        edge.set_from(route.GetGraph().GetEdges()[i].from);
        edge.set_to(route.GetGraph().GetEdges()[i].to);
        edge.set_weight(route.GetGraph().GetEdges()[i].weight);
        *t_catalogue_.mutable_transport_router()->mutable_graph()->mutable_edges()->Add() = edge;
    }

    for (size_t i = 0; i < route.GetGraph().GetIncidenceLists().size(); ++i) {
        t_catalogue_proto::IncidenceList incidence_list;
        for (size_t j = 0; j < route.GetGraph().GetIncidenceLists()[i].size(); ++j) {
            *incidence_list.mutable_list()->Add() = route.GetGraph().GetIncidenceLists()[i][j];
        }
        *t_catalogue_.mutable_transport_router()->mutable_graph()->mutable_incidence_lists()->Add() = incidence_list;
    }
}

void proto_info::ProtoInfo::HelperAddEdgesInProto(TransportRouter& route) {
    for (auto& [id, info] : route.GetEdges()) {
        t_catalogue_proto::EdgeInfo edge_info;
        edge_info.set_edge_id(id);
        edge_info.set_name(std::string(info.name));
        edge_info.set_count(info.span_count);
        edge_info.set_time(info.time);
        edge_info.set_is_bus(info.type);
        *t_catalogue_.mutable_transport_router()->mutable_edges_()->Add() = edge_info;
    }
}


void proto_info::ProtoInfo::HelperAddStopnamesToIdInProto(TransportRouter& route) {
    for (auto& [stopname_, id_] : route.GetStopnameToId()) {
        t_catalogue_proto::StopnamesToId stopnames_to_id;
        stopnames_to_id.set_stopname(std::string(stopname_));
        stopnames_to_id.set_id(id_);
        *t_catalogue_.mutable_transport_router()->mutable_stopnames_to_id_()->Add() = stopnames_to_id;
    }
}

void proto_info::ProtoInfo::HelperAddRouterInProto(TransportRouter& route) {
    for (size_t i = 0; i < route.GetRouter()->GetRoutesInternalData().size(); ++i) {
        t_catalogue_proto::HelpRepeated help_repeated;
        for (size_t j = 0; j < route.GetRouter()->GetRoutesInternalData()[i].size(); ++j) {
            t_catalogue_proto::HelpOpt help_opt;
            if (route.GetRouter()->GetRoutesInternalData()[i][j].has_value()) {
                help_opt.mutable_help_opt()->set_weight(route.GetRouter()->GetRoutesInternalData()[i][j].value().weight);
                if (route.GetRouter()->GetRoutesInternalData()[i][j].value().prev_edge.has_value()) {
                    help_opt.mutable_help_opt()->set_prev_edge(route.GetRouter()->
                                                               GetRoutesInternalData()[i][j].value().prev_edge.value());
                }
            }
            *help_repeated.mutable_help_repeated()->Add() = help_opt;
        }
        *t_catalogue_.mutable_transport_router()->mutable_router_()->
         mutable_routes_internal_data()->mutable_routes()->Add() = help_repeated;
    }
}

void proto_info::ProtoInfo::ParseProtoGraph(TransportRouter& route) {
    for (size_t i = 0; i < t_catalogue_.mutable_transport_router()->mutable_graph()->edges_size(); ++i) {
        graph::Edge<double> edge;
        edge.from = t_catalogue_.mutable_transport_router()->mutable_graph()->mutable_edges(i)->from();
        edge.to = t_catalogue_.mutable_transport_router()->mutable_graph()->mutable_edges(i)->to();
        edge.weight = t_catalogue_.mutable_transport_router()->mutable_graph()->mutable_edges(i)->weight();
        route.GetGraph().GetEdges().push_back(edge);
    }

    for (size_t i = 0; i < t_catalogue_.mutable_transport_router()->mutable_graph()->incidence_lists_size(); ++i) {
        std::vector<graph::EdgeId> incidence_list;
        incidence_list.reserve(t_catalogue_.mutable_transport_router()->
                               mutable_graph()->mutable_incidence_lists(i)->list_size());
        for (size_t j = 0; j < t_catalogue_.mutable_transport_router()->
                               mutable_graph()->mutable_incidence_lists(i)->list_size(); ++j) {
            incidence_list.push_back(t_catalogue_.mutable_transport_router()->
                                     mutable_graph()->mutable_incidence_lists(i)->list(j));
        }
        route.GetGraph().GetIncidenceLists().push_back(incidence_list);
    }
}

void proto_info::ProtoInfo::ParseProtoEdgeInfo(TransportRouter& route, TransportCatalogue& db) {
    for (size_t i = 0; i < t_catalogue_.mutable_transport_router()->edges__size(); ++i) {
        EdgeInfo edge_info;
        if (t_catalogue_.mutable_transport_router()->mutable_edges_(i)->is_bus()) {
            edge_info.type = EdgeType::BUS_T;
            edge_info.name = db.FindStop(t_catalogue_.mutable_transport_router()->mutable_edges_(i)->name())->stopname;
        } else {
            edge_info.type = EdgeType::WAIT;
            edge_info.name = db.FindBus(t_catalogue_.mutable_transport_router()->mutable_edges_(i)->name())->busname;
        }
        edge_info.span_count = t_catalogue_.mutable_transport_router()->mutable_edges_(i)->count();
        edge_info.time = t_catalogue_.mutable_transport_router()->mutable_edges_(i)->time();
        route.GetEdges().insert({ t_catalogue_.mutable_transport_router()->mutable_edges_(i)->edge_id(), edge_info });
    }
}

void proto_info::ProtoInfo::ParseProtoStopnamesToId(TransportRouter& route) {
    for (size_t i = 0; i < t_catalogue_.mutable_transport_router()->stopnames_to_id__size(); ++i) {
        route.GetStopnameToId().insert({ t_catalogue_.mutable_transport_router()->mutable_stopnames_to_id_(i)->stopname(),
                                  t_catalogue_.mutable_transport_router()->mutable_stopnames_to_id_(i)->id() });
    }
}

void proto_info::ProtoInfo::ParseProtoRouter(TransportRouter& route) {
    std::vector<std::vector<std::optional<graph::Router<double>::RouteInternalData>>> buffer;
    buffer.reserve(t_catalogue_.mutable_transport_router()->
                   mutable_router_()->mutable_routes_internal_data()->routes_size());

    for (size_t i = 0; i < t_catalogue_.mutable_transport_router()->
                           mutable_router_()->mutable_routes_internal_data()->routes_size(); ++i) {
        std::vector<std::optional<graph::Router<double>::RouteInternalData>> result;
        result.reserve(t_catalogue_.mutable_transport_router()->
                       mutable_router_()->mutable_routes_internal_data()->mutable_routes(i)->help_repeated_size());

        for (size_t j = 0; j < t_catalogue_.mutable_transport_router()->
                               mutable_router_()->mutable_routes_internal_data()->mutable_routes(i)->help_repeated_size(); ++j) {
            graph::Router<double>::RouteInternalData route_internal_data;
            if (t_catalogue_.mutable_transport_router()->
                mutable_router_()->mutable_routes_internal_data()->
                mutable_routes(i)->mutable_help_repeated(j)->h_opt_case() == 1) {
                route_internal_data.weight = t_catalogue_.mutable_transport_router()->
                                             mutable_router_()->mutable_routes_internal_data()->
                                             mutable_routes(i)->mutable_help_repeated(j)->mutable_help_opt()->weight();
                if (t_catalogue_.mutable_transport_router()->
                    mutable_router_()->mutable_routes_internal_data()->
                    mutable_routes(i)->mutable_help_repeated(j)->mutable_help_opt()->TEST_case() == 2) {
                    route_internal_data.prev_edge = std::make_optional<size_t>(t_catalogue_.mutable_transport_router()->
                                                                               mutable_router_()->mutable_routes_internal_data()->
                                                                               mutable_routes(i)->mutable_help_repeated(j)->mutable_help_opt()->prev_edge());
                }
                result.push_back(std::make_optional<graph::Router<double>::RouteInternalData>(route_internal_data));
            }
            else {
                result.push_back(std::nullopt);
            }

        }
        buffer.push_back(result);
    }

    route.GetRouter() = std::make_unique<graph::Router<double>>(route.GetGraph(), buffer);
}

std::vector<Stop> proto_info::ProtoInfo::ParseProtoStops() {
    std::vector<Stop> result_stops;
    result_stops.reserve(t_catalogue_.stops_size());

    for (size_t i = 0; i < t_catalogue_.stops_size(); ++i) {
        Stop stop;
        //stop.vertex_id = t_catalogue_.mutable_stops(i)->vertex_id();
        stop.stopname = t_catalogue_.mutable_stops(i)->stop_name();
        stop.coordinates.lat = t_catalogue_.mutable_stops(i)->coordinates_().lat();
        stop.coordinates.lng = static_cast<double>(t_catalogue_.mutable_stops(i)->coordinates_().lng());
        result_stops.push_back(stop);
    }
    return result_stops;
}

using StopsDistances = std::unordered_map<PairStops, size_t, PairStopsHasher>;

StopsDistances proto_info::ProtoInfo::ParseProtoDistance(TransportCatalogue& db) {
    StopsDistances result_distance;
    result_distance.reserve(t_catalogue_.stops_distance_size());

    for (size_t i = 0; i < t_catalogue_.stops_distance_size(); ++i) {
        PairStops stops;
        stops.first = db.FindStop(t_catalogue_.mutable_stops_distance(i)->stop_one());
        stops.second = db.FindStop(t_catalogue_.mutable_stops_distance(i)->stop_two());
        result_distance[stops] = t_catalogue_.mutable_stops_distance(i)->distance();
    }

    return result_distance;
}
