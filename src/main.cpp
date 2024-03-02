#include <iostream>
#include <dpp/dpp.h>
#include <cstdlib>
#include <utility>
#include <blt/std/logging.h>
#include <blt/parse/argparse.h>
#include <sqlite_orm/sqlite_orm.h>
#include "blt/std/types.h"
#include "blt/std/utility.h"
#include "blt/std/time.h"
#include <data_structs.h>
#include <curl/curl.h>
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace sql = sqlite_orm;
using namespace db;

struct db_obj
{
    private:
        blt::u64 guildID;
        std::atomic_bool loaded_channels = false;
        std::atomic_bool loaded_members = false;
        blt::u64 user_count = -1;
        std::atomic_uint64_t loaded_users = 0;
        std::queue<blt::u64> user_load_queue;
        std::mutex user_load_queue_mutex;
        database_type db;
        std::thread* thread = nullptr;
    
    public:
        explicit db_obj(blt::u64 guildID, const std::string& path): guildID(guildID), db(make_database(path + "/" + std::to_string(guildID) + "/"))
        {
            db.sync_schema();
        }
        
        void load(dpp::cluster& bot, const dpp::guild& guild)
        {
            server_info_t server_inf;
            server_inf.name = guild.name;
            server_inf.member_count = guild.member_count;
            server_inf.icon = guild.get_icon_url();
            server_inf.discovery_splash = guild.get_discovery_splash_url();
            server_inf.description = guild.description;
            server_inf.banner = guild.get_banner_url();
            server_inf.splash = guild.get_splash_url();
            commit(server_inf);
            
            bot.channels_get(guild.id, [this, guild, &bot](const dpp::confirmation_callback_t& event) {
                if (event.is_error())
                {
                    BLT_WARN("Failed to fetch channels for guild %ld", guildID);
                    BLT_WARN("Cause: %s", event.get_error().human_readable.c_str());
                    loaded_channels = true;
                    return;
                }
                auto channel_map = event.get<dpp::channel_map>();
                BLT_INFO("Guild '%s' channel count: %ld", guild.name.c_str(), channel_map.size());
                
                for (const auto& channel : channel_map)
                {
                    if (channel.second.is_category())
                        continue;
                    BLT_DEBUG("\tFetched channel id %ld with name '%s'", channel.first, channel.second.name.c_str());
                    channel_info_t channels;
                    channels.channel_name = channel.second.name;
                    channels.channel_topic = channel.second.topic;
                    channels.channelID = channel.first;
                    commit(channels);
                }
                loaded_channels = true;
                BLT_INFO("Finished loading channels for guild '%s'", guild.name.c_str());
            });
            
            bot.guild_get_members(guildID, 1000, 0, [this, &bot, guild](const dpp::confirmation_callback_t& event) {
                if (event.is_error())
                {
                    BLT_WARN("Failed to fetch members for guild %ld", guildID);
                    BLT_WARN("Cause: %s", event.get_error().human_readable.c_str());
                    loaded_members = true;
                    return;
                }
                auto member_map = event.get<dpp::guild_member_map>();
                BLT_INFO("Guild '%s' member count: %ld", guild.name.c_str(), member_map.size());
                {
                    user_count = member_map.size();
                    std::scoped_lock lock(user_load_queue_mutex);
                    for (const auto& member : member_map)
                    {
                        BLT_DEBUG("\tFetched member '%s'", member.second.get_nickname().c_str());
                        user_load_queue.push(member.first);
                    }
                }
                BLT_INFO("Finished loading members for guild '%s'", guild.name.c_str());
                loaded_members = true;
            });
            
            BLT_DEBUG("Finished requesting info for guild '%s'", guild.name.c_str());
            process_queue(bot);
        }
        
        void commit(const user_info_t& edited)
        {
            auto existing_user = db.select(sql::object<user_info_t>(), sql::from<user_info_t>(),
                                           sql::where(sql::c(&user_info_t::userID) == edited.userID));
            
            if (!existing_user.empty())
            {
                for (const auto& v : existing_user)
                {
                    user_history_t history;
                    history.userID = v.userID;
                    history.old_username = v.username;
                    history.old_global_nickname = v.global_nickname;
                    history.old_server_name = v.server_name;
                    history.time_changed = blt::system::getCurrentTimeMilliseconds();
                    commit(history);
                }
            }
            
            db.replace(edited);
        }
        
        void commit(const user_history_t& edited)
        {
            db.insert(edited);
        }
        
        void commit(const channel_info_t& channel)
        {
        
        }
        
        void commit(const channel_history_t& channel)
        {
            db.insert(channel);
        }
        
        void commit(const message_t& message)
        {
        
        }
        
        void commit(const attachment_t& attachment)
        {
        
        }
        
        void commit(const message_edits_t& edited)
        {
            db.insert(edited);
        }
        
        void commit(const message_deletes_t& deleted)
        {
            db.insert(deleted);
        }
        
        void commit(const server_info_t& server)
        {
            using namespace sql;
            auto data = db.select(columns(&server_info_t::name), from<server_info_t>(),
                                  where((c(&server_info_t::member_count) == server.member_count) &&
                                        (c(&server_info_t::name) == server.name) &&
                                        (c(&server_info_t::description) == server.description) &&
                                        (c(&server_info_t::icon) == server.icon) &&
                                        (c(&server_info_t::splash) == server.splash) &&
                                        (c(&server_info_t::discovery_splash) == server.discovery_splash) &&
                                        (c(&server_info_t::banner) == server.banner)));
            if (data.empty())
                db.insert(server);
        }
        
        void process_queue(dpp::cluster& bot)
        {
            thread = new std::thread([this, &bot]() {
//                while (user_count != loaded_users.load())
                while (true)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    blt::u64 member = 0;
                    {
                        std::scoped_lock lock(user_load_queue_mutex);
                        if (user_load_queue.empty())
                            continue;
                        member = user_load_queue.front();
                        user_load_queue.pop();
                    }
                    bot.user_get(member, [member, this](const dpp::confirmation_callback_t& event) {
                        if (event.is_error())
                        {
                            BLT_WARN("Failed to fetch user %ld for guild '%ld'", member, guildID);
                            BLT_WARN("Cause: %s", event.get_error().human_readable.c_str());
                            BLT_INFO("Error code %d with error message '%s'", event.get_error().code, event.get_error().message.c_str());
                            // requeue on rate limit
                            if (event.get_error().code == 0)
                            {
                                for (const auto& v : event.get_error().errors)
                                {
                                    BLT_TRACE0_STREAM << "\t" << v.code << '\n';
                                    BLT_TRACE0_STREAM << "\t" << v.field << '\n';
                                    BLT_TRACE0_STREAM << "\t" << v.index << '\n';
                                    BLT_TRACE0_STREAM << "\t" << v.object << '\n';
                                    BLT_TRACE0_STREAM << "\t" << v.reason << '\n';
                                }
                                std::scoped_lock lock(user_load_queue_mutex);
                                user_load_queue.push(member);
                            } else
                                loaded_users++;
                            BLT_INFO("%ld vs %ld", user_count, loaded_users.load());
                            return;
                        }
                        auto user = event.get<dpp::user_identified>();
                        
                        user_info_t user_inf;
                        user_inf.username = user.username;
                        user_inf.global_nickname = user.global_name;
                        user_inf.userID = user.id;
                        commit(user_inf);
                        
                        BLT_DEBUG("We got user '%s' with global name '%s'", user.username.c_str(), user.global_name.c_str());
                        loaded_users++;
                    });
                }
            });
        }
        
        inline void queue_user(blt::u64 userID)
        {
            std::scoped_lock lock(user_load_queue_mutex);
            user_load_queue.push(userID);
        }
        
        bool loading_complete()
        {
            return loaded_channels.load() && loaded_members.load() && user_count != -1ul && user_count == loaded_users.load();
        }
        
        ~db_obj()
        {
            thread->join();
            delete thread;
        }
    
};

blt::hashmap_t<blt::u64, std::unique_ptr<db_obj>> databases;
std::string path;
blt::u64 total_guilds = 0;
std::atomic_uint64_t completed_guilds = 0;
std::atomic_bool finished_loading = false;

bool loading_complete()
{
    bool finished = true;
    if (!finished_loading.load())
    {
        for (const auto& v : databases)
        {
            if (!v.second->loading_complete())
                finished = false;
        }
        finished_loading.store(finished);
        if (finished)
            BLT_INFO("Loading complete!");
    }
    return finished && total_guilds != 0 && total_guilds == completed_guilds.load();
}

db_obj& get(blt::u64 id)
{
    if (databases.find(id) == databases.end())
        databases.insert({id, std::make_unique<db_obj>(id, path)});
    return *databases.at(id);
}

template<typename event_type>
std::function<void(const event_type& event)> wait_wrapper(std::function<void(const event_type& event)>&& func)
{
    return [func](const event_type& event) {
        if (!loading_complete())
            return;
        func(event);
    };
}

int main(int argc, const char** argv)
{
    blt::arg_parse parser;
    parser.addArgument(blt::arg_builder("-t", "--token").setAction(blt::arg_action_t::STORE).setHelp("The discord bot token").build());
    parser.addArgument(blt::arg_builder("-p", "--path").setAction(blt::arg_action_t::STORE).setHelp("Path to store the archive data").build());
    
    auto args = parser.parse_args(argc, argv);
    path = args.get<std::string>("path");
    std::filesystem::create_directories(path);
    
    dpp::cluster bot(args.get<std::string>("token"), dpp::i_default_intents | dpp::i_message_content | dpp::i_all_intents | dpp::i_guild_members);
    
    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct fetch_active_guilds>())
        {
            total_guilds = event.guild_count;
            for (blt::u64 server : event.guilds)
            {
                bot.guild_get(server, [&bot, server](const dpp::confirmation_callback_t& event) {
                    if (event.is_error())
                    {
                        BLT_WARN("Failed to fetch for guild %ld", server);
                        BLT_WARN("Cause: %s", event.get_error().human_readable.c_str());
                        return;
                    }
                    BLT_INFO("Fetched data for %ld ('%s')", server, event.get<dpp::guild>().name.c_str());
                    auto& db = get(server);
                    
                    db.load(bot, event.get<dpp::guild>());
                    
                    completed_guilds++;
                });
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });
    
    bot.on_user_update(wait_wrapper<dpp::user_update_t>([&bot](const dpp::user_update_t& event) {
        BLT_INFO("User '%s' updated in some way; global name: '%s'", event.updated.username.c_str(), event.updated.global_name.c_str());
        for (const auto& guild : databases)
        {
            user_info_t info;
            info.userID = event.updated.id;
            info.username = event.updated.username;
            info.global_nickname = event.updated.global_name;
            guild.second->commit(info);
        }
    }));
    
    bot.on_guild_member_update(wait_wrapper<dpp::guild_member_update_t>([&bot](const dpp::guild_member_update_t& event) {
        user_info_t info;
        info.userID = event.updated.user_id;
        info.server_name = event.updated.get_nickname();
        auto& storage = get(event.updated.guild_id);
        storage.commit(info);
        storage.queue_user(event.updated.user_id);
    }));
    
    bot.on_guild_member_add(wait_wrapper<dpp::guild_member_add_t>([](const dpp::guild_member_add_t& event) {
        user_info_t info;
        info.userID = event.added.user_id;
        info.server_name = event.added.get_nickname();
        auto& storage = get(event.added.guild_id);
        storage.commit(info);
        storage.queue_user(event.added.user_id);
    }));
    
    bot.on_message_delete(wait_wrapper<dpp::message_delete_t>([&bot](const dpp::message_delete_t& event) {
        BLT_DEBUG("Message %ld deleted content in %ld", event.id, event.channel_id);
        message_deletes_t deleted;
        deleted.channelID = event.channel_id;
        deleted.messageID = event.id;
        get(event.guild_id).commit(deleted);
    }));
    
    bot.on_message_delete_bulk(wait_wrapper<dpp::message_delete_bulk_t>([&bot](const dpp::message_delete_bulk_t& event) {
        BLT_INFO("Bulk delete!");
        if (event.deleting_channel == nullptr || event.deleting_guild == nullptr)
        {
            BLT_WARN("Unable to handle bulk delete, guild or channel is not in cache!");
            BLT_WARN("Raw String:");
            BLT_WARN("\t%s", event.raw_event.c_str());
            return;
        }
        for (auto v : event.deleted)
        {
            BLT_TRACE("\tBulk Delete: %ld", v);
            message_deletes_t deleted;
            deleted.messageID = v;
            deleted.channelID = event.deleting_channel->id;
            get(event.deleting_guild->id).commit(deleted);
        }
    }));
    
    bot.on_message_update(wait_wrapper<dpp::message_update_t>([&bot](const dpp::message_update_t& event) {
        message_edits_t edited;
        edited.messageID = event.msg.id;
        edited.new_content = event.msg.content;
        get(event.msg.guild_id).commit(edited);
        BLT_INFO("%ld (from user %ld in channel %ld ['%s']) -> '%s'", event.msg.id, event.msg.author.id, event.msg.channel_id,
                 event.msg.author.username.c_str(), event.msg.content.c_str());
    }));
    
    bot.on_message_create(wait_wrapper<dpp::message_create_t>([&bot](const dpp::message_create_t& event) {
        if (event.msg.id == bot.me.id)
            return;
        if (blt::string::starts_with(event.msg.content, "!dump"))
        {}
        auto& storage = get(event.msg.guild_id);
        message_t message;
        message.messageID = event.msg.id;
        message.channelID = event.msg.channel_id;
        message.userID = event.msg.author.id;
        message.content = event.msg.content;
        storage.commit(message);
        for (const dpp::attachment& attach : event.msg.attachments)
        {
            attachment_t attachment;
            attachment.messageID = event.msg.id;
            attachment.attachmentID = attach.id;
            attachment.url = attach.url;
            attachment.filename = attach.filename;
            attachment.description = attach.description;
            storage.commit(attachment);
        }
    }));
    
    bot.on_channel_create(wait_wrapper<dpp::channel_create_t>([](const dpp::channel_create_t& event) {
        if (event.created == nullptr)
        {
            BLT_WARN("Unable to handle channel creation, channel is not in cache!");
            BLT_WARN("Raw String:");
            BLT_WARN("\t%s", event.raw_event.c_str());
            return;
        }
        channel_info_t channel;
        channel.channelID = event.created->id;
        channel.channel_topic = event.created->topic;
        channel.channel_name = event.created->name;
        get(event.created->guild_id).commit(channel);
    }));
    
    bot.on_channel_delete(wait_wrapper<dpp::channel_delete_t>([](const dpp::channel_delete_t& event) {
        channel_info_t channel;
        channel.channelID = event.deleted.id;
        channel.channel_name = event.deleted.name;
        channel.channel_topic = event.deleted.topic;
        get(event.deleted.guild_id).commit(channel);
    }));
    
    bot.on_channel_update(wait_wrapper<dpp::channel_update_t>([](const dpp::channel_update_t& event) {
        channel_info_t channel;
        channel.channelID = event.updated->guild_id;
        channel.channel_name = event.updated->name;
        channel.channel_topic = event.updated->topic;
        get(event.updated->guild_id).commit(channel);
    }));
    
    bot.start(dpp::st_wait);
    
    return 0;
}
