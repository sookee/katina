SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='TRADITIONAL,ALLOW_INVALID_DATES';

-- -----------------------------------------------------
-- Table `awards`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `awards` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `type` INT(1) UNSIGNED NOT NULL ,
  `count` INT(2) UNSIGNED NOT NULL DEFAULT '0' ,
  PRIMARY KEY (`game_id`, `guid`, `type`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `caps`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `caps` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `count` INT(2) UNSIGNED NOT NULL DEFAULT '0' ,
  PRIMARY KEY (`game_id`, `guid`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `damage`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `damage` (
  `game_id` INT(4) NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `mod` INT(1) NOT NULL ,
  `hits` INT(4) NOT NULL ,
  `dmgDone` INT(4) NOT NULL ,
  `hitsRecv` INT(4) NOT NULL ,
  `dmgRecv` INT(4) NOT NULL ,
  PRIMARY KEY (`game_id`, `guid`, `mod`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `deaths`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `deaths` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `weap` INT(1) UNSIGNED NOT NULL ,
  `count` INT(2) UNSIGNED NOT NULL DEFAULT '0' ,
  PRIMARY KEY (`game_id`, `guid`, `weap`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `game`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `game` (
  `game_id` INT(4) UNSIGNED NOT NULL AUTO_INCREMENT ,
  `host` INT(4) UNSIGNED NOT NULL ,
  `port` INT(2) UNSIGNED NOT NULL ,
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP ,
  `map` VARCHAR(32) NOT NULL ,
  PRIMARY KEY USING BTREE (`game_id`) )
ENGINE = MyISAM
AUTO_INCREMENT = 47
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `kills`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `kills` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `weap` INT(1) UNSIGNED NOT NULL ,
  `count` INT(2) UNSIGNED NOT NULL DEFAULT '0' ,
  PRIMARY KEY (`game_id`, `guid`, `weap`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `ovo`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `ovo` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid1` VARCHAR(8) NOT NULL ,
  `guid2` VARCHAR(8) NOT NULL ,
  `count` INT(2) UNSIGNED NOT NULL DEFAULT '0' ,
  PRIMARY KEY (`game_id`, `guid1`, `guid2`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `player`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `player` (
  `guid` VARCHAR(8) NOT NULL ,
  `name` VARCHAR(32) NOT NULL ,
  `count` INT(4) UNSIGNED NOT NULL ,
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP ,
  PRIMARY KEY (`guid`, `name`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `playerstats`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `playerstats` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `fragsFace` INT(2) UNSIGNED NOT NULL ,
  `fragsBack` INT(2) UNSIGNED NOT NULL ,
  `fraggedInFace` INT(2) UNSIGNED NOT NULL ,
  `fraggedInBack` INT(2) UNSIGNED NOT NULL ,
  `spawnKillsDone` INT(2) UNSIGNED NOT NULL ,
  `spawnKillsRecv` INT(2) UNSIGNED NOT NULL ,
  `pushesDone` INT(2) UNSIGNED NOT NULL ,
  `pushesRecv` INT(2) UNSIGNED NOT NULL ,
  `healthPickedUp` INT(4) UNSIGNED NOT NULL ,
  `armorPickedUp` INT(4) UNSIGNED NOT NULL ,
  `holyShitFrags` INT(2) UNSIGNED NOT NULL ,
  `holyShitFragged` INT(2) UNSIGNED NOT NULL ,
  `carrierFrags` INT(2) UNSIGNED NOT NULL ,
  `carrierFragsRecv` INT(2) UNSIGNED NOT NULL ,
  PRIMARY KEY (`game_id`, `guid`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `speed`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `speed` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `flag` TINYINT(1) NOT NULL ,
  `dist` INT(4) UNSIGNED NOT NULL ,
  `time` INT(4) UNSIGNED NOT NULL ,
  PRIMARY KEY (`game_id`, `guid`, `flag`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `polls`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `polls` (
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP ,
  `type` VARCHAR(8) NOT NULL ,
  `item` VARCHAR(32) NOT NULL ,
  `love` INT(4) UNSIGNED NOT NULL ,
  `hate` INT(4) UNSIGNED NOT NULL ,
  PRIMARY KEY (`date`, `type`, `item`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `time`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `time` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `count` INT(4) UNSIGNED NOT NULL COMMENT '// seconds in game' ,
  PRIMARY KEY (`game_id`, `guid`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `user`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `user` (
  `guid` VARCHAR(8) NOT NULL ,
  `name` VARCHAR(32) NOT NULL ,
  PRIMARY KEY (`guid`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `version`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `version` (
  `maj` INT(10) UNSIGNED NOT NULL ,
  `min` INT(2) UNSIGNED ZEROFILL NOT NULL ,
  PRIMARY KEY (`maj`, `min`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `votes`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `votes` (
  `type` VARCHAR(8) NOT NULL ,
  `item` VARCHAR(32) NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `count` INT(4) NOT NULL ,
  PRIMARY KEY (`type`, `item`, `guid`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `weapon_usage`
-- -----------------------------------------------------

CREATE  TABLE IF NOT EXISTS `weapon_usage` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `weap` INT(1) NOT NULL ,
  `shots` INT(4) NOT NULL ,
  PRIMARY KEY (`game_id`, `guid`, `weap`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
