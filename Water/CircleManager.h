#ifndef __CIRCLE_MANAGER_H_INCLUDED__
#define __CIRCLE_MANAGER_H_INCLUDED__

#include "OrCircle.h"
typedef int CIRCLE_HANDLE;
#include "..\Units\CircleManagerParam.h"


class CircleManager:public cBaseGraphObject
{
public:
	CircleManager();
	~CircleManager();

	void PreDraw(cCamera *pCamera);
	void Draw(cCamera *pCamera);
	void Animate(float dt);
	virtual const MatXf& GetPosition() const{return MatXf::ID;}

	void addCircle(const Vect2f& pos,float radius);

	void SetParam(const CircleManagerParam& param);
	void SetLegionColor(sColor4c color);
	virtual int GetSpecialSortIndex()const{return 1;}

	void clear();

	CIRCLE_MANAGER_DRAW_ORDER GetDrawOrder(){return draw_order;}
	void SetDrawOrder(CIRCLE_MANAGER_DRAW_ORDER order){draw_order=order;}
protected:
	CircleManagerParam param;
	SAMPLER_DATA sampler_circle;

	sColor4c legionColor;

	OrCircle circles;
	cTexture* pTexture;
	CIRCLE_MANAGER_DRAW_ORDER draw_order;
	void DrawSpline(vector<OrCircleSpline>& spline);
};

#endif
