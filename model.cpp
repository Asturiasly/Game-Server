#include "model.h"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <set>

namespace model {
using namespace std::literals;
// Road methods
bool Road::IsYRoad() const noexcept {
    return start_.x == end_.x;
}

bool Road::IsXRoad() const noexcept {
    return start_.y == end_.y;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

const EdgeCoords& Road::GetEdgeCoords() const noexcept
{
    return edge_;
}

void Road::CalculateEdgeCoords()
{
    std::pair<double, double> x_edge{ 0, 0 };
    std::pair<double, double> y_edge{ 0, 0 };

    bool is_x_road = false;

    if (this->IsXRoad())
        is_x_road = true;

    if (is_x_road)
    {
        int start_x = start_.x;
        int end_x = end_.x;
        if (IsInvertedStartX())//описания начала дороги может начинаться НЕ из меньшей координаты!
        {
            int temp = std::move(start_x);
            start_x = std::move(end_.x);
            end_x = std::move(temp);
        }

        x_edge = { start_x - HALF_ROAD_WIDTH, end_x + HALF_ROAD_WIDTH }; //В рамки допустимых координат дороги также входит любое допустимое смещение по X или Y
        y_edge = { start_.y - HALF_ROAD_WIDTH, start_.y + HALF_ROAD_WIDTH };
    }
    else
    {
        int start_y = start_.y;
        int end_y = end_.y;
        if (IsInvertedStartY())//описания начала дороги может начинаться НЕ из меньшей координаты!
        {
            int temp = std::move(start_y);
            start_y = std::move(end_y);
            end_y = std::move(temp);
        }

        y_edge = { start_y - HALF_ROAD_WIDTH, end_y + HALF_ROAD_WIDTH }; //В рамки допустимых координат дороги также входит любое допустимое смещение по X или Y
        x_edge = { start_.x - HALF_ROAD_WIDTH, start_.x + HALF_ROAD_WIDTH };
    }
    edge_ = std::move(EdgeCoords{ x_edge, y_edge });
}

bool Road::IsInvertedStartX() const
{
    return start_.x > end_.x;
}

bool Road::IsInvertedStartY() const
{
    return start_.y > end_.y;
}

/////////////////////////////////////////////////////////////////////////////////////////
//Map methods
using Id = util::Tagged<std::string, Map>;
using Roads = std::vector<Road>;
using Buildings = std::vector<Building>;
using Offices = std::vector<Office>;

Map::Map(Id id, std::string name) noexcept
    : id_(std::move(id))
    , name_(std::move(name)) {
}

const Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Offices& Map::GetOffices() const noexcept {
    return offices_;
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

void Map::AddDefaultMapSpeed(double speed)
{
    default_map_speed_ = { speed, speed };
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    }
    catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

const DogSpeed& Map::GetDogSpeed() const noexcept
{
    return default_map_speed_;
}

DogCoord Map::GetStartPosDog() const noexcept
{
    Point p = roads_[0].GetStart();
    double d_x = static_cast<double>(p.x);
    double d_y = static_cast<double>(p.y);
    return DogCoord{ d_x, d_y };
}

DogCoord Map::GetRandomPosDog() const noexcept
{
    int index = GetRandomRoadIndex();
    return GetRandomCoord(index);
}

LootCoord Map::GetRandomPosLoot() const noexcept
{
    return GetRandomPosDog();
}

int Map::GetRandomRoadIndex() const
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, static_cast<int>(roads_.size() - 1));

    return dist(rng);
}

int Map::GetRandomLootTypeIndex(size_t loot_types_size) const
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, static_cast<int>(loot_types_size - 1));

    return dist(rng);
}

DogCoord Map::GetRandomCoord(const int index) const
{
    Road road = roads_[index];
    std::random_device dev;
    std::mt19937 rng(dev());

    double start_x = static_cast<double>(road.GetStart().x - HALF_ROAD_WIDTH);
    double end_x = static_cast<double>(road.GetEnd().x + HALF_ROAD_WIDTH);

    double start_y = static_cast<double>(road.GetStart().y - HALF_ROAD_WIDTH);
    double end_y = static_cast<double>(road.GetEnd().y + HALF_ROAD_WIDTH);

    //Проверки необходимы, поскольку uniform_real_distribution строго требует a <= b в конструкторе.
    if (road.IsInvertedStartX())
    {
        double temp = std::move(start_x);
        start_x = std::move(end_x);
        end_x = std::move(temp);
    }
    if (road.IsInvertedStartY())
    {
        double temp = std::move(start_y);
        start_y = std::move(end_y);
        end_y = std::move(temp);
    }

    std::uniform_real_distribution<double> dist(start_x, end_x);
    std::uniform_real_distribution<double> dist2(start_y, end_y);

    return { dist(rng), dist2(rng) };
}

std::vector<Road> Map::WhatRoadsDogOn(const DogCoord coords) const
{
    std::vector<Road> roads;
    for (size_t i = 0; i < roads_.size(); ++i)
    {
        Road road = roads_[i];
        double start_x = static_cast<double>(road.GetStart().x);
        double end_x = static_cast<double>(road.GetEnd().x);
        double start_y = static_cast<double>(road.GetStart().y);
        double end_y = static_cast<double>(road.GetEnd().y);

        bool is_x_road = road.IsXRoad();

        if (road.IsInvertedStartX() || road.IsInvertedStartY())
        {
            if (is_x_road)
            {
                double temp_x = std::move(start_x);
                start_x = std::move(end_x);
                end_x = std::move(temp_x);
            }
            else
            {
                double temp_y = std::move(start_y);
                start_y = std::move(end_y);
                end_y = std::move(temp_y);
            }
        }

        bool is_point_inside_road = 
            coords.x >= start_x - HALF_ROAD_WIDTH &&
            coords.x <= end_x + HALF_ROAD_WIDTH &&
            coords.y >= start_y - HALF_ROAD_WIDTH &&
            coords.y <= end_y + HALF_ROAD_WIDTH;

        if (is_point_inside_road)
        {
            roads.push_back(road);
            continue;
        }
    }
    return roads; // молимся о NRVO
}

void Map::SetLootGenerator(std::unique_ptr<loot_gen::LootGenerator> gen)
{
    loot_gen_ = std::move(gen);
}

const std::unique_ptr<loot_gen::LootGenerator>* Map::GetLootGenerator() const
{
    return &loot_gen_;
}

void Map::SetBagCapacity(int capacity)
{
    bag_capacity_ = std::move(capacity);
}

int Map::GetBagCapacity() const
{
    return bag_capacity_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//DogMethods

Dog::Dog(Direction dir, DogSpeed speed, DogCoord start_pos) : coords_(std::move(start_pos)), default_speed_(std::move(speed)), dir_(std::move(dir)), idle_time_(0)
{
    ++generation_id_;
    object_id_ = generation_id_;
}

const int Dog::GetGenerationId() const noexcept {
    return generation_id_;
}

const int Dog::GetObjectId() const noexcept
{
    return object_id_;
}

const DogCoord& Dog::GetCoords() const noexcept
{
    return coords_;
}

const DogSpeed& Dog::GetSpeed() const noexcept
{
    return speed_;
}

const DogSpeed& Dog::GetDefaultSpeed() const noexcept
{
    return default_speed_;
}

const Direction Dog::GetDirection() const noexcept
{
    return dir_;
}

void Dog::SetCoords(DogCoord coord)
{
    coords_ = std::move(coord);
}

void Dog::StopDog()
{
    speed_.speed_x = 0;
    speed_.speed_y = 0;
}

void Dog::SetInGameSpeed()
{
    switch (dir_)
    {
    case Direction::UP:
    {
        if (speed_.speed_x != 0)
        {
            speed_.speed_x = 0;
            speed_.speed_y = default_speed_.speed_y * -1;
        }
        else
            speed_.speed_y = default_speed_.speed_y * -1;
        break;
    }
    case Direction::DOWN:
    {
        if (speed_.speed_x != 0)
        {
            speed_.speed_x = 0;
            speed_.speed_y = default_speed_.speed_y;
        }
        else
            speed_.speed_y = default_speed_.speed_y;
        break;
    }
    case Direction::LEFT:
    {
        if (speed_.speed_y != 0)
        {
            speed_.speed_y = 0;
            speed_.speed_x = default_speed_.speed_x * -1;
        }
        else
            speed_.speed_x = default_speed_.speed_x * -1;
        break;
    }
    case Direction::RIGHT:
    {
        if (speed_.speed_y != 0)
        {
            speed_.speed_y = 0;
            speed_.speed_x = default_speed_.speed_x;
        }
        else
            speed_.speed_x = default_speed_.speed_x;
        break;
    }
    }
}

DogCoord Dog::CalculateCoordinates(const std::chrono::milliseconds deltaTime) const
{
    DogCoord coords = coords_;
    double temp = static_cast<double>((deltaTime.count()));
    double time = temp / 1000;
    double multiply_x = speed_.speed_x * time;
    double multiply_y = speed_.speed_y * time;

    if (dir_ == Direction::LEFT || dir_ == Direction::RIGHT)
        coords.x += multiply_x;
    else
        coords.y += multiply_y;

    return coords;
}

void Dog::SetInGameDirection(Direction dir)
{
    dir_ = std::move(dir);
}

const std::vector<std::shared_ptr<Loot>>& Dog::GetLoot() const
{
    return bag_;
}

void Dog::DropLoot()
{
    for (size_t i = 0; i < bag_.size(); ++i)
    {
        score_ += bag_[i]->GetValue();
    }

    bag_.clear();
}

void Dog::AddLootElem(const std::shared_ptr<Loot> loot)
{
    std::vector <std::shared_ptr<Loot>> t;
    t.resize(bag_.size() + 1);

    for (size_t i = 0; i < t.size(); ++i)
    {
        if (i == bag_.size())
        {
            t[i] = loot;
            break;
        }
        t[i] = bag_[i];
    }
    bag_.clear();
    bag_.resize(bag_.size() + 1);
    bag_ = std::move(t);
}

int Dog::GetCurrentScore() const
{
    return score_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//Game methods


using Maps = std::vector<std::shared_ptr<Map>>;

const Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const std::shared_ptr<Map> Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return maps_.at(it->second);
    }
    return nullptr;
}

void Game::AddSession(std::shared_ptr<GameSession> session, std::string map_id)
{
    sessions_[map_id] = std::move(session);
}

void Game::UpdateSession(std::shared_ptr<GameSession> session, std::string map_id)
{
    Game::AddSession(session, map_id);
}

bool Game::IsSessionExist(std::string map_id) const
{
    if (sessions_.contains(map_id))
        return true;
    return false;
}

bool Game::IsAnySession() const
{
    return sessions_.size() != 0;
}

const std::shared_ptr<GameSession> Game::GetSession(std::string map_id) const noexcept
{
    if (sessions_.contains(map_id))
        return sessions_.at(map_id);
    return nullptr;
}


void Game::UpdateGameState(const std::chrono::duration<double, std::milli> deltaTime)
{
    std::chrono::milliseconds time = std::chrono::duration_cast<std::chrono::milliseconds>(deltaTime);

    for (const auto& session_ : this->sessions_)
    {
        auto dogs = session_.second->GetDogs();
        auto map = session_.second->GetMap();
        collision_detector::Provider provider{};

        GenerateLoot(provider, map, dogs.size(), time);
        CalculatePositions(provider, dogs, map, time);
        FindGatherEvents(provider, map, session_.second);
    }
}

const std::unordered_map<std::string, std::shared_ptr<GameSession>>& Game::GetSessions() const noexcept
{
    return sessions_;
}

void Game::AddMap(std::shared_ptr<Map> map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map->GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map->GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

void Game::SetExtraData(ExtraData json_data)
{
    data_ = std::move(json_data);
}

const ExtraData& Game::GetExtraData() const
{
    return data_;
}

void Game::GenerateLoot(collision_detector::Provider& provider, std::shared_ptr<Map> map, const unsigned dogs_count, const std::chrono::milliseconds time)
{
    unsigned new_loot_count = map->GetLootGenerator()->get()->Generate(time, map->GetLootCount(), static_cast<unsigned>(dogs_count));
    auto loot_types = this->GetExtraData().GetJSONLootType().at(*map->GetId());
    for (unsigned i = 0; i < new_loot_count; ++i)
    {
        int index = map->GetRandomLootTypeIndex(loot_types.size());
        auto random_pos = map->GetRandomPosLoot();
        std::shared_ptr<Loot> ptr_loot = std::make_shared<Loot>(index, random_pos, static_cast<int>(data_.GetJSONLootType().at(*map->GetId())[index].at("value").get_int64()));
        map->AddLoot(ptr_loot);
    }
}
void Game::CalculatePositions(collision_detector::Provider& provider, 
    std::unordered_map<int, std::shared_ptr<model::Dog>>& dogs, 
    std::shared_ptr<Map> map, 
    const std::chrono::milliseconds time)
{
    DogCoord new_coords = { 0.0, 0.0 };
    DogCoord old_coords = { 0.0, 0.0 };
    for (const auto& [id, dog] : dogs)
    {
        old_coords.x = dog->GetCoords().x;
        old_coords.y = dog->GetCoords().y;

        new_coords = dog->CalculateCoordinates(time);
        std::vector<Road> new_roads = map->WhatRoadsDogOn(new_coords);
        bool is_on_road = new_roads.size() != 0;

        if (is_on_road)
        {
            dog->SetCoords(new_coords);
            collision_detector::Gatherer gatherer{ id, {old_coords.x, old_coords.y}, {new_coords.x, new_coords.y}, HALF_DOG_WIDTH };
            provider.AddGatherer(gatherer);
            continue;
        }

        Direction dir = dog->GetDirection();
        std::vector<Road> roads = map->WhatRoadsDogOn(old_coords);
        bool crossroad = roads.size() > 1;

        if (dir == Direction::UP || dir == Direction::DOWN)
        {
            EdgeCoords edge;

            if (!crossroad)
            {
                Road road = roads.back();
                edge = road.GetEdgeCoords();
                dir == Direction::UP ? new_coords.y = edge.y_edge.first : new_coords.y = edge.y_edge.second;
                dog->StopDog();
                dog->SetCoords(new_coords);
                collision_detector::Gatherer gatherer{ id, {old_coords.x, old_coords.y}, {new_coords.x, new_coords.y}, HALF_DOG_WIDTH };
                provider.AddGatherer(gatherer);
                continue;
            }

            //Важным моментом является отсутствие продолжающихся дорог, что сокращает количество условий и позволяет интерпретировать ситуацию перекрестка
            //как 2 дороги, а не 4. Поэтому достаточно найти единственную дорогу Y в векторе дорог предыдущей позиции. 
            //За счет метода WhatRoadsDogOn решение адаптивно под продолжающиеся дороги при необходимости.

            auto y_road_iterator = std::find_if(roads.begin(), roads.end(), [](Road r)
                {
                    return r.IsYRoad();
                });
            edge = y_road_iterator.operator*().GetEdgeCoords();
            dir == Direction::UP ? new_coords.y = edge.y_edge.first : new_coords.y = edge.y_edge.second;
        }
        else
        {
            EdgeCoords edge;

            if (!crossroad)
            {
                Road road = roads.back();
                edge = road.GetEdgeCoords();
                dir == Direction::LEFT ? new_coords.x = edge.x_edge.first : new_coords.x = edge.x_edge.second;
                dog->StopDog();
                dog->SetCoords(new_coords);
                collision_detector::Gatherer gatherer{ id, {old_coords.x, old_coords.y}, {new_coords.x, new_coords.y}, HALF_DOG_WIDTH };
                provider.AddGatherer(gatherer);
                continue;
            }

            //Важным моментом является отсутствие продолжающихся дорог, что сокращает количество условий и позволяет интерпретировать ситуацию перекрестка
            //как 2 дороги, а не 4. Поэтому достаточно найти единственную дорогу X в векторе дорог предыдущей позиции. 
            //За счет метода WhatRoadsDogOn решение адаптивно под продолжающиеся дороги при необходимости.

            auto x_road_iterator = std::find_if(roads.begin(), roads.end(), [](Road r)
                {
                    return r.IsXRoad();
                });
            edge = x_road_iterator.operator*().GetEdgeCoords();
            dir == Direction::LEFT ? new_coords.x = edge.x_edge.first : new_coords.x = edge.x_edge.second;
        }
        dog->StopDog();
        dog->SetCoords(new_coords);
        collision_detector::Gatherer gatherer{ id, {old_coords.x, old_coords.y}, {new_coords.x, new_coords.y}, HALF_DOG_WIDTH };
        provider.AddGatherer(gatherer);
    }
}
void Game::FindGatherEvents(collision_detector::Provider& provider, std::shared_ptr<Map> map, const std::shared_ptr<GameSession> session)
{
    for (const auto& loot : map->GetMapLoot())
    {
        LootCoord pos = loot->GetCoord();
        collision_detector::Item item{ loot->GetIndex(), {pos.x, pos.y}, ITEM_WIDTH };
        provider.AddItem(item);
    }

    for (const auto& office : map->GetOffices())
    {
        double curr_position_x = static_cast<double>(office.GetPosition().x);
        double curr_position_y = static_cast<double>(office.GetPosition().y);
        collision_detector::Item item{ .position{curr_position_x, curr_position_y}, .width = HALF_OFFICE_WIDTH, .is_office = true };
        provider.AddItem(item);
    }

    auto collected_loot = collision_detector::FindGatherEvents(provider);
    if (collected_loot.empty())
        return;


    //"C:\\Users\\impro\\Desktop\\fall_logs\\test_log.txt"
    std::set<geom::Point2D> deleted_poses;
    try
    {
        for (const auto& loot_event : collected_loot)
        {
            auto item = provider.GetItem(loot_event.item_id);
            auto pos = item.position;
            auto dog_gatherer = session->GetDogs().at(provider.GetGatherer(loot_event.gatherer_id).gatherer_id);

            if (!item.is_office)
            {
                if (deleted_poses.count(pos))
                    continue;

                std::pair<size_t, std::shared_ptr<Loot>> index_loot = map->GetRequiredMapLootByPos({ pos.x, pos.y });

                if (dog_gatherer->GetLoot().size() < map->GetBagCapacity())
                {
                    dog_gatherer->AddLootElem(index_loot.second);

                    std::fstream out;
                    if (!out.is_open())
                    {
                        out.open("/tmp/volume/log.txt", std::ios::out | std::ios::app);
                    }

                    out << "START DELETE REQUIREDLOOTBYINDX" << std::endl;
                    map->DeleteRequiredLootByIndx(index_loot.first);
                    deleted_poses.insert(pos);
                    out << "END DELETE REQUIREDLOOTBYINDX" << std::endl;
                }
            }
            else
                dog_gatherer->DropLoot();
        }
    }
    catch (std::exception& ex)
    {
        std::fstream out;
        if (!out.is_open())
        {
            out.open("/tmp/volume/log.txt", std::ios::out | std::ios::app);
        }
        out << "OOPS, EXCEPTION" << std::endl;
        out << ex.what() << std::endl;
    }

    std::fstream out;
    if (!out.is_open())
    {
        out.open("/tmp/volume/log.txt", std::ios::out | std::ios::app);
    }
    out << "END OF CYCLE OF LOOT_EVENT" << std::endl;
}

}  // namespace model