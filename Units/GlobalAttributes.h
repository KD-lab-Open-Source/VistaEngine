#ifndef __GLOBAL_ATTRIBUTES_H__
#define __GLOBAL_ATTRIBUTES_H__
#include "XTL\Rect.h"
#include "Serialization\LibraryWrapper.h"
#include "Serialization\ResourceSelector.h"
#include "Parameters.h"
#include "CircleManagerParam.h"
#include "Units\AbnormalStateAttribute.h"
#include "TriggerChainName.h"
#include "Units\DirectControlMode.h"
#include "UserInterface\UI_Sprite.h"

struct CameraRestriction
{
	//горизонтальное перемещение камеры
	float CAMERA_SCROLL_SPEED_DELTA;
	float CAMERA_BORDER_SCROLL_SPEED_DELTA;
	float CAMERA_SCROLL_SPEED_DAMP;

	float CAMERA_BORDER_SCROLL_AREA_UP;
	float CAMERA_BORDER_SCROLL_AREA_DN;
	float CAMERA_BORDER_SCROLL_AREA_HORZ;

	//вращение и наклон
	float CAMERA_KBD_ANGLE_SPEED_DELTA;
	float CAMERA_MOUSE_ANGLE_SPEED_DELTA;
	float CAMERA_ANGLE_SPEED_DAMP;

	//zoom
	float zoomKeyAcceleration;
	float zoomWheelImpulse;
	float zoomDamping;

	float CAMERA_FOLLOW_AVERAGE_TAU;

	//ограничения
	float heightMax;
	float heightMin;
	float CAMERA_MOVE_ZOOM_SCALE;

	float zoomMin;
	float zoomMax;
	float zoomDefault;
	float zoomMaxTheta;
	float thetaMinHigh;
	float thetaMinLow;
	float thetaMaxHigh;
	float thetaMaxLow;
	float thetaDefault;

	float CAMERA_WORLD_SCROLL_BORDER;

	// прямое управление.
	float unitFollowDistance;
	float unitFollowTheta;
	float unitHumanFollowDistance;
	float unitHumanFollowTheta;

	float directControlThetaFactor;
	float directControlPsiMax;
	float directControlRelaxation;

	bool aboveWater;

	CameraRestriction();

	void serialize(Archive& ar);
};

struct CameraBorder{
	float CAMERA_WORLD_BORDER_TOP;
	float CAMERA_WORLD_BORDER_BOTTOM;
	float CAMERA_WORLD_BORDER_LEFT;
	float CAMERA_WORLD_BORDER_RIGHT;

	CameraBorder();
	void serialize(Archive& ar);

	void setRect(const Recti& rect);
	Recti rect() const;
	void clampRect();
};

//---------------------------------------------------------------------------------------

struct ShowHeadName
{
	ShowHeadName(const char* nameIn = "") : fileName_(nameIn) {}
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
	operator const char* () const { return fileName_.c_str(); }

	std::string fileName_;
};
typedef vector<ShowHeadName> ShowHeadNames;

//---------------------------------------------------------------------------------------

enum ObjectLodPredefinedType
{
	OBJECT_LOD_VERY_SMALL=0,
	OBJECT_LOD_SMALL,
	OBJECT_LOD_NORMAL,
	OBJECT_LOD_BIG,
	OBJECT_LOD_VERY_BIG,
	OBJECT_LOD_SIZE,
	OBJECT_LOD_DEFAULT
};

//---------------------------------------------------------------------------------------

struct MapSizeName
{
	MapSizeName() : size(0.f) {}

	float size;
	LocString name;

	bool operator< (const MapSizeName& right) const { return size < right.size; }

	void serialize(Archive& ar);
};

typedef vector<MapSizeName> MapSizeNames;

// --------------------------------------------------------------------------

struct LanguageCombo : public ComboListString
{
	LanguageCombo(const char* lang = "");
	bool serialize(Archive& ar, const char* name, const char* nameAlt);

	bool valid() const;
};
typedef vector<LanguageCombo> AvaiableLanguages;

//---------------------------------------------------------------------------------------

struct GlobalAttributes : public LibraryWrapper<GlobalAttributes>
{
	string windowTitle;
	string savePath;
	string icon;
	AvaiableLanguages languagesList;//список экспортируемых/импортируемых языков в Excel

	MapSizeNames mapSizeNames;

	string startScreenPicture_;
	
	bool enableSilhouettes;
	bool terrainSelfShadowing;
	bool enableFieldDispatcher;

	struct LodBorder
	{
		float lod12,lod23;/// переход от lod1 к lod2, переход от lod2 к lod3
		float radius;
		float hideDistance;
		void serialize(Archive& ar);
	};

	LodBorder lod_border[OBJECT_LOD_SIZE];

	bool enablePathTrackingRadiusCheck;
	bool moveToImpassabilities;
	float minRadiusCheck;
	bool enemyRadiusCheck;
	bool enableEnemyMakeWay;
	bool enableMakeWay;
	bool enableFogOfWar;
	float pathTrackingStopRadius;
	bool checkImpassabilityInDC;

	bool enableUnitVisibilityTrace;

	bool enableAutoImpassability;

	bool uniformCursor;
	bool useStackSelect;
	bool serverCanChangeClientOptions;

	bool drawUnitSideSpritesAfterInterface;

	float analyzeAreaRadius;

	float assemblyCommandShowTime;

	ShowHeadNames showHeadNames;
	
	float debrisLyingTime;
	float debrisProjectileLyingTime;
	int treeLyingTime;
    float FallTreeTime;
	vector<Color4f> playerColors;
	vector<UnitColor> underwaterColors;
	vector<Color4f> silhouetteColors;
	struct Sign{
		string unitTexture;
		UI_Sprite sprite;
		void serialize(Archive& ar);
	};
	typedef vector<Sign> Signs;
	Signs playerSigns;

	CameraRestriction cameraRestriction;
	float cameraDefaultTheta_;
	float cameraDefaultDistance_;

	void setCameraCoordinate();

	ParameterCustom resourseStatisticsFactors;
	TriggerChainNames worldTriggers;
	Difficulty assistantDifficulty;
	bool applyArithmeticsOnCreation;
    
	int version;

	bool enableAnimationInterpolation;

	int animationInterpolationTime;

	DirectControlMode directControlMode;

	CircleManagerDrawOrder circleManagerDrawOrder;

	int cheatTypeSpeed;

	ParameterCustom profileParameters;
	
	float opacityMin;
	float opacitySpeed;
	float opacityRestoreSpeed;
	float opacityBoundFactorXY;
	float opacityBoundFactorZ;

	float frameTimeAvrTau;

	float unitSpriteMaxDistance;

	int installerOffset;

	GlobalAttributes();

	int playerAllowedColorSize() const { return playerColors.size() - 1; }
	int playerAllowedUnderwaterColorSize() const { return underwaterColors.size() - 1; }
	int playerAllowedSignSize() const { return playerSigns.size() - 1; }
	int playerAllowedSilhouetteSize() const { return silhouetteColors.size() - 1; }

	void serializeHeadLibrary(Archive& ar);
	void serializeGameScenario(Archive& ar);
	void serialize(Archive& ar);

private:
};

#endif // __GLOBAL_ATTRIBUTES_H__
