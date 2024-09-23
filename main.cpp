#define BOOST_BEAST_USE_STD_STRING_VIEW
//#include "sdk.h" //включить, если не нужен boost.log
//
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <thread>
#include <filesystem>
#include <chrono>
#include <optional>

#include "application.h"
#include "json_support.h"
#include "http_server.h"
#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "ticker.h"
#include "database.h"
#include "connection_pool.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace http = boost::beast::http;
namespace fs = std::filesystem;
namespace keywords = boost::log::keywords;
using Strand = net::strand<net::io_context::executor_type>;

namespace th { 
// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
    }

}  // namespace

struct Args
{
    uint32_t tick_period;
    uint32_t save_period;
    std::filesystem::path config_path;
    std::filesystem::path data_path;
    std::filesystem::path save_path;
    bool randomize_spawn_points = false;
    bool save_mode = false;
    bool auto_save_mode = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{ "Allowed options"s };

    Args args;
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.tick_period)->multitoken()->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_path)->multitoken()->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.data_path)->multitoken()->value_name("dir"s), "set static files root")
        ("state-file", po::value(&args.save_path)->multitoken()->value_name("save_file"s), "set save file path")
        ("save-state-period", po::value(&args.save_period)->multitoken()->value_name("save_period"s), "set save period")
        ("randomize-spawn-points", "spawn dogs at random positions");

    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file"s))
    {
        throw std::runtime_error("Config file have not been specified"s);
    }
    if (!vm.contains("www-root"s))
    {
        throw std::runtime_error("Static data files path is not specified"s);
    }
    if (!vm.contains("tick-period"s))
        args.tick_period = 0;
    if (vm.contains("randomize-spawn-points"s))
        args.randomize_spawn_points = true;
    if (vm.contains("state-file"))
        args.save_mode = true;
    if (vm.contains("save-state-period") && args.save_mode == true)
        args.auto_save_mode = true;
    else
        args.save_period = 0;

    return args;
}

std::string GetTimeStamp()
{
    const auto t_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t_c), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

 int main(int argc, const char* argv[]) {
     //const char* db_url = "postgres://postgres:Asturias1997@localhost:5432/postgres";
     const char* db_url = std::getenv("GAME_DB_URL");
    try {
        if (!db_url)
        {
            throw std::runtime_error("DB URL is not specified");
        }

        std::optional<Args> args = ParseCommandLine(argc, argv);
        if (args == std::nullopt)
            return EXIT_FAILURE;

        bool is_auto_tick = args->tick_period != 0;
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(args->config_path);

        GameSettings settings
        { .is_auto_tick = args->tick_period != 0,
          .is_save_mode = args->save_mode,
          .is_auto_save_mode = args->save_period != 0,
          .randomize_spawn_points = args->randomize_spawn_points,
          .save_period = args->save_period
        };

        const unsigned num_threads = std::thread::hardware_concurrency();
        std::shared_ptr<ConnectionPool> pool_ptr = std::make_shared<ConnectionPool>(num_threads, [db_url] {
                 auto conn = std::make_shared<pqxx::connection>(db_url);
                 conn->prepare("select_one", "SELECT 1;");
                 return conn;
             });

        std::unique_ptr<postgres::Database> db = std::make_unique<postgres::Database>(pool_ptr);
        Application app(game, settings, args->save_path, std::move(db));
        app.Deserialize();

        net::io_context ioc(num_threads);
        auto api_strand = net::make_strand(ioc);
        // 3. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = std::make_shared<http_handler::RequestHandler>(app, args->data_path, api_strand);

        server_logging::LoggingRequestHandler logging_handler{
    [handler](auto&& req, auto&& send) {
                // Обрабатываем запрос
                (*handler)(
                    std::forward<decltype(req)>(req),
                    std::forward<decltype(send)>(send));
                    } };

        // 4. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, &logging_handler](const boost::system::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                logging_handler.LogServerStopped();
            }
            });
        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        if (is_auto_tick)
        {
            auto ticker = std::make_shared<Ticker>(api_strand, std::chrono::milliseconds(args->tick_period),
                [&app](std::chrono::milliseconds delta)
                { 
                  app.OnTick(delta);
                  app.DeleteUnusedInfo(delta);
                  app.SerializeOnTick(delta);
                }
            );
            ticker->Start();
        }

        http_server::ServeHttp(ioc, { address, port }, [&logging_handler](auto&& endp, auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(endp)>(endp), std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });
        
        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        logging_handler.LogServerStarted(port, address);
        // 6. Запускаем обработку асинхронных операций

        th::RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        if (settings.is_save_mode)
            app.Serialize();
    } catch (const std::exception& ex) 
    {
        auto err = json_support::MakeJSONExceptionLog(GetTimeStamp(), ex);
        std::cout << err << std::endl;
        return EXIT_FAILURE;
    }
}
