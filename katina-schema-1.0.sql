SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='TRADITIONAL,ALLOW_INVALID_DATES';

CREATE SCHEMA IF NOT EXISTS `oadb` DEFAULT CHARACTER SET latin1 ;
USE `oadb` ;

-- -----------------------------------------------------
-- Table `oadb`.`awards`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`awards` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`awards` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `type` INT(1) UNSIGNED NOT NULL ,
  `count` INT(2) UNSIGNED NOT NULL DEFAULT '0' ,
  PRIMARY KEY (`game_id`, `guid`, `type`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `oadb`.`caps`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`caps` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`caps` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `count` INT(2) UNSIGNED NOT NULL DEFAULT '0' ,
  PRIMARY KEY (`game_id`, `guid`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `oadb`.`damage`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`damage` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`damage` (
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
-- Table `oadb`.`deaths`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`deaths` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`deaths` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `weap` INT(1) UNSIGNED NOT NULL ,
  `count` INT(2) UNSIGNED NOT NULL DEFAULT '0' ,
  PRIMARY KEY (`game_id`, `guid`, `weap`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `oadb`.`game`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`game` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`game` (
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
-- Table `oadb`.`kills`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`kills` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`kills` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `weap` INT(1) UNSIGNED NOT NULL ,
  `count` INT(2) UNSIGNED NOT NULL DEFAULT '0' ,
  PRIMARY KEY (`game_id`, `guid`, `weap`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `oadb`.`ovo`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`ovo` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`ovo` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid1` VARCHAR(8) NOT NULL ,
  `guid2` VARCHAR(8) NOT NULL ,
  `count` INT(2) UNSIGNED NOT NULL DEFAULT '0' ,
  PRIMARY KEY (`game_id`, `guid1`, `guid2`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `oadb`.`player`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`player` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`player` (
  `guid` VARCHAR(8) NOT NULL ,
  `name` VARCHAR(32) NOT NULL ,
  `count` INT(4) UNSIGNED NOT NULL ,
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP ,
  PRIMARY KEY (`guid`, `name`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `oadb`.`playerstats`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`playerstats` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`playerstats` (
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
-- Table `oadb`.`polls`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`polls` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`polls` (
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP ,
  `type` VARCHAR(8) NOT NULL ,
  `item` VARCHAR(32) NOT NULL ,
  `love` INT(4) UNSIGNED NOT NULL ,
  `hate` INT(4) UNSIGNED NOT NULL ,
  PRIMARY KEY (`date`, `type`, `item`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `oadb`.`time`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`time` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`time` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `count` INT(4) UNSIGNED NOT NULL COMMENT '// seconds in game' ,
  PRIMARY KEY (`game_id`, `guid`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `oadb`.`user`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`user` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`user` (
  `guid` VARCHAR(8) NOT NULL ,
  `name` VARCHAR(32) NOT NULL ,
  PRIMARY KEY (`guid`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `oadb`.`version`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`version` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`version` (
  `maj` INT(10) UNSIGNED NOT NULL ,
  `min` INT(2) UNSIGNED ZEROFILL NOT NULL ,
  PRIMARY KEY (`maj`, `min`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `oadb`.`votes`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`votes` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`votes` (
  `type` VARCHAR(8) NOT NULL ,
  `item` VARCHAR(32) NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `count` INT(4) NOT NULL ,
  PRIMARY KEY (`type`, `item`, `guid`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `oadb`.`weapon_usage`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `oadb`.`weapon_usage` ;

CREATE  TABLE IF NOT EXISTS `oadb`.`weapon_usage` (
  `game_id` INT(4) UNSIGNED NOT NULL ,
  `guid` VARCHAR(8) NOT NULL ,
  `weap` INT(1) NOT NULL ,
  `shots` INT(4) NOT NULL ,
  PRIMARY KEY (`game_id`, `guid`, `weap`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;

USE `oadb` ;


SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
