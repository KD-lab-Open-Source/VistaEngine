#include "StdAfx.h"
#include "Player.h"
#include "Universe.h"
#include "..\Environment\Environment.h"
#include "..\Environment\Anchor.h"

#include "UnitTrail.h"

//REGISTER_CLASS(AttributeBase, AttributeTrail, "Поезд");
//REGISTER_CLASS(UnitBase, UnitTrail, "Поезд")
//REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_TRAIL, UnitTrail)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(TrailPoint, DockingSide, "DockingSide")
REGISTER_ENUM_ENCLOSED(TrailPoint, SIDE_FRONT, "FRONT");
REGISTER_ENUM_ENCLOSED(TrailPoint, SIDE_BACK, "BACK");
REGISTER_ENUM_ENCLOSED(TrailPoint, SIDE_LEFT, "LEFT");
REGISTER_ENUM_ENCLOSED(TrailPoint, SIDE_RIGHT, "RIGHT");
END_ENUM_DESCRIPTOR_ENCLOSED(TrailPoint, DockingSide)

////////////////////////////////////////////////////////////

AttributeTrail::AttributeTrail()
: AttributeLegionary()
{
	unitClass_ = UNIT_CLASS_TRAIL;
}

void AttributeTrail::serialize( Archive& ar )
{
    __super::serialize(ar);

}

////////////////////////////////////////////////////////////

void TrailPoint::serialize( Archive& ar )
{
	ar.serialize(name, "name", "Имя якоря");
	ar.serialize(dockingSide, "dockingSide", "Сторона привязки");

}

////////////////////////////////////////////////////////////

UnitTrail::UnitTrail( const UnitTemplate& data )
: UnitLegionary(data)
{

}

void UnitTrail::serialize( Archive& ar )
{
	__super::serialize(ar);
	
	ar.serialize(trailPointList_, "trailPointList", "Точки привязки");
}

void UnitTrail::Quant()
{
	__super::Quant();
}

TrailPoint* UnitTrail::checkTrailPoint( Vect2f& point )
{
	TrailPointList::iterator ti;
	FOR_EACH(trailPointList_, ti){
		const Anchor* anchor = environment->findAnchor(ti->name.c_str());
		if(anchor && anchor->position2D().distance2(point) < sqr(anchor->radius())){
			ti->pose = anchor->pose();
			ti->radius = anchor->radius();
			return &(*ti);
		}
	}

	return 0;
}

void UnitLegionary::setTrail(UnitTrail* trail, const Vect3f& point)
{
	setWayPoint(point);

	if(trail->isDocked())
		rigidBody()->setTrail(trail->rigidBody());
}