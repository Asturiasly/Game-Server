#pragma once
#include <string>
#include <filesystem>
#include <variant>

#include "http_server.h"
#include "request_handler_api.h"
#include "request_handler_static.h"


namespace http_handler {

    namespace net = boost::asio;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace fs = std::filesystem;
    namespace json = boost::json;
    namespace sys = boost::system;
    using namespace std::literals;
    using Strand = net::strand<net::io_context::executor_type>;

    struct AllowedRequests
    {
        AllowedRequests() = delete;
        constexpr static std::string_view API = "/api/"sv;
    };

    class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
    public:
        explicit RequestHandler(Application& app, fs::path game_data, Strand& api_strand) 
            : app_(app), 
              req_api_{ app }, 
              req_static_{ game_data }, 
              api_strand_(api_strand) 
              {}

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            //Обработать запрос request и отправить ответ, используя send
            std::string request = std::string(req.target());
            auto version = req.version();
            auto keep_alive = req.keep_alive();

                try
                {
                    if (IsAPIRequest(request))
                    {
                        auto handle = [self = shared_from_this(), send,
                            req = std::forward<decltype(req)>(req), version, keep_alive] {
                            try {
                                assert(self->api_strand_.running_in_this_thread());
                                return send(self->req_api_.operator()(req));
                            }
                            catch (...) {
                                send(self->ReportServerError(version, keep_alive));
                            }
                            };
                        return net::dispatch(api_strand_, handle);
                    }

                    return std::visit(
                        [&send](auto&& result) {
                            send(std::forward<decltype(result)>(result));
                        },
                        req_static_.operator()(req));
                }
                catch (...) {
                    send(ReportServerError(version, keep_alive));
                }
        }

    private:
        http_handler_api::RequestHandlerAPI req_api_;
        http_handler_static::RequestHandlerStatic req_static_;
        Application& app_;
        Strand& api_strand_;

        http::response<http::string_body> ReportServerError(unsigned int version, bool keep_alive)
        {
            http::response<http::string_body> response(http::status::internal_server_error, version);
            std::string_view content_type = ContentType::JSON_APP;
            std::string body = "Internal server error";
            response.set(http::field::content_type, content_type);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        bool IsAPIRequest(const std::string& req)
        {
            return req.substr(0, 4) == "/api";
        }
    };
} // namespace http_handler
