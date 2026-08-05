-- Run against the character database.

CREATE TABLE IF NOT EXISTS `char_transfer_names` (
  `name` varchar(12) NOT NULL DEFAULT '',
  PRIMARY KEY (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci ROW_FORMAT=DYNAMIC COMMENT='SEA Character Transfer Reserved Names';
