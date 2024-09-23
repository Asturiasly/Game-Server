#pragma once

#include <chrono>
#include <sstream>
#include <iostream>
#include <variant>
#include <fstream>

#include <boost/log/core.hpp>        
#include <boost/log/expressions.hpp> 
#include <boost/asio/io_context.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time.hpp>

#include "json_support.h"

namespace net = boost::asio;
namespace logging = boost::log;
namespace json = boost::json;
namespace expr = boost::log::expressions;
namespace http = boost::beast::http;

using namespace std::literals;

using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using Response = std::variant<StringResponse, FileResponse>;
using Strand = net::strand<net::io_context::executor_type>;

BOOST_LOG_ATTRIBUTE_KEYWORD(data, "AdditionalData", json::object)

namespace server_logging
{
    template <class SomeRequestHandler>
    class LoggingRequestHandler
    {
    public:

        static void Format(logging::record_view const& rec, logging::formatting_ostream& strm)
        {
            auto data = logging::extract<json::object>("AdditionalData", rec).get();
            strm << data;
        }

        explicit LoggingRequestHandler(SomeRequestHandler handler) : decorated_(handler)
        {
            logging::add_console_log(
                std::cout,
                logging::keywords::format = &Format,
                logging::keywords::auto_flush = true
            );
            out_.open("/tmp/volume/log.txt", std::ios::out | std::ios::app);
        }
        //"/tmp/volume/log.txt"
        //"C:\\Users\\impro\\Desktop\\fall_logs\\test_log.txt"


        template <typename Body, typename Allocator, typename Send>
        void operator()(const boost::asio::ip::tcp::endpoint& endp, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req, Send&& send)
        {
            std::string request = std::string(req.target());
            std::string method = std::string(req.method_string());
            std::string ip = endp.address().to_string();
            LogRequest(request, method, ip);
            std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now();

            //Передача в лямбду любых ссылок критична - происходит инвалидация ссылки/указателя при использовании
            //асинхронности к API запросам (без асинхронности - работает). 
            //При использовании ссылок ошибка проявляется в зависимости от способа запуска
            //(Win-работает, Linux-работает, Linux via Docker(-))
            auto log_send = [send = std::forward<decltype(send)>(send), ip, start_time, this](auto&& response)
                {
                    std::chrono::system_clock::time_point end_time = std::chrono::system_clock::now();
                    auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                    this->LogResponse(delta_time.count(), response.result_int(), response.at(http::field::content_type), ip);
                    send(response);
                };

            decorated_.operator()(std::move(req), std::move(log_send));
        }

        void LogServerStarted(const net::ip::port_type port, const boost::asio::ip::address address)
        {
            auto message = json_support::MakeJSONServerStartLog(GetTimeStamp(), port, address.to_string());
            BOOST_LOG_TRIVIAL(info) << logging::add_value(data, message);
            out_ << message << std::endl;
        }

        void LogServerStopped()
        {
            auto message = json_support::MakeJSONServerStoppedLog(GetTimeStamp());
            BOOST_LOG_TRIVIAL(info) << logging::add_value(data, message);
            out_ << message << std::endl;
        }

    private:
        SomeRequestHandler decorated_;
        std::fstream out_;

        void LogRequest(const std::string& target, const std::string& method, const std::string& ip)
        {
            auto message = json_support::MakeJSONRequestReceivedLog(GetTimeStamp(), ip, method, target);
            BOOST_LOG_TRIVIAL(info) << logging::add_value(data, message);
            out_ << message << std::endl;
        }

        void LogResponse(const int time, const int code, const std::string_view content_type, const std::string& ip)
        {
            auto message = json_support::MakeJSONResponseSentLog(GetTimeStamp(), ip, time, code, content_type);
            BOOST_LOG_TRIVIAL(info) << logging::add_value(data, message);
            out_ << message << std::endl;
        }

        static std::string GetTimeStamp()
        {
            const auto t_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::stringstream ss;
            ss << std::put_time(std::localtime(&t_c), "%Y-%m-%dT%H:%M:%S");
            return ss.str();
        }
    };
}
