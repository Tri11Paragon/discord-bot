#include <iostream>
#include <dpp/dpp.h>
#include <cstdlib>
#include <blt/std/logging.h>
#include <blt/parse/argparse.h>

int main(int argc, const char** argv)
{
    blt::arg_parse parser;
    parser.addArgument(blt::arg_builder("--token").setRequired().build());
    
    auto args = parser.parse_args(argc, argv);
    
    dpp::cluster bot(args.get<std::string>("--token"));
    
    bot.on_slashcommand([](auto event) {
        if (event.command.get_command_name() == "ping") {
            event.reply("Pong!");
        }
    });
    
    bot.on_ready([&bot](auto event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_command_create(
                    dpp::slashcommand("ping", "Ping pong!", bot.me.id)
            );
        }
    });
    
    bot.start(dpp::st_wait);
    return 0;
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
