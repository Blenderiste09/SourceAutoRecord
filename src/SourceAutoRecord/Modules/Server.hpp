#pragma once
#include "../Commands.hpp"
#include "Offsets.hpp"

#define IN_JUMP (1 << 1)

using _CheckJumpButton = int(__thiscall*)(void* thisptr);

using namespace Commands;

// server.dll
namespace Server
{
	bool CantJumpNextTime = false;

	namespace Original
	{
		_CheckJumpButton CheckJumpButton;
	}

	namespace Detour
	{
		int __fastcall CheckJumpButton(void* thisptr, int edx)
		{
			int *pM_nOldButtons = NULL;
			int origM_nOldButtons = 0;

			if (!_sv_bonus_challenge.GetBool() && sv_autojump.GetBool()) {
				pM_nOldButtons = (int*)(*((uintptr_t*)thisptr + Offsets::mv) + Offsets::m_nOldButtons);
				origM_nOldButtons = *pM_nOldButtons;

				if (!CantJumpNextTime)
					*pM_nOldButtons &= ~IN_JUMP;
			}
			CantJumpNextTime = false;

			bool result = Original::CheckJumpButton(thisptr);

			if (!_sv_bonus_challenge.GetBool() && sv_autojump.GetBool()) {
				if (!(*pM_nOldButtons & IN_JUMP))
					*pM_nOldButtons = origM_nOldButtons;
			}

			if (result) CantJumpNextTime = true;
			return result;
		}
	}
}