/*
 * codees.h
 *
 *  Created on: Jan 2, 2013
 *      Author: oasookee@gmail.com
 */

#ifndef _OASTATS_CODES_H_
#define _OASTATS_CODES_H_

enum // AWARD
{
	AW_GAUNTLET
	, AW_EXCELLENT
	, AW_IMPRESSIVE
	, AW_DEFENCE
	, AW_CAPTURE
	, AW_ASSIST
};

enum // FLAG
{
	FL_TAKEN = 0
	, FL_RED = 0
	, FL_BLUE = 1
	, FL_CAPTURED = 1
	, FL_RETURNED = 2
	, FL_DROPPED = 3
	, FL_SIZE
};

//enum class CTF
//{
//	TAKEN
//	, CAPTURED
//	, RETURNED
//	, KILLED
//};

enum // WEAPON
{
	MOD_UNKNOWN
	, MOD_SHOTGUN
	, MOD_GAUNTLET
	, MOD_MACHINEGUN
	, MOD_GRENADE
	, MOD_GRENADE_SPLASH
	, MOD_ROCKET
	, MOD_ROCKET_SPLASH
	, MOD_PLASMA
	, MOD_PLASMA_SPLASH
	, MOD_RAILGUN
	, MOD_LIGHTNING
	, MOD_BFG
	, MOD_BFG_SPLASH
	, MOD_WATER
	, MOD_SLIME
	, MOD_LAVA
	, MOD_CRUSH
	, MOD_TELEFRAG
	, MOD_FALLING
	, MOD_SUICIDE
	, MOD_TARGET_LASER
	, MOD_TRIGGER_HURT
	, MOD_NAIL
	, MOD_CHAINGUN
	, MOD_PROXIMITY_MINE
	, MOD_KAMIKAZE
	, MOD_JUICED
	, MOD_GRAPPLE
};

template <typename E>
typename std::underlying_type<E>::type to_underlying(E e)
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}

#endif /* _OASTATS_CODES_H_ */
