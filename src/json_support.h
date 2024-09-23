#pragma once
#include <boost/json/value.hpp>
#include <boost/json/object.hpp>
#include <boost/json.hpp>
#include <boost/json/string.hpp>
#include <boost/json/array.hpp>

#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <memory>

#include "model.h"
#include "player.h"
#include "extra_data.h"

namespace json_support
{
    namespace json = boost::json;
    using namespace std::literals;
    //For API
    json::array GetJSONAllMaps(const model::Game::Maps& maps);
    json::object GetJSONRequiredMap(const std::shared_ptr<model::Map>& map, const std::unordered_map<std::string_view, boost::json::array>);
    json::object GetJSONNotFound();
    json::object GetJSONBadRequest();
    json::object GetJSONNotAllowedMethod();

    json::array MakeJSONRoadsArr(const std::vector<model::Road>&);
    json::array MakeJSONBuildingsArr(const std::vector<model::Building>&);
    json::array MakeJSONOfficesArr(const std::vector<model::Office>&);

    //For logging_request_handler
    json::object MakeJSONServerStartLog(const std::string& timestamp, const uint_least16_t port, const std::string& address);
    json::object MakeJSONServerStoppedLog(const std::string& timestamp);
    json::object MakeJSONRequestReceivedLog(const std::string& timestamp, const std::string& ip, const std::string& method, const std::string& uri);
    json::object MakeJSONResponseSentLog(const std::string& timestamp, const std::string& ip, const int time, const int code, const std::string_view content_type);
    json::object MakeJSONExceptionLog(const std::string& timestamp, const std::exception& ex);

    //For request_handler_game
    json::object MakeJSONNotFoundMap();
    json::object MakeJSONEmptyPlayerName();
    json::object MakeJSONEmptyMapId();
    json::object MakeJSONBadJSONInput();
    json::object MakeJSONNotAllowedMethodForGame();
    json::object MakeJSONNotAllowedMethodForPlayerList();
    json::object MakeJSONToken(const std::string& token, const int id);
    json::object MakeJSONPlayerList(const std::vector<std::shared_ptr<Player>>& players_in_session);
    json::object MakeJSONUnauthorizedNoToken();
    json::object MakeJSONUnauthorizedUnknownToken();
    json::object MakeJSONStateGame(const std::shared_ptr<model::GameSession> session, const std::vector<std::shared_ptr<Player>> players_in_session);
    json::object MakeJSONInvalidEndpoint();
    json::array MakeJSONRetiredPlayers(std::vector<std::tuple<std::string, int, int>>&);

    //Additional funcs for MakeJSONStateGame
    json::object MakeJSONPlayers(const std::vector<std::shared_ptr<Player>> players_in_session);
    json::object MakeJSONLostObjects(const std::shared_ptr<model::GameSession> session);

    template<typename json>
    std::string GetFormattedJSONStr(const json json_value)
    {
        std::stringstream read;
        std::string buffer;
        std::string body;
        read << json_value << std::endl;
        while (std::getline(read, buffer)) {
            body += buffer;
        }
        return body;
    }
}