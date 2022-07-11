#ifndef __WEAPON_ENUMS_H__
#define __WEAPON_ENUMS_H__

/// режимы воздействия оружия
enum AffectMode
{
	/// только на своих юнитов
	AFFECT_OWN_UNITS,
	/// на своих и союзных юнитов
	AFFECT_FRIENDLY_UNITS,
	/// союзных юнитов
	AFFECT_ALLIED_UNITS,
	/// на вражеских юнитов
	AFFECT_ENEMY_UNITS,
	/// на всех юнитов
	AFFECT_ALL_UNITS,
	/// не воздействует на юниты
	AFFECT_NONE_UNITS
};

/// Режимы стрельбы.
enum WeaponMode
{
	LONG_RANGE,		///< стрелять оружием дальнего боя
	SHORT_RANGE,	///< стрелять оружием ближнего боя
	ANY_RANGE,		///< стрелять любым оружием
};

/// Режим работы оружия в прямом управлении.
enum WeaponDirectControlMode
{
	WEAPON_DIRECT_CONTROL_DISABLE,		///< не стрелять вообще
	WEAPON_DIRECT_CONTROL_NORMAL,		///< стрелять куда указано
	WEAPON_DIRECT_CONTROL_ALTERNATE,	///< стрелять по правому клику
	WEAPON_DIRECT_CONTROL_AUTO			///< автоматически выбирать цель
};

/// Режим работы оружия в прямом управлении.
enum WeaponSyndicateControlMode
{
	WEAPON_SYNDICATE_CONTROL_DISABLE,		///< не стрелять вообще
	WEAPON_SYNDICATE_CONTROL_NORMAL,		///< стрелять куда указано
	WEAPON_SYNDICATE_CONTROL_FORCE,			///< стрелять по клику куда нацелено
	WEAPON_SYNDICATE_CONTROL_AUTO			///< автоматически выбирать цель
};

/// Требуемая оружием от юнита визуализация.
enum WeaponAnimationMode
{
	/// анимация не нужна
	WEAPON_ANIMATION_NONE,
	/// анимация перезарядки
	WEAPON_ANIMATION_RELOAD,
	/// анимация прицеливания
	WEAPON_ANIMATION_AIM,
	/// анимация стрельбы
	WEAPON_ANIMATION_FIRE
};

/// Режим создания источников от оружия/снарядов.
enum WeaponSourcesCreationMode
{
	WEAPON_SOURCES_CREATE_ON_TARGET_HIT,	///< при попадании в цель
	WEAPON_SOURCES_CREATE_ON_GROUND_HIT,	///< при попадании в землю
	WEAPON_SOURCES_CREATE_ALWAYS			///< всегда
};

#endif /* __WEAPON_ENUMS_H__ */
