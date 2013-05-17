
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
