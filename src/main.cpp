namespace FavoriteMiscItem
{
	struct CanBeFavorited
	{
		static bool func(RE::TESForm* a_form)
		{
			bool result = false;

			if (a_form) {
				switch (a_form->GetFormType()) {
				case RE::FormType::Book:
				case RE::FormType::Misc:
				case RE::FormType::Apparatus:
				case RE::FormType::KeyMaster:
				case RE::FormType::Note:
				case RE::FormType::SoulGem:
				case RE::FormType::Scroll:
				case RE::FormType::Armor:
				case RE::FormType::Ingredient:
				case RE::FormType::Light:
				case RE::FormType::Weapon:
				case RE::FormType::Ammo:
				case RE::FormType::AlchemyItem:
				case RE::FormType::Projectile:
					result = true;
					break;
				default:
					break;
				}
			}
			return result;
		}
		static inline constexpr std::size_t size = 0x6F;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> func{ RELOCATION_ID(50944, 51821) };
		stl::asm_replace<CanBeFavorited>(func.address());

		logger::info("patching FavoritesManager::CanBeFavorited");
	}
}

namespace FavoriteBook
{
	struct detail
	{
		static bool read_book(RE::TESObjectBOOK* a_book, RE::TESObjectREFR* a_reader)
		{
			using func_t = decltype(&read_book);
			REL::Relocation<func_t> func{ RELOCATION_ID(17439, 17842) };
			return func(a_book, a_reader);
		}

		static void open_book_menu(const RE::BSString& a_description, const RE::ExtraDataList* a_extraList, RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book, const RE::NiPoint3& a_pos, const RE::NiMatrix3& a_rot, float a_scale, bool a_defaultPos)
		{
			using func_t = decltype(&open_book_menu);
			REL::Relocation<func_t> func{ RELOCATION_ID(50122, 51053) };
			return func(a_description, a_extraList, a_ref, a_book, a_pos, a_rot, a_scale, a_defaultPos);
		}
	};

	struct ToggleEquipItem
	{
		static void thunk(RE::ActorEquipManager* a_equipMgr, RE::Actor* a_actor, RE::InventoryEntryData* a_entryData, RE::BGSEquipSlot* a_equipSlot, bool a_unk05)
		{
			func(a_equipMgr, a_actor, a_entryData, a_equipSlot, a_unk05);

			const auto object = a_entryData->object;
			if (const auto book = object ? object->As<RE::TESObjectBOOK>() : nullptr) {
				if (book->TeachesSpell()) {
					if (const auto player = RE::PlayerCharacter::GetSingleton(); detail::read_book(book, player)) {
						player->RemoveItem(book, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
					}
					return;
				}
			    RE::BSString str;
				book->GetDescription(str, nullptr);
				if (const auto UIMsgQueue = RE::UIMessageQueue::GetSingleton(); !str.empty()) {
					UIMsgQueue->AddMessage(RE::FavoritesMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

				    const RE::ExtraDataList* list = a_entryData->extraLists ? a_entryData->extraLists->front() : nullptr;

				    RE::NiMatrix3 rot{};
					rot.SetEulerAnglesXYZ(-0.05f, -0.05f, 1.50f);

				    detail::open_book_menu(str, list, nullptr, book, RE::NiPoint3(), rot, 1.0f, true);
				}
				str.clear();
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(50654, 51548), OFFSET(0xC4, 0xC2) };  //FavoritesMenu::UseQuickslotItem
		stl::write_thunk_call<ToggleEquipItem>(target.address());

		logger::info("patching FavoritesMenu::UseQuickslotItem");
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Favorite Misc Items");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary();
	v.UsesNoStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Favorite Misc Items";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("loaded");

	SKSE::Init(a_skse);

	SKSE::AllocTrampoline(14);

	FavoriteMiscItem::Install();
	FavoriteBook::Install();

	return true;
}
