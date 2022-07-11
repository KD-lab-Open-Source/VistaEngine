#include "stdafxTr.h"
#include "TerToolCtrl.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\Serialization.h"
#include "terTools.h"

Toolser::Toolser()
{
	scale_ = 1.f;
	//vscale_ = 2.f;
}

void Toolser::serialize(Archive& ar)
{
	ar.serialize(controllers_, "controllers", "Обработчики тулзеров");
	ar.serialize(RangedWrapperf(scale_, 0.5f, 5.f), "scale", "Общий маштаб");

	//bool vscale = false;
	//if(ar.isEdit()){
	//	TerToolCtrls::const_iterator it;
	//	FOR_EACH(controllers_, it)
	//		if(!it->isEmpty() && it->terToolReference->terTool->isVerticalScalable()){
	//			vscale = true;
	//			break;
	//		}
	//}
	//else
	//	vscale = true;
	//if(vscale)
	//	ar.serialize(RangedWrapperf(vscale_, 0.2f, 5.f), "vscale", "Маштаб глубины");
}

void Toolser::start(const Se3f& pos)
{
	TerToolCtrls::iterator it;
	FOR_EACH(controllers_, it)
		it->start(pos, scale_);
}

void Toolser::setPosition(const Se3f& pos)
{
	TerToolCtrls::iterator it;
	FOR_EACH(controllers_, it)
		it->setPosition(pos);
}

void Toolser::stop()
{
	TerToolCtrls::iterator it;
	FOR_EACH(controllers_, it)
		it->stop();
}

bool Toolser::isFinished() const
{
	TerToolCtrls::const_iterator it;
	FOR_EACH(controllers_, it)
		if(!it->isFinished())
			return false;
	return true;
}

bool Toolser::isEmpty() const
{
	TerToolCtrls::const_iterator it;
	FOR_EACH(controllers_, it)
		if(!it->isEmpty())
			return false;
	return true;
}