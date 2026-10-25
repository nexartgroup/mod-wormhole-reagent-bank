-- mod-portable-wormhole
-- 38059 Tragbares Wurmloch (36 Plaetze, Client-/Protokoll-Maximum)
-- 21924 Muster: Tragbares Wurmloch (Schneiderei 125, wiederverwendbar)
--
-- Der 3.3.5a-Client holt Symbol und Taschen-Typ aus seiner eigenen Item.dbc.
-- Neue IDs (z. B. 90000) stehen dort nicht und erscheinen als "?". Deshalb
-- werden zwei ungenutzte Platzhalter ("NPC Equip") belegt, die der Client als
-- Tasche (Symbol INV_Misc_Bag_27) bzw. Schneiderei-Muster (INV_Scroll_03) kennt.

DELETE FROM `item_template` WHERE `entry` IN (90000, 90001, 38059, 21924);
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `BuyCount`, `BuyPrice`, `SellPrice`,
     `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `RequiredSkill`, `RequiredSkillRank`, `maxcount`, `stackable`, `ContainerSlots`,
     `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellcooldown_1`, `spellcategorycooldown_1`,
     `bonding`, `description`, `Material`, `ScriptName`)
VALUES
    (38059, 1, 0, 'Portable Wormhole', 50457, 4, 1, 600000000, 10000,
     18, -1, -1, 80, 0,
     0, 0, 0, 1, 36,
     0, 0, 0, -1, -1,
     2, '', 8, ''),
    (21924, 9, 2, 'Pattern: Portable Wormhole', 1102, 3, 1, 100000, 0,
     0, -1, -1, 20, 0,
     197, 125, 1, 1, 0,
     36177, 0, 0, -1, -1,
     1, 'Use: Sews 10 Bolt of Silk Cloth, 6 Heavy Leather, 4 Silken Thread and 2 Iridescent Pearl into a Portable Wormhole. The pattern is not consumed.', -1, 'item_pattern_portable_wormhole');

DELETE FROM `item_template_locale` WHERE `ID` IN (90000, 90001, 38059, 21924) AND `locale` = 'deDE';
INSERT INTO `item_template_locale` (`ID`, `locale`, `Name`, `Description`) VALUES
    (38059, 'deDE', 'Tragbares Wurmloch', ''),
    (21924, 'deDE', 'Muster: Tragbares Wurmloch',
     'Benutzen: Näht 10 Seidenstoffballen, 6 Schweres Leder, 4 Seidenfaden und 2 Schillernde Perlen zu einem Tragbaren Wurmloch. Das Muster wird nicht verbraucht.');

-- Haris Pilton (Shattrath) verkauft die Tasche fuer 60.000 Gold
DELETE FROM `npc_vendor` WHERE `item` IN (90000, 38059);
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`) VALUES
    (18756, 0, 38059, 0, 0, 0);

-- Alle Schneiderei-Lehrer verkaufen das Muster
DELETE FROM `npc_vendor` WHERE `item` IN (90001, 21924);
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`)
SELECT DISTINCT `cdt`.`CreatureId`, 0, 21924, 0, 0, 0
FROM `creature_default_trainer` `cdt`
JOIN `trainer_spell` `ts` ON `ts`.`TrainerId` = `cdt`.`TrainerId`
WHERE `ts`.`ReqSkillLine` = 197;

UPDATE `creature_template` `ct`
JOIN (SELECT DISTINCT `cdt`.`CreatureId`
      FROM `creature_default_trainer` `cdt`
      JOIN `trainer_spell` `ts` ON `ts`.`TrainerId` = `cdt`.`TrainerId`
      WHERE `ts`.`ReqSkillLine` = 197) `t` ON `t`.`CreatureId` = `ct`.`entry`
SET `ct`.`npcflag` = `ct`.`npcflag` | 128;

-- Gossip-Option "Waren ansehen" fuer Lehrer-Menues, die noch keine haben.
-- OptionNpcFlag 128 blendet sie bei NPCs ohne Haendler-Flag automatisch aus.
DELETE FROM `gossip_menu_option` WHERE `OptionID` = 90 AND `OptionBroadcastTextID` = 3370 AND `OptionType` = 3;
INSERT INTO `gossip_menu_option`
    (`MenuID`, `OptionID`, `OptionIcon`, `OptionText`, `OptionBroadcastTextID`, `OptionType`, `OptionNpcFlag`)
SELECT DISTINCT `ct`.`gossip_menu_id`, 90, 1, 'I want to browse your goods.', 3370, 3, 128
FROM `creature_template` `ct`
JOIN `creature_default_trainer` `cdt` ON `cdt`.`CreatureId` = `ct`.`entry`
JOIN `trainer_spell` `ts` ON `ts`.`TrainerId` = `cdt`.`TrainerId`
WHERE `ts`.`ReqSkillLine` = 197 AND `ct`.`gossip_menu_id` <> 0
  AND NOT EXISTS (SELECT 1 FROM `gossip_menu_option` `g`
                  WHERE `g`.`MenuID` = `ct`.`gossip_menu_id` AND `g`.`OptionType` = 3);
