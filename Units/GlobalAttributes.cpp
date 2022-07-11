#include "StdAfx.h"
#include "GlobalAttributes.h"
#include "..\Terra\vmap.h"
#include "CameraManager.h"

WRAP_LIBRARY(GlobalAttributes, "GlobalAttributes", "Глобальные параметры", "Scripts\\Content\\GlobalAttributes", 0, false);

BEGIN_ENUM_DESCRIPTOR(ObjectLodPredefinedType, "ObjectLodPredefinedType")
REGISTER_ENUM(OBJECT_LOD_DEFAULT, "LOD: По умолчанию")
REGISTER_ENUM(OBJECT_LOD_VERY_SMALL, "LOD: Очень маленький объект")
REGISTER_ENUM(OBJECT_LOD_SMALL, "LOD: Маленький объект")
REGISTER_ENUM(OBJECT_LOD_NORMAL, "LOD: Средний объект")
REGISTER_ENUM(OBJECT_LOD_BIG, "LOD: Большой объект")
REGISTER_ENUM(OBJECT_LOD_VERY_BIG, "LOD: Очень большой объект")
END_ENUM_DESCRIPTOR(ObjectLodPredefinedType)	


CameraBorder::CameraBorder()
{
	CAMERA_WORLD_BORDER_TOP = 0.f;
	CAMERA_WORLD_BORDER_LEFT = 0.f;
	CAMERA_WORLD_BORDER_RIGHT = 0.f;
	CAMERA_WORLD_BORDER_BOTTOM = 0.f;
}

void CameraBorder::serialize(Archive& ar)
{
	ar.serialize(CAMERA_WORLD_BORDER_TOP,    "CAMERA_WORLD_BORDER_TOP",    "сверху");
	ar.serialize(CAMERA_WORLD_BORDER_BOTTOM, "CAMERA_WORLD_BORDER_BOTTOM", "снизу");
	ar.serialize(CAMERA_WORLD_BORDER_LEFT,   "CAMERA_WORLD_BORDER_LEFT",   "слева");
	ar.serialize(CAMERA_WORLD_BORDER_RIGHT,  "CAMERA_WORLD_BORDER_RIGHT",  "справа");
	clampRect();
}


CameraRestriction::CameraRestriction(){
	//горизонтальное перемещение камеры
	CAMERA_SCROLL_SPEED_DELTA = 10.0f;
	CAMERA_BORDER_SCROLL_SPEED_DELTA = 10.0f;
	CAMERA_SCROLL_SPEED_DAMP = 0.7f;

	CAMERA_BORDER_SCROLL_AREA_UP = 0.008f;
	CAMERA_BORDER_SCROLL_AREA_DN = 0.014f;
	CAMERA_BORDER_SCROLL_AREA_HORZ = 0.008f;

	//вращение и наклон
	CAMERA_KBD_ANGLE_SPEED_DELTA = M_PI/30.f;
	CAMERA_MOUSE_ANGLE_SPEED_DELTA = M_PI;
	CAMERA_ANGLE_SPEED_DAMP = 0.7f;

	//zoom
	CAMERA_ZOOM_SPEED_DELTA = 20.0f;
	CAMERA_ZOOM_MOUSE_MULT = 600.0f;
	CAMERA_ZOOM_SPEED_DAMP = 0.7f;

	CAMERA_FOLLOW_AVERAGE_TAU = 0.1f;

	//ограничения
	CAMERA_MOVE_ZOOM_SCALE = 500.0f;

	CAMERA_ZOOM_MAX = 1000.0f;
	CAMERA_ZOOM_MIN = 300.0f;

	CAMERA_MIN_HEIGHT = 0.0f;
	CAMERA_MAX_HEIGHT = 1000.0f;

	CAMERA_ZOOM_DEFAULT = 300.0f;
    CAMERA_THETA_MAX = M_PI/3.f;
	CAMERA_THETA_MIN = M_PI/10.f;
	CAMERA_THETA_DEFAULT = M_PI/6.f;

	CAMERA_WORLD_SCROLL_BORDER = 100.0f;

	unitFollowDistance = 200.0f;
	unitFollowTheta = 1.3f;
	unitHumanFollowDistance = 140.0f;
	unitHumanFollowTheta = 1.42f;

	directControlThetaFactor = 0.5f;
	directControlPsiMax = M_PI / 4.0f;
	directControlRelaxation = 0.4f;
}

void CameraBorder::clampRect()
{
	CAMERA_WORLD_BORDER_TOP = clamp(CAMERA_WORLD_BORDER_TOP, -1000.f, 8192.f);
	CAMERA_WORLD_BORDER_BOTTOM = clamp(CAMERA_WORLD_BORDER_BOTTOM, -1000.f, 8192.f);
	CAMERA_WORLD_BORDER_LEFT = clamp(CAMERA_WORLD_BORDER_LEFT, -1000.f, 8192.f);
	CAMERA_WORLD_BORDER_RIGHT = clamp(CAMERA_WORLD_BORDER_RIGHT, -1000.f, 8192.f);
}

void CameraRestriction::serialize(Archive &ar)
{
	ar.serialize(CAMERA_SCROLL_SPEED_DELTA, "CAMERA_SCROLL_SPEED_DELTA", "скорость сдвига камеры");
	CAMERA_SCROLL_SPEED_DELTA = clamp(CAMERA_SCROLL_SPEED_DELTA, 0.f, 100.f);
	
	ar.serialize(CAMERA_BORDER_SCROLL_SPEED_DELTA, "CAMERA_BORDER_SCROLL_SPEED_DELTA", "скорость сдвига камеры при смещении указателя за край");
	CAMERA_BORDER_SCROLL_SPEED_DELTA = clamp(CAMERA_BORDER_SCROLL_SPEED_DELTA, 0.f, 100.f);
	
	float tmp = 1 / CAMERA_SCROLL_SPEED_DAMP;
	ar.serialize(tmp, "CAMERA_SCROLL_SPEED_DAMP", "инерция сдвига камеры");
	if(tmp >= 1.0f) CAMERA_SCROLL_SPEED_DAMP = 1 / tmp;
	CAMERA_SCROLL_SPEED_DAMP = clamp(CAMERA_SCROLL_SPEED_DAMP, 0.1f, 10.f);

	tmp = CAMERA_BORDER_SCROLL_AREA_UP * 100.f;
	ar.serialize(tmp, "CAMERA_BORDER_SCROLL_AREA_UP", "бордюр сверху для скрола мышью (%)");
	CAMERA_BORDER_SCROLL_AREA_UP = clamp(tmp, 0.f, 20.f) / 100.f;

	tmp = CAMERA_BORDER_SCROLL_AREA_DN * 100.f;
	ar.serialize(tmp, "CAMERA_BORDER_SCROLL_AREA_DN", "бордюр снизу для скрола мышью (%)");
	CAMERA_BORDER_SCROLL_AREA_DN = clamp(tmp, 0.f, 20.f) / 100.f;

	tmp = CAMERA_BORDER_SCROLL_AREA_HORZ * 100.f;
	ar.serialize(tmp, "CAMERA_BORDER_SCROLL_AREA_HORZ", "бордюр по бокам для скрола мышью (%)");
	CAMERA_BORDER_SCROLL_AREA_HORZ = clamp(tmp, 0.f, 20.f) / 100.f;

	tmp = CAMERA_KBD_ANGLE_SPEED_DELTA / M_PI * 180.f;
	ar.serialize(tmp, "CAMERA_KBD_ANGLE_SPEED_DELTA", "скорость поворота камеры клавиатурой");
	CAMERA_KBD_ANGLE_SPEED_DELTA = clamp(tmp, 0.f, 280.f) / 180.f * M_PI;

	tmp = CAMERA_MOUSE_ANGLE_SPEED_DELTA / M_PI * 180.f;
	ar.serialize(tmp, "CAMERA_MOUSE_ANGLE_SPEED_DELTA", "скорость поворота камеры мышью");
	CAMERA_MOUSE_ANGLE_SPEED_DELTA = clamp(tmp, 0.f, 280.f) / 180.f * M_PI;

	tmp = 1 / CAMERA_ANGLE_SPEED_DAMP;
	ar.serialize(tmp, "CAMERA_ANGLE_SPEED_DAMP", "инерция камеры при повороте");
	if(tmp >= 1.0f) CAMERA_ANGLE_SPEED_DAMP = 1 / tmp;
	CAMERA_ANGLE_SPEED_DAMP = clamp(CAMERA_ANGLE_SPEED_DAMP, 0.1f, 10.f);

	ar.serialize(CAMERA_ZOOM_SPEED_DELTA, "CAMERA_ZOOM_SPEED_DELTA", "скорость приближения камеры (зума)");
	CAMERA_ZOOM_SPEED_DELTA = clamp(CAMERA_ZOOM_SPEED_DELTA, 0.f, 100.f);

	ar.serialize(CAMERA_ZOOM_MOUSE_MULT, "CAMERA_ZOOM_MOUSE_MULT", "чувствительность зума мышью");
	CAMERA_ZOOM_MOUSE_MULT = clamp(CAMERA_ZOOM_MOUSE_MULT, 0.f, 5000.f);

	tmp = 1 / CAMERA_ZOOM_SPEED_DAMP;
	ar.serialize(tmp, "CAMERA_ZOOM_SPEED_DAMP", "инерция зума");
	if(tmp >= 1.0f) CAMERA_ZOOM_SPEED_DAMP = 1 / tmp;
	CAMERA_ZOOM_SPEED_DAMP = clamp(CAMERA_ZOOM_SPEED_DAMP, 0.1f, 10.f);

	ar.serialize(CAMERA_FOLLOW_AVERAGE_TAU, "CAMERA_FOLLOW_AVERAGE_TAU", "жесткость привязки к юниту");
	CAMERA_FOLLOW_AVERAGE_TAU = clamp(CAMERA_FOLLOW_AVERAGE_TAU, 0.0001f, 0.01f);

	ar.serialize(CAMERA_MOVE_ZOOM_SCALE, "CAMERA_MOVE_ZOOM_SCALE", "масштабирование движения");
	CAMERA_MOVE_ZOOM_SCALE = clamp(CAMERA_MOVE_ZOOM_SCALE, 0.f, 5000.f);

	ar.serialize(CAMERA_ZOOM_MAX, "CAMERA_ZOOM_MAX", "максимальное удаление от точки наблюдения");
	CAMERA_ZOOM_MAX = clamp(CAMERA_ZOOM_MAX, 0.f, 5000.f);

	ar.serialize(CAMERA_ZOOM_MIN, "CAMERA_ZOOM_MIN", "минимальное удаление от точки наблюдения");
	CAMERA_ZOOM_MIN = clamp(CAMERA_ZOOM_MIN, 0.f, 0.95f * CAMERA_ZOOM_MAX);

	ar.serialize(CAMERA_MAX_HEIGHT, "CAMERA_MAX_HEIGHT", "максимальная высота над миром");
	CAMERA_MAX_HEIGHT = clamp(CAMERA_MAX_HEIGHT, 0.f, 5000.f);

	ar.serialize(CAMERA_MIN_HEIGHT, "CAMERA_MIN_HEIGHT", "минимальная высота над миром");
	CAMERA_MIN_HEIGHT = clamp(CAMERA_MIN_HEIGHT, 0.f, 5000.f);

	ar.serialize(CAMERA_ZOOM_DEFAULT, "CAMERA_ZOOM_DEFAULT", "удаление от точки наблюдения по умолчанию");
	CAMERA_ZOOM_DEFAULT = clamp(CAMERA_ZOOM_DEFAULT, CAMERA_ZOOM_MIN, CAMERA_ZOOM_MAX);

	tmp = CAMERA_THETA_MAX / M_PI * 180.f;
	ar.serialize(tmp, "CAMERA_THETA_MAX", "максимальный угол наклона камеры");
	CAMERA_THETA_MAX = clamp(tmp, 0.f, 85.f) / 180.f * M_PI;

	tmp = CAMERA_THETA_MIN / M_PI * 180.f;
	ar.serialize(tmp, "CAMERA_THETA_MIN", "минимальный угол наклона камеры");
	CAMERA_THETA_MIN = clamp(tmp / 180.f * M_PI, M_PI/36.f, 0.95f * CAMERA_THETA_MAX) ;

	tmp = CAMERA_THETA_DEFAULT / M_PI * 180.f;
	ar.serialize(tmp, "CAMERA_THETA_DEFAULT", "угол наклона камеры по умолчанию");
	CAMERA_THETA_DEFAULT = clamp(tmp / 180.f * M_PI, CAMERA_THETA_MIN, CAMERA_THETA_MAX) ;

	ar.serialize(CAMERA_WORLD_SCROLL_BORDER, "CAMERA_WORLD_SCROLL_BORDER", "ограничение выезда точки наблюдения за край при максимальном удалении");
	CAMERA_WORLD_SCROLL_BORDER = clamp(CAMERA_WORLD_SCROLL_BORDER, -50.f, 1000.f);
	
	ar.serialize(unitFollowDistance, "unitFollowDistance", 0);
	ar.serialize(unitFollowTheta, "unitFollowTheta", 0);
	ar.serialize(unitHumanFollowDistance, "unitHumanFollowDistance", 0);
	ar.serialize(unitHumanFollowTheta, "unitHumanFollowTheta", 0);

	ar.serialize(directControlThetaFactor, "directControlThetaFactor", "Коэффициент для скорости поворота камеры в прямом управление");
	tmp = directControlPsiMax / M_PI * 180.f;
	ar.serialize(tmp, "directControlPsiMax", "Ограничение камеры в прямом управлении");
	directControlPsiMax = clamp(tmp, 5.0f, 120.0f) / 180.f * M_PI;
	ar.serialize(directControlRelaxation, "directControlRelaxation", "Жесткость камеры в прямом управлении");
}

void CameraBorder::setRect(const Recti& rect)
{
	CAMERA_WORLD_BORDER_LEFT	= rect.left();
	CAMERA_WORLD_BORDER_TOP		= rect.top();
	CAMERA_WORLD_BORDER_RIGHT	= static_cast<int>(vMap.H_SIZE) - rect.right();
	CAMERA_WORLD_BORDER_BOTTOM	= static_cast<int>(vMap.V_SIZE) - rect.bottom();

	clampRect();
}

Recti CameraBorder::rect() const
{
	Recti result(CAMERA_WORLD_BORDER_LEFT, CAMERA_WORLD_BORDER_TOP,
				 vMap.H_SIZE - CAMERA_WORLD_BORDER_RIGHT - CAMERA_WORLD_BORDER_LEFT,
				 vMap.V_SIZE - CAMERA_WORLD_BORDER_BOTTOM - CAMERA_WORLD_BORDER_TOP);
	return result;
}

//---------------------------------------------------------------------------------------

Language::Language()
{
	codePage = 1250;
}

void Language::serialize(Archive& ar) {
	ar.serialize(language, "language", "Язык");
	ar.serialize(codePage, "codePage", "Кодовая страница");
}

//---------------------------------------------------------------------------------------

bool ShowHeadName::serialize(Archive& ar, const char* name, const char* nameAlt) 
{
	return ar.serialize(ModelSelector (fileName_, ModelSelector::HEAD_OPTIONS), name, nameAlt);
}

//---------------------------------------------------------------------------------------

void MapSizeName::serialize(Archive& ar)
{
	ar.serialize(size, "size", "Максимальная площадь карты в (тыс.ед)^2");
	ar.serialize(name, "name", "name");
}

//---------------------------------------------------------------------------------------

GlobalAttributes::GlobalAttributes() 
{
	enableSilhouettes = true;
	enablePathTrackingRadiusCheck = false;
	checkImpassabilityInDC = false;
	pathTrackingStopRadius = 100.0f;
	minRadiusCheck = 10.0f;
	enemyRadiusCheck = false;
	enableEnemyMakeWay = false;
	enableMakeWay = true;
	enableAutoImpassability = true;
	debrisLyingTime = 10;
	debrisProjectileLyingTime = 10;
	treeLyingTime = 10;
	for(int i = 0; i < 5; i++)
		playerColors.push_back(sColor4f(1,1,1,1));
	for(int i = 0; i < 5; i++)
		underwaterColors.push_back(UnitColor(sColor4c(0, 0, 255, 255), false));
	for(int i = 0; i < 5; i++)
		silhouetteColors.push_back(sColor4f(1,1,1,1));
	analyzeAreaRadius = 15.f;
	uniformCursor = true;
	useStackSelect = false;
	serverCanChangeClientOptions = false;
	version = 0;
	enableAnimationInterpolation = true;
	animationInterpolationTime = 300;
	directControlMode = DIRECT_CONTROL_DISABLED;
	circleManagerDrawOrder=CIRCLE_MANAGER_DRAW_BEFORE_GRASS_NOZ;
	float radius = 12.5f;
	for(int i=0;i<OBJECT_LOD_SIZE;i++)
	{
		lod_border[i].radius = radius;
		radius*=2;
		lod_border[i].lod12=200;
		lod_border[i].lod23=600;
		lod_border[i].hideDistance = 500;
	}

	cameraDefaultDistance_ = 175.0f;
	cameraDefaultTheta_ = 0.8f;

	cheatTypeSpeed = 60;
}

void GlobalAttributes::serializeHeadLibrary (Archive& ar) 
{
	ar.serialize(showHeadNames, "showHeadNames", "Головы"); 
}

void GlobalAttributes::Sign::serialize(Archive& ar)
{
	ar.serialize(unitTexture, "unitTexture", "Текстура на модель");
	ar.serialize(sprite, "sprite", "Спрайт для интрефейса");
}

void GlobalAttributes::LodBorder::serialize(Archive& ar)
{
	ar.serialize(radius,"radius","Радиус объекта");
	ar.serialize(lod12,"lod12","Переход от lod1 к lod2");
	ar.serialize(lod23,"lod23","Переход от lod2 к lod3");
	ar.serialize(hideDistance,"hideDistance","Расстояние исчезновения");
}

void GlobalAttributes::serializeGameScenario(Archive& ar) 
{
	if(!ar.serialize(directControlMode, "directControlMode", "Режим прямого управления по умолчанию")){ // conversion 7.11
		bool enableDirectControl = false;
		ar.serialize(enableDirectControl, "enableDirectControlAtStart", 0);
		if(enableDirectControl)
			directControlMode = DIRECT_CONTROL_ENABLED;
	}

	ar.serialize(ResourceSelector(startScreenPicture_, ResourceSelector::UI_TEXTURE_OPTIONS), "startScreenPicture", "Стартовый банер");
	ar.serialize(enableSilhouettes, "enableSilhouettes", "Включить поддержку силуэтов");

	if(ar.isInput())
	{//Конверсия
		float lod12=200; /// переход от lod1 к lod2
		float lod23=600; /// переход от lod2 к lod3
		if(ar.serialize(lod12, "lod12", "Переход от lod1 к lod2") &&
		 ar.serialize(lod23, "lod23", "Переход от lod2 к lod3"))
		{
			for(int i=0;i<OBJECT_LOD_SIZE;i++)
			{
				lod_border[i].lod12=lod12;
				lod_border[i].lod23=lod23;
			}
		}
	}

	for(int i = 0; i < OBJECT_LOD_SIZE; ++i)
	{
		ar.serialize(lod_border[i], getEnumName(ObjectLodPredefinedType(i)), getEnumNameAlt(ObjectLodPredefinedType(i)));
		if(i>0&&lod_border[i].radius < lod_border[i-1].radius)
		{
			lod_border[i].radius = lod_border[i-1].radius;
			XBuffer buf;
			buf < "Радиус \"" <  getEnumNameAlt(ObjectLodPredefinedType(i)) < "\" должен быть больше радиуса предыдущего лода ";
			xxassert(false,buf);
		}
	}

	ar.openBlock("PathTracking", "PathTracking");
	ar.serialize(enablePathTrackingRadiusCheck, "enablePathTrackingRadiusCheck", "PathTracking:Большые юниты игнорируют меньших");
	ar.serialize(enemyRadiusCheck, "enemyRadiusCheck", "PathTracking:Враги игнорируют меньших");
	ar.serialize(minRadiusCheck, "minRadiusCheck", "PathTracking:Минимальный игнорируемый радиус");
	ar.serialize(enableMakeWay, "enableMakeWay", "PathTracking:Уступать дорогу");
	ar.serialize(enableEnemyMakeWay, "enableEnemyMakeWay", "PathTracking:Уступать дорогу врагу");
	ar.serialize(pathTrackingStopRadius, "pathTrackingStopRadius", "PathTracking:Радиус остановки");
	ar.serialize(analyzeAreaRadius, "analyzeAreaRadius", "Радиус для точного анализа поверхности");
	ar.serialize(checkImpassabilityInDC, "checkImpassabilityInDC", "Учитывать непроходимость в прямом управлении");
	ar.serialize(enableAutoImpassability, "enableAutoImpassability", "Включить автоматическую генерацию зон непроходимости");
	ar.closeBlock();

	ar.serialize(cameraRestriction, "cameraRestriction", "Ограничения камеры");
	if(ar.isInput()) // CONVERSION 26.09.2006
		ar.serialize(cameraBorder, "cameraRestriction", 0);

	ar.serialize(mapSizeNames, "mapSizeNames", "Названия рамеров карт");

	ar.serialize(cameraDefaultTheta_, "cameraDefaultTheta", 0);
	ar.serialize(cameraDefaultDistance_, "cameraDefaultDistance", 0);
	ar.serialize(environmentAttributes_, "environmentColors", "Общие пареметры миров");

	ar.serialize(playerColors,     "playerColors",     "Цвета игроков"); 
	ar.serialize(playerSigns, "playerSignes", "Эмблемы игроков");
	ar.serialize(silhouetteColors, "silhouetteColors", "Цвета силуэтов"); 
	ar.serialize(serverCanChangeClientOptions, "serverCanChangeClientOptions", "Сервер сетевой игры может менять настройки клиентов");

	ar.serialize(underwaterColors, "underwaterUnitColors", "Цвета игроков под водой");

	ar.serialize(uniformCursor,	"uniformCursor",	"Курсоры действий показывать если все в селекте могут");
	ar.serialize(useStackSelect, "useStackSelect", "Использовать стековый селект");
	ar.serialize(languagesList, "languagesList", "Доступные языки");

	ar.serialize(resourseStatisticsFactors, "resourseStatisticsFactors", "Коэффициенты для статистики ресурсов (умножаются на соответствующие параметры и складываются)");
	ar.serialize(worldTriggers, "worldTriggers", "Триггера для мира");
	ar.serialize(assistantDifficulty, "assistantDifficulty", "Сложность АИ ассистента");

	ar.serialize(debrisLyingTime, "debrisLyingTime", "Время, которое осколки лежат после взрыва");
	ar.serialize(debrisProjectileLyingTime, "debrisProjectileLyingTime", "Время, которое осколки снарядов лежат после взрыва");
	ar.serialize(treeLyingTime, "treeLyingTime", "Время, которое деревья лежат после падения");

	ar.serialize(enableAnimationInterpolation, "enableAnimationInterpolation", "Разрешить интерполяцию анимации");
	if(enableAnimationInterpolation)
		ar.serialize(animationInterpolationTime, "animationInterpolationTime", "Время интерполяции анимации");

	if(ar.isInput()){
		vector<sColor4f>::iterator i;
		FOR_EACH(playerColors, i)
			i->a = 1;
	}

	ar.serialize(circleManagerDrawOrder, "circleManagerDrawOrder", "Момент отрисовки селектов юнитов");
	ar.serialize(cheatTypeSpeed, "cheatTypeSpeed", "Минимальная скорость ввода читов, знаков в минуту");
}

void GlobalAttributes::serialize(Archive& ar) 
{
	serializeHeadLibrary (ar);
	serializeGameScenario(ar);

	ar.serialize(version, "version", 0);
}

void GlobalAttributes::setCameraCoordinate()
{
	if(cameraManager) {
		cameraDefaultDistance_ = cameraManager->coordinate().distance();
		cameraDefaultTheta_ = cameraManager->coordinate().theta();
	}
}


