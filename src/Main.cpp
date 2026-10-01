#include "Hooks.h"

#include "SKSE/SKSE.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace
{
	void SetupLog()
	{
		auto path = SKSE::log::log_directory();
		if (!path) {
			return;
		}

		*path /= "SexLabSceneFreeLook.log";
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto logger = std::make_shared<spdlog::logger>("global log", std::move(sink));
		logger->set_level(spdlog::level::info);
		logger->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(logger));
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();

	Hooks::Install();
	SKSE::log::info("SexLabSceneFreeLook loaded, ThirdPersonState::SetFreeRotationMode hooked.");
	return true;
}
