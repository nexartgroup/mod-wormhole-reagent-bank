#ifndef MOD_REAGENT_BANK_CRAFTING_H
#define MOD_REAGENT_BANK_CRAFTING_H

#include "Define.h"

#include <utility>
#include <vector>

class Player;

// Holt fehlende Reagenzien aus der Reagenzienbank (mod-reagent-bank-account)
// in die Taschen. Es wird nur entnommen, wenn danach alle Reagenzien
// vollstaendig vorhanden sind. Rueckgabe: true, wenn danach alles da ist.
bool ReagentBankCrafting_PullMissing(Player* player, std::vector<std::pair<uint32, uint32>> const& reagents);

#endif
