#include "http_server.h"
#include "logging_request_handler.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server
{
    void ReportError(beast::error_code ec, std::string_view what) {
        using namespace server_logging;
        boost::json::object error
        {
            {"code"s, ec.what()},
            {"text"s, ec.message()},
            {"where"s, what}
        };
        BOOST_LOG_TRIVIAL(fatal) << boost::log::add_value(data, error) << "error"sv;
    }
}  // namespace http_server
