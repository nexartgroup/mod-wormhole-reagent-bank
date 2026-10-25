/*
 * mod-wormhole-reagent-bank: Tragbares Wurmloch
 *
 * Das "Muster: Tragbares Wurmloch" ist ein wiederverwendbares Schnittmuster.
 * Ein eigener Schneiderei-Zauber ginge nur mit Client-Patch (Spell.dbc,
 * SkillLineAbility.dbc), daher stellt das Muster die Tasche per Rechtsklick
 * direkt her, wenn Fertigkeit und Reagenzien vorhanden sind.
 */

#include "Chat.h"
#include "Item.h"
#include "Player.h"
#include "ScriptMgr.h"

// Reagenzienbank-Teil liegt im selben Modul; ohne den Header wird nur aus den Taschen genommen.
#if __has_include("ReagentBankCrafting.h")
#include "ReagentBankCrafting.h"
#define PW_HAS_REAGENT_BANK 1
#endif

namespace
{
    // Eintrag muss in der Client-Item.dbc stehen, sonst zeigt der Client nur "?"
    constexpr uint32 ITEM_PORTABLE_WORMHOLE = 38059;

    constexpr uint32 SKILL_TAILORING_ID     = 197;
    constexpr uint32 REQUIRED_SKILL_VALUE   = 80;

    struct Reagent
    {
        uint32 itemId;
        uint32 count;
        char const* name;
    };

    constexpr Reagent REAGENTS[] =
    {
        { 4305, 2, "Seidenstoffballen" },
        { 2321, 5, "Feiner Faden"      },
        { 2318, 1, "Leichtes Leder"    },
    };
}

class item_pattern_portable_wormhole : public ItemScript
{
public:
    item_pattern_portable_wormhole() : ItemScript("item_pattern_portable_wormhole") { }

    bool OnUse(Player* player, Item* /*item*/, SpellCastTargets const& /*targets*/) override
    {
        ChatHandler chat(player->GetSession());

        if (player->GetSkillValue(SKILL_TAILORING_ID) < REQUIRED_SKILL_VALUE)
        {
            chat.PSendSysMessage("Ihr benötigt mindestens {} Punkte in Schneiderei.", REQUIRED_SKILL_VALUE);
            return true;
        }

#ifdef PW_HAS_REAGENT_BANK
        // Fehlende Reagenzien zuerst aus der Reagenzienbank holen
        std::vector<std::pair<uint32, uint32>> needed;
        for (Reagent const& reagent : REAGENTS)
            needed.emplace_back(reagent.itemId, reagent.count);
        ReagentBankCrafting_PullMissing(player, needed);
#endif

        for (Reagent const& reagent : REAGENTS)
        {
            if (!player->HasItemCount(reagent.itemId, reagent.count))
            {
                chat.PSendSysMessage("Euch fehlen Reagenzien: {}x {}.", reagent.count, reagent.name);
                return true;
            }
        }

        ItemPosCountVec dest;
        if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, ITEM_PORTABLE_WORMHOLE, 1) != EQUIP_ERR_OK)
        {
            player->SendEquipError(EQUIP_ERR_INVENTORY_FULL, nullptr, nullptr, ITEM_PORTABLE_WORMHOLE);
            return true;
        }

        for (Reagent const& reagent : REAGENTS)
            player->DestroyItemCount(reagent.itemId, reagent.count, true);

        if (Item* bag = player->StoreNewItem(dest, ITEM_PORTABLE_WORMHOLE, true))
            player->SendNewItem(bag, 1, false, true);

        return true;
    }
};

void AddSC_portable_wormhole()
{
    new item_pattern_portable_wormhole();
}
