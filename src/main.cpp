#include "version.h"


class FavoriteMiscItems
{
public:
	static void PatchFunc1()
	{
		REL::Relocation<std::uintptr_t> setFavorite{ REL::ID(50990) };

		auto& trampoline = SKSE::GetTrampoline();
		_IsValidFormType = trampoline.write_call<5>(setFavorite.address() + 0x30, IsValidFormType);
	}


	static void PatchFunc2()
	{
		REL::Relocation<std::uintptr_t> setFavorite2{ REL::ID(50221) };

		auto& trampoline = SKSE::GetTrampoline();
		_IsValidFormType = trampoline.write_call<5>(setFavorite2.address() + 0x2F, IsValidFormType);
	}

private:
	static bool IsValidFormType(RE::TESForm* a_form)
	{
		if (a_form) {
			switch (a_form->GetFormType()) {
			case RE::FormType::Book:
			case RE::FormType::Misc:
			case RE::FormType::Apparatus:
			case RE::FormType::KeyMaster:
			case RE::FormType::Note:
			case RE::FormType::SoulGem:
				return true;
			}
		}
		return _IsValidFormType(a_form);
	}
	static inline REL::Relocation<decltype(IsValidFormType)> _IsValidFormType;
};


class FavoriteBooks
{
public:
	static void SendBookEvent()
	{
		REL::Relocation<std::uintptr_t> equipFavorites{ REL::ID(37951) };

		auto& trampoline = SKSE::GetTrampoline();
		_GetIsWorn = trampoline.write_call<5>(equipFavorites.address() + 0x2D, GetIsWorn);	//picked an arbitary function
	}

private:
	static RE::ExtraDataList* GetIsWorn(RE::InventoryEntryData* a_entryData, bool a_unk01)
	{
		auto object = a_entryData->object;
		if (object && object->GetFormType() == RE::FormType::Book) {
			auto book = static_cast<RE::TESObjectBOOK*>(object);
			ReadBook(book);
		}
		return _GetIsWorn(a_entryData, a_unk01);
	}
	static inline REL::Relocation<decltype(GetIsWorn)> _GetIsWorn;


	static bool ProcessSpellBook(RE::TESObjectBOOK* a_book, RE::PlayerCharacter* a_player)
	{
		using func_t = decltype(&FavoriteBooks::ProcessSpellBook);
		REL::Relocation<func_t> func{ REL::ID(17439) };
		return func(a_book, a_player);
	}


	static void DisplayBookMenu(RE::BSString* a_description, RE::ExtraDataList* a_list, RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book, RE::NiPoint3* a_pos, RE::NiMatrix3* a_rot, float a_scale, bool a_defaultPos)
	{
		using func_t = decltype(&FavoriteBooks::DisplayBookMenu);
		REL::Relocation<func_t> func{ REL::ID(50122) };
		return func(a_description, a_list, a_ref, a_book, a_pos, a_rot, a_scale, a_defaultPos);
	}


	static void ReadBook(RE::TESObjectBOOK* a_book)
	{
		if (a_book) {
			auto player = RE::PlayerCharacter::GetSingleton();
			if (a_book->data.flags.all(RE::OBJ_BOOK::Flag::kTeachesSpell)) {
				if (ProcessSpellBook(a_book, player)) {
					player->RemoveItem(a_book, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				}
				return;
			}
			RE::BSString str;
			a_book->GetDescription(str, nullptr);
			if (!str.empty()) {
				auto UI = RE::UIMessageQueue::GetSingleton();
				auto strings = RE::InterfaceStrings::GetSingleton();
				if (UI && strings) {
					UI->AddMessage(strings->favoritesMenu, RE::UI_MESSAGE_TYPE::kHide, nullptr);
				}
				auto pos = RE::NiPoint3();
				auto rot = RE::NiMatrix3();
				rot.SetEulerAnglesXYZ(-0.05f, -0.05f, 1.50f);
				DisplayBookMenu(&str, &player->extraList, nullptr, a_book, &pos, &rot, 1.0f, true);
			}
		}
	}
};


class ActivateBook
{
public:
	static void DumpPos()
	{
		REL::Relocation<std::uintptr_t> equipFavorites{ REL::ID(17437) };

		auto& trampoline = SKSE::GetTrampoline();
		_DisplayBookMenu = trampoline.write_call<5>(equipFavorites.address() + 0x186, DisplayBookMenu);
	}

private:
	static void DisplayBookMenu(RE::BSString* a_description, RE::ExtraDataList* a_list, RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book, RE::NiPoint3* a_pos, RE::NiMatrix3* a_rot, float a_scale, bool a_defaultPos)
	{
		if (a_book && a_rot && a_pos) {
			float x, y, z;
			a_rot->ToEulerAnglesXYZ(x, y, z);
			logger::info("{} - pos({}, {}, {}) | rot({} , {} ,{})", a_book->GetName(), a_pos->x, a_pos->y, a_pos->z, x, y, z);
		}

		return _DisplayBookMenu(a_description, a_list, a_ref, a_book, a_pos, a_rot, a_scale, a_defaultPos);
	}

	static inline REL::Relocation<decltype(DisplayBookMenu)> _DisplayBookMenu;
};


void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		{
			logger::info("patching favorite function");
			FavoriteMiscItems::PatchFunc1();
			logger::info("patching unfavorite function");
			FavoriteMiscItems::PatchFunc2();
			logger::info("patching favorite equip function");
			FavoriteBooks::SendBookEvent();

			//ActivateBook::DumpPos();
		}
		break;
	}
}


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	try {
		auto path = logger::log_directory().value() / "po3_FavoriteMiscItems.log";
		auto log = spdlog::basic_logger_mt("global log", path.string(), true);
		log->flush_on(spdlog::level::info);

#ifndef NDEBUG
		log->set_level(spdlog::level::debug);
		log->sinks().push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#else
		log->set_level(spdlog::level::info);

#endif
		spdlog::set_default_logger(log);
		spdlog::set_pattern("[%H:%M:%S] [%l] %v");

		logger::info("Favorite Misc Items {}", SOS_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "Favorite Misc Items";
		a_info->version = SOS_VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			logger::critical("Loaded in editor, marking as incompatible");
			return false;
		}

		const auto ver = a_skse->RuntimeVersion();
		if (ver < SKSE::RUNTIME_1_5_39) {
			logger::critical("Unsupported runtime version {}", ver.string());
			return false;
		}
	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	try {
		logger::info("Favorite Misc Items loaded");

		SKSE::Init(a_skse);
		SKSE::AllocTrampoline(1 << 5);

		auto messaging = SKSE::GetMessagingInterface();
		if (!messaging->RegisterListener("SKSE", OnInit)) {
			logger::critical("Failed to register messaging listener!");
			return false;
		}
	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}
