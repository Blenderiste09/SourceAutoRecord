#pragma once
#include "Modules/ConCommand.hpp"
#include "Modules/ConVar.hpp"

using namespace Tier1;

namespace Commands
{
	// Rebinder
	ConVar sar_rebinder_save;
	ConVar sar_rebinder_reload;
	ConVar sar_bind_save;
	ConVar sar_bind_reload;

	// Info
	ConCommand sar_time_demo;
	ConCommand sar_server_tick;
	ConCommand sar_about;

	// Experimental
	ConVar cl_showtime;

	// Cheats
	ConVar sv_autojump;
	
	// Anti-Cheat
	ConVar _sv_cheats;
	ConVar _sv_bonus_challenge;
}