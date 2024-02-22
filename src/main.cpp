#include <iostream>
#include <dpp/dpp.h>
#include <cstdlib>
#include <blt/std/logging.h>
#include <blt/parse/argparse.h>

int main(int argc, const char** argv)
{
    blt::arg_parse parser;
    parser.addArgument(blt::arg_builder("-t", "--token").setDefault("uWU").setAction(blt::arg_action_t::STORE).setHelp("The discord bot token").setRequired().build());
    
//    for (int i = 0; i < argc; i++)
//        BLT_TRACE(argv[i]);
    
    auto args = parser.parse_args(argc, argv);
    
    BLT_TRACE(args.get<std::string>("--token"));
    BLT_TRACE(argv[argc-1]);
    dpp::cluster bot(argv[argc-1]);
    
    bot.on_slashcommand([](auto event) {
        if (event.command.get_command_name() == "ping")
        {
            event.reply("Pong!");
        }
    });
    
    bot.on_ready([&bot](auto event) {
        if (dpp::run_once<struct register_bot_commands>())
        {
            bot.global_command_create(
                    dpp::slashcommand("ping", "Ping pong!", bot.me.id)
            );
        }
    });
    
    bot.on_message_create([&bot](auto event) {
        BLT_TRACE("(%s)> %s", event.msg.author.username.c_str(), event.msg.content.c_str());
        if (event.msg.author.username == "")
        {
        
        }
    });
    
    bot.start(dpp::st_wait);
    return 0;
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
