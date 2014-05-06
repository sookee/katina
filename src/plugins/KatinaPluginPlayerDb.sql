SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='TRADITIONAL,ALLOW_INVALID_DATES';

-- -----------------------------------------------------
-- Table `info`
-- -----------------------------------------------------
-- DROP TABLE IF EXISTS `info` ;

CREATE  TABLE IF NOT EXISTS `info` (
  `guid` VARCHAR(8) NOT NULL ,
  `ip` INT(4) UNSIGNED NOT NULL ,
  `name` VARCHAR(32) NOT NULL ,
  PRIMARY KEY (`guid`, `ip`, `name`) )
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;


