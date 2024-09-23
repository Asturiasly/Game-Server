#pragma once

#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "model.h"

class Player;

namespace model
{
	template<typename Archive>
	void serialize(Archive& ar, model::Direction& dir, [[maybe_unused]] const unsigned version)
	{
		ar& dir;
	}

	template<typename Archive>
	void serialize(Archive& ar, model::Point& p, [[maybe_unused]] const unsigned version)
	{
		ar& p.x;
		ar& p.y;
	}

	template<typename Archive>
	void serialize(Archive& ar, model::Size& s, [[maybe_unused]] const unsigned version)
	{
		ar& s.height;
		ar& s.width;
	}

	template<typename Archive>
	void serialize(Archive& ar, model::Rectangle& r, [[maybe_unused]] const unsigned version)
	{
		ar& r.position;
		ar& r.size;
	}

	template<typename Archive>
	void serialize(Archive& ar, model::Offset& off, [[maybe_unused]] const unsigned version)
	{
		ar& off.dx;
		ar& off.dy;
	}

	template<typename Archive>
	void serialize(Archive& ar, model::DogCoord& dc, [[maybe_unused]] const unsigned version)
	{
		ar& dc.x;
		ar& dc.y;
	}

	template<typename Archive>
	void serialize(Archive& ar, model::DogSpeed& ds, [[maybe_unused]] const unsigned version)
	{
		ar& ds.speed_x;
		ar& ds.speed_y;
	}

	class LootSer
	{
	public:
		LootSer();
		explicit LootSer(const model::Loot& loot);
		std::shared_ptr<model::Loot> Restore() const;

		template <typename Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
			ar& generation_id_;
			ar& object_id_;
			ar& index_;
			ar& coord_;
			ar& value_;
		}

	private:
		int generation_id_;
		int object_id_;
		int index_;
		model::LootCoord coord_;
		int value_;
	};

	class DogSer {
	public:
		DogSer();
		explicit DogSer(const model::Dog& dog);
		[[nodiscard]] std::shared_ptr<model::Dog> Restore() const;

		template <typename Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
			ar& dir_;
			ar& generation_id_;
			ar& object_id_;
			ar& score_;
			ar& coords_;
			ar& speed_;
			ar& default_speed_;
			ar& bag_;
			ar& idle_time_; //œ–Œ“≈—“»–Œ¬¿“‹
		}

	private:
		Direction dir_;
		int generation_id_;
		int object_id_;
		int score_ = 0;
		DogCoord coords_{ 0, 0 };
		DogSpeed speed_{ 0, 0 };
		DogSpeed default_speed_{ 0, 0 };
		std::vector<LootSer> bag_;
		int64_t idle_time_;
	};

	class PlayerSer
	{
	public:
		PlayerSer();
		explicit PlayerSer(Player& player);
		std::shared_ptr<Player> Restore(std::shared_ptr<model::GameSession> s, std::shared_ptr<model::Dog> d);

		template <typename Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
			ar& generation_id_;
			ar& object_id_;
			ar& dog_id;
			ar& token_;
			ar& player_name_;
			ar& play_time_;
		}

	private:
		int generation_id_ = -1;
		int object_id_;
		int dog_id;
		std::string token_;
		std::string player_name_;
		int64_t play_time_;
	};

}

