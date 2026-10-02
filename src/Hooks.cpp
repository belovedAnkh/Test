#include "Hooks.h"

#include "RE/Skyrim.h"
#include "REL/Relocation.h"
#include "SKSE/SKSE.h"

// SexLabSceneFreeLook 2.0
//
// Goal: when a SexLab scene that involves the player starts while the player is in first person,
// Improved Camera SE (bScripted=1 in its profile) should show the scene from the character's eyes
// and keep doing so until the scene ends. Outside SexLab scenes nothing is changed.
//
// Why a plugin is needed (Improved Camera SE 1.1.2 source, ImprovedCameraSE-NG):
//  * SexLab 1.66 first forces third person in sslActorAlias.ClearEffects(), before it disables the
//    player's movement controls. Improved Camera sees one frame of "normal third person"; with the
//    NEFARAM profile (bThirdPerson=0) it then drops its "came from first person" flag for the scene.
//  * Improved Camera treats the scene as finished as soon as movement controls are enabled again;
//    SLSO's widget script calls Game.EnablePlayerControls() every second while the widget is shown,
//    which kicks the camera into vanilla first person (standing eye height, stiff body).
//  * Improved Camera only switches to the eyes when the third person zoom target sits at the
//    closest step; on a scripted switch the game restores the saved third person distance.
//
// What this plugin does, only for the player and only while the player fills an actor alias of one
// of SexLab's 15 thread quests and SexLab has not unlocked the player yet:
//  * When the camera goes first person -> third person for the scene (ThirdPersonState::Begin), turn
//    on PlayerControls::data.povScriptMode (what Game.DisablePlayerControls(abCamSwitch=true) does).
//    Improved Camera reads this flag as "scripted", so the very first third person frame and every
//    later frame count as a scripted scene no matter who toggles the movement controls.
//  * Set the third person zoom target to fMinCurrentZoom so Improved Camera goes to the eyes at once.
//  * Keep ThirdPersonState::freeRotationEnabled on, so the mouse turns the view, not the body.
//  * When SexLab unlocks the player (AnimatingFaction rank 0) or the player leaves the thread alias,
//    turn povScriptMode back off if this plugin turned it on. Improved Camera then returns to first
//    person by itself.

namespace Hooks
{
	namespace
	{
		constexpr std::string_view kSexLab = "SexLab.esm"sv;
		constexpr RE::FormID       kAnimatingFaction = 0x00E50F;  // SexLabAnimatingFaction
		constexpr std::array<RE::FormID, 15> kThreadQuests{     // SexLabThread00 .. SexLabThread14
			0x061EEF, 0x062452, 0x06C62C, 0x06C62D, 0x06C62E, 0x06C62F, 0x06C630, 0x06C631,
			0x06C632, 0x06C633, 0x06C634, 0x06C635, 0x06C636, 0x06C637, 0x06C638
		};

		RE::TESFaction*            g_animatingFaction = nullptr;
		std::vector<RE::TESQuest*> g_threadQuests;

		bool g_leftFirstPerson = false;   // set by FirstPersonState::End, consumed by ThirdPersonState::Begin
		bool g_active = false;            // a SexLab scene camera is being managed
		bool g_ownsPovScriptMode = false;  // this plugin switched povScriptMode on and has to switch it off
		bool g_zoomPending = false;        // apply the zoom target once more on the next player update

		float MinZoom()
		{
			if (const auto ini = RE::INISettingCollection::GetSingleton()) {
				if (const auto setting = ini->GetSetting("fMinCurrentZoom:Camera")) {
					return setting->GetFloat();
				}
			}
			return -0.2f;
		}

		bool PlayerInSexLabThread(RE::PlayerCharacter* a_player)
		{
			if (!a_player || g_threadQuests.empty()) {
				return false;
			}
			const auto aliases = a_player->extraList.GetByType<RE::ExtraAliasInstanceArray>();
			if (!aliases) {
				return false;
			}
			RE::BSReadLockGuard locker{ aliases->lock };
			for (const auto instance : aliases->aliases) {
				if (instance && instance->quest &&
					std::find(g_threadQuests.begin(), g_threadQuests.end(), instance->quest) != g_threadQuests.end()) {
					return true;
				}
			}
			return false;
		}

		// sslActorAlias.UnlockActor() sets the AnimatingFaction rank to 0 right before it gives control back.
		bool SexLabUnlockedPlayer(RE::PlayerCharacter* a_player)
		{
			return g_animatingFaction && a_player->IsInFaction(g_animatingFaction) &&
			       a_player->GetFactionRank(g_animatingFaction, true) == 0;
		}

		bool SceneRunning(RE::PlayerCharacter* a_player)
		{
			return PlayerInSexLabThread(a_player) && !SexLabUnlockedPlayer(a_player);
		}

		RE::ThirdPersonState* CurrentThirdPersonState()
		{
			const auto camera = RE::PlayerCamera::GetSingleton();
			if (!camera || !camera->currentState || camera->currentState->id != RE::CameraState::kThirdPerson) {
				return nullptr;
			}
			return static_cast<RE::ThirdPersonState*>(camera->currentState.get());
		}

		void KeepPovScriptMode()
		{
			const auto controls = RE::PlayerControls::GetSingleton();
			if (controls && !controls->data.povScriptMode) {
				controls->data.povScriptMode = true;
				g_ownsPovScriptMode = true;
			}
		}

		void StartScene(RE::ThirdPersonState* a_state)
		{
			g_active = true;
			g_ownsPovScriptMode = false;
			KeepPovScriptMode();

			const float zoom = MinZoom();
			a_state->targetZoomOffset = zoom;
			a_state->freeRotationEnabled = true;
			g_zoomPending = true;

			SKSE::log::info("SexLab scene from first person: povScriptMode on (owned={}), zoom target {:.3f}",
				g_ownsPovScriptMode, zoom);
		}

		void EndScene(const char* a_reason)
		{
			if (!g_active) {
				return;
			}
			if (g_ownsPovScriptMode) {
				if (const auto controls = RE::PlayerControls::GetSingleton()) {
					controls->data.povScriptMode = false;
				}
			}
			SKSE::log::info("SexLab scene camera released ({}), povScriptMode restored={}", a_reason, g_ownsPovScriptMode);
			g_active = false;
			g_ownsPovScriptMode = false;
			g_zoomPending = false;
		}

		struct FirstPersonStateHook
		{
			static void Install()
			{
				REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_FirstPersonState[0] };
				_End = vtbl.write_vfunc(0x02, End);
			}

			static void End(RE::FirstPersonState* a_this)
			{
				_End(a_this);
				g_leftFirstPerson = true;
			}

			static inline REL::Relocation<decltype(End)> _End;
		};

		struct ThirdPersonStateHook
		{
			static void Install()
			{
				REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_ThirdPersonState[0] };
				_Begin = vtbl.write_vfunc(0x01, Begin);
				_SetFreeRotationMode = vtbl.write_vfunc(0x0D, SetFreeRotationMode);
			}

			// Runs inside PlayerCamera::SetState, i.e. inside SexLab's Game.ForceThirdPerson() call,
			// before Improved Camera looks at the new third person state.
			static void Begin(RE::ThirdPersonState* a_this)
			{
				_Begin(a_this);

				const bool fromFirstPerson = std::exchange(g_leftFirstPerson, false);
				if (!fromFirstPerson || g_active) {
					return;
				}
				const auto player = RE::PlayerCharacter::GetSingleton();
				if (!SceneRunning(player)) {
					return;
				}
				// Improved Camera does not take over a forced third person while the dialogue menu is open.
				if (const auto ui = RE::UI::GetSingleton(); ui && ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME)) {
					SKSE::log::info("SexLab scene from first person, dialogue menu open: left to the game");
					return;
				}
				StartScene(a_this);
			}

			static void SetFreeRotationMode(RE::ThirdPersonState* a_this, bool a_weaponSheathed)
			{
				_SetFreeRotationMode(a_this, a_weaponSheathed);
				if (g_active) {
					a_this->freeRotationEnabled = true;
				}
			}

			static inline REL::Relocation<decltype(Begin)>               _Begin;
			static inline REL::Relocation<decltype(SetFreeRotationMode)> _SetFreeRotationMode;
		};

		struct PlayerCharacterHook
		{
			static void Install()
			{
				REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
				_Update = vtbl.write_vfunc(0xAD, Update);
			}

			static void Update(RE::PlayerCharacter* a_this, float a_delta)
			{
				_Update(a_this, a_delta);

				// A first person End that was not followed by a third person Begin in the same switch.
				g_leftFirstPerson = false;

				if (!g_active) {
					return;
				}
				if (!PlayerInSexLabThread(a_this)) {
					EndScene("player left the SexLab thread");
					return;
				}
				if (SexLabUnlockedPlayer(a_this)) {
					EndScene("SexLab unlocked the player");
					return;
				}

				KeepPovScriptMode();
				if (const auto state = CurrentThirdPersonState()) {
					if (std::exchange(g_zoomPending, false)) {
						state->targetZoomOffset = MinZoom();
					}
					state->freeRotationEnabled = true;
				}
			}

			static inline REL::Relocation<decltype(Update)> _Update;
		};
	}

	void Install()
	{
		FirstPersonStateHook::Install();
		ThirdPersonStateHook::Install();
		PlayerCharacterHook::Install();
	}

	void OnDataLoaded()
	{
		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return;
		}
		g_animatingFaction = dataHandler->LookupForm<RE::TESFaction>(kAnimatingFaction, kSexLab);
		g_threadQuests.clear();
		for (const auto id : kThreadQuests) {
			if (const auto quest = dataHandler->LookupForm<RE::TESQuest>(id, kSexLab)) {
				g_threadQuests.push_back(quest);
			}
		}
		SKSE::log::info("SexLab forms: AnimatingFaction {}, thread quests {}/{}",
			g_animatingFaction ? "found" : "MISSING", g_threadQuests.size(), kThreadQuests.size());
	}

	void OnGameLoad()
	{
		// The game rebuilds PlayerControls on load; only forget our own state.
		g_active = false;
		g_ownsPovScriptMode = false;
		g_zoomPending = false;
		g_leftFirstPerson = false;
	}
}
