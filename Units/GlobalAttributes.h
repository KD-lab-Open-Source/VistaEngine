#ifndef __GLOBAL_ATTRIBUTES_H__
#define __GLOBAL_ATTRIBUTES_H__
#include "Rect.h"
#include "LibraryWrapper.h"
#include "ResourceSelector.h"
#include "Parameters.h"
#include "..\Environment\EnvironmentColors.h"
#include "CircleManagerParam.h"
#include "..\Units\AbnormalStateAttribute.h"
#include "TriggerChainName.h"
#include "..\Units\DirectControlMode.h"

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
	float CAMERA_ZOOM_SPEED_DELTA;
	float CAMERA_ZOOM_MOUSE_MULT;
	float CAMERA_ZOOM_SPEED_DAMP;

	float CAMERA_FOLLOW_AVERAGE_TAU;

	//ограничения
	float CAMERA_MAX_HEIGHT;
	float CAMERA_MIN_HEIGHT;
	float CAMERA_MOVE_ZOOM_SCALE;

	float CAMERA_ZOOM_MIN;
	float CAMERA_ZOOM_MAX;
	float CAMERA_ZOOM_DEFAULT;
	float CAMERA_THETA_MIN;
	float CAMERA_THETA_MAX;
	float CAMERA_THETA_DEFAULT;

	float CAMERA_WORLD_SCROLL_BORDER;

	// прямое управление.
	float unitFollowDistance;
	float unitFollowTheta;
	float unitHumanFollowDistance;
	float unitHumanFollowTheta;

	float directControlThetaFactor;
	float directControlPsiMax;
	float directControlRelaxation;

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

struct Language
{
	string language;
	int codePage;

	Language();
	void serialize(Archive& ar);
};
typedef vector<Language> LanguagesList;

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

typedef ResourceSelectorTypified<&ResourceSelector::TEXTURE_OPTIONS> Texture;

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

//---------------------------------------------------------------------------------------

struct GlobalAttributes : public LibraryWrapper<GlobalAttributes>
{
	LanguagesList languagesList;//список экспортируемых/импортируемых языков в Excel

	MapSizeNames mapSizeNames;

	string startScreenPicture_;
	
	bool enableSilhouettes;

	struct LodBorder
	{
		float lod12,lod23;/// переход от lod1 к lod2, переход от lod2 к lod3
		float radius;
		float hideDistance;
		void serialize(Archive& ar);
	};

	LodBorder lod_border[OBJECT_LOD_SIZE];

	bool enablePathTrackingRadiusCheck;
	float minRadiusCheck;
	bool enemyRadiusCheck;
	bool enableEnemyMakeWay;
	bool enableMakeWay;
	float pathTrackingStopRadius;
	bool checkImpassabilityInDC;

	bool enableAutoImpassability;

	bool uniformCursor;
	bool useStackSelect;
	bool serverCanChangeClientOptions;

	float analyzeAreaRadius;

	ShowHeadNames showHeadNames;
	
	float debrisLyingTime;
	float debrisProjectileLyingTime;
	float treeLyingTime;
    float FallTreeTime;
	vector<sColor4f> playerColors;
	vector<UnitColor> underwaterColors;
	vector<sColor4f> silhouetteColors;
	struct Sign{
		Texture unitTexture;
		UI_Sprite sprite;
		void serialize(Archive& ar);
	};
	typedef vector<Sign> Signs;
	Signs playerSigns;

	CameraRestriction cameraRestriction;
	CameraBorder cameraBorder; // CONVERSION 26.09.2006
	float cameraDefaultTheta_;
	float cameraDefaultDistance_;

	void setCameraCoordinate();

	ParameterCustom resourseStatisticsFactors;
	TriggerChainNames worldTriggers;
	Difficulty assistantDifficulty;
    
	int version;

	bool enableAnimationInterpolation;

	int animationInterpolationTime;

	DirectControlMode directControlMode;

	EnvironmentAttributes environmentAttributes_;
	CIRCLE_MANAGER_DRAW_ORDER circleManagerDrawOrder;

	int cheatTypeSpeed;

	ParameterCustom profileParameters;

	GlobalAttributes();

	int playerAllowedColorSize() const { return playerColors.size() - 1; }
	int playerAllowedUnderwaterColorSize() const { return underwaterColors.size() - 1; }
	int playerAllowedSignSize() const { return playerSigns.size() - 1; }
	int playerAllowedSilhouetteSize() const { return silhouetteColors.size() - 1; }

	void serializeHeadLibrary(Archive& ar);
	void serializeGameScenario(Archive& ar);
	void serialize(Archive& ar);
};

#endif // __GLOBAL_ATTRIBUTES_H__
