#include <iostream>
#include <map>
#include <string>

#define con(m) std::cout << m

typedef std::string str;
typedef std::size_t siz;

bool allow_bots = false;
bool stop_stats = true;

int idsource = 0;

struct GUID
{
	int id = 0;
	bool bot = false;
	bool is_bot() const { return bot; }
	GUID(bool bot): id(++idsource), bot(bot) {}
	bool operator<(const GUID& guid) const { return id < guid.id; }
};

typedef std::map<GUID, siz> guid_siz_map;

guid_siz_map teams;

const siz TEAM_R = 1;
const siz TEAM_B = 2;

void stall_clients()
{
	con("stall_clients()");
}

void unstall_clients()
{
	con("unstall_clients()");
}

void check_bots_and_players()
{
	con("== checking: ==");
	con("allow_bots: " << allow_bots);

	bool stats_stopped = stop_stats;

	stop_stats = false;
	siz human_players_r = 0;
	siz human_players_b = 0;
	siz bot_players_r = 0;
	siz bot_players_b = 0;

	for(auto ci = teams.begin(); ci != teams.end(); ++ci)
	{
		if(ci->first.is_bot())
		{
			if(ci->second == TEAM_R)
				++bot_players_r;
			else if(ci->second == TEAM_B)
				++bot_players_b;
			if(!allow_bots)
				stop_stats = true;
		}
		else
		{
			if(ci->second == TEAM_R)
				++human_players_r;
			else if(ci->second == TEAM_B)
				++human_players_b;
		}
	}

	siz players_r = human_players_r;
	siz players_b = human_players_b;

	if(allow_bots)
	{
		players_r += bot_players_r;
		players_b += bot_players_b;
	}

	con("human_players_r: " << human_players_r);
	con("human_players_b: " << human_players_b);
	con("bot_players_r: " << bot_players_r);
	con("bot_players_b: " << bot_players_b);
	con("players_r: " << players_r);
	con("players_b: " << players_b);

	if(stop_stats || !players_r || !players_b)
	{
		stop_stats = true;
		stall_clients();

		if(stats_stopped != stop_stats)
			con("^2Stats recording deactivated^7");
	}
	else
	{
		unstall_clients();
		if(stats_stopped != stop_stats)
			con("^2Stats recording activated^7");
	}
}

int main()
{
	allow_bots = false;

	check_bots_and_players();

	teams = {{{true}, TEAM_R}};
	check_bots_and_players();

	teams = {{{true}, TEAM_R}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}};
	check_bots_and_players();

	teams = {{{true}, TEAM_R}, {{true}, TEAM_R}};
	check_bots_and_players();

	teams = {{{true}, TEAM_R}, {{false}, TEAM_R}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}, {{true}, TEAM_R}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}, {{false}, TEAM_R}};
	check_bots_and_players();

	teams = {{{true}, TEAM_R}, {{true}, TEAM_B}};
	check_bots_and_players();

	teams = {{{true}, TEAM_R}, {{false}, TEAM_B}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}, {{true}, TEAM_B}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}, {{false}, TEAM_B}};
	check_bots_and_players();

	allow_bots = true;

	check_bots_and_players();

	teams = {{{true}, TEAM_R}};
	check_bots_and_players();

	teams = {{{true}, TEAM_R}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}};
	check_bots_and_players();

	teams = {{{true}, TEAM_R}, {{true}, TEAM_R}};
	check_bots_and_players();

	teams = {{{true}, TEAM_R}, {{false}, TEAM_R}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}, {{true}, TEAM_R}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}, {{false}, TEAM_R}};
	check_bots_and_players();

	teams = {{{true}, TEAM_R}, {{true}, TEAM_B}};
	check_bots_and_players();

	teams = {{{true}, TEAM_R}, {{false}, TEAM_B}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}, {{true}, TEAM_B}};
	check_bots_and_players();

	teams = {{{false}, TEAM_R}, {{false}, TEAM_B}};
	check_bots_and_players();
}
