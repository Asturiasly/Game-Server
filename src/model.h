#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <random>
#include <iostream>
#include <chrono>

#include "collision_detector.h"
#include "loot_generator.h"
#include "tagged.h"
#include "extra_data.h"
//#include "model_serialization.h"
using namespace std::chrono_literals;

namespace model {

constexpr double ITEM_WIDTH = 0.0;
constexpr double HALF_DOG_WIDTH = 0.3;
constexpr double HALF_ROAD_WIDTH = 0.4;
constexpr double HALF_OFFICE_WIDTH = 0.25;

using Dimension = int;
using Coord = Dimension;
enum class Direction : char
{
    UP = 'U',
    DOWN = 'D',
    LEFT = 'L',
    RIGHT = 'R'
};

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct DogCoord
{
    double x;
    double y;
};

using LootCoord = DogCoord;

struct EdgeCoords
{
    std::pair<double, double> x_edge;
    std::pair<double, double> y_edge;
};

struct DogSpeed
{
    double speed_x;
    double speed_y;
};

class Road {
private:
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };
public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{ start }
        , end_{ end_x, start.y } 
    {
        CalculateEdgeCoords();
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{ start }
        , end_{ start.x, end_y } 
    {
        CalculateEdgeCoords();
    }

    bool IsYRoad() const noexcept;
    bool IsXRoad() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;
    const EdgeCoords& GetEdgeCoords() const noexcept;
    bool IsInvertedStartX() const;
    bool IsInvertedStartY() const;

private:
    Point start_;
    Point end_;
    EdgeCoords edge_;

    void CalculateEdgeCoords();
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{std::move(position)}
        , offset_{std::move(offset)} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Loot
{
    friend class LootSer;
public:
    Loot() = default;
    Loot(int index, LootCoord coord, int value) : index_(index), coord_(coord), value_(value)
    {
        ++generation_id_;
        object_id_ = generation_id_;
    }

    int GetId() const
    {
        return object_id_;
    }

    int GetIndex() const
    {
        return index_;
    }

    LootCoord GetCoord() const
    {
        return coord_;
    }

    int GetValue() const
    {
        return value_;
    }

    int GetGenerationId() const
    {
        return generation_id_;
    }
private:
    static inline int generation_id_ = -1;
    int object_id_;
    int index_;
    LootCoord coord_;
    int value_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept;
    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;
    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddDefaultMapSpeed(const double speed);
    void AddOffice(const Office office);
    const DogSpeed& GetDogSpeed() const noexcept;
    DogCoord GetStartPosDog() const noexcept;
    DogCoord GetRandomPosDog() const noexcept;
    int GetRandomLootTypeIndex(size_t loot_types_size) const;
    LootCoord GetRandomPosLoot() const noexcept;
    std::vector<Road> WhatRoadsDogOn(const DogCoord coords) const;
    void SetLootGenerator(std::unique_ptr<loot_gen::LootGenerator>);
    const std::unique_ptr<loot_gen::LootGenerator>* GetLootGenerator() const;
    void SetBagCapacity(int capacity);
    int GetBagCapacity() const;

    void AddLoot(std::shared_ptr<Loot> ptr_loot)
    {
        std::vector <std::shared_ptr<Loot>> t;
        t.resize(all_loot_.size() + 1);

        for (size_t i = 0; i < t.size(); ++i)
        {
            if (i == all_loot_.size())
            {
                t[i] = ptr_loot;
                break;
            }
            t[i] = all_loot_[i];
        }
        all_loot_ = std::move(t);
    }

    const std::vector<std::shared_ptr<Loot>>& GetMapLoot() const
    {
        return all_loot_;
    }

    int GetLootCount() const
    {
        return static_cast<int>(all_loot_.size());
    }

    const std::pair<size_t, std::shared_ptr<Loot>> GetRequiredMapLootByPos(LootCoord pos) const
    {
        for (size_t i = 0; i < all_loot_.size(); ++i)
        {
            auto loot_pos = all_loot_[i]->GetCoord();

            if (std::fabs(pos.x - loot_pos.x) < std::numeric_limits<double>::epsilon()
                &&
                std::fabs(pos.y - loot_pos.y) < std::numeric_limits<double>::epsilon())
            {
                return std::make_pair(i, all_loot_[i]);
            }
        }

        throw std::logic_error("Can't get right loot by pos");
    }

    void DeleteRequiredLootByIndx(size_t indx)
    {
        std::vector <std::shared_ptr<Loot>> t;
        t.resize(all_loot_.size() - 1);
        bool erased_item_has_found = false;
        for (size_t i = 0; i < all_loot_.size(); ++i)
        {
            if (erased_item_has_found)
            {
                t[i - 1] = all_loot_[i];
                continue;
            }
            if (i == indx)
            {
                erased_item_has_found = true;
                continue;
            }

            t[i] = all_loot_[i];
        }
        all_loot_ = std::move(t);
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    DogSpeed default_map_speed_{ 0, 0 };
    std::unique_ptr<loot_gen::LootGenerator> loot_gen_;
    std::vector<std::shared_ptr<Loot>> all_loot_; //loot index and coords
    size_t bag_capacity_ = 0;

    int GetRandomRoadIndex() const;
    DogCoord GetRandomCoord(const int index) const;
};

class Dog
{
    friend class DogSer;
public:
    Dog(Direction dir, DogSpeed speed, DogCoord start_pos);
    const int GetGenerationId() const noexcept;
    const int GetObjectId() const noexcept;
    const DogCoord& GetCoords() const noexcept;
    const DogSpeed& GetSpeed() const noexcept;
    const DogSpeed& GetDefaultSpeed() const noexcept;
    const Direction GetDirection() const noexcept;
    void SetCoords(DogCoord coord);
    void StopDog();
    void SetInGameSpeed();
    DogCoord CalculateCoordinates(const std::chrono::milliseconds deltaTime) const;
    void SetInGameDirection(const Direction dir);
    const std::vector<std::shared_ptr<Loot>>& GetLoot() const;
    void AddLootElem(const std::shared_ptr<Loot>);
    void DropLoot();
    int GetCurrentScore() const;
    void AddIdleTime(std::chrono::milliseconds idle_time)
    {
        idle_time_ += idle_time;
    }

    void ClearIdleTime()
    {
        idle_time_ = 0ms;
    }

    std::chrono::milliseconds GetIdleTime() const
    {
        return idle_time_;
    }

private:
    Direction dir_;
    static inline int generation_id_ = -1;
    int object_id_;
    int score_ = 0;
    DogCoord coords_{ 0, 0 };
    DogSpeed speed_{ 0, 0 };
    DogSpeed default_speed_{ 0, 0 };
    std::vector<std::shared_ptr<Loot>> bag_;
    std::chrono::milliseconds idle_time_;
};

class GameSession
{
public:
    explicit GameSession(const std::shared_ptr<Map> map) : map_(map)
    {
        ++generation_id_;
        object_id_ = generation_id_;
    }

    void AddDog(std::shared_ptr<Dog> dog_ptr)
    {
        int id = dog_ptr->GetObjectId();
        dogs_.insert({id, dog_ptr});
    }

    const int& GetGenerationId() const noexcept 
    {
        return generation_id_;
    }

    const int GetObjectId() const noexcept
    {
        return object_id_;
    }

    const std::shared_ptr<Map> GetMap() const noexcept
    {
        return map_;
    }

    const std::unordered_map<int, std::shared_ptr<Dog>>& GetDogs() const noexcept
    {
        return dogs_;
    }

    void DeleteDog(int id)
    {
        auto it = dogs_.find(id);
        
        if (it == dogs_.end())
            throw std::runtime_error("Can't delete uncreated dog");
        dogs_.erase(it);
    }

private:
    static inline int generation_id_ = -1;
    int object_id_;
    const std::shared_ptr<Map> map_;
    std::unordered_map<int, std::shared_ptr<Dog>> dogs_;
};

class Game {
public:
    using Maps = std::vector<std::shared_ptr<Map>>;
    void AddMap(std::shared_ptr<Map> map);
    const Maps& GetMaps() const noexcept;
    const std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;
    void AddSession(std::shared_ptr<GameSession> session, std::string map_id);
    void UpdateSession(std::shared_ptr<GameSession> session, std::string map_id);
    bool IsSessionExist(const std::string map_id) const;
    bool IsAnySession() const;
    const std::unordered_map<std::string, std::shared_ptr<GameSession>>& GetSessions() const noexcept;
    const std::shared_ptr<GameSession> GetSession(const std::string map_id) const noexcept;
    void UpdateGameState(const std::chrono::duration<double, std::milli> deltaTime);
    void SetExtraData(ExtraData);
    const ExtraData& GetExtraData() const;

    void SetRetirementTime(std::chrono::milliseconds dog_retirement_time)
    {
        dog_retirement_time_ = std::move(dog_retirement_time);
    }
    std::chrono::milliseconds GetRetirementTime() const noexcept
    {
        return dog_retirement_time_;
    }

    void DeleteSession(std::string map_id)
    {
        auto it = sessions_.find(map_id);

        if (it == sessions_.end())
            throw std::runtime_error("Can't delete uncreated session");

        sessions_.erase(it);
    }

    std::vector<int> GetIdleDogs(const std::chrono::milliseconds time)
    {
        std::vector<int> idle_id;
        for (const auto& session : sessions_)
        {
            auto dogs = session.second->GetDogs();
            for (const auto& dog : dogs)
            {
                DogSpeed speed = dog.second->GetSpeed();
                if (speed.speed_x == 0 && speed.speed_y == 0)
                    dog.second->AddIdleTime(time);

                if (dog.second->GetIdleTime() >= dog_retirement_time_)
                {
                    idle_id.push_back(dog.first);
                    session.second->DeleteDog(dog.first);
                }
            }
        }
        return idle_id;
    }

    void DeleteEmptySessions()
    {
        if (sessions_.size() == 0)
            return;

        for (auto it = sessions_.begin(); it != sessions_.end();)
        {
            if (it->second->GetDogs().size() == 0)
            {
                it = sessions_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::unordered_map<std::string, std::shared_ptr<GameSession>> sessions_;
    std::vector<std::shared_ptr<Map>> maps_;
    MapIdToIndex map_id_to_index_;

    void GenerateLoot(collision_detector::Provider&, 
        std::shared_ptr<Map> map, 
        const unsigned dogs_count, 
        const std::chrono::milliseconds time);
    void CalculatePositions(collision_detector::Provider& provider, 
        std::unordered_map<int, std::shared_ptr<model::Dog>>& dogs, 
        std::shared_ptr<Map> map, 
        const std::chrono::milliseconds time);
    void FindGatherEvents(collision_detector::Provider& provider, 
        std::shared_ptr<Map> map, 
        const std::shared_ptr<GameSession> session);

    ExtraData data_;
    std::chrono::milliseconds dog_retirement_time_;
};

}  // namespace model
