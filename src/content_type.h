#pragma once
#include <string_view>
using namespace std::literals;
struct ContentType {
    ContentType() = delete;
    //JSON types
    constexpr static std::string_view JSON_APP = "application/json"sv;
    //MIME-types
    constexpr static std::string_view TEXT_HTML = "text/html";
    constexpr static std::string_view TEXT_CSS = "text/css";
    constexpr static std::string_view TEXT_TXT = "text/plain";
    constexpr static std::string_view TEXT_JS = "text/javascript";
    constexpr static std::string_view TEXT_JSON = "application/json";
    constexpr static std::string_view TEXT_XML = "application/xml";
    constexpr static std::string_view TEXT_PNG = "image/png";
    constexpr static std::string_view TEXT_JPG = "image/jpeg";
    constexpr static std::string_view TEXT_GIF = "image/gif";
    constexpr static std::string_view TEXT_BMP = "image/bmp";
    constexpr static std::string_view TEXT_ICO = "image/vnd.microsoft.icon";
    constexpr static std::string_view TEXT_TIF = "image/tiff";
    constexpr static std::string_view TEXT_SVG = "image/svg+xml";
    constexpr static std::string_view TEXT_MP3 = "audio/mpeg";
    constexpr static std::string_view TEXT_UNKNOWN = "application/octet-stream";
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};