#include "StatBoost.h"

void StatBoosterPlayer::OnLogin(Player* player, bool /*firstLogin*/)
{
    if (!sBoostConfigMgr->Enable)
    {
        ChatHandler(player->GetSession()).SendSysMessage("StatBooster is disabled.");
        return;
    }

    if(sBoostConfigMgr->OnLoginEnable)
    {
        ChatHandler(player->GetSession()).SendSysMessage(sBoostConfigMgr->OnLoginMessage);
    }
}

void StatBoosterPlayer::OnLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/)
{
    if (!sBoostConfigMgr->Enable)
    {
        return;
    }

    if (sBoostConfigMgr->OnLootItemEnable)
    {
        bool result = StatBoostMgr::BoostItem(player, item, sBoostConfigMgr->LootItemChance);

        if (result)
        {
            if (sBoostConfigMgr->AnnounceBoostEnable)
            {
                ChatHandler(player->GetSession()).SendSysMessage(sBoostConfigMgr->AnnounceLoot);
            }

            if (sBoostConfigMgr->PlaySoundEnable)
            {
                player->PlayDirectSound(sBoostConfigMgr->SoundId);
            }

            if (sBoostConfigMgr->SoulbindOnEnchantLoot && !item->IsSoulBound())
            {
                StatBoostMgr::MakeSoulbound(item, player);
            }
        }
    }
}

void StatBoosterPlayer::OnQuestRewardItem(Player* player, Item* item, uint32 /*count*/)
{

    std::cout << "StatBoosterPlayer::OnQuestRewardItem A" << std::endl;
    if (!sBoostConfigMgr->Enable)
    {
        return;
    }

    std::cout << "StatBoosterPlayer::OnQuestRewardItem B" << std::endl;

    if (sBoostConfigMgr->OnQuestRewardItemEnable)
    {
        std::cout << "StatBoosterPlayer::OnQuestRewardItem C" << std::endl;
        bool result = StatBoostMgr::BoostItem(player, item, sBoostConfigMgr->QuestRewardChance);

        if (result)
        {
            std::cout << "StatBoosterPlayer::OnQuestRewardItem D" << std::endl;
            if (sBoostConfigMgr->AnnounceBoostEnable)
            {
                ChatHandler(player->GetSession()).SendSysMessage(sBoostConfigMgr->AnnounceQuest);
            }

            if (sBoostConfigMgr->PlaySoundEnable)
            {
                player->PlayDirectSound(sBoostConfigMgr->SoundId);
            }

            if (sBoostConfigMgr->SoulbindOnEnchantQuest && !item->IsSoulBound())
            {
                StatBoostMgr::MakeSoulbound(item, player);
            }
        }
    }
}

void StatBoosterPlayer::OnCreateItem(Player* player, Item* item, uint32 /*count*/)
{
    if (!sBoostConfigMgr->Enable)
    {
        return;
    }

    if (sBoostConfigMgr->OnCraftItemEnable)
    {
        bool result = StatBoostMgr::BoostItem(player, item, sBoostConfigMgr->CraftItemChance);

        if (result)
        {
            if (sBoostConfigMgr->AnnounceBoostEnable)
            {
                ChatHandler(player->GetSession()).SendSysMessage(sBoostConfigMgr->AnnounceCraft);
            }

            if (sBoostConfigMgr->PlaySoundEnable)
            {
                player->PlayDirectSound(sBoostConfigMgr->SoundId);
            }

            if (sBoostConfigMgr->SoulbindOnEnchantCraft && !item->IsSoulBound())
            {
                StatBoostMgr::MakeSoulbound(item, player);
            }
        }
    }
}

void StatBoosterPlayer::OnGroupRollRewardItem(Player* player, Item* item, uint32 /*count*/, RollVote /*voteType*/, Roll* /*roll*/)
{
    if (!sBoostConfigMgr->Enable)
    {
        return;
    }

    if (sBoostConfigMgr->OnLootItemEnable)
    {
        bool result = StatBoostMgr::BoostItem(player, item, sBoostConfigMgr->LootItemChance);

        if (result)
        {
            if (sBoostConfigMgr->AnnounceBoostEnable)
            {
                ChatHandler(player->GetSession()).SendSysMessage(sBoostConfigMgr->AnnounceLoot);
            }

            if (sBoostConfigMgr->PlaySoundEnable)
            {
                player->PlayDirectSound(sBoostConfigMgr->SoundId);
            }

            if (sBoostConfigMgr->SoulbindOnEnchantRoll && !item->IsSoulBound())
            {
                StatBoostMgr::MakeSoulbound(item, player);
            }
        }
    }
}

void StatBoosterPlayer::OnAfterStoreOrEquipNewItem(Player* player, uint32 /*vendorslot*/, Item* item, uint8 /*count*/, uint8 /*bag*/, uint8 /*slot*/, ItemTemplate const* pProto, Creature* pVendor, VendorItem const* crItem, bool /*bStore*/)
{
    if (!sBoostConfigMgr->Enable)
    {
        return;
    }

    if (sBoostConfigMgr->OnLootItemEnable)
    {
        if (player->IsGameMaster()){
            // TC_LOG_ERROR("StatBooster", "Trying to StatBooster OnAfterStoreOrEquipNewItem: {}", item->GetTemplate()->ItemId);
        }

        // log all passed arguments
        // TC_LOG_ERROR("StatBooster", "ItemTemplate {}, Creature {}, VendorItem {}", pProto->ItemId, pVendor->GetEntry(), crItem->item);

        bool result = StatBoostMgr::BoostItem(player, item, sBoostConfigMgr->LootItemChance);

        if (result)
        {
            if (sBoostConfigMgr->AnnounceBoostEnable)
            {
                ChatHandler(player->GetSession()).SendSysMessage(sBoostConfigMgr->AnnounceVendor);
            }

            if (sBoostConfigMgr->PlaySoundEnable)
            {
                player->PlayDirectSound(sBoostConfigMgr->SoundId);
            }

            // if (sBoostConfigMgr->SoulbindOnEnchantLoot && !item->IsSoulBound())
            // {
            //     StatBoostMgr::MakeSoulbound(item, player);
            // }
        }
    }
}

bool StatBoosterPlayer::CanCastItemUseSpell(Player* player, Item* item, SpellCastTargets const& targets, uint8 /*cast_count*/, uint32 /*glyphIndex*/)
{
    if (!item)
    {
        return true;
    }

    auto itemTemplate = item->GetTemplate();
    if (!itemTemplate)
    {
        return true;
    }

    if (itemTemplate->ItemId != 41605)
    {
        return true;
    }

    if (!sBoostConfigMgr->Enable)
    {
        ChatHandler(player->GetSession()).SendSysMessage("This item is disabled.");
        return false;
    }

    auto targetItem = targets.GetItemTarget();
    if (!targetItem)
    {
        return false;
    }

    if (sConfigMgr->GetBoolDefault("StatBooster.Reroll.AllowOwnedItemsOnly", true) &&
        targetItem->GetOwner()->GetGUID() != player->GetGUID())
    {
        ChatHandler(player->GetSession()).SendSysMessage("You cannot re-roll items other than your own.");
        return false;
    }

    if (sConfigMgr->GetBoolDefault("StatBooster.Reroll.AllowBoostedItemsOnly", false))
    {
        if (!StatBoostMgr::IsBoosted(targetItem))
        {
            ChatHandler(player->GetSession()).SendSysMessage("You cannot re-roll items that do not have a boosted stat.");
            return false;
        }
    }

    if (StatBoostMgr::BoostItem(player, targetItem, 100))
    {
        player->DestroyItemCount(itemTemplate->ItemId, 1, true);

        uint32 visualId = sConfigMgr->GetIntDefault("StatBooster.Reroll.VisualId", 62015);
        player->CastSpell(player, visualId);
        //player->HandleEmoteCommand(EMOTE_ONESHOT_LOOT);
    }
    else
    {
        ChatHandler(player->GetSession()).SendSysMessage("You cannot re-roll this item.");
    }

    return false;
}

void StatBoosterWorld::OnConfigLoad(bool /*reload*/)
{
    sBoostConfigMgr->Enable = sConfigMgr->GetBoolDefault("StatBooster.Enable", false);

    // No point loading all of this information if the module is not enabled.
    if (sBoostConfigMgr->Enable)
    {
        sBoostConfigMgr->VerboseEnable = sConfigMgr->GetBoolDefault("StatBooster.VerboseEnable", false);
        sBoostConfigMgr->OnLoginEnable = sConfigMgr->GetBoolDefault("StatBooster.OnLoginEnable", true);
        sBoostConfigMgr->OnLoginMessage = sConfigMgr->GetStringDefault("StatBooster.OnLoginMessage", "This server is running the StatBooster module.");

        sBoostConfigMgr->OnLootItemEnable = sConfigMgr->GetBoolDefault("StatBooster.OnLootItemEnable", true);
        sBoostConfigMgr->OnQuestRewardItemEnable = sConfigMgr->GetBoolDefault("StatBooster.OnQuestRewardItemEnable", true);
        sBoostConfigMgr->OnCraftItemEnable = sConfigMgr->GetBoolDefault("StatBooster.OnCraftItemEnable", true);

        sBoostConfigMgr->LootItemChance = sConfigMgr->GetIntDefault("StatBooster.LootItemChance", 100);
        sBoostConfigMgr->QuestRewardChance = sConfigMgr->GetIntDefault("StatBooster.QuestRewardChance", 100);
        sBoostConfigMgr->CraftItemChance = sConfigMgr->GetIntDefault("StatBooster.CraftItemChance", 100);

        sBoostConfigMgr->MinQuality = sConfigMgr->GetIntDefault("StatBooster.MinQuality", ITEM_QUALITY_UNCOMMON);
        sBoostConfigMgr->MaxQuality = sConfigMgr->GetIntDefault("StatBooster.MaxQuality", ITEM_QUALITY_EPIC);

        sBoostConfigMgr->PlaySoundEnable = sConfigMgr->GetBoolDefault("StatBooster.PlaySoundEnable", true);
        sBoostConfigMgr->SoundId = sConfigMgr->GetIntDefault("StatBooster.SoundId", 120);

        sBoostConfigMgr->SoulbindOnEnchantRoll = sConfigMgr->GetBoolDefault("StatBooster.SoulbindOnEnchantRoll", false);
        sBoostConfigMgr->SoulbindOnEnchantLoot = sConfigMgr->GetBoolDefault("StatBooster.SoulbindOnEnchantLoot", false);
        sBoostConfigMgr->SoulbindOnEnchantQuest = sConfigMgr->GetBoolDefault("StatBooster.SoulbindOnEnchantQuest", false);
        sBoostConfigMgr->SoulbindOnEnchantCraft = sConfigMgr->GetBoolDefault("StatBooster.SoulbindOnEnchantCraft", false);

        sBoostConfigMgr->AnnounceBoostEnable = sConfigMgr->GetBoolDefault("StatBooster.AnnounceBoostEnable", true);
        sBoostConfigMgr->AnnounceLoot = sConfigMgr->GetStringDefault("StatBooster.AnnounceLoot", "You looted a boosted item.");
        sBoostConfigMgr->AnnounceQuest = sConfigMgr->GetStringDefault("StatBooster.AnnounceQuest", "You received a boosted item.");
        sBoostConfigMgr->AnnounceCraft = sConfigMgr->GetStringDefault("StatBooster.AnnounceCraft", "You crafted a boosted item.");
        sBoostConfigMgr->AnnounceVendor = sConfigMgr->GetStringDefault("StatBooster.AnnounceVendor", "You crafted a boosted item.");

        sBoostConfigMgr->OverwriteEnchantEnable = sConfigMgr->GetBoolDefault("StatBooster.OverwriteEnchantEnable", true);

        sBoostConfigMgr->EnchantPool.Load();
        sBoostConfigMgr->EnchantScores.Load();
    }
}

ChatCommandTable StatBoosterCommands::GetCommands() const
{
    static ChatCommandTable sbCommandTable =
    {
        { "additem", HandleSBAddItemCommand, rbac::RBAC_ROLE_PLAYER, Console::No }
    };

    static ChatCommandTable commandTable =
    {
        { "sb", sbCommandTable }
    };

    return commandTable;
}

bool StatBoosterCommands::HandleSBAddItemCommand(ChatHandler* handler, uint32 itemId, uint32 count, Optional<uint32> suffixId)
{
    if (!itemId || !count)
    {
        handler->SendSysMessage("Invalid arguments, you must supply a valid itemId and count.");
        handler->SendSysMessage("Ex: '.sb additem <itemId> <count> [suffixId]'");
        handler->SetSentErrorMessage(true);
        return false;
    }

    ItemTemplate const* itemTemp = sObjectMgr->GetItemTemplate(itemId);

    if (!itemTemp)
    {
        handler->SendSysMessage("Item template could not be found. Is this a valid item id?");
        handler->SetSentErrorMessage(true);
        return false;
    }

    Player* player = handler->GetPlayer();

    if (!player)
    {
        return false;
    }

    if (player->GetTarget() && player->GetTarget().IsPlayer())
    {
        player = ObjectAccessor::FindPlayer(player->GetTarget());
    }

    uint32 noSpaceForCount = 0;
    ItemPosCountVec dest;
    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count, &noSpaceForCount);
    
    if (msg != EQUIP_ERR_OK)
    {
        count -= noSpaceForCount;
    }

    if (dest.empty())
    {
        return false;
    }

    Item* item = player->StoreNewItem(dest, itemId, true);

    StatBoostMgr statBoostMgr;
    bool result = statBoostMgr.BoostItem(player, item, 100);

    if (!item)
    {
        return false;
    }

    if (suffixId.has_value())
    {
        item->SetItemRandomProperties(suffixId.value());
    }

    if (result)
    {
        handler->SendSysMessage(Trinity::StringFormat("Added boosted item '{}' to '{}'.", itemId, player->GetName()));
    }
    else
    {
        handler->SendSysMessage(Trinity::StringFormat("Added item '{}' to '{}'.", itemId, player->GetName()));
    }
    
    player->SendNewItem(item, count, true, false, false, true);

    return true;
}

void AddSC_StatBoosterScripts()
{
    new StatBoosterCommands();
    new StatBoosterWorld();
    new StatBoosterPlayer();
}
