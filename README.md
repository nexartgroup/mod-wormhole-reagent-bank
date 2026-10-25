# mod-wormhole-reagent-bank

AzerothCore-Modul (WotLK 3.3.5a) mit zwei Teilen:

1. **Tragbares Wurmloch**: Tasche mit 36 Plätzen und ein wiederverwendbares Schneiderei-Muster.
2. **Reagenzienbank-Crafting**: Beim Herstellen werden fehlende Reagenzien automatisch
   aus der Reagenzienbank von [mod-reagent-bank-account](https://github.com/Day36512/mod-reagent-bank-account) geholt.

Kein Core-Patch nötig, beide Teile nutzen vorhandene Script-Hooks.

## Installation

```bash
cd azerothcore/modules
git clone https://github.com/nexartgroup/mod-wormhole-reagent-bank.git
```

Danach cmake neu laufen lassen und bauen. Das SQL unter `data/sql/` spielt der
Worldserver beim Start selbst ein (`Updates.EnableDatabases`). Die Config
`mod_reagent_bank_crafting.conf.dist` nach `etc/modules/mod_reagent_bank_crafting.conf` kopieren.

Voraussetzung für den Crafting-Teil: mod-reagent-bank-account (Tabelle `mod_reagent_bank_account`).

## Tragbares Wurmloch

Tasche **Tragbares Wurmloch** (Item 38059, 36 Plätze) und **Muster: Tragbares Wurmloch** (Item 21924).

- Haris Pilton (Shattrath) verkauft die Tasche für 60.000 Gold.
- Alle Schneiderei-Lehrer verkaufen das Muster (10 Gold) und bekommen dafür das Händler-Flag.
- Muster benutzen (Schneiderei ≥ 125): 10x Seidenstoffballen, 6x Schweres Leder,
  4x Seidenfaden und 2x Schillernde Perle werden zu einem Tragbaren Wurmloch
  (Material aus dem Bereich Stufe 20 bis 30). Das Muster bleibt erhalten.
  Fehlendes Material wird aus der Reagenzienbank genommen.

Grenzen:

- 36 Plätze sind das Maximum: Der 3.3.5a-Client kennt nur 36 Container-Slots.
- Ein echtes Rezept im Berufsfenster bräuchte einen Client-Patch
  (Spell.dbc, SkillLineAbility.dbc), deshalb arbeitet das Muster per Rechtsklick.
- Die IDs 38059 und 21924 sind ungenutzte "NPC Equip"-Platzhalter, die in der
  Item.dbc des Clients als Tasche bzw. Schneiderei-Muster stehen. Eigene neue IDs
  zeigt der 3.3.5a-Client ohne Client-Patch nur als rotes Fragezeichen.

## Reagenzienbank-Crafting

Hook `AllSpellScript::CanPrepare`: Vor der Reagenzienprüfung eines Berufszaubers
werden fehlende Reagenzien aus der Bank in die Taschen gelegt. Entnommen wird nur,
wenn Bank und Taschen zusammen für eine Herstellung reichen. Das gilt für jeden Weg,
der den Zauber auslöst (Berufsfenster, `/cast`-Makros, Addons mit `DoTradeSkill`).

Grenze: Der 3.3.5-Client graut den „Herstellen“-Knopf im Berufsfenster aus, solange
das Material nicht in den Taschen liegt. Dort hilft „Withdraw Needed“ von mod-reagent-bank-account.

Andere Module können `ReagentBankCrafting_PullMissing()` aus `ReagentBankCrafting.h` nutzen.

Optionen in `mod_reagent_bank_crafting.conf`: `ReagentBankCrafting.Enable`,
`ReagentBankCrafting.ProfessionsOnly`, `ReagentBankCrafting.Announce`.
