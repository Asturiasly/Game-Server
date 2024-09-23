#include "json_support.h"

namespace json_support
{
    //keys
    char const* X = "x";
    char const* Y = "y";
    char const* W = "w";
    char const* H = "h";
    char const* ID = "id";
    char const* NAME = "name";
    char const* CODE = "code";
    char const* MESSAGE = "message";
    char const* TIMESTAMP = "timestamp";
    char const* DATA = "data";

    //values
    char const* INVALID_ARG = "invalidArgument";
    char const* INVALID_METHOD = "invalidMethod";
    char const* INVALID_TOKEN = "invalidToken";
    char const* UNKNOWN_TOKEN = "unknownToken";
    char const* BAD_REQUEST = "badRequest";

    namespace json = boost::json;
    using namespace std::literals;
    json::array GetJSONAllMaps(const model::Game::Maps& maps)
    {
        boost::json::array maps_arr;
        for (const auto& map : maps)
        {
            json::string str_id = json::string(*(map->GetId()));
            json::string str_name = json::string(map->GetName());
            boost::json::object json_maps;
            json_maps[ID] = str_id;
            json_maps[NAME] = str_name;
            maps_arr.push_back(json_maps);
        }
        return maps_arr;
    }

    json::object GetJSONRequiredMap(const std::shared_ptr<model::Map>& map, const std::unordered_map<std::string_view, boost::json::array> loot_types)
    {
        boost::json::object json_map;
        std::string id = *(map->GetId());
        std::string name = map->GetName();
        std::vector<model::Road> roads = map->GetRoads();
        std::vector<model::Building> buildings = map->GetBuildings();
        std::vector<model::Office> offices = map->GetOffices();

        json_map[ID] = json::string(id);
        json_map[NAME] = json::string(name);
        json_map["roads"] = MakeJSONRoadsArr(roads);
        json_map["buildings"] = MakeJSONBuildingsArr(buildings);
        json_map["offices"] = MakeJSONOfficesArr(offices);
        json_map["lootTypes"] = loot_types.at(id);

        return json_map;
    }

    json::object GetJSONNotFound()
    {
        json::object not_found;
        not_found[CODE] = json::string("mapNotFound");
        not_found[MESSAGE] = json::string("Map not found");
        return not_found;
    }

    json::object GetJSONBadRequest()
    {
        json::object bad_request;
        bad_request[CODE] = json::string("badRequest");
        bad_request[MESSAGE] = json::string("Bad request");
        return bad_request;
    }

    json::object GetJSONNotAllowedMethod()
    {
        json::object not_allowed_method;
        not_allowed_method[CODE] = INVALID_METHOD;
        not_allowed_method[MESSAGE] = json::string("Method not allowed");
        return not_allowed_method;
    }

    json::array MakeJSONRoadsArr(const std::vector<model::Road>& roads)
    {
        json::array road_arr;
        for (const auto& road : roads)
        {
            json::object json_road;

            model::Point coords = road.GetStart();
            model::Dimension x = coords.x;
            model::Dimension y = coords.y;
            model::Point end = road.GetEnd();
            bool is_x_road = road.IsXRoad();

            json_road["x0"] = x;
            json_road["y0"] = y;
            if (is_x_road)
                json_road["x1"] = end.x;
            else
                json_road["y1"] = end.y;

            road_arr.push_back(json_road);
        }
        return road_arr;
    }
    json::array MakeJSONBuildingsArr(const std::vector<model::Building>& buildings)
    {
        json::array building_arr;

        for (const auto& building : buildings)
        {
            json::object json_building;
            model::Rectangle figure = building.GetBounds();
            model::Point pos = figure.position;
            model::Size size = figure.size;

            json_building[X] = pos.x;
            json_building[Y] = pos.y;
            json_building[W] = size.width;
            json_building[H] = size.height;

            building_arr.push_back(json_building);
        }

        return building_arr;
    }
    json::array MakeJSONOfficesArr(const std::vector<model::Office>& offices)
    {
        json::array offices_arr;

        for (const auto& office : offices)
        {
            json::object json_office;

            std::string id = *(office.GetId());
            model::Point pos = office.GetPosition();
            model::Offset offset = office.GetOffset();

            json_office[ID] = json::string(id);
            json_office[X] = pos.x;
            json_office[Y] = pos.y;
            json_office["offsetX"] = offset.dx;
            json_office["offsetY"] = offset.dy;

            offices_arr.push_back(json_office);
        }

        return offices_arr;
    }

    json::object MakeJSONServerStartLog(const std::string& timestamp, const uint_least16_t port, const std::string& address)
    {
        json::object starting;
        json::object data;
        starting[TIMESTAMP] = timestamp;
        data["port"] = port;
        data["address"] = address;
        starting[DATA] = data;
        starting[MESSAGE] = "server started";
        return starting;
    }
    json::object MakeJSONRequestReceivedLog(const std::string& timestamp, const std::string& ip, const std::string& method, const std::string& uri)
    {
        json::object request;
        json::object data;
        request["timestamp"] = timestamp;
        data["ip"] = ip;
        data["URI"] = uri;
        data["method"] = method;
        request[DATA] = data;
        request[MESSAGE] = "request received";
        return request;
    }
    json::object MakeJSONResponseSentLog(const std::string& timestamp, const std::string& ip, const int time, const int code, const std::string_view content_type)
    {
        json::object response;
        json::object data;
        response["timestamp"] = timestamp;
        data["ip"] = ip;
        data["response_time"] = time;
        data[CODE] = code;
        data["content_type"] = std::string(content_type);
        response[DATA] = data;
        response[MESSAGE] = "response sent";
        return response;
    }

    json::object MakeJSONServerStoppedLog(const std::string& timestamp)
    {
        json::object stopping;
        json::object data;
        stopping[TIMESTAMP] = timestamp;
        data[CODE] = 0;
        stopping[DATA] = data;
        stopping[MESSAGE] = "server exited";
        return stopping;
    }

    json::object MakeJSONExceptionLog(const std::string& timestamp, const std::exception& ex)
    {
        json::object exception;
        exception[TIMESTAMP] = timestamp;
        exception[CODE] = EXIT_FAILURE;
        exception[DATA] = ex.what();
        return exception;
    }

    json::object MakeJSONNotFoundMap()
    {
        json::object not_found;
        not_found[CODE] = "mapNotFound";
        not_found[MESSAGE] = "Map not found";
        return not_found;
    }
    json::object MakeJSONEmptyPlayerName()
    {
        json::object empty_player_name;
        empty_player_name[CODE] = INVALID_ARG;
        empty_player_name[MESSAGE] = "Invalid name";
        return empty_player_name;
    }

    json::object MakeJSONEmptyMapId()
    {
        json::object empty_map_id;
        empty_map_id[CODE] = INVALID_ARG;
        empty_map_id[MESSAGE] = "Empty field <map_id>";
        return empty_map_id;
    }

    json::object MakeJSONBadJSONInput()
    {
        json::object bad_json_parsing;
        bad_json_parsing[CODE] = INVALID_ARG;
        bad_json_parsing[MESSAGE] = "Invalid JSON or its arguments";
        return bad_json_parsing;
    }
    json::object MakeJSONNotAllowedMethodForGame()
    {
        json::object bad_method;
        bad_method[CODE] = INVALID_METHOD;
        bad_method[MESSAGE] = "Only POST method is expected";
        return bad_method;
    }

    json::object MakeJSONNotAllowedMethodForPlayerList()
    {
        json::object bad_method;
        bad_method[CODE] = INVALID_METHOD;
        bad_method[MESSAGE] = "Only GET or HEAD method is expected";
        return bad_method;
    }

    json::object MakeJSONToken(const std::string& token, const int id)
    {
        json::object token_json;
        token_json["authToken"] = token;
        token_json["playerId"] = id;
        return token_json;
    }

    json::object MakeJSONPlayerList(const std::vector<std::shared_ptr<Player>>& players_in_session)
    {
        json::object data;

        for (const auto& player : players_in_session)
        {
            std::string str_id = std::to_string(player->GetObjectId());
            json::object name;
            name[NAME] = player->GetName();
            data[str_id] = name;
        }    

        return data;
    }

    json::object MakeJSONUnauthorizedNoToken()
    {
        json::object invalid_token;
        invalid_token[CODE] = INVALID_TOKEN;
        invalid_token[MESSAGE] = "Authorization header is missing or Token is empty";
        return invalid_token;
    }

    json::object MakeJSONUnauthorizedUnknownToken()
    {
        json::object unknown_token;
        unknown_token[CODE] = UNKNOWN_TOKEN;
        unknown_token[MESSAGE] = "Player token has not been found";
        return unknown_token;
    }

    json::object MakeJSONInvalidEndpoint()
    {
        json::object invalid_endp;
        invalid_endp[CODE] = BAD_REQUEST;
        invalid_endp[MESSAGE] = "Invalid endpoint";
        return invalid_endp;
    }

    json::array MakeJSONRetiredPlayers(std::vector<std::tuple<std::string, int, int>>& ret_plrs)
    {
        json::array retired_players;
        json::object inf_json;

        for (int i = 0; i < ret_plrs.size(); ++i)
        {
            auto info = ret_plrs[i];
            inf_json["name"] = std::get<0>(info);
            inf_json["score"] = std::get<1>(info);
            double t = std::get<2>(info);
            t /= 1000;
            inf_json["playTime"] = t;

            retired_players.push_back(inf_json);
        }
        return retired_players;
    }

    json::object MakeJSONStateGame(const std::shared_ptr<model::GameSession> session, const std::vector<std::shared_ptr<Player>> players_in_session)
    {
        json::object state_game;
        state_game["players"] = MakeJSONPlayers(players_in_session);
        state_game["lostObjects"] = MakeJSONLostObjects(session);
        return state_game;
    }

    json::object MakeJSONPlayers(const std::vector<std::shared_ptr<Player>> players_in_session)
    {
        json::object players_js;

        for (const auto& player : players_in_session)
        {
            auto dog = player->GetDog();
            int id = dog->GetObjectId();
            double x = dog->GetCoords().x;
            double y = dog->GetCoords().y;

            json::object data;
            json::array coords;
            json::array speed;

            coords.push_back(x);
            coords.push_back(y);
            speed.push_back(dog->GetSpeed().speed_x);
            speed.push_back(dog->GetSpeed().speed_y);

            json::array gathered_loot;

            auto bag = dog->GetLoot();

            for (const auto& loot_elem : bag)
            {
                json::object loot;
                loot["id"] = loot_elem->GetId();
                loot["type"] = loot_elem->GetIndex();
                gathered_loot.push_back(loot);
            }

            data["pos"] = coords;
            data["speed"] = speed;
            data["dir"] = std::string{ static_cast<char>(dog->GetDirection()) }; // Фронт и тесты ожидают сугубо тип стринг!
            data["bag"] = gathered_loot;
            data["score"] = dog->GetCurrentScore();
            std::string str_id = std::to_string(id);
            players_js[str_id] = data;
        }
        return players_js;
    }

    json::object MakeJSONLostObjects(const std::shared_ptr<model::GameSession> session)
    {
        json::object lost_objects;
        std::vector<std::shared_ptr<model::Loot>> all_map_loot = session->GetMap()->GetMapLoot();
        for (const auto& elem : all_map_loot)
        {
            json::object data;
            json::array coords;
            data["type"] = elem->GetIndex();
            coords.push_back(elem->GetCoord().x);
            coords.push_back(elem->GetCoord().y);
            data["pos"] = coords;
            lost_objects[std::to_string(elem->GetId())] = data;
        }
        return lost_objects;
    }

}
