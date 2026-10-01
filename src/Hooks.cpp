#include "Hooks.h"

#include "RE/Skyrim.h"
#include "REL/Relocation.h"
#include "SKSE/SKSE.h"

namespace Hooks
{
	namespace
	{
		// Same "scripted" state that Improved Camera SE (NG) reads: the scene/script has switched the
		// player controls into POV script mode and has taken away movement control.
		bool IsScriptedScene()
		{
			const auto playerControls = RE::PlayerControls::GetSingleton();
			const auto controlMap = RE::ControlMap::GetSingleton();
			if (!playerControls || !controlMap) {
				return false;
			}

			return playerControls->data.povScriptMode && !controlMap->IsMovementControlsEnabled();
		}

		struct ThirdPersonStateHook
		{
			static void Install()
			{
				REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_ThirdPersonState[0] };
				_SetFreeRotationMode = vtbl.write_vfunc(0x0D, SetFreeRotationMode);
			}

			static void SetFreeRotationMode(RE::ThirdPersonState* a_this, bool a_weaponSheathed)
			{
				_SetFreeRotationMode(a_this, a_weaponSheathed);

				// Mouse then only rotates the camera (freeRotation), not the player's body.
				if (IsScriptedScene()) {
					a_this->freeRotationEnabled = true;
				}
			}

			static inline REL::Relocation<decltype(SetFreeRotationMode)> _SetFreeRotationMode;
		};
	}

	void Install()
	{
		ThirdPersonStateHook::Install();
	}
}
