#include "StatBoostCfgMgr.h"

std::vector<StatBoosterConfig::EnchantScore>* StatBoosterConfig::EnchantScorePool::Get()
{
    return &scores;
}

void StatBoosterConfig::EnchantScorePool::Add(EnchantScore score)
{
    scores.push_back(score);
}

void StatBoosterConfig::EnchantScorePool::Clear()
{
    scores.clear();
}

void StatBoosterConfig::EnchantScorePool::Evaluate(uint32 modType, uint32 modId, uint32 subclass, uint32& tankScore, uint32& physScore, uint32& spellScore, uint32& hybridScore)
{
    if (sBoostConfigMgr->VerboseEnable)
    {
        TC_LOG_INFO("module", "Finding scores for ModType: {}, ModId: {}, Subclass: {}", modType, modId, subclass);
    }

    auto scoreIter = std::find_if(scores.begin(), scores.end(), [modType, modId, subclass](EnchantScore& enchantScore)
    {
            return (enchantScore.modType == modType && enchantScore.modId == modId && (enchantScore.subclass == subclass || enchantScore.subclass == 0));
    });

    if (scoreIter != scores.end())
    {
        if (sBoostConfigMgr->VerboseEnable)
        {
            TC_LOG_INFO("module", "Scoring Item: Tank({}), Phys({}), Spell({}), Hybrid({})", scoreIter->tankScore, scoreIter->physScore, scoreIter->spellScore, scoreIter->hybridScore);
        }

        tankScore += scoreIter->tankScore;
        physScore += scoreIter->physScore;
        spellScore += scoreIter->spellScore;
        hybridScore += scoreIter->hybridScore;
    }
}

bool StatBoosterConfig::EnchantScorePool::Load()
{
    try
    {
        uint32 enchantCount = 0;

        QueryResult qResult = WorldDatabase.Query("SELECT `mod_type`, `mod_id`, `subclass`, `tank_score`, `phys_score`, `spell_score`, `hybrid_score` FROM `statbooster_enchant_scores`");

        if (!qResult)
        {
            TC_LOG_INFO("module", "Failed to load StatBooster enchant scores from statbooster_enchant_scores table.");
            return false;
        }

        TC_LOG_INFO("module", "Loading StatBooster enchant scores from statbooster_enchant_scores...");

        sBoostConfigMgr->EnchantScores.Clear();

        do
        {
            Field* fields = qResult->Fetch();

            EnchantScore enchantScore;

            enchantScore.modType = fields[0].GetUInt32();
            enchantScore.modId = fields[1].GetUInt32();
            enchantScore.subclass = fields[2].GetUInt32();
            enchantScore.tankScore = fields[3].GetUInt32();
            enchantScore.physScore = fields[4].GetUInt32();
            enchantScore.spellScore = fields[5].GetUInt32();
            enchantScore.hybridScore = fields[6].GetUInt32();

            enchantCount++;
            sBoostConfigMgr->EnchantScores.Add(enchantScore);

            if (sBoostConfigMgr->VerboseEnable)
            {
                TC_LOG_INFO("module", ">> Loaded enchant score type {} with id {} subclass {} with scores {}, {}, {}, {}",
                    enchantScore.modType, enchantScore.modId, enchantScore.subclass, enchantScore.tankScore, enchantScore.physScore, enchantScore.spellScore, enchantScore.hybridScore);
            }
        } while (qResult->NextRow());

        TC_LOG_INFO("module", ">> Loaded {} stat booster enchant scores", enchantCount);
    }
    catch (std::exception ex)
    {
        TC_LOG_INFO("module", "Failed to load enchant scores with message: {}", ex.what());
        TC_LOG_INFO("module", "Disabling StatBooster module.");
        sBoostConfigMgr->Enable = false;
        return false;
    }
    return true;
}

StatBoosterConfig* StatBoosterConfig::GetInstance()
{
    if (!instance)
    {
        instance = new StatBoosterConfig();
    }

    return instance;
}

void StatBoosterConfig::EnchantPool::Add(EnchantDefinition definition)
{
    pool.push_back(definition);
}

EnchantDefinition* StatBoosterConfig::EnchantPool::Get(uint32 roleMask, uint32 classMask, uint32 subClassMask, uint32 itemTypeMask, uint32 itemLevel)
{
    std::shuffle(std::begin(pool), std::end(pool), randomEngine);

    auto iterator = std::find_if(pool.begin(), pool.end(), [&](const EnchantDefinition& data)
    {
            return ((data.RoleMask & roleMask) == roleMask || data.RoleMask == 0) &&
                ((data.ClassMask & classMask) == classMask || data.ClassMask == 0) &&
                ((data.SubClassMask & subClassMask) == subClassMask || data.SubClassMask == 0) &&
                ((data.ItemTypeMask & itemTypeMask) == itemTypeMask || data.ItemTypeMask == 0) &&
                (itemLevel >= data.ILvlMin && itemLevel <= data.ILvlMax);
    });

    if (!(iterator == pool.end()))
    {
        return &(*iterator);
    }

    return 0;
}

EnchantDefinition* StatBoosterConfig::EnchantPool::GetByPlayerLevel(uint32 roleMask, uint32 classMask, uint32 subClassMask, uint32 itemTypeMask, uint32 playerLevel)
{
    uint32 itemLevel = 1;
    if (playerLevel <= 59){
        itemLevel = playerLevel;
    } else if (playerLevel == 60) {
        itemLevel = urand(60, 92);
    } else if (playerLevel <= 69) {
        itemLevel = urand(93, 115);
    } else if (playerLevel == 70) {
        itemLevel = urand(116, 164);
    } else if (playerLevel <= 79) {
        itemLevel = urand(165, 200);
    } else {
        itemLevel = urand(201, 251);
    }
    return Get(roleMask, classMask, subClassMask, itemTypeMask, itemLevel);
}

bool StatBoosterConfig::EnchantPool::Load()
{
    try
    {
        uint32 enchantCount = 0;

        QueryResult qResult = WorldDatabase.Query("SELECT `Id`, `iLvlMin`, `iLvlMax`, `RoleMask`, `ClassMask`, `SubClassMask`, `ItemTypeMask` FROM `statbooster_enchant_template`");

        if (!qResult)
        {
            TC_LOG_INFO("module", "Failed to load StatBooster enchant definitions from statbooster_enchant_template table.");
            return false;
        }

        TC_LOG_INFO("module", "Loading StatBooster enchants from statbooster_enchant_template...");

        sBoostConfigMgr->EnchantPool.Clear();

        do
        {
            Field* fields = qResult->Fetch();

            EnchantDefinition enchantDef;

            enchantDef.Id = fields[0].GetUInt32();
            enchantDef.ILvlMin = fields[1].GetUInt32();
            enchantDef.ILvlMax = fields[2].GetUInt32();
            enchantDef.RoleMask = fields[3].GetUInt32();
            enchantDef.ClassMask = fields[4].GetUInt32();
            enchantDef.SubClassMask = fields[5].GetUInt32();
            enchantDef.ItemTypeMask = fields[6].GetUInt32();

            enchantCount++;
            sBoostConfigMgr->EnchantPool.Add(enchantDef);

            if (sBoostConfigMgr->VerboseEnable)
            {
                TC_LOG_INFO("module", ">> Loaded enchant id {} with role mask {}, class mask {}, subclass mask {}, and type mask {}", enchantDef.Id, enchantDef.RoleMask, enchantDef.ClassMask, enchantDef.SubClassMask, enchantDef.ItemTypeMask);
            }
        } while (qResult->NextRow());

        TC_LOG_INFO("module", ">> Loaded {} stat booster enchant definitions", enchantCount);
    }
    catch (std::exception ex)
    {
        TC_LOG_INFO("module", "Failed to load enchant table with message: {}", ex.what());
        TC_LOG_INFO("module", "Disabling StatBooster module.");
        sBoostConfigMgr->Enable = false;
        return false;
    }

    return true;
}

void StatBoosterConfig::EnchantPool::Clear()
{
    pool.clear();
}
