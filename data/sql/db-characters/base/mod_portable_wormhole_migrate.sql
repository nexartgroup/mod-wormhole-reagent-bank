-- mod-portable-wormhole: bereits vorhandene Taschen/Muster der ersten Version
-- (IDs 90000/90001) auf die vom Client bekannten IDs umziehen.
UPDATE `item_instance` SET `itemEntry` = 38059 WHERE `itemEntry` = 90000;
UPDATE `item_instance` SET `itemEntry` = 21924 WHERE `itemEntry` = 90001;
