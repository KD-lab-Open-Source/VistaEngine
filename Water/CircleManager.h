#ifndef __CIRCLE_MANAGER_H_INCLUDED__
#define __CIRCLE_MANAGER_H_INCLUDED__

#include "MTSection.h"
#include "OrCircle.h"
#include "Units\CircleManagerParam.h"
#include "XTL\StaticMap.h"
#include "Render\inc\IVisGenericInternal.h"
#include "Render\D3D\RenderStates.h"


class CircleManager : public BaseGraphObject
{
public:
	CircleManager();
	~CircleManager();

	void PreDraw(Camera* camera);
	void Draw(Camera* camera);

	void addCircle(const Vect2f& pos, float radius, const CircleManagerParam& param); // Выбор слоя идет по адресу параметра, т.е. сливаются круги одного цвета

	void SetLegionColor(Color4c color);
	int sortIndex()const{return 1;}

	void clear();
	void clearLayers(); // Чистит круги в слоях, но сами слои не чистит

	CircleManagerDrawOrder GetDrawOrder(){return drawOrder_;}
	void SetDrawOrder(CircleManagerDrawOrder order){drawOrder_=order;}

protected:
	SAMPLER_DATA samplerCircle_;

	CircleManagerDrawOrder drawOrder_;
	Color4c legionColor_;

	struct Layer {
		ShareHandle<OrCircle> circles;
		UnknownHandle<cTexture> texture;
		CircleManagerParam param;

		Layer(const CircleManagerParam& param);
		void draw();
		void drawSpline(OrCircleSpline& spline);
	};
	typedef StaticMap<DWORD, Layer> Layers;
	Layers layers_;

	static CircleManagerDrawOrder currentDrawOrder_;
	static Color4c currentLegionColor_;
	static MTSection lock_;
};

#endif
