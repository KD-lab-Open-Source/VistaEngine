#ifndef _VISGENERIC_DEFINE_H_
#define _VISGENERIC_DEFINE_H_

enum AttributeCamera
{
	ATTRCAMERA_PERSPECTIVE			=	1<<4,	// перспектива
	ATTRCAMERA_WRITE_ALPHA			=	1<<5, 			
	ATTRCAMERA_ZBUFFER				=	1<<6, 	
	ATTRCAMERA_FLOAT_ZBUFFER		=	1<<7, 	
	ATTRCAMERA_MIRAGE				=   1<<13,
	ATTRCAMERA_REFLECTION			=	1<<16,	// камера рендерит портал-отражение
	ATTRCAMERA_SHADOW				=	1<<17,	// камера рендерит портал-тень
	ATTRCAMERA_CLEARZBUFFER			=	1<<19,
	ATTRCAMERA_SHOWCLIP				=	1<<20,
	ATTRCAMERA_SHADOWMAP			=	1<<21,
	ATTRCAMERA_ZMINMAX				=	1<<22,
	ATTRCAMERA_NOT_CALC_PROJ		=	1<<23,
	ATTRCAMERA_ZINVERT				=	1<<24,
	ATTRCAMERA_NOCLEARTARGET		=	1<<25,
	ATTRCAMERA_NOZWRITE				=	1<<26,
};

enum eAttributeUnkObj
{
// general
	ATTRUNKOBJ_IGNORE				=	1<<0,	// объект игнорируется = является невидимымы = не выводится
	ATTRUNKOBJ_DELETED				=	1<<1,
	ATTRUNKOBJ_ATTACHED				=	1<<2,		
	ATTRUNKOBJ_CREATED_IN_LOGIC		=	1<<3,	//Объекты созданные в логическом потоке должны удаляться после того как на них нет ссылок в интерполяции

	ATTRUNKOBJ_ADDBLEND				=	1<<7,	// сложение цветов, должен совпадать с MAT_ALPHA_ADDBLEND
	ATTRUNKOBJ_COLLISIONTRACE		=	1<<9,	// учитывать при трассировке
	ATTRUNKOBJ_NO_USELOD			=	1<<12,
	ATTRUNKOBJ_MIRAGE				=	ATTRCAMERA_MIRAGE,

	ATTRUNKOBJ_REFLECTION			=	ATTRCAMERA_REFLECTION,//==16 объект может отражаться
	ATTRUNKOBJ_SHADOW				=	ATTRCAMERA_SHADOW,	//==17 объект откидывает правильную тень (Не факт что правильно, см ATTRCAMERA_SHADOWMAP)
	ATTR3DX_NOUPDATEMATRIX			=	1<<18,  //Для 3dx не пересчитывать матрицы
	ATTRUNKOBJ_IGNORE_NORMALCAMERA	=	1<<20,	// объект не выводится в нормальной камере

	//3dx
	ATTR3DX_UNDERWATER				=   1<<22,
	ATTR3DX_NO_RESIZE_TEXTURES		=   1<<23,  // 
	ATTRUNKOBJ_2PASS_ZBUFFER		=   1<<24,  // для полупрозрачных объектов, идет отрисовка сначала в Z буффер без отрисовки в ColorBuffer,
												// а вторым проходом отрисовка в ColorBuffer, без отрисовки в Z буффер
	ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE	=	1<<26,  // Для объектов, которые будут видны, за другими объектами  в виде одноцветно закрашенного силуэта.
												// Выставлять только при включенном RENDERDEVICE_MODE_STENCIL
	ATTRUNKOBJ_HIDE_BY_DISTANCE		=	1<<27,  // Объект перестает быть видимым, когда становится слишком маленьким.
	ATTR3DX_HIDE_LIGHTS				=   1<<28,  // Источники света перестают быть видимыми.
	ATTR3DX_NO_SELFILLUMINATION		=   1<<29,  // Выключается текстура самосвечения (пользоваться функцией EnableSelfIllumination)
	ATTR3DX_ALWAYS_FLAT_SILUETTE	=	1<<30,
	ATTRUNKOBJ_NOLIGHT				=	1<<31,	// объект не освещается источниками света сцены
};

enum eAttributeLight
{
// general
	ATTRLIGHT_IGNORE				=	1<<0,	// источник света игнорируется = является невидимымы = не выводится
// private
	ATTRLIGHT_DIRECTION				=	1<<4,	// напрвленный источник света
	ATTRLIGHT_SPHERICAL_SPRITE		=	1<<5,	// источник света рисуется кучей спрайтов
	//ATTRLIGHT_SPHERICAL_OBJECT и ATTRLIGHT_SPHERICAL_TERRAIN можно комбинировать вместе при создании.
	ATTRLIGHT_SPHERICAL_OBJECT		=	1<<6,	// сферический источник света, отбрасывает свет только(!!!!)на объекты
	ATTRLIGHT_SPHERICAL_TERRAIN		=   1<<7,	// сферический источник света, отбрасывает свет только на землю
};

enum eAttributeSimply
{
	ATTRSIMPLY3DX_OPACITY				=	1<<26,  //Внутренний флаг, не устанавливать.
};

#endif // _VISGENERIC_DEFINE_H_
