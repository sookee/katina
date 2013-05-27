#ifndef _OASTATS_CODES_H_
#define _OASTATS_CODES_H_
/*
 * codees.h
 *
 *  Created on: Jan 2, 2013
 *      Author: oasookee@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@gmail.com               |
'------------------------------------------------------------------'

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

http://www.gnu.org/licenses/gpl-2.0.html

'-----------------------------------------------------------------*/

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

enum // MEANS OF DEATH
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
	, MOD_MAXVALUE
};

enum // WEAPONS
{
	WP_NONE,

	WP_GAUNTLET,
	WP_MACHINEGUN,
	WP_SHOTGUN,
	WP_GRENADE_LAUNCHER,
	WP_ROCKET_LAUNCHER,
	WP_LIGHTNING,
	WP_RAILGUN,
	WP_PLASMAGUN,
	WP_BFG,
	WP_GRAPPLING_HOOK,
	WP_NAILGUN,
	WP_PROX_LAUNCHER,
	WP_CHAINGUN,

	WP_NUM_WEAPONS
};

#endif /* _OASTATS_CODES_H_ */
