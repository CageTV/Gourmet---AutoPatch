/*
 * Gourmet - AutoPatch
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#include "PCH.h"
#include "Config.h"
#include "Conform.h"
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
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();
	SKSE::log::info("GourmetAutoPatch 1.0.0, game {}", REL::Module::get().version().string());
	Config::Load();
	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}
