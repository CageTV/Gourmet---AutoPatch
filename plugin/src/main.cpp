/*
 * Gourmet - AutoPatch
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#include "PCH.h"
#include "Config.h"
#include "Conform.h"
#include "Animations.h"
#include "Menu.h"

namespace
{
	void SetupLog()
	{
		const auto dir = SKSE::log::log_directory();
		if (!dir) {
			return;
		}
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((*dir / "GourmetAutoPatch.log").string(), true);
		auto log = std::make_shared<spdlog::logger>("GourmetAutoPatch", std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		log->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
		spdlog::set_default_logger(std::move(log));
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kPostLoad:
			Menu::Register();  // everything is loaded by now, SKSE Menu Framework included
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			// every plugin and every record is loaded and nothing is saved yet: the foods can be changed here
			if (Config::Get().enabled) {
				try {
					Conform::Run();
				} catch (const std::exception& e) {
					SKSE::log::error("failed: {}", e.what());
				}
			} else {
				SKSE::log::info("switched off in config.json");
			}
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
		case SKSE::MessagingInterface::kNewGame:
			// by now every SkyPatcher patch of Eating Animations and Sounds has added its animation effects, so the foods still without one are known
			if (Config::Get().enabled && Conform::Active()) {
				Animations::Apply();
			}
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();
	SKSE::log::info("GourmetAutoPatch 1.2.1, game {}", REL::Module::get().version().string());
	Config::Load();
	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}
