#include <iostream>
#include <dpp/dpp.h>
#include <cstdlib>
#include <blt/std/logging.h>
#include <blt/parse/argparse.h>
#include <filemanager.h>


int main(int argc, const char** argv)
{
    blt::arg_parse parser;
    parser.addArgument(blt::arg_builder("-t", "--token").setAction(blt::arg_action_t::STORE).setHelp("The discord bot token").build());
    parser.addArgument(blt::arg_builder("-p", "--path").setAction(blt::arg_action_t::STORE).setHelp("Path to store the archive data").build());
    
    auto args = parser.parse_args(argc, argv);
    
    dpp::cluster bot(args.get<std::string>("token"), dpp::i_default_intents | dpp::i_message_content | dpp::i_all_intents);
    
    db::fs_manager manager{args.get<std::string>("path"), bot};
    
    bot.start_timer([&manager](auto) {
        manager.flush();
    }, 60);
    
    bot.on_message_delete([&bot](const dpp::message_delete_t& event) {
    
    });
    
    bot.on_message_delete_bulk([&bot](const dpp::message_delete_bulk_t& event) {
    
    });
    
    bot.on_message_update([&bot](const dpp::message_update_t& event) {
    
    });
    
    bot.on_message_create([&bot](const dpp::message_create_t& event) {
        if (event.msg.id == bot.me.id)
            return;
        bot.guild_get(event.msg.guild_id, [](const dpp::confirmation_callback_t& con) {
            BLT_INFO("Guild name: %s", con.get<dpp::guild>().name.c_str());
        });
        BLT_TRACE("(%s)> %s", event.msg.author.username.c_str(), event.msg.content.c_str());
        for (const dpp::attachment& attach : event.msg.attachments)
        {
            BLT_INFO("\tAttachment: %s", attach.url.c_str());
        }
        for (const dpp::embed& embed : event.msg.embeds)
        {
            BLT_INFO("\tEmbedding: %s", embed.url.c_str());
        }
        //BLT_TRACE_STREAM << event.msg.channel_id.str() << " " << event.msg.channel_id.str() << " "  << "\n";
//        if (blt::string::toLowerCase(event.msg.author.username) != "autismbot" && blt::string::toLowerCase(event.msg.author.username) != "jewhack")
//        {
//            //event.reply("Test response " + event.msg.author.global_name);
//        }
    });
    
    bot.start(dpp::st_wait);
    return 0;
}
