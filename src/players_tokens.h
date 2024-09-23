#pragma once

#include <random>
#include <string>
#include <sstream>
#include <unordered_map>

#include "player.h"
#include "tagged.h"

namespace detail {
    struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class TokenHasher
{
public:
    size_t operator()(const Token& token) const
    {
        return hasher_(*token);
    }
private:
    std::hash<std::string> hasher_;
};

namespace token
{
    class PlayersTokens {
    public:

        PlayersTokens() = default;

        Token SetTokenForPlayer(std::shared_ptr<Player> player)
        {
            std::uint64_t num1 = generator1_();
            std::uint64_t num2 = generator2_();

            std::ostringstream oss;
            oss << std::hex << std::setw(16) << std::setfill('0') << num1;
            oss << std::hex << std::setw(16) << std::setfill('0') << num2;
            Token token = Token(oss.str());
            token_to_player_[token] = player;
            return token;
        }

        const std::shared_ptr<Player> FindPlayerByToken(Token token) const
        {
            if (token_to_player_.contains(token))
                return token_to_player_.at(token);
            return nullptr;
        }

        const size_t GetSizeOfTokens() const
        {
            return token_to_player_.size();
        }

        void AddTokenAndPlayer(std::shared_ptr<Player> player, Token tok)
        {
            token_to_player_[tok] = player;
        }

        void DeletePlayerByToken(Token token)
        {
            auto it = token_to_player_.find(token);
            if (it == token_to_player_.end())
                throw std::runtime_error("Can't delete uncreated player");
            token_to_player_.erase(it);
        }

    private:
        std::random_device random_device_;
        std::mt19937_64 generator1_{ [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }() };
        std::mt19937_64 generator2_{ [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }() };
        // Чтобы сгенерировать токен, получите из generator1_ и generator2_
        // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
        // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
        // чтобы сделать их подбор ещё более затруднительным
        std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> token_to_player_;
    };
}