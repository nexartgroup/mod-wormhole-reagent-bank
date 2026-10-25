/*
 * mod-wormhole-reagent-bank: Reagenzienbank-Crafting
 *
 * Ergaenzung zu mod-reagent-bank-account: Fehlen beim Wirken eines
 * Berufszaubers Reagenzien in den Taschen, werden sie vor der
 * Reagenzienpruefung (Spell::CheckItems) aus der Reagenzienbank geholt.
 * Kein Core-Patch, nur der Hook AllSpellScript::CanPrepare.
 */

#include "ReagentBankCrafting.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "WorldSession.h"

#include <algorithm>
#include <map>

namespace
{
    bool g_enabled = true;
    bool g_professionsOnly = true;
    bool g_announce = true;

    // Gleiche Schluessellogik wie GetStorageKeys() in mod-reagent-bank-account.
    void GetStorageKeys(Player const* player, uint32& accountKey, uint32& guidKey)
    {
        bool const accountWide = sConfigMgr->GetOption<bool>("ReagentBankAccount.AccountWide", false, false);

        uint32 key = accountWide ? player->GetSession()->GetAccountId() : uint32(player->GetGUID().GetCounter());

        if (QueryResult result = CharacterDatabase.Query(
            "SELECT owner_key FROM mod_reagent_bank_share_members WHERE member_key = {}", key))
            if (uint32 owner = (*result)[0].Get<uint32>())
                key = owner;

        accountKey = accountWide ? key : 0;
        guidKey = accountWide ? 0 : key;
    }

    uint32 GetBankAmount(uint32 accountKey, uint32 guidKey, uint32 itemEntry)
    {
        QueryResult result = CharacterDatabase.Query(
            "SELECT amount FROM mod_reagent_bank_account "
            "WHERE account_id = {} AND guid = {} AND item_entry = {}",
            accountKey, guidKey, itemEntry);

        return result ? (*result)[0].Get<uint32>() : 0;
    }

    bool IsProfessionSpell(uint32 spellId)
    {
        SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellId);
        for (auto itr = bounds.first; itr != bounds.second; ++itr)
            if (IsProfessionSkill(itr->second->SkillLine))
                return true;

        return false;
    }
}

bool ReagentBankCrafting_PullMissing(Player* player, std::vector<std::pair<uint32, uint32>> const& reagents)
{
    if (!player || !player->GetSession())
        return false;

    // Benoetigte Menge pro Item zusammenfassen
    std::map<uint32, uint32> needed;
    for (auto const& [itemId, count] : reagents)
        if (itemId && count)
            needed[itemId] += count;

    std::map<uint32, uint32> missing;
    for (auto const& [itemId, count] : needed)
    {
        uint32 const have = player->GetItemCount(itemId, false);
        if (have < count)
            missing[itemId] = count - have;
    }

    if (missing.empty())
        return true;

    if (!g_enabled || !sConfigMgr->GetOption<bool>("ReagentBankAccount.Enable", true, false))
        return false;

    uint32 accountKey = 0;
    uint32 guidKey = 0;
    GetStorageKeys(player, accountKey, guidKey);

    // Erst pruefen, ob Bank und Taschen zusammen reichen und alles in die Taschen passt
    for (auto const& [itemId, count] : missing)
    {
        if (GetBankAmount(accountKey, guidKey, itemId) < count)
            return false;

        ItemPosCountVec dest;
        InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count);
        if (msg != EQUIP_ERR_OK)
        {
            player->SendEquipError(msg, nullptr, nullptr, itemId);
            return false;
        }
    }

    for (auto const& [itemId, count] : missing)
    {
        ItemPosCountVec dest;
        if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count) != EQUIP_ERR_OK)
            return false;

        // Synchron abbuchen, damit die naechste Herstellung den neuen Bestand sieht
        CharacterDatabase.DirectExecute(
            "UPDATE mod_reagent_bank_account SET amount = amount - {} "
            "WHERE account_id = {} AND guid = {} AND item_entry = {} AND amount >= {}",
            count, accountKey, guidKey, itemId, count);
        CharacterDatabase.DirectExecute(
            "DELETE FROM mod_reagent_bank_account "
            "WHERE account_id = {} AND guid = {} AND item_entry = {} AND amount = 0",
            accountKey, guidKey, itemId);

        Item* item = player->StoreNewItem(dest, itemId, true);
        if (!item)
            return false;

        player->SendNewItem(item, count, true, false);

        if (g_announce)
            if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId))
                ChatHandler(player->GetSession()).PSendSysMessage("Reagenzienbank: {}x {} entnommen.", count, proto->Name1);
    }

    return true;
}

class ReagentBankCrafting_SpellScript : public AllSpellScript
{
public:
    ReagentBankCrafting_SpellScript() : AllSpellScript("ReagentBankCrafting_SpellScript", { ALLSPELLHOOK_CAN_PREPARE }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) override
    {
        if (!g_enabled)
            return true;

        Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        if (!player || spell->HasTriggeredCastFlag(TRIGGERED_IGNORE_POWER_AND_REAGENT_COST))
            return true;

        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (player->CanNoReagentCast(spellInfo))
            return true;

        if (spell->m_CastItem && spell->m_CastItem->GetTemplate()->HasFlag(ITEM_FLAG_NO_REAGENT_COST))
            return true;

        if (g_professionsOnly && !IsProfessionSpell(spellInfo->Id))
            return true;

        std::vector<std::pair<uint32, uint32>> reagents;
        for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
            if (spellInfo->Reagent[i] > 0 && spellInfo->ReagentCount[i] > 0)
                reagents.emplace_back(uint32(spellInfo->Reagent[i]), spellInfo->ReagentCount[i]);

        if (!reagents.empty())
            ReagentBankCrafting_PullMissing(player, reagents);

        // Nie blockieren: die normale Reagenzienpruefung entscheidet danach
        return true;
    }
};

class ReagentBankCrafting_WorldScript : public WorldScript
{
public:
    ReagentBankCrafting_WorldScript() : WorldScript("ReagentBankCrafting_WorldScript", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        g_enabled = sConfigMgr->GetOption<bool>("ReagentBankCrafting.Enable", true);
        g_professionsOnly = sConfigMgr->GetOption<bool>("ReagentBankCrafting.ProfessionsOnly", true);
        g_announce = sConfigMgr->GetOption<bool>("ReagentBankCrafting.Announce", true);
    }
};

void AddSC_mod_reagent_bank_crafting()
{
    new ReagentBankCrafting_SpellScript();
    new ReagentBankCrafting_WorldScript();
}
