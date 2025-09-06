#ifndef MODULE_STAT_BOOST
#define MODULE_STAT_BOOST

#include "ChatCommand.h"
#include "Config.h"
#include "Spell.h"

#include "StatBoostCommon.h"
#include "StatBoostMgr.h"
#include "ScriptMgr.h"
#include "Player.h"

using namespace Trinity::ChatCommands;

class StatBoosterPlayer : public PlayerScript
{
public:
    StatBoosterPlayer() : PlayerScript("StatBoosterPlayer") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override;
    void OnLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override;
    void OnQuestRewardItem(Player* player, Item* item, uint32 /*count*/) override;
    void OnCreateItem(Player* player, Item* item, uint32 /*count*/) override;
    void OnGroupRollRewardItem(Player* player, Item* item, uint32 /*count*/, RollVote /*voteType*/, Roll* /*roll*/) override;
    bool CanCastItemUseSpell(Player* /*player*/, Item* /*item*/, SpellCastTargets const& /*targets*/, uint8 /*cast_count*/, uint32 /*glyphIndex*/) override;
    void OnAfterStoreOrEquipNewItem(Player* player, uint32 /*vendorslot*/, Item* item, uint8 /*count*/, uint8 /*bag*/, uint8 /*slot*/, ItemTemplate const* pProto, Creature* pVendor, VendorItem const* crItem, bool /*bStore*/) override;
};

class StatBoosterWorld : public WorldScript
{
public:
    StatBoosterWorld() : WorldScript("StatBoosterWorld") { }

    void OnConfigLoad(bool reload) override;
};

class StatBoosterCommands : public CommandScript
{
public:
    StatBoosterCommands() : CommandScript("StatBoosterCommands") { }

    ChatCommandTable GetCommands() const override;
    static bool HandleSBAddItemCommand(ChatHandler* handler, uint32 itemId = 0, uint32 count = 0, Optional<uint32> suffixId = std::nullopt);
};

#endif
