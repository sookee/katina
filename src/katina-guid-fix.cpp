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

#include <katina/str.h>
#include <katina/types.h>
#include <katina/utils.h>
#include <katina/Database.h>
#include <katina/Properties.h>

using namespace oastats;
using namespace oastats::types;
using namespace oastats::utils;
using namespace oastats::string;

class Database
: public oastats::data::Database
{
public:
	
	
};

// `guid` varchar(8) NOT NULL,
// `name` varchar(32) NOT NULL,
// `count` int(4) unsigned NOT NULL,
// `date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
// PRIMARY KEY (`guid`,`name`)

int main(int argc, const char* argv[])
{
	str config_dir = expand_env(argc > 1 ? argv[1] : "$HOME/.katina");

	log("Setting config dir: " << config_dir);

	std::ifstream ifs((config_dir + "/katina.conf").c_str());

	Properties props;

	log("Reading config file:");

	props.load(ifs);
	ifs.close();

	Database db;
	
	// const str& host, siz port, const str& user, const str& pass, const str& base
	db.config(props.get("db.host"), props.get<siz>("db.port")
		, props.get("db.user"), props.get("db.pass"), props.get("db.base"));

	
}
