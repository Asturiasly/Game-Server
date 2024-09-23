#pragma once
#include <string>
#include <string_view>
#include <filesystem>
#include <variant>

#include "content_type.h"

int constexpr hex_size = 2;

namespace http_handler_static
{
    using namespace std::literals;
    namespace http = boost::beast::http;
    namespace json = boost::json;
    namespace sys = boost::system;
    namespace fs = std::filesystem;
    using StringResponse = http::response<http::string_body>;
    using FileResponse = http::response<http::file_body>;
    using FileResponseResult = std::variant<StringResponse, FileResponse>;

    const std::unordered_map<std::string, std::string_view> supported_extensions = {
        { ".json", ContentType::TEXT_JSON },
        { ".htm",  ContentType::TEXT_HTML },
        { ".html",  ContentType::TEXT_HTML },
        { ".css",  ContentType::TEXT_CSS },
        { ".txt",  ContentType::TEXT_TXT },
        { ".js",  ContentType::TEXT_JS },
        { ".xml",  ContentType::TEXT_XML },
        { ".png",  ContentType::TEXT_PNG },
        { ".jpg",  ContentType::TEXT_JPG },
        { ".jpe",  ContentType::TEXT_JPG },
        { ".jpeg",  ContentType::TEXT_JPG },
        { ".gif",  ContentType::TEXT_GIF },
        { ".bmp",  ContentType::TEXT_BMP },
        { ".ico",  ContentType::TEXT_ICO },
        { ".tiff",  ContentType::TEXT_TIF },
        { ".tif",  ContentType::TEXT_TIF },
        { ".svg",  ContentType::TEXT_SVG },
        { ".svgz",  ContentType::TEXT_SVG },
        { ".mp3",  ContentType::TEXT_MP3 },
        { ".fbx", ContentType::TEXT_UNKNOWN },
        { ".obj", ContentType::TEXT_UNKNOWN},
        { ".webmanifest", ContentType::TEXT_UNKNOWN}
    };

    class RequestHandlerStatic
    {
    public:
        explicit RequestHandlerStatic(fs::path game_data) : game_data_(game_data) {}
        RequestHandlerStatic(const RequestHandlerStatic&) = delete;
        RequestHandlerStatic& operator=(const RequestHandlerStatic&) = delete;

        template <typename Body>
        FileResponseResult operator()(Body&& req)
        {
             return GetStaticResponse(req);
        }

    private:
        fs::path game_data_;

        template <typename Body>
        FileResponseResult GetStaticResponse(Body&& req)
        {
            std::string request = std::string(req.target());
            std::string decoded_uri = URIDecoding(request);
            fs::path request_path = game_data_.string() + decoded_uri;
            bool is_correct = IsCorrectPath(request_path, game_data_);

            if (request == "/"s)
            {
                fs::path request_path = game_data_.string() + "/index.html";
                fs::path ext = ".html";
                return GetFileStaticDataResponse(request_path, ext);
            }
            else if (is_correct)
            {
                auto extension = request_path.extension();

                if (!supported_extensions.contains(extension.string()))
                    return GetWrongExtensionStaticDataResponse(req.version(), req.keep_alive());
                else
                {
                    if (fs::exists(request_path))
                        return GetFileStaticDataResponse(request_path, extension);
                    else
                        return GetNotFoundStaticDataResponse(req.version(), req.keep_alive());
                }
            }
            else
                return GetBadRequestStaticDataResponse(req.version(), req.keep_alive());
        }

        StringResponse GetBadRequestStaticDataResponse(const unsigned int version, const bool keep_alive)
        {
            StringResponse response(http::status::bad_request, version);
            std::string_view content_type = ContentType::TEXT_HTML;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = "Bad request";
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetNotFoundStaticDataResponse(const unsigned int version, const bool keep_alive)
        {
            StringResponse response(http::status::not_found, version);
            std::string_view content_type = ContentType::TEXT_TXT;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = "File does not exist";
            response.keep_alive(keep_alive);
            return response;
        }

        FileResponse GetFileStaticDataResponse(fs::path& request_path, fs::path& extension)
        {
            FileResponse response;
            response.version(11);
            response.result(http::status::ok);
            std::string_view content_type = supported_extensions.at(extension.string());
            response.insert(http::field::content_type, content_type);
            http::file_body::value_type body;

            if (sys::error_code ec; body.open(request_path.string().c_str(), boost::beast::file_mode::read, ec), ec) {
                std::cout << ec.message() + request_path.string() << std::endl;
            }

            response.body() = std::move(body);
            response.prepare_payload();
            return response;
        }


        StringResponse GetWrongExtensionStaticDataResponse(const unsigned int version, const bool keep_alive)
        {
            StringResponse response(http::status::bad_request, version);
            std::string_view content_type = ContentType::TEXT_UNKNOWN;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = "Extension is incoreect or unsupported";
            response.keep_alive(keep_alive);
            return response;
        }

        StringResponse GetEmptyStaticDataResponse(const unsigned int version, const bool keep_alive)
        {
            StringResponse response(http::status::ok, version);
            std::string_view content_type = ContentType::TEXT_HTML;
            response.set(http::field::content_type, content_type);
            response.set(http::field::cache_control, "no-cache"sv);
            response.body() = "";
            response.keep_alive(keep_alive);
            return response;
        }

        std::string URIDecoding(std::string request)
        {
            std::string decoding_str;
            for (int i = 0; i < request.size(); ++i)
            {
                if (request[i] == '+')
                    decoding_str.push_back(' ');
                else if (request[i] == '%' && i + hex_size < request.size())
                {
                        std::string hex;
                        hex.push_back(request[i + 1]);
                        hex.push_back(request[i + hex_size]);
                        i = i + hex_size;
                        int hex_val;
                        std::istringstream(hex) >> std::hex >> hex_val;
                        char c = static_cast<char>(hex_val);
                        decoding_str.push_back(c);
                }
                else
                {
                    decoding_str.push_back(request[i]);
                }
            }
            return decoding_str;
        }

        bool IsCorrectPath(fs::path path, fs::path base)
        {
            // Приводим оба пути к каноничному виду (без . и ..)
            path = fs::weakly_canonical(path);
            base = fs::weakly_canonical(base);

            // Проверяем, что все компоненты base содержатся внутри path
            for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
                if (p == path.end() || *p != *b) {
                    return false;
                }
            }
            return true;
        }
    };
}