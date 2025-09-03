DROP TABLE IF EXISTS `quest_reward_item_boost`;

CREATE TABLE IF NOT EXISTS `quest_reward_item_boost` (
  `original_item_entry` int unsigned NOT NULL COMMENT '原始物品的Entry ID',
  `player_level` tinyint unsigned NOT NULL COMMENT '玩家等级',
  `player_class` tinyint unsigned NOT NULL COMMENT '玩家职业（1-12: 对应不同职业）',
  `player_spec` tinyint unsigned NOT NULL COMMENT '玩家天赋分支（0, 1, 2）',
  `boosted_item_entry` int unsigned NOT NULL COMMENT '提升后的物品Entry ID',
  PRIMARY KEY (`original_item_entry`, `player_level`, `player_class`, `player_spec`),
  KEY `idx_player_level` (`player_level`),
  KEY `idx_player_class` (`player_class`),
  KEY `idx_player_spec` (`player_spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='任务奖励装备提升表';