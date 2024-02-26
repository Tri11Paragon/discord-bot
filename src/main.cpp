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

struct attachment_t
{
    blt::u64 messageID;
    blt::u64 channelID;
    std::string url;
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

struct message_deletes_t
{
    blt::u64 messageID;
    blt::u64 channelID;
};


struct db_obj
{
    private:
        blt::u64 guildID;
    public:
        std::vector<message_t> messages;
        std::vector<user_info_t> user_data;
        std::vector<channel_info_t> channel_data;
        std::vector<message_edits_t> message_edits;
        std::vector<message_deletes_t> message_deletes;
        std::vector<attachment_t> attachments;
    public:
        explicit db_obj(blt::u64 guildID): guildID(guildID)
        {}
        
        void dump()
        {
        
        }
        
        void check_for_updates(dpp::cluster& bot)
        {
        
        }
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

db_obj& get(blt::u64 id)
{
    if (databases.find(id) == databases.end())
        databases.insert({id, db_obj{id}});
    return databases.at(id);
}

int main(int argc, const char** argv)
{
    using namespace sqlite_orm;
    
    blt::arg_parse parser;
    parser.addArgument(blt::arg_builder("-t", "--token").setAction(blt::arg_action_t::STORE).setHelp("The discord bot token").build());
    parser.addArgument(blt::arg_builder("-p", "--path").setAction(blt::arg_action_t::STORE).setHelp("Path to store the archive data").build());
    
    auto args = parser.parse_args(argc, argv);
    
    dpp::cluster bot(args.get<std::string>("token"), dpp::i_default_intents | dpp::i_message_content | dpp::i_all_intents);
    
    bot.on_message_delete([&bot](const dpp::message_delete_t& event) {
        BLT_DEBUG("Message %ld deleted content in %ld", event.id, event.channel_id);
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
        if (blt::string::contains(event.msg.content, "/dump"))
        {
            for (auto g : databases)
                g.second.dump();
        }
        auto& storage = get(event.msg.guild_id);
        storage.messages.push_back({
                                           event.msg.id,
                                           event.msg.channel_id,
                                           event.msg.author.id,
                                           event.msg.content
                                   });
        
        for (const dpp::attachment& attach : event.msg.attachments)
            storage.attachments.push_back({event.msg.id, event.msg.channel_id, attach.url});
    });
    
    bot.start(dpp::st_wait);
    return 0;
}
