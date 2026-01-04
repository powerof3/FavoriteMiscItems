#include "Hooks.h"

namespace FavoriteMiscItems
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

		static inline constexpr std::size_t size =
#ifndef SKYRIMVR
			0x6F;
#else
			0x60;
#endif
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> func{
#ifndef SKYRIMVR
			RELOCATION_ID(50944, 51821)
#else
			REL::Offset(0x08B8EF0)
#endif
		};
		stl::asm_replace<CanBeFavorited>(func.address());

		logger::info("patching FavoritesManager::CanBeFavorited");
	}
}

namespace FavoriteBooks
{
#ifdef SKYRIMVR
	struct detail
	{
		static void open_book_menu(const RE::BSString& a_description, const RE::ExtraDataList* a_extraList, RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book, const RE::NiPoint3& a_pos, const RE::NiMatrix3& a_rot, float a_scale, bool a_defaultPos, RE::NiAVObject* a_ref3D)
		{
			using func_t = decltype(&open_book_menu);
			static REL::Relocation<func_t> func{ RELOCATION_ID(50122, 51053) };
			return func(a_description, a_extraList, a_ref, a_book, a_pos, a_rot, a_scale, a_defaultPos, a_ref3D);
		}
	};
#endif

	struct ToggleEquipItem
	{
		static void thunk(RE::ActorEquipManager* a_equipMgr, RE::Actor* a_actor, RE::InventoryEntryData* a_entryData, RE::BGSEquipSlot* a_equipSlot, bool a_unk05)
		{
			func(a_equipMgr, a_actor, a_entryData, a_equipSlot, a_unk05);

			const auto object = a_entryData->object;
			if (const auto book = object ? object->As<RE::TESObjectBOOK>() : nullptr) {
				if (book->TeachesSpell()) {
					if (a_actor && book->Read(a_actor)) {
						a_actor->RemoveItem(book, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
					}
					return;
				}
				if (const auto UIMsgQueue = RE::UIMessageQueue::GetSingleton()) {
					UIMsgQueue->AddMessage(RE::FavoritesMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

					const RE::ExtraDataList* list = a_entryData->extraLists ? a_entryData->extraLists->front() : nullptr;

					RE::NiMatrix3 rot{};
					rot.SetEulerAnglesXYZ(-0.05f, -0.05f, 1.50f);

#ifndef SKYRIMVR
					RE::BookMenu::OpenMenuFromBaseForm(book, list, RE::NiPoint3(), rot, 1.0f, true);
#else
					RE::BSString str;
					book->GetDescription(str, nullptr);
					if (!str.empty()) {
						detail::open_book_menu(str, list, nullptr, book, RE::NiPoint3(), rot, 1.0f, true, a_actor->Get3D2());
					}
					str.clear();
#endif
				}
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{
#ifndef SKYRIMVR
			RELOCATION_ID(50654, 51548), OFFSET(0xC4, 0xC2)
#else
			REL::Offset(0x08A5110 + 0xC4),
#endif
		};  //FavoritesMenu::UseQuickslotItem

		stl::write_thunk_call<ToggleEquipItem>(target.address());

		logger::info("patching FavoritesMenu::UseQuickslotItem");
	}
}
