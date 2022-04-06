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

		logger::info("patching favorite function");
	}
}

namespace FavoriteBook
{
	struct detail
	{
		static bool process_book(RE::TESObjectBOOK* a_book, RE::TESObjectREFR* a_reader)
		{
			using func_t = decltype(&process_book);
			REL::Relocation<func_t> func{ RELOCATION_ID(17439, 17842) };
			return func(a_book, a_reader);
		}

		static void display_book_menu(RE::BSString* a_description, RE::ExtraDataList* a_list, RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book, const RE::NiPoint3& a_pos, const RE::NiMatrix3& a_rot, float a_scale, bool a_defaultPos)
		{
			using func_t = decltype(&display_book_menu);
			REL::Relocation<func_t> func{ RELOCATION_ID(50122, 51053) };
			return func(a_description, a_list, a_ref, a_book, a_pos, a_rot, a_scale, a_defaultPos);
		}

		static void read_book(RE::TESObjectBOOK* a_book)
		{
			const auto player = RE::PlayerCharacter::GetSingleton();
			const auto UI = RE::UI::GetSingleton();

			if (!player || !UI || !UI->IsMenuOpen(RE::FavoritesMenu::MENU_NAME)) {
				return;
			}

		    if (a_book->TeachesSpell()) {
				if (process_book(a_book, player)) {
					player->RemoveItem(a_book, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				}
			} else {
				RE::BSString str;
				a_book->GetDescription(str, nullptr);
				if (!str.empty()) {
                    if (const auto UIMsgQueue = RE::UIMessageQueue::GetSingleton()) {
						UIMsgQueue->AddMessage(RE::FavoritesMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
					}
					RE::NiPoint3 pos{};
					RE::NiMatrix3 rot{};
					rot.SetEulerAnglesXYZ(-0.05f, -0.05f, 1.50f);
					display_book_menu(&str, &player->extraList, nullptr, a_book, pos, rot, 1.0f, true);
				}
			}
		}
	};

	struct GetIsWorn
	{
		static RE::ExtraDataList* thunk(RE::InventoryEntryData* a_entryData, bool a_unk01)
		{
            const auto object = a_entryData->object;
            if (const auto book = object ? object->As<RE::TESObjectBOOK>() : nullptr) {
				detail::read_book(book);
			}
			return func(a_entryData, a_unk01);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> func{ RELOCATION_ID(37951, 38907), 0x2D };
		stl::write_thunk_call<GetIsWorn>(func.address());

		logger::info("patching favorite equip function");
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Favorite Misc Items");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary(true);
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
