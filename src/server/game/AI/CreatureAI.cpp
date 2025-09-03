/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// @tswow-begin
#include "InstanceScript.h"
// @tswow-end
#include "CreatureAI.h"
#include "AreaBoundary.h"
#include "Creature.h"
#include "CreatureAIImpl.h"
#include "CreatureTextMgr.h"
#include "DBCStructure.h"
#include "Language.h"
#include "Log.h"
#include "Map.h"
#include "MapReference.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellHistory.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "World.h"
// @enlight-begin
#include <vector>
#include <algorithm>
#include <numeric>
#include "GameTime.h" // 确保包含 GameTime 头文件
#include "GridNotifiersImpl.h"
// @enlight-end

AISpellInfoType* UnitAI::AISpellInfo;
AISpellInfoType* GetAISpellInfo(uint32 i) { return &UnitAI::AISpellInfo[i]; }

CreatureAI::CreatureAI(Creature* creature) : UnitAI(creature), me(creature), _boundary(nullptr), _negateBoundary(false), _isEngaged(false), _moveInLOSLocked(false)
{
    // @tswow-begin load custom boss boundaries
    auto imap = creature->GetMap()->ToInstanceMap();
    if (!imap) return;
    uint32 bossId = sObjectMgr->GetCreatureBoss(creature->GetSpawnId());
    if (bossId == UINT32_MAX) return;
    imap->GetInstanceScript()->GetBossBoundary(bossId);
    SetBoundary(imap->GetInstanceScript()->GetBossBoundary(bossId));
    // @tswow-end
}

CreatureAI::~CreatureAI()
{
}

void CreatureAI::Talk(uint8 id, WorldObject const* whisperTarget /*= nullptr*/)
{
    sCreatureTextMgr->SendChat(me, id, whisperTarget);
}

// Disable CreatureAI when charmed
void CreatureAI::OnCharmed(bool isNew)
{
    if (isNew && !me->IsCharmed() && me->LastCharmerGUID)
    {
        if (!me->HasReactState(REACT_PASSIVE))
        {
            if (Unit* lastCharmer = ObjectAccessor::GetUnit(*me, me->LastCharmerGUID))
                me->EngageWithTarget(lastCharmer);
        }

        me->LastCharmerGUID.Clear();

        if (!me->IsInCombat())
            EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
    }

    UnitAI::OnCharmed(isNew);
}

void CreatureAI::DoZoneInCombat(Creature* creature /*= nullptr*/)
{
    if (!creature)
        creature = me;

    Map* map = creature->GetMap();
    if (!map->IsDungeon()) // use IsDungeon instead of Instanceable, in case battlegrounds will be instantiated
    {
        TC_LOG_ERROR("scripts.ai", "CreatureAI::DoZoneInCombat: call for map that isn't an instance ({})", creature->GetGUID().ToString());
        return;
    }

    if (!map->HavePlayers())
        return;

    for (MapReference const& ref : map->GetPlayers())
    {
        if (Player* player = ref.GetSource())
        {
            if (!player->IsAlive() || !CombatManager::CanBeginCombat(creature, player))
                continue;

            creature->EngageWithTarget(player);

            for (Unit* pet : player->m_Controlled)
                creature->EngageWithTarget(pet);

            if (Unit* vehicle = player->GetVehicleBase())
                creature->EngageWithTarget(vehicle);
        }
    }
}

// scripts does not take care about MoveInLineOfSight loops
// MoveInLineOfSight can be called inside another MoveInLineOfSight and cause stack overflow
void CreatureAI::MoveInLineOfSight_Safe(Unit* who)
{
    if (_moveInLOSLocked == true)
        return;
    _moveInLOSLocked = true;
    MoveInLineOfSight(who);
    _moveInLOSLocked = false;
}

// @enlight-begin
uint32 CreatureAI::MaxLevelOfNearByPlayers(float radius)
{
    std::vector<uint32> playerLevels;

    // 遍历附近的玩家，记录所有玩家的等级
    std::list<Player*> players;
    Trinity::AnyPlayerInObjectRangeCheck checker(me, radius);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, players, checker);

    Cell::VisitWorldObjects(me, searcher, radius);

    for (Player* player : players)
    {
        playerLevels.push_back(player->GetLevel());
    }

    // 如果没有玩家，返回1
    if (playerLevels.empty())
    {
        return 1;
    }

    // 按等级从高到低排序
    std::sort(playerLevels.begin(), playerLevels.end(), std::greater<uint32>());

    // 创建最大长度为5的等级列表
    std::vector<uint32> selectedLevels;
    if (!playerLevels.empty())
    {
        selectedLevels.push_back(playerLevels[0]);
        for (size_t i = 1; i < playerLevels.size() && selectedLevels.size() < 5; ++i)
        {
            if (std::abs(static_cast<int>(playerLevels[i]) - static_cast<int>(selectedLevels[0])) <= 6)
            {
                selectedLevels.push_back(playerLevels[i]);
            }
            else
            {
                break;
            }
        }
    }

    // 计算新列表的均值
    if (selectedLevels.empty())
    {
        return 1;
    }

    uint32 sum = std::accumulate(selectedLevels.begin(), selectedLevels.end(), 0);
    uint32 averageLevel = sum / selectedLevels.size();

    // 确保averageLevel至少为1
    if (averageLevel < 1)
    {
        averageLevel = 1;
    }

    return averageLevel;
}

// 升级生物到新等级
void CreatureAI::LevelUpTo(int8 highestNearbyPlayerLevel)
{
    // 数值错误
    if (highestNearbyPlayerLevel <= 1){
        return;
    }

    int8 creatureDefaultLevel = me->getDefaultLevel(); // 获取生物默认等级
    // 有些小动物默认等级为1，不做任何处理
    if (creatureDefaultLevel == 1){
        return;
    }
    
    int8 creatureCurrentLevel = me->GetLevel(); // 获取生物当前等级
    int8 minDefaultLevel = me->GetCreatureTemplate()->minlevel; // 获取生物最小等级
    int8 maxDefaultLevel = me->GetCreatureTemplate()->maxlevel; // 获取生物最大等级
    minDefaultLevel = std::max(minDefaultLevel, int8(1)); // 确保minDefaultLevel至少为1
    maxDefaultLevel = std::max(maxDefaultLevel, minDefaultLevel); // 确保maxDefaultLevel至少为1

    // 有主人的野怪，等级调整至主人等级
    int8 ownerLevel = 0;
    if (me->GetOwner() && (ownerLevel = me->GetOwner()->GetLevel())){
        if (ownerLevel != creatureCurrentLevel){
            TC_LOG_INFO("RSAI", "Creature {} [entry {}] is leveling to owner level {} from {}", me->GetSpawnId(), me->GetEntry(), ownerLevel, creatureCurrentLevel);
            me->SelectLevel(true, ownerLevel, ownerLevel); 
        }
        return;
    }

    // 玩家等级在5级以下，恢复野怪等级至默认
    if (highestNearbyPlayerLevel <= 5) {
        if (creatureCurrentLevel < minDefaultLevel || creatureCurrentLevel > maxDefaultLevel) {
            TC_LOG_INFO("RSAI", "Creature {} [entry {}] is leveling to default level {}-{} from {}", me->GetSpawnId(), me->GetEntry(), minDefaultLevel, maxDefaultLevel, creatureCurrentLevel);
            me->SelectLevel(true, minDefaultLevel, maxDefaultLevel);
        }
        return;
    }

    // 玩家等级与生物当前等级相差不超过3级，不做任何处理
    if (std::abs(highestNearbyPlayerLevel - creatureCurrentLevel) <= 2) {
        return;
    }

    // 玩家等级小于野怪等级3级以上，野怪调整至默认等级
    if (highestNearbyPlayerLevel < creatureCurrentLevel - 2) {
        if (creatureCurrentLevel < minDefaultLevel || creatureCurrentLevel > maxDefaultLevel) {
            TC_LOG_INFO("RSAI", "Creature {} [entry {}] is leveling to default level {}-{} from {}", me->GetSpawnId(), me->GetEntry(), minDefaultLevel, maxDefaultLevel, creatureCurrentLevel);
            me->SelectLevel(true, minDefaultLevel, maxDefaultLevel);
        }
        return;
    }

    // 野怪等级小于玩家等级3级以上，调整至玩家等级-3到+3范围内
    if (highestNearbyPlayerLevel > creatureCurrentLevel + 2) {
        int8 newLevel = highestNearbyPlayerLevel - 2 + rand() % 5; // 在玩家等级-3到+3范围内随机调整
        newLevel = std::max(newLevel, int8(creatureCurrentLevel + 1)); // 确保新等级高于当前等级
        TC_LOG_INFO("RSAI", "Creature {} [entry {}] is leveling to higher level {} from {}", me->GetSpawnId(), me->GetEntry(), newLevel, creatureCurrentLevel);
        me->SelectLevel(true, newLevel, newLevel);
        return;
    }
}

void CreatureAI::MoveInLineOfSight(Unit* who)
{
    if (me->IsEngaged())
        return;

    time_t curTime = GameTime::GetGameTime(); // 获取当前时间

    // 检查时间间隔是否超过1秒
    if (curTime - lastLevelUpTime >= 1) {
        uint8 playerLevel = MaxLevelOfNearByPlayers(100.0f);
        LevelUpTo(playerLevel);
        lastLevelUpTime = curTime; // 更新上次调整等级的时间
    }

    if (me->HasReactState(REACT_AGGRESSIVE) && me->CanStartAttack(who, false))
        me->EngageWithTarget(who);
}
// @enlight-end

void CreatureAI::OnOwnerCombatInteraction(Unit* target)
{
    if (!target || !me->IsAlive())
        return;

    if (!me->HasReactState(REACT_PASSIVE) && me->CanStartAttack(target, true))
        me->EngageWithTarget(target);
}

// Distract creature, if player gets too close while stealthed/prowling
void CreatureAI::TriggerAlert(Unit const* who) const
{
    // If there's no target, or target isn't a player do nothing
    if (!who || who->GetTypeId() != TYPEID_PLAYER)
        return;

    // If this unit isn't an NPC, is already distracted, is fighting, is confused, stunned or fleeing, do nothing
    if (me->GetTypeId() != TYPEID_UNIT || me->IsEngaged() || me->HasUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_STUNNED | UNIT_STATE_FLEEING | UNIT_STATE_DISTRACTED))
        return;

    // Only alert for hostiles!
    if (me->IsCivilian() || me->HasReactState(REACT_PASSIVE) || !me->IsHostileTo(who) || !me->_IsTargetAcceptable(who))
        return;

    // Send alert sound (if any) for this creature
    me->SendAIReaction(AI_REACTION_ALERT);

    // Face the unit (stealthed player) and set distracted state for 5 seconds
    me->GetMotionMaster()->MoveDistract(5 * IN_MILLISECONDS, me->GetAbsoluteAngle(who));
}

// adapted from logic in Spell:EffectSummonType before commit 8499434
static bool ShouldFollowOnSpawn(SummonPropertiesEntry const* properties)
{
    // Summons without SummonProperties are generally scripted summons that don't belong to any owner
    if (!properties)
        return false;

    switch (properties->Control)
    {
        case SUMMON_CATEGORY_PET:
            return true;
        case SUMMON_CATEGORY_WILD:
        case SUMMON_CATEGORY_ALLY:
        case SUMMON_CATEGORY_UNK:
            if (properties->Flags & 512)
                return true;
            switch (properties->Title)
            {
                case SUMMON_TYPE_PET:
                case SUMMON_TYPE_GUARDIAN:
                case SUMMON_TYPE_GUARDIAN2:
                case SUMMON_TYPE_MINION:
                case SUMMON_TYPE_MINIPET:
                    return true;
                default:
                    return false;
            }
        default:
            return false;
    }
}

void CreatureAI::JustAppeared()
{
    if (!IsEngaged())
    {
        if (TempSummon* summon = me->ToTempSummon())
        {
            // Only apply this to specific types of summons
            if (!summon->GetVehicle() && ShouldFollowOnSpawn(summon->m_Properties) && summon->CanFollowOwner())
            {
                if (Unit* owner = summon->GetCharmerOrOwner())
                {
                    summon->GetMotionMaster()->Clear();
                    summon->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, summon->GetFollowAngle());
                }
            }
        }
    }
}

void CreatureAI::JustEnteredCombat(Unit* who)
{
    if (!IsEngaged() && !me->CanHaveThreatList())
        EngagementStart(who);
}

void CreatureAI::EnterEvadeMode(EvadeReason why)
{
    if (!_EnterEvadeMode(why))
        return;

    TC_LOG_DEBUG("scripts.ai", "CreatureAI::EnterEvadeMode: entering evade mode (why: {}) ({})", why, me->GetGUID().ToString());

    if (!me->GetVehicle()) // otherwise me will be in evade mode forever
    {
        if (Unit* owner = me->GetCharmerOrOwner())
        {
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, me->GetFollowAngle());
        }
        else
        {
            // Required to prevent attacking creatures that are evading and cause them to reenter combat
            // Does not apply to MoveFollow
            me->AddUnitState(UNIT_STATE_EVADE);
            me->GetMotionMaster()->MoveTargetedHome();
        }
    }

    Reset();
}

bool CreatureAI::UpdateVictim()
{
    if (!IsEngaged())
        return false;

    if (!me->IsAlive())
    {
        EngagementOver();
        return false;
    }

    if (!me->HasReactState(REACT_PASSIVE))
    {
        if (Unit* victim = me->SelectVictim())
            if (victim != me->GetVictim())
                AttackStart(victim);

        return me->GetVictim() != nullptr;
    }
    else if (!me->IsInCombat())
    {
        EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
        return false;
    }
    else if (me->GetVictim())
        me->AttackStop();

    return true;
}

void CreatureAI::EngagementStart(Unit* who)
{
    if (_isEngaged)
    {
        TC_LOG_ERROR("scripts.ai", "CreatureAI::EngagementStart called even though creature is already engaged. Creature debug info:\n{}", me->GetDebugInfo());
        return;
    }
    _isEngaged = true;

    me->AtEngage(who);
}

void CreatureAI::EngagementOver()
{
    if (!_isEngaged)
    {
        TC_LOG_DEBUG("scripts.ai", "CreatureAI::EngagementOver called even though creature is not currently engaged. Creature debug info:\n{}", me->GetDebugInfo());
        return;
    }
    _isEngaged = false;

    me->AtDisengage();
}

bool CreatureAI::_EnterEvadeMode(EvadeReason /*why*/)
{
    if (me->IsInEvadeMode())
        return false;

    if (!me->IsAlive())
    {
        EngagementOver();
        return false;
    }

    me->RemoveAurasOnEvade();
    me->ClearComboPointHolders(); // Remove all combo points targeting this unit
    me->CombatStop(true);
    me->LoadCreaturesAddon();
    me->SetLootRecipient(nullptr);
    me->ResetPlayerDamageReq();
    me->SetLastDamagedTime(0);
    me->SetCannotReachTarget(false);
    me->DoNotReacquireSpellFocusTarget();
    me->SetTarget(ObjectGuid::Empty);
    me->GetSpellHistory()->ResetAllCooldowns();
    EngagementOver();

    return true;
}

static const uint32 BOUNDARY_VISUALIZE_CREATURE = 15425;
static const float BOUNDARY_VISUALIZE_CREATURE_SCALE = 0.25f;
static const int8 BOUNDARY_VISUALIZE_STEP_SIZE = 1;
static const int32 BOUNDARY_VISUALIZE_FAILSAFE_LIMIT = 750;
static const float BOUNDARY_VISUALIZE_SPAWN_HEIGHT = 5.0f;
int32 CreatureAI::VisualizeBoundary(Seconds duration, Unit* owner, bool fill) const
{
    typedef std::pair<int32, int32> coordinate;

    if (!owner)
        return -1;

    if (!_boundary || _boundary->empty())
        return LANG_CREATURE_MOVEMENT_NOT_BOUNDED;

    std::queue<coordinate> Q;
    std::unordered_set<coordinate> alreadyChecked;
    std::unordered_set<coordinate> outOfBounds;

    Position startPosition = owner->GetPosition();
    if (!IsInBoundary(&startPosition)) // fall back to creature position
    {
        startPosition = me->GetPosition();
        if (!IsInBoundary(&startPosition)) // fall back to creature home position
        {
            startPosition = me->GetHomePosition();
            if (!IsInBoundary(&startPosition))
                return LANG_CREATURE_NO_INTERIOR_POINT_FOUND;
        }
    }
    float spawnZ = startPosition.GetPositionZ() + BOUNDARY_VISUALIZE_SPAWN_HEIGHT;

    bool boundsWarning = false;
    Q.push({ 0,0 });
    while (!Q.empty())
    {
        coordinate front = Q.front();
        bool hasOutOfBoundsNeighbor = false;
        for (coordinate const& off : std::list<coordinate>{ {1, 0}, {0, 1}, {-1, 0}, {0, -1} })
        {
            coordinate next(front.first + off.first, front.second + off.second);
            if (next.first > BOUNDARY_VISUALIZE_FAILSAFE_LIMIT || next.first < -BOUNDARY_VISUALIZE_FAILSAFE_LIMIT || next.second > BOUNDARY_VISUALIZE_FAILSAFE_LIMIT || next.second < -BOUNDARY_VISUALIZE_FAILSAFE_LIMIT)
            {
                boundsWarning = true;
                continue;
            }
            if (alreadyChecked.find(next) == alreadyChecked.end()) // never check a coordinate twice
            {
                Position nextPos(startPosition.GetPositionX() + next.first*BOUNDARY_VISUALIZE_STEP_SIZE, startPosition.GetPositionY() + next.second*BOUNDARY_VISUALIZE_STEP_SIZE, startPosition.GetPositionZ());
                if (IsInBoundary(&nextPos))
                    Q.push(next);
                else
                {
                    outOfBounds.insert(next);
                    hasOutOfBoundsNeighbor = true;
                }
                alreadyChecked.insert(next);
            }
            else if (outOfBounds.find(next) != outOfBounds.end())
                hasOutOfBoundsNeighbor = true;
        }
        if (fill || hasOutOfBoundsNeighbor)
        {
            if (TempSummon* point = owner->SummonCreature(BOUNDARY_VISUALIZE_CREATURE, Position(startPosition.GetPositionX() + front.first * BOUNDARY_VISUALIZE_STEP_SIZE, startPosition.GetPositionY() + front.second * BOUNDARY_VISUALIZE_STEP_SIZE, spawnZ), TEMPSUMMON_TIMED_DESPAWN, duration))
            {
                point->SetObjectScale(BOUNDARY_VISUALIZE_CREATURE_SCALE);
                point->SetUnitFlag(UNIT_FLAG_STUNNED);
                point->SetImmuneToAll(true);
                if (!hasOutOfBoundsNeighbor)
                    point->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            }
        }

        Q.pop();
    }
    return boundsWarning ? LANG_CREATURE_MOVEMENT_MAYBE_UNBOUNDED : 0;
}

bool CreatureAI::IsInBoundary(Position const* who) const
{
    if (!_boundary)
        return true;

    if (!who)
        who = me;

    return CreatureAI::IsInBounds(*_boundary, who) != _negateBoundary;
}

bool CreatureAI::IsInBounds(CreatureBoundary const& boundary, Position const* pos)
{
    for (AreaBoundary const* areaBoundary : boundary)
        if (!areaBoundary->IsWithinBoundary(pos))
            return false;

    return true;
}

bool CreatureAI::CheckInRoom()
{
    if (IsInBoundary())
        return true;
    else
    {
        EnterEvadeMode(EVADE_REASON_BOUNDARY);
        return false;
    }
}

void CreatureAI::SetBoundary(CreatureBoundary const* boundary, bool negateBoundaries /*= false*/)
{
    _boundary = boundary;
    _negateBoundary = negateBoundaries;
    me->DoImmediateBoundaryCheck();
}

Creature* CreatureAI::DoSummon(uint32 entry, Position const& pos, Milliseconds despawnTime, TempSummonType summonType)
{
    return me->SummonCreature(entry, pos, summonType, despawnTime);
}

Creature* CreatureAI::DoSummon(uint32 entry, WorldObject* obj, float radius, Milliseconds despawnTime, TempSummonType summonType)
{
    Position pos = obj->GetRandomNearPosition(radius);
    return me->SummonCreature(entry, pos, summonType, despawnTime);
}

Creature* CreatureAI::DoSummonFlyer(uint32 entry, WorldObject* obj, float flightZ, float radius, Milliseconds despawnTime, TempSummonType summonType)
{
    Position pos = obj->GetRandomNearPosition(radius);
    pos.m_positionZ += flightZ;
    return me->SummonCreature(entry, pos, summonType, despawnTime);
}
