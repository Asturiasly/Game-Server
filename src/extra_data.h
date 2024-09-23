#pragma once
#include <boost/json/value.hpp>
#include <boost/json/object.hpp>
#include <boost/json.hpp>
#include <boost/json/string.hpp>
#include <boost/json/array.hpp>

#include <optional>
#include <unordered_map>
#include <string_view>

struct RecordsParams
{
    size_t start = 0;
    size_t maxItems = 100;
};

class ExtraData
{
public:

    void SetJSONLootTypes(std::string_view id, boost::json::array lootTypes)
    {
        lootTypes_[id] = std::move(lootTypes);
    }

    const std::unordered_map<std::string_view, boost::json::array>& GetJSONLootType() const
    {
        return lootTypes_;
    }

private:
    std::unordered_map<std::string_view, boost::json::array> lootTypes_;
};