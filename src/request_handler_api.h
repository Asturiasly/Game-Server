#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>

#include "application.h"
#include "json_support.h"
#include "content_type.h"
#include "request_handler_game.h"

namespace http = boost::beast::http;
namespace json = boost::json;
using namespace std::literals;
using StringResponse = http::response<http::string_body>;

constexpr int required_map_size_request = 13;

namespace http_handler_api
{

    struct SupportedAPIRequests
    {
        SupportedAPIRequests() = delete;
        constexpr static std::string_view ALL_MAPS = "/api/v1/maps"sv;
        constexpr static std::string_view REQUIRED_MAP = "/api/v1/maps/"sv;
    };

    const std::unordered_set<std::string_view> MapRequests(
        {
            {"/api/v1/maps"sv},
            {"/api/v1/maps/"sv}
        });

    const std::unordered_set<std::string_view> SupportedMaps(
    {
        {"/api/v1/maps/map1"sv},
        {"/api/v1/maps/town"sv}
    });

    const std::unordered_set<std::string_view> SupportedGameRequests(
        {
            {"/api/v1/game/join"sv},
            {"/api/v1/game/players"sv},
            {"/api/v1/game/state"sv},
            {"/api/v1/game/player/action"sv},
            {"/api/v1/game/tick"sv},
            {"/api/v1/game/records"}
        }
    );

	class RequestHandlerAPI
	{
	public:
        explicit RequestHandlerAPI(Application& app) : app_(app), req_game_(app) {}
        RequestHandlerAPI(const RequestHandlerAPI&) = delete;
        RequestHandlerAPI& operator=(const RequestHandlerAPI&) = delete;
        
        template <typename Body>
        StringResponse operator()(Body&& req)
        {
            if (SupportedGameRequests.count(req.target()))
                return req_game_.operator()(std::forward<decltype(req)>(req));
            else if (req.target().substr(0, records_size_no_args) == "/api/v1/game/records")
                return req_game_.operator()(std::forward<decltype(req)>(req));
            else
                return GetMapAPIResponse(req);
        }

	private:
        Application& app_;
        http_handler_game::RequestHandlerGame req_game_;

        // API functions returns API requests
        template <typename Body>
        StringResponse GetMapAPIResponse(Body&& req)
        {
            if (req.method() != http::verb::get && req.method() != http::verb::head)
                return GetNotAllowedMethodAPIResponse(req.version());
            std::string request = std::string(req.target());
            if (request == SupportedAPIRequests::ALL_MAPS)
                return GetAllMapsAPIResponse(req.version(), req.keep_alive());
            else if (request.substr(0, required_map_size_request) == SupportedAPIRequests::REQUIRED_MAP)
            {
                std::string map_id = request.substr(required_map_size_request);
                if (IsMapFound(map_id))
                    return GetRequiredMapAPIResponse(map_id, req.version(), req.keep_alive());
                else
                    return GetMapNotFoundAPIResponse(req.version(), req.keep_alive());
            }
            else
                return GetBadRequestAPIResponse(req.version());
        }

        StringResponse GetAllMapsAPIResponse(const unsigned int version, const bool keep_alive)
        {
            json::array JSONMaps = json_support::GetJSONAllMaps(app_.GetGame().GetMaps());
            std::string body = json_support::GetFormattedJSONStr(JSONMaps);

            http::response<http::string_body> response(http::status::ok, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetRequiredMapAPIResponse(const std::string_view id, const unsigned int version, const bool keep_alive)
        {
            json::object JSONMap = 
                json_support::GetJSONRequiredMap(
                    app_.GetGame().FindMap(model::Map::Id(std::string(id))), 
                    app_.GetGame().GetExtraData().GetJSONLootType());

            std::string body = json_support::GetFormattedJSONStr(JSONMap);

            http::response<http::string_body> response(http::status::ok, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetMapNotFoundAPIResponse(const unsigned int version, const bool keep_alive)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::GetJSONNotFound());

            http::response<http::string_body> response(http::status::not_found, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
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

        StringResponse GetNotAllowedMethodAPIResponse(const unsigned int version)
        {
            std::string body = json_support::GetFormattedJSONStr(json_support::GetJSONNotAllowedMethod());

            http::response<http::string_body> response(http::status::method_not_allowed, version);
            std::string_view content_type = ContentType::JSON_APP;
            response.set(http::field::allow, "GET, HEAD");
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(false);
            return response;
        }

        bool IsMapFound(const std::string_view id)
        {
            if (app_.GetGame().FindMap(model::Map::Id(std::string(id))) == nullptr)
                return false;
            return true;
        }
	};
}