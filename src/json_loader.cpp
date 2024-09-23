#include <chrono>

#include "json_loader.h"
#include "extra_data.h"

constexpr int BAG_CAPACITY_BY_DEFAULT = 3;
constexpr int RETIREMENT_TIME_BY_DEFAULT = 60; //1 min

namespace json_loader {
    using namespace std::chrono_literals;
    model::Game LoadGame(const std::filesystem::path& json_path) {
        // Загрузить содержимое файла json_path, например, в виде строки
        // Распарсить строку как JSON, используя boost::json::parse
        // Загрузить модель игры из файла

        std::string text = ReadJSONFromPath(json_path);
        auto json_text = boost::json::parse(text);

        double default_speed = 0;
        int dog_retirement_time = 0;

        int default_bag_capacity = BAG_CAPACITY_BY_DEFAULT;
        if (json_text.as_object().count("defaultDogSpeed"))
            default_speed = json_text.as_object().at("defaultDogSpeed").get_double();
        if (json_text.as_object().count("defaultBagCapacity"))
            default_bag_capacity = json_text.as_object().at("defaultBagCapacity").get_int64();
        if (json_text.as_object().count("dogRetirementTime"))
            dog_retirement_time = static_cast<int>(json_text.as_object().at("dogRetirementTime").get_double() * 1000);
        else
            dog_retirement_time = RETIREMENT_TIME_BY_DEFAULT;

        auto maps = json_text.as_object().at("maps").get_array();

        model::Game game;
        game.SetRetirementTime(std::chrono::milliseconds(dog_retirement_time));
        ExtraData json_data;
        for (size_t map_num = 0; map_num < maps.size(); ++map_num)
        {
            std::string id = std::string(maps[map_num].at("id").get_string());
            std::string name = std::string(maps[map_num].at("name").get_string());

            std::shared_ptr<model::Map> map = std::make_shared<model::Map>(model::Map::Id(id), name);

            bool is_defined_map_speed = maps[map_num].as_object().contains("dogSpeed");
            bool is_defined_map_bag_capacity = maps[map_num].as_object().contains("bagCapacity");
            is_defined_map_speed ? map->AddDefaultMapSpeed(maps[map_num].at("dogSpeed").get_double()) : map->AddDefaultMapSpeed(default_speed);
            is_defined_map_bag_capacity ? map->SetBagCapacity(maps[map_num].at("bagCapacity").get_int64()) : map->SetBagCapacity(default_bag_capacity);

            auto roads = maps[map_num].at("roads").get_array();
            AddingRoadsToMap(roads, *map);
            auto buildings = maps[map_num].at("buildings").get_array();
            AddingBuildingsToMap(buildings, *map);
            auto offices = maps[map_num].at("offices").get_array();
            AddingOfficesToMap(offices, *map);
            int seconds = static_cast<int>(json_text.as_object().at("lootGeneratorConfig").as_object().at("period").get_double() * 100);
            double probability = json_text.at("lootGeneratorConfig").as_object().at("probability").get_double();
            std::unique_ptr<loot_gen::LootGenerator> gen_ptr = std::make_unique<loot_gen::LootGenerator>(std::chrono::milliseconds(seconds), probability);
            map->SetLootGenerator(std::move(gen_ptr));

            auto loot_types = maps[map_num].at("lootTypes").get_array();
            json_data.SetJSONLootTypes(*map->GetId(), std::move(loot_types));

            game.AddMap(std::move(map));
        }
        game.SetExtraData(std::move(json_data));

        return game;
    }

void AddingRoadsToMap(const boost::json::array& roads, model::Map& map)
{
    for (size_t road_num = 0; road_num < roads.size(); ++road_num)
    {
        model::Coord x = static_cast<int>(roads[road_num].get_object().at("x0").get_int64());
        model::Coord y = static_cast<int>(roads[road_num].at("y0").get_int64());
        model::Point position = { x, y };

        if (roads[road_num].as_object().count("x1"))
        {
            model::Coord end = static_cast<int>(roads[road_num].get_object().at("x1").get_int64());
            map.AddRoad(model::Road(model::Road::HORIZONTAL, position, end));
        }
        else
        {
            model::Coord end = static_cast<int>(roads[road_num].get_object().at("y1").get_int64());
            map.AddRoad(model::Road(model::Road::VERTICAL, position, end));
        }
    }
}

void AddingBuildingsToMap(const boost::json::array& buildings, model::Map& map)
{
    for (size_t building_num = 0; building_num < buildings.size(); ++building_num)
    {
        model::Coord x = static_cast<int>(buildings[building_num].get_object().at("x").get_int64());
        model::Coord y = static_cast<int>(buildings[building_num].as_object().at("y").get_int64());

        model::Dimension w = static_cast<int>(buildings[building_num].get_object().at("w").get_int64());
        model::Dimension h = static_cast<int>(buildings[building_num].get_object().at("h").get_int64());

        model::Rectangle rect(model::Point(x, y), model::Size(w, h));

        map.AddBuilding(model::Building(std::move(rect)));
    }
}

void AddingOfficesToMap(const boost::json::array& offices, model::Map& map)
{
    for (size_t office_num = 0; office_num < offices.size(); ++office_num)
    {
        std::string office_id = std::string(offices[office_num].get_object().at("id").get_string());
        model::Coord x = static_cast<int>(offices[office_num].get_object().at("x").get_int64());
        model::Coord y = static_cast<int>(offices[office_num].get_object().at("y").get_int64());

        model::Dimension dx = static_cast<int>(offices[office_num].get_object().at("offsetX").get_int64());
        model::Dimension dy = static_cast<int>(offices[office_num].get_object().at("offsetY").get_int64());

        map.AddOffice(model::Office(model::Office::Id(office_id), model::Point(x, y), model::Offset(dx, dy)));
    }
}

std::string ReadJSONFromPath(const std::filesystem::path& json_path)
{
    std::ifstream input;
    std::string path = json_path.string();
    input.open(json_path);

    if (!input.is_open())
    {
        std::string err_mes = "Can't open the file: " + path;
        throw std::logic_error(err_mes);
    }

    std::string buffer;
    std::string text;
    while (getline(input, buffer)) {
        text += buffer;
    }
    return text;
}

std::string ReadJSONFromStream(std::stringstream& input)
{   
    std::string buffer;
    std::string text;
    while (std::getline(input, buffer)) {
        text += buffer;
    }
    return text;
}

}  // namespace json_loader
