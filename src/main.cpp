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
		REL::Relocation<std::uintptr_t> func{ REL::ID(50944) };
		stl::asm_replace<CanBeFavorited>(func.address());

		logger::info("patching favorite function");
	}
}

namespace FavoriteBook
{
	struct detail
	{
		static bool process_spell_book(RE::TESObjectBOOK* a_book, RE::PlayerCharacter* a_player)
		{
			using func_t = decltype(&process_spell_book);
			REL::Relocation<func_t> func{ REL::ID(17439) };
			return func(a_book, a_player);
		}

		static void display_book_menu(RE::BSString* a_description, RE::ExtraDataList* a_list, RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book, const RE::NiPoint3& a_pos, const RE::NiMatrix3& a_rot, float a_scale, bool a_defaultPos)
		{
			using func_t = decltype(&display_book_menu);
			REL::Relocation<func_t> func{ REL::ID(50122) };
			return func(a_description, a_list, a_ref, a_book, a_pos, a_rot, a_scale, a_defaultPos);
		}

		static void read_book(RE::TESObjectBOOK* a_book)
		{
			if (a_book) {
				auto player = RE::PlayerCharacter::GetSingleton();
				if (a_book->data.flags.all(RE::OBJ_BOOK::Flag::kTeachesSpell)) {
					if (process_spell_book(a_book, player)) {
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
					display_book_menu(&str, &player->extraList, nullptr, a_book, pos, rot, 1.0f, true);
				}
			}
		}
	};

	struct GetIsWorn
	{
		static RE::ExtraDataList* thunk(RE::InventoryEntryData * a_entryData, bool a_unk01)
		{
			auto object = a_entryData->object;
			auto book = object ? object->As<RE::TESObjectBOOK>() : nullptr;
			if (book) {
				detail::read_book(book);
			}
			return func(a_entryData, a_unk01);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> func{ REL::ID(37951) };
		stl::write_thunk_call<GetIsWorn>(func.address() + 0x2D);

		logger::info("patching favorite equip function");
	}
}

namespace DebugBook
{
	struct DisplayBookMenu
	{
		static void thunk(RE::BSString* a_description, RE::ExtraDataList* a_list, RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book, const RE::NiPoint3& a_pos, const RE::NiMatrix3& a_rot, float a_scale, bool a_defaultPos)
		{
			if (a_book) {
				float x, y, z;
				a_rot.ToEulerAnglesXYZ(x, y, z);
				logger::info("{} - pos({}, {}, {}) | rot({} , {} ,{})", a_book->GetName(), a_pos.x, a_pos.y, a_pos.z, x, y, z);
			}

			return func(a_description, a_list, a_ref, a_book, a_pos, a_rot, a_scale, a_defaultPos);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> func{ REL::ID(17437) };
		stl::write_thunk_call<DisplayBookMenu>(func.address() + 0x186);
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

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

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded plugin");

	SKSE::Init(a_skse);

	SKSE::AllocTrampoline(28);

	FavoriteMiscItem::Install();
	FavoriteBook::Install();
	DebugBook::Install();

	return true;
}
