#include "model_serialization.h"
#include "player.h"
//LootSer
	model::LootSer::LootSer::LootSer() = default;
	model::LootSer::LootSer(const model::Loot& loot) :
		object_id_(loot.GetId()),
		index_(loot.GetIndex()),
		coord_(loot.GetCoord()),
		value_(loot.GetValue()),
		generation_id_(loot.GetGenerationId())
	{}

	std::shared_ptr<model::Loot> model::LootSer::Restore() const
	{
		std::shared_ptr<model::Loot> l_ptr = std::make_shared<model::Loot>(index_, coord_, value_);
		l_ptr->generation_id_ = generation_id_;
		l_ptr->object_id_ = object_id_;
		return l_ptr;
	}

	model::DogSer::DogSer() = default;
	model::DogSer::DogSer(const model::Dog& dog)
		: dir_(dog.GetDirection()),
		generation_id_(dog.GetGenerationId()),
		object_id_(dog.GetObjectId()),
		score_(dog.GetCurrentScore()),
		coords_(dog.GetCoords()),
		speed_(dog.GetSpeed()),
		default_speed_(dog.GetDefaultSpeed()),
		idle_time_(dog.GetIdleTime().count())
	{
		for (const auto& l : dog.GetLoot())
		{
			bag_.emplace_back(LootSer(*l));
		}
	}

//DogSer
	[[nodiscard]] std::shared_ptr<model::Dog> model::DogSer::Restore() const {
		std::shared_ptr<model::Dog> dog = std::make_shared<model::Dog>(dir_, default_speed_, coords_);
		dog->generation_id_ = generation_id_;
		dog->object_id_ = object_id_;
		dog->score_ = score_;
		dog->speed_ = speed_;
		dog->idle_time_ = std::chrono::duration<int64_t, std::milli>(idle_time_);

		std::vector<std::shared_ptr<model::Loot>> bag;
		for (const auto& l : bag_)
		{
			bag.push_back(l.Restore());
		}
		dog->bag_ = std::move(bag);
		return dog;
	}

//PlayerSer
	model::PlayerSer::PlayerSer() = default;
	model::PlayerSer::PlayerSer(Player& player)
		:
		generation_id_(player.GetGenerationId()),
		object_id_(player.GetObjectId()),
		dog_id(player.GetDog()->GetObjectId()),
		token_(player.GetPlayerToken()),
		player_name_(player.GetName()),
		play_time_(player.GetPlayTime().count())
	{}

	std::shared_ptr<Player> model::PlayerSer::Restore(std::shared_ptr<model::GameSession> s, std::shared_ptr<model::Dog> d)
	{
		std::shared_ptr<Player> p_ptr = std::make_shared<Player>(s, d, player_name_);
		p_ptr->generation_id_ = generation_id_;
		p_ptr->object_id_ = object_id_;
		p_ptr->token_ = token_;
		p_ptr->play_time_ = std::chrono::duration<int64_t, std::milli>(play_time_);
		return p_ptr;
	}
