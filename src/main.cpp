#include <iostream>
#include <dpp/dpp.h>
#include <cstdlib>
#include <utility>
#include <blt/std/logging.h>
#include <blt/parse/argparse.h>
#include <sqlite_orm/sqlite_orm.h>
#include "blt/std/types.h"

struct message_t
{
    blt::u64 messageID;
    blt::u64 channelID;
    blt::u64 userID;
    std::string content;
};

struct user_info_t
{
    blt::u64 userID;
    blt::u64 changeTime;
    std::string username;
    std::string global_nickname;
    std::string server_name;
};

struct channel_info_t
{
    blt::u64 channelID;
    blt::u64 changeTime;
    std::string channel_name;
};

struct message_edits_t
{
    blt::u64 messageID;
    blt::u64 channelID;
    blt::u64 userID;
    std::string new_content;
};


struct db_obj
{
    private:
        blt::u64 guildID;
        std::string path;
    public:
        db_obj(blt::u64 guildID, std::string_view path): guildID(guildID), path(path)
        {}
};

namespace db
{
    void sync_databases()
    {
        using namespace sqlite_orm;
        auto storage = make_storage(":memory:",
                                    make_table("messages", make_column("id", &message_t::userID, primary_key())));
    }
}

blt::hashmap_t<blt::u64, db_obj> databases;

int main(int argc, const char** argv)
{
    using namespace sqlite_orm;
    
    blt::arg_parse parser;
    parser.addArgument(blt::arg_builder("-t", "--token").setAction(blt::arg_action_t::STORE).setHelp("The discord bot token").build());
    parser.addArgument(blt::arg_builder("-p", "--path").setAction(blt::arg_action_t::STORE).setHelp("Path to store the archive data").build());
    
    auto args = parser.parse_args(argc, argv);
    
    dpp::cluster bot(args.get<std::string>("token"), dpp::i_default_intents | dpp::i_message_content | dpp::i_all_intents);
    
    bot.on_message_delete([&bot](const dpp::message_delete_t& event) {
    
    });
    
    bot.on_message_delete_bulk([&bot](const dpp::message_delete_bulk_t& event) {
    
    });
    
    bot.on_message_update([&bot](const dpp::message_update_t& event) {
        BLT_INFO("%ld (from user %ld in channel %ld ['%s']) -> '%s'", event.msg.id, event.msg.author.id, event.msg.channel_id,
                 event.msg.author.username.c_str(), event.msg.content.c_str());
    });
    
    bot.on_message_create([&bot](const dpp::message_create_t& event) {
        if (event.msg.id == bot.me.id)
            return;
        bot.guild_get(event.msg.guild_id, [](const dpp::confirmation_callback_t& con) {
            BLT_INFO("Guild name: %s", con.get<dpp::guild>().name.c_str());
        });
        
        BLT_TRACE("(%s '%s' aka '%s' with mention '%s')> %s", event.msg.author.username.c_str(), event.msg.author.global_name.c_str(),
                  event.msg.member.get_nickname().c_str(), event.msg.member.get_mention().c_str(), event.msg.content.c_str());
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
