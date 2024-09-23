#pragma once
#include <string>
#include <sstream>
#include <memory>
#include <optional>
#include <boost/url.hpp>
#include <fstream>

#include "application.h"
#include "json_loader.h"
#include "json_support.h"
#include "content_type.h"
#include "extra_data.h"


constexpr int records_size_no_args = 20;

namespace http = boost::beast::http;
namespace json = boost::json;
namespace urls = boost::urls;
using StringResponse = http::response<http::string_body>;

namespace http_handler_game
{
    class RequestHandlerGame
    {
    public:
        explicit RequestHandlerGame(Application& app) : 
            app_(app),
            out_("C:\\Users\\impro\\Desktop\\fall_logs\\test_log.txt")
        {}

        RequestHandlerGame(const RequestHandlerGame&) = delete;
        RequestHandlerGame& operator=(const RequestHandlerGame&) = delete;

        template <typename Body>
        StringResponse operator()(Body&& req)
        {
            std::string target = std::string(req.target());

            if (target == "/api/v1/game/join")
                return GetJoinGameResponse(req);
            else if (target == "/api/v1/game/players")
                return GetPlayersListResponse(req);
            else if (target == "/api/v1/game/player/action")
                return GetActionResponse(req);
            else if (target == "/api/v1/game/state")
                return GetStateResponse(req);
            else if (target == "/api/v1/game/tick")
                return GetTickResponse(req);
            else if (target.substr(0, records_size_no_args) == "/api/v1/game/records")
            {
                RecordsParams params = GetParametres(target);

                if (params.maxItems > 100)
                    return GetBadRequestAPIResponse(req.version());
                return GetRecordsResponse(req.keep_alive(), req.version(), params);
            }
            else
                return GetBadRequestAPIResponse(req.version());
        }

    private:
        Application& app_;
        std::fstream out_;

        template<typename Body>
        StringResponse GetStateResponse(Body&& req)
        {
            if (req.method() != http::verb::get && req.method() != http::verb::head)
                return PostNotAllowed(req.keep_alive(), req.version());

            std::pair<std::pair<bool, bool>, bool> correctness = AuthorizationChecks(req);
            //1 - header_correctness, 2 - player existing, 3 = is one in container false
            if (correctness.second)
                return GetStateGame(req.keep_alive(), req.version(), GetToken(req));
            return BadAuthorization(correctness.first, req.keep_alive(), req.version());
        }

        template<typename Body>
        StringResponse GetPlayersListResponse(Body&& req)
        {
            if (req.method() != http::verb::get && req.method() != http::verb::head)
                return PostNotAllowed(req.keep_alive(), req.version());

            std::pair<std::pair<bool, bool>, bool> correctness = AuthorizationChecks(req);
            //1 - header_correctness, 2 - player existing, 3 = is one in container false
            if (correctness.second)
                return GetPlayerList(req.keep_alive(), req.version(), GetToken(req));
            return BadAuthorization(correctness.first, req.keep_alive(), req.version());
        }

        template<typename Body>
        StringResponse GetActionResponse(Body&& req)
        {
            if (req.method() != http::verb::post)
                return GetOrHeadNotAllowed(req.keep_alive(), req.version());

            std::pair<std::pair<bool, bool>, bool> correctness = AuthorizationChecks(req);
            //1 - header_correctness, 2 - player existing, 3 = is one in container false
            if (correctness.second)
            {
                Token token(GetToken(req));
                auto player = app_.FindPlayerByToken(token);

                //Нельзя помещать в другую функцию, т.к. иначе придется использовать либо try-catch, либо std::variant.
                //Более эффективное решение, но с дублированием.
                boost::json::value data;
                boost::system::error_code ec;
                boost::json::value result;
                std::stringstream input;
                input << req.body();
                std::string text = json_loader::ReadJSONFromStream(input);
                data = boost::json::parse(text, ec);
                if (ec)
                    return GetBadJSONInput(req.keep_alive(), req.version());

                std::string is_stop = std::string(data.at("move").as_string());

                player->GetDog()->ClearIdleTime();
                if (is_stop == "")
                    player->GetDog()->StopDog();
                else
                {
                    model::Direction move_pattern = static_cast<model::Direction>(is_stop.back());
                    player->GetDog()->SetInGameDirection(move_pattern);
                    player->GetDog()->SetInGameSpeed();
                }
                return GetActionGame(req.keep_alive(), req.version());
            }
            return BadAuthorization(correctness.first, req.keep_alive(), req.version());
        }

        template <typename Body>
        StringResponse GetJoinGameResponse(Body&& req)
        {
            bool keep_alive = req.keep_alive();
            unsigned int version = req.version();

            if (req.method() != http::verb::post)
                return GetOrHeadNotAllowed(keep_alive, version);

            //Нельзя помещать в другую функцию, т.к. иначе придется использовать либо try-catch, либо std::variant.
            //Более эффективное решение, но с дублированием.
            boost::json::value data;
            boost::system::error_code ec;
            boost::json::value result;
            std::stringstream input;
            input << req.body();
            std::string text = json_loader::ReadJSONFromStream(input);
            data = boost::json::parse(text, ec);
            if (ec)
                return GetBadJSONInput(req.keep_alive(), req.version());

            std::string mapId = std::string(data.as_object().at("mapId").get_string());

            if (!IsCorrectUserName(data))
                return GetEmptyPlayerName(keep_alive, version);
            else if (!IsCorrectMapId(data))
                return GetEmptyMapId(keep_alive, version);
            else if (!IsMapFound(mapId))
                return GetMapNotFound(keep_alive, version);

            std::string userName = std::string(data.as_object().at("userName").as_string());

            std::shared_ptr<model::GameSession> session_ptr_shared;
            std::shared_ptr<model::Map> map_ptr_shared;
            bool is_session_exist = app_.GetGame().IsSessionExist(mapId);

            if (is_session_exist)
            {
                session_ptr_shared = app_.GetGame().GetSession(mapId);
                map_ptr_shared = session_ptr_shared->GetMap();
            }
            else
            {
                map_ptr_shared = app_.GetGame().FindMap(model::Map::Id(mapId));
                session_ptr_shared = std::make_shared<model::GameSession>(map_ptr_shared);
            }

            int player_id;
            std::string token;
            model::DogCoord start_coords;
            app_.GetSettings().randomize_spawn_points ? start_coords = map_ptr_shared->GetRandomPosDog() : start_coords = map_ptr_shared->GetStartPosDog();
            std::shared_ptr<model::Dog> dog_ptr_shared = std::make_shared<model::Dog>(model::Direction::UP, map_ptr_shared->GetDogSpeed(), start_coords);
            session_ptr_shared->AddDog(dog_ptr_shared);
            std::shared_ptr<Player>player_shared_ptr = std::make_shared<Player>(session_ptr_shared, dog_ptr_shared, userName);
            app_.AddPlayer(player_shared_ptr);
            token = *(app_.SetTokenForPlayer(player_shared_ptr));
            player_shared_ptr->SetToken(token);
            std::cout << token << std::endl;
            player_id = player_shared_ptr->GetObjectId();

            if (is_session_exist)
                app_.GetGame().UpdateSession(session_ptr_shared, mapId);
            else
                app_.GetGame().AddSession(session_ptr_shared, mapId);

            return GetTokenForClient(keep_alive, version, token, player_id);
        }

        template<typename Body>
        StringResponse GetTickResponse(Body&& req)
        {
            if (app_.GetSettings().is_auto_tick)
                return GetInvalidEndpoint(req.keep_alive(), req.version());

            if (req.method() != http::verb::post)
                return GetOrHeadNotAllowed(req.keep_alive(), req.version());

            //Нельзя помещать в другую функцию, т.к. иначе придется использовать либо try-catch, либо std::variant.
            //Более эффективное решение, но с дублированием.
            boost::json::value data;
            boost::system::error_code ec;
            boost::json::value result;
            std::stringstream input;
            input << req.body();
            std::string text = json_loader::ReadJSONFromStream(input);
            data = boost::json::parse(text, ec);

            if (ec)
                return GetBadJSONInput(req.keep_alive(), req.version());

            if (!data.as_object().at("timeDelta").is_int64())
                return GetBadJSONInput(req.keep_alive(), req.version());

            if (!out_.is_open())
            {
                out_.open("/tmp/volume/log.txt", std::ios::out | std::ios::app);
            }

            //"/tmp/volume/log.txt"
            //"C:\\Users\\impro\\Desktop\\fall_logs\\test_log.txt"

            int64_t time_value = data.as_object().at("timeDelta").get_int64();
            std::chrono::milliseconds deltaTime(time_value);
            out_ << "START ON TICK" << std::endl;
            app_.OnTick(deltaTime);
            //out_ << "END ON TICK" << std::endl;
            //out_ << "START DELETE UNUSED INFO" << std::endl;
            app_.DeleteUnusedInfo(deltaTime);
            //out_ << "END DELETE UNUSED INFO" << std::endl;
            //out_ << "START SERIALIZE ON TICK" << std::endl;
            app_.SerializeOnTick(deltaTime);
            //out_ << "END SERIALIZE ON TICK" << std::endl;
            out_.close();
            return GetGameTick(req.keep_alive(), req.version());
        }
        
        StringResponse GetRecordsResponse(bool keep_alive, unsigned int version, RecordsParams& params)
        {
            auto retired_players = app_.GetRetiredPlayersInfo(params);
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONRetiredPlayers(retired_players));
            http::response<http::string_body> response(http::status::ok, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////

        StringResponse GetTokenForClient(bool keep_alive, unsigned int version, std::string token, int player_id)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONToken(token, player_id));
            http::response<http::string_body> response(http::status::ok, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetMapNotFound(bool keep_alive, unsigned int version)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONNotFoundMap());
            http::response<http::string_body> response(http::status::not_found, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetEmptyMapId(bool keep_alive, unsigned int version)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONEmptyMapId());
            http::response<http::string_body> response(http::status::bad_request, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetEmptyPlayerName(bool keep_alive, unsigned int version)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONEmptyPlayerName());
            http::response<http::string_body> response(http::status::bad_request, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetBadJSONInput(bool keep_alive, unsigned int version)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONBadJSONInput());
            http::response<http::string_body> response(http::status::bad_request, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetOrHeadNotAllowed(bool keep_alive, unsigned int version)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONNotAllowedMethodForGame());
            http::response<http::string_body> response(http::status::method_not_allowed, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.set(http::field::allow, "POST"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse PostNotAllowed(bool keep_alive, unsigned int version)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONNotAllowedMethodForPlayerList());
            http::response<http::string_body> response(http::status::method_not_allowed, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.set(http::field::allow, "GET, HEAD"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetPlayerList(bool keep_alive, unsigned int version, std::string token)
        {
            auto player = app_.FindPlayerByToken(Token(token));
            auto session = player->GetSession();
            auto players = app_.GetPlayersInSession(session->GetObjectId());

            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONPlayerList(players));
            http::response<http::string_body> response(http::status::ok, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetUnauthorizedInvalidToken(bool keep_alive, unsigned int version)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONUnauthorizedNoToken());
            http::response<http::string_body> response(http::status::unauthorized, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetUnauthorizedUnknownToken(bool keep_alive, unsigned int version)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONUnauthorizedUnknownToken());
            http::response<http::string_body> response(http::status::unauthorized, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetStateGame(bool keep_alive, unsigned int version, std::string token)
        {
            auto player = app_.FindPlayerByToken(Token(token)); 
            auto session = player->GetSession();
            auto players = app_.GetPlayersInSession(session->GetObjectId());
            // Запрос state происходит через токен, соответственно необходимо выдать статус той карты/сессии, где находится игрок
            // Если токена не существует, статус выдан не будет.
            std::string body = json_support::GetFormattedJSONStr(json_support::MakeJSONStateGame(session, players));
            http::response<http::string_body> response(http::status::ok, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }


        StringResponse GetActionGame(bool keep_alive, unsigned int version)
        {
            json::object body;
            http::response<http::string_body> response(http::status::ok, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = json::serialize(body);
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            response.prepare_payload();
            return response;
        }

        StringResponse GetGameTick(bool keep_alive, unsigned int version)
        {
            json::object body;
            http::response<http::string_body> response(http::status::ok, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = json::serialize(body);
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            response.prepare_payload();
            return response;
        }

        StringResponse GetInvalidEndpoint(bool keep_alive, unsigned int version)
        {
            json::object body = json_support::MakeJSONInvalidEndpoint();
            http::response<http::string_body> response(http::status::bad_request, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = json::serialize(body);
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            response.prepare_payload();
            return response;
        }

        StringResponse GetBadRequestAPIResponse(const unsigned int version)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::GetJSONBadRequest());
            http::response<http::string_body> response(http::status::bad_request, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(false);
            return response;
        }

        StringResponse BadAuthorization(std::pair<bool, bool> correctness, bool keep_alive, unsigned int version)
        {
            if (correctness.first == false)
                return GetUnauthorizedInvalidToken(keep_alive, version);
            else if (correctness.second == false)
                return GetUnauthorizedUnknownToken(keep_alive, version);
            else
                return GetUnauthorizedInvalidToken(keep_alive, version);
        }

        boost::json::value GetJSON(std::string body)
        {
            boost::json::value result;
            std::stringstream input;
            input << body;
            std::string text = json_loader::ReadJSONFromStream(input);
            return boost::json::parse(text);
        }

        bool IsCorrectUserName(boost::json::value& data)
        {
            std::string userName;
            if (data.as_object().contains("userName"))
            {
                userName = std::string(data.as_object().at("userName").as_string());
                return userName.size() != 0;
            }
            else
                return false;
        }

        bool IsCorrectMapId(boost::json::value& data)
        {
            std::string mapId;
            if (data.as_object().contains("mapId"))
            {
                mapId = std::string(data.as_object().at("mapId").get_string());
                return mapId.size() != 0;
            }
            else
                return false;
        }

        bool IsMapFound(std::string& mapId)
        {
            auto ptr = app_.GetGame().FindMap(model::Map::Id(mapId));
            return ptr != nullptr;
        }

        template <typename Body>
        std::string GetToken(Body&& req)
        {
            std::string token = "";
            for (const auto& header : req.base())
            {
                const boost::beast::string_view header_name = header.name_string();
                if (header_name == "Authorization" || header_name == "authorization")
                {
                    token = header.value();
                    token = token.substr(token.find_first_of(' ') + 1, token.size());
                    return token;
                }
            }
            return token;
        }

        template <typename Body>
        bool IsAuthHeaderCorrect(Body&& req)
        {
            for (const auto& header : req.base())
            {
                const boost::beast::string_view header_name = header.name_string();
                if (header_name == "Authorization" || header_name == "authorization")
                {
                    std::string_view token = header.value();
                    size_t pos = token.find_first_of(' ');
                    if (pos == token.npos)
                        return false;
                    token = token.substr(token.find_first_of(' ') + 1, token.size());
                    if (token.size() != 32)
                        return false;
                    return true;
                }
            }
            return false;
        }

        template<typename Body>
        std::pair<std::pair<bool, bool>, bool> AuthorizationChecks(Body&& req)
        {
            bool token_correct = IsAuthHeaderCorrect(req);
            Token token(GetToken(req));
            bool player_exist = app_.FindPlayerByToken(token) != nullptr;

            std::pair<bool, bool> correctness;
            correctness.first = token_correct;
            correctness.second = player_exist;
            bool is_one_of_checks_false = token_correct && player_exist;
            return std::make_pair(correctness, is_one_of_checks_false);
        }

        RecordsParams GetParametres(std::string_view target)
        {
            // /api/v1/game/records?start=0&maxItems=100
            RecordsParams params;
            urls::url_view base_url(target);
            auto p = base_url.encoded_params();
            
            std::string start;
            if (p.contains("start"))
            {
                auto it = p.find("start");
                if (it.operator*().has_value && it.operator*().value != "")
                {
                    start = std::string(it.operator*().value);
                    params.start = static_cast<size_t>(std::stoi(start));
                }
            }

            std::string maxItems;
            if (p.contains("maxItems"))
            {
                auto it = p.find("maxItems");
                if (it.operator*().has_value && it.operator*().value != "")
                {
                    maxItems = std::string(it.operator*().value);
                    params.maxItems = static_cast<size_t>(std::stoi(maxItems));
                }
            }
            return params;
        }
    };
}