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
		bool IsScriptedScene(bool& a_povScriptMode, bool& a_movementEnabled)
		{
			const auto playerControls = RE::PlayerControls::GetSingleton();
			const auto controlMap = RE::ControlMap::GetSingleton();
			if (!playerControls || !controlMap) {
				return false;
			}

			a_povScriptMode = playerControls->data.povScriptMode;
			a_movementEnabled = controlMap->IsMovementControlsEnabled();
			return a_povScriptMode && !a_movementEnabled;
		}

		// Logs only when the observed state changes, so one test session gives a short, readable trace.
		void LogStateChange(bool a_povScriptMode, bool a_movementEnabled, bool a_freeRotationBefore, bool a_freeRotationAfter)
		{
			static std::uint8_t last = 0xFF;
			const std::uint8_t now = static_cast<std::uint8_t>(
				(a_povScriptMode ? 1 : 0) | (a_movementEnabled ? 2 : 0) | (a_freeRotationBefore ? 4 : 0) | (a_freeRotationAfter ? 8 : 0));
			if (now == last) {
				return;
			}
			last = now;
			SKSE::log::info(
				"ThirdPersonState::SetFreeRotationMode: povScriptMode={} movementEnabled={} freeRotation vanilla={} final={}",
				a_povScriptMode, a_movementEnabled, a_freeRotationBefore, a_freeRotationAfter);
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
				const bool vanilla = a_this->freeRotationEnabled;

				// Mouse then only rotates the camera (freeRotation), not the player's body.
				bool povScriptMode = false;
				bool movementEnabled = true;
				if (IsScriptedScene(povScriptMode, movementEnabled)) {
					a_this->freeRotationEnabled = true;
				}

				LogStateChange(povScriptMode, movementEnabled, vanilla, a_this->freeRotationEnabled);
			}

			static inline REL::Relocation<decltype(SetFreeRotationMode)> _SetFreeRotationMode;
		};
	}

	void Install()
	{
		ThirdPersonStateHook::Install();
	}
}
