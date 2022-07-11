#include "StdAfx.h"
#include "PlaceOperators.h"
#include "..\util\ScanningShape.h"


WeaponScanOp::WeaponScanOp(const UnitActing* unit, Player& aiPlayer, float scanRadius, WeaponPrmReference weaponPrm)
	: unit_(unit), aiPlayer_(aiPlayer), invalidPosition_(true), weaponPrm_(weaponPrm)
{
	scan_radius = scanRadius;
	attrVirtualUnit = AuxAttributeReference(AUX_ATTRIBUTE_ZONE);
}

void WeaponScanOp::checkPosition(const Vect2f& pos)
{	
	invalidPosition_ = false;

	int id = weaponPrm_.get()->ID();
	float dist = pos.distance(unit_->position2D());

	if (dist < unit_->radius() + unit_->fireRadiusMin()  || dist > unit_->radius() + unit_->fireRadiusOfWeapon(id))
		invalidPosition_ = true;
	if (!unit_->fireCheck(WeaponTarget(Vect3f(pos.x, pos.y, 0), id)))
		invalidPosition_ = true;

	if (!invalidPosition_)
	universe()->unitGrid.Scan(pos.x, pos.y, scan_radius, *this);
	
}

void WeaponScanOp::operator()(UnitBase* unit2)
{
	const AttributeBase& attr = unit2->attr();
	if(unit_ != unit2 && (attr.isBuilding() || attr.isEnvironmentBuilding() || &attr == attrVirtualUnit))
		invalidPosition_ = true;
}

WeaponUnitScanOp::WeaponUnitScanOp(const UnitActing* unit, Player& aiPlayer, WeaponPrmReference weaponPrm, bool aimUnit, bool health)
	: unit_(unit), aiPlayer_(aiPlayer), 
	foundPosition_(false), weaponPrm_(weaponPrm), aimUnit_(aimUnit), health_(health)
{
}

void WeaponUnitScanOp::checkPosition(const Vect2f& pos)
{	
	foundPosition_ = false;
	targetUnit_ = 0;
	maxCriteria = 0;
	bestDist_ = FLT_INF;
	universe()->unitGrid.Scan(pos.x, pos.y, unit_->radius() + unit_->fireRadiusOfWeapon(weaponPrm_.get()->ID()), *this);
}

void WeaponUnitScanOp::operator()(UnitBase* unit2)
{	
	if(unit2->player() == universe()->worldPlayer())
		return;

	if(unit2->attr().excludeFromAutoAttack || unit2->attr().isStrategicPoint)
		return;

	int weaponID = weaponPrm_.get()->ID();
	Vect3f position = unit2->position();

	float tmpDist = 0.f;
	
	if(!unit2->alive() || !unit2->attr().isActing())
		return;

	WeaponBase* weapon = unit_->findWeapon(weaponID);
	const WeaponTarget& weaponTarget = WeaponTarget(safe_cast<UnitInterface*>(unit2), position, weaponID);
	if(aimUnit_){ // My
		if (unit2->player() == &aiPlayer_ && unit_->fireCheck(WeaponTarget(position, weaponID)) &&
			weapon->checkFogOfWar(weaponTarget) &&
			(health_ ? weapon->canAttack(weaponTarget) : true) && 
			1.f - unit2->health() > maxCriteria && bestDist_ > (tmpDist = unit2->position2D().distance2(unit_->position2D()))) {
			bestDist_ = tmpDist;
			maxCriteria = 1.f - unit2->health();
			foundPosition_ = true;
			position_ = position;
			targetUnit_ = unit2;
		}
	}
	else{ // Enemy
		if (aiPlayer_.isEnemy(unit2->player()) && unit_->fireCheck(WeaponTarget(position, weaponID)) &&
			weapon->checkFogOfWar(weaponTarget) &&
			(health_ ? weapon->canAttack(weaponTarget) : true) && 
			unit2->health() > maxCriteria && bestDist_ > (tmpDist = unit2->position2D().distance2(unit_->position2D()))) {
			bestDist_ = tmpDist;
			maxCriteria = unit2->health();
			foundPosition_ = true;
			position_ = position;
			targetUnit_ = unit2;
		}
	}
}

RadiusScanOp::RadiusScanOp(const UnitActing* unit,const AttributeBase* attr, Player& aiPlayer, float extraRadius)
	: unit_(unit), attr_(attr), aiPlayer_(aiPlayer), extraRadius_(extraRadius), invalidPosition_(true)
{
	scan_radius = unit_->attr().radius() + extraRadius_;
}

void RadiusScanOp::checkPosition(const Vect2f& pos)
{	
	invalidPosition_ = false;

	float radius = unit_->attr().radius();

	if(pos.x  < radius || pos.x >= vMap.H_SIZE - radius || pos.y < radius || pos.y >= vMap.V_SIZE - radius 
		|| !unit_->rigidBody()->checkImpassability(pos)){
		invalidPosition_ = true;
		return;
	}

	universe()->unitGrid.Scan(pos.x, pos.y, scan_radius, *this);
	
}

void RadiusScanOp::operator()(UnitBase* unit2)
{
	const AttributeBase& attr = unit2->attr();
	// Учитываем только здания 
	if(&attr == &unit_->attr() && safe_cast<UnitReal*>(unit2)->isUpgrading()) 
		invalidPosition_ = true;
	if(attr.isBuilding())
		invalidPosition_ = true;
	if(attr.isResourceItem() && safe_cast<UnitItemResource*>(unit2)->attr().barrierUpgrade)
		invalidPosition_ = true;
}

UnitTargetScanOp::UnitTargetScanOp(UnitInterface* unit, Player* player) : aimUnit_(0), unit_(unit), player_(player)
{
	position_ = unit_->position2D();
}

bool UnitTargetScanOp::operator()(UnitBase* unit)
{
	Player* unitPlayer = unit->player();
	if(player_ != unitPlayer && !unitPlayer->isWorld()){
		const AttributeBase& attr = unit->attr();
		if(attr.isActing())
			if(unit->position2D().distance2(position_) < sqr(unit_->sightRadius())){
				if(safe_cast<UnitActing*>(unit)->isFireTarget(unit_)){
					aimUnit_ = safe_cast<UnitInterface*>(unit);
					return false;
				}
			}
	}

	return true;
}

bool PlaceScanOp::checkBuildingPosition(const AttributeBuilding* attr, const Vect2f& position, Player* player, bool checkUnits, Vect2f& snapPosition_) const
{
	Vect2i points[4];
	attr->calcBasementPoints(0, position, points);
	int x0 = INT_INF, y0 = INT_INF; 
	int x1 = -INT_INF, y1 = -INT_INF;
	for(int i = 0; i < 4; i++){
		const Vect2i& v = points[i];
		if(x0 > v.x)
			x0 = v.x;
		if(y0 > v.y)
			y0 = v.y;
		if(x1 < v.x)
			x1 = v.x;
		if(y1 < v.y)
			y1 = v.y;
	}

	int sx = x1 - x0 + 2;
	int sy = y1 - y0 + 2;

	ScanGroundLineOp line_op(x0, y0, sx, sy);
	ScanningShape shape;
	shape.setPolygon(&points[0], 4);
	ScanningShape::const_iterator j;
	int y = shape.rect().y;
	FOR_EACH(shape.intervals(), j){
		line_op((*j).xl, (*j).xr, y);
		y++;
		if(!line_op.valid())
			break;
	}
	if(line_op.valid() && attr->checkBuildingPosition(position, Mat2f::ID, player, checkUnits, snapPosition_))
		return true;

	return false;
}

PlaceScanOp::PlaceScanOp(UnitReal* unitToOrient, const AttributeBase* attribute, Player& aiPlayer, 
						 bool closeToEnemy, float radius)
		: unitToOrient_(unitToOrient), attribute_(attribute),
		aiPlayer_(aiPlayer), closeToEnemy_(closeToEnemy), radius_(radius), foundPosition_(false)
{
	installBound = attribute_->basementExtent*2;
	orientBound = unitToOrient_->attr().basementExtent*2;
	fullBound = installBound * 2 + orientBound;
	bestPosition_ = Vect2f::ZERO;
}

bool PlaceScanOp::checkEnemyDirection(Vect2f posCenter, Vect2f posInstall, float bestAngle)
{
	PlayerVect::iterator pi;
	FOR_EACH(universe()->Players, pi)
		if(aiPlayer_.isEnemy(*pi)){
			Vect2f enemyCenter = (*pi)->centerPosition();
			if(!enemyCenter.eq(Vect2f::ZERO, FLT_EPS)){
				float angleEnemy = 0;
				if((angleEnemy = fabs((posInstall - posCenter).angle(enemyCenter - posCenter)) < M_PI * 0.33f) && angleEnemy < bestAngle){ 
					bestAngle = angleEnemy;
					return true;
				}		
			}
		}
	return false;
}

void PlaceScanOp::checkPosition(const Vect2f& pos)
{	

	Vect2f snapPoint_(Vect2f::ZERO);
	const AttributeBuilding* attrBuilding = safe_cast<const AttributeBuilding*>(attribute_);

	foundPosition_ = false;
	float bestPriority = -1.f;

	if(!radius_){

		float bestAngle = FLT_INF;

		float scanRadius = max((orientBound.x + installBound.x) * 0.5f, (orientBound.y + installBound.y) * 0.5f);
	
		Vect2f startPoint = -orientBound;
		startPoint *= 0.5f;
		//startPoint -= installBound;
		startPoint += pos;
		//startPoint -= Vect2f(4.f, 4.f);
		float rEdge = startPoint.x + fullBound.x - installBound.x;
		float dEdge = startPoint.y + fullBound.y - installBound.y;
		float lEdge = startPoint.x - installBound.x;
		float uEdge = startPoint.y - installBound.y;
		Vect2f endPoint = Vect2f(startPoint.x + orientBound.x, startPoint.y + orientBound.y);

		int pass = logicRND(4);

        for(int i = pass; i < pass + 4; i++){
			int curPass = i % 4;
			Vect2f checkPoint = Vect2f::ZERO;
			if(curPass == 0){
				checkPoint = startPoint;
				checkPoint.y -= installBound.y;
				checkPoint.y -= 4.f;
				while(checkPoint.x + installBound.x < rEdge)
				{
					installPos = installBound;
					installPos *= 0.5;
					installPos += checkPoint;
					// debug
					show(installPos, aiPlayer_.unitColor());
					if(checkBuildingPosition(attrBuilding, installPos, &aiPlayer_, true, snapPoint_)){
						priority_ = FLT_INF;
						universe()->unitGrid.Scan(pos.x, pos.y, scanRadius, *this);
						if(priority_ > bestPriority){
							if(!closeToEnemy_){
								bestPosition_ = installPos;
								foundPosition_ = true;
								show(installPos, universe()->worldPlayer()->unitColor());
								bestPriority = priority_;
							}
							if(closeToEnemy_ && checkEnemyDirection(pos, installPos, bestAngle)){
								bestPosition_ = installPos;
								foundPosition_ = true;
								show(installPos, universe()->worldPlayer()->unitColor());
								bestPriority = priority_;
							}
						}
					}
					checkPoint.x += installBound.x; 
					checkPoint.x += 4.f;
				}
			}

			if(curPass == 1){
				checkPoint = startPoint;
				checkPoint.x -= installBound.x;
				checkPoint.x -= 4.f;
				while(checkPoint.y + installBound.y < dEdge)
				{
					installPos = installBound;
					installPos *= 0.5;
					installPos += checkPoint;
					// debug
					show(installPos, aiPlayer_.unitColor());
					if(checkBuildingPosition(attrBuilding, installPos, &aiPlayer_, true, snapPoint_)){
						priority_ = FLT_INF;
						universe()->unitGrid.Scan(pos.x, pos.y, scanRadius, *this);
						if(priority_ > bestPriority){
							if(!closeToEnemy_){
								bestPosition_ = installPos;
								foundPosition_ = true;
								show(installPos, universe()->worldPlayer()->unitColor());
								bestPriority = priority_;
							}
							if(closeToEnemy_ && checkEnemyDirection(pos, installPos, bestAngle)){
								bestPosition_ = installPos;
								foundPosition_ = true;
								show(installPos, universe()->worldPlayer()->unitColor());
								bestPriority = priority_;
							}
						}
					}

					checkPoint.y += installBound.y; 
					checkPoint.y += 4.f;
				}
			}

			if(curPass == 2){
				checkPoint = endPoint;
				checkPoint.y += installBound.y + 4.f;
				while(checkPoint.x - installBound.x > lEdge)
				{
					installPos = -installBound;
					installPos *= 0.5;
					installPos += checkPoint;
					// debug
					show(installPos, aiPlayer_.unitColor());
					if(checkBuildingPosition(attrBuilding, installPos, &aiPlayer_, true, snapPoint_)){
						priority_ = FLT_INF;
						universe()->unitGrid.Scan(pos.x, pos.y, scanRadius, *this);
						if(priority_ > bestPriority){
							if(!closeToEnemy_){
								bestPosition_ = installPos;
								foundPosition_ = true;
								show(installPos, universe()->worldPlayer()->unitColor());
								bestPriority = priority_;
							}
							if(closeToEnemy_ && checkEnemyDirection(pos, installPos, bestAngle)){
								bestPosition_ = installPos;
								foundPosition_ = true;
								show(installPos, universe()->worldPlayer()->unitColor());
								bestPriority = priority_;
							}
						}
					}

					checkPoint.x -= installBound.x;
					checkPoint.x -= 4.f;
				}
			}

			if(curPass == 3){
				checkPoint = endPoint;
				checkPoint.x += installBound.x + 4.f;
				while(checkPoint.y - installBound.y > uEdge)
				{
					installPos = -installBound;
					installPos *= 0.5;
					installPos += checkPoint;
					// debug
					show(installPos, aiPlayer_.unitColor());
					if(checkBuildingPosition(attrBuilding, installPos, &aiPlayer_, true, snapPoint_)){
						priority_ = FLT_INF;
						universe()->unitGrid.Scan(pos.x, pos.y, scanRadius, *this);
						if(priority_ > bestPriority){
							if(!closeToEnemy_){
								bestPosition_ = installPos;
								foundPosition_ = true;
								show(installPos, universe()->worldPlayer()->unitColor());
								bestPriority = priority_;
							}
							if(closeToEnemy_ && checkEnemyDirection(pos, installPos, bestAngle)){
								bestPosition_ = installPos;
								foundPosition_ = true;
								show(installPos, universe()->worldPlayer()->unitColor());
								bestPriority = priority_;
							}
						}
					}

					checkPoint.y -= installBound.y; 
					checkPoint.y -= 4.f;
				}
			}
		}
	}
	else{ 
		float orientRadius = 0;//max(orientBound.x * 0.5f, orientBound.y * 0.5f);
		float installRadius = 0;//max(installBound.x * 0.5f, installBound.y * 0.5f);
		float fullRadius = orientRadius + radius_ + installRadius;
		float maxSize = installBound.norm();
		float stepAngle = 2 * atan((maxSize * 0.5f) / fullRadius);
		xassert(stepAngle > -FLT_EPS && "Очень плохо, попробуйте увеличить радиус в действии Заказать здание");
		float angle = 0.f;

		float bestAngle = FLT_INF;

		while(angle < 2.f * M_PI)
		{
			installPos = pos;
			installPos += Vect2f(cos(angle) * fullRadius, sin(angle) * fullRadius);
			
			show(installPos, aiPlayer_.unitColor());
			
			if(checkBuildingPosition(attrBuilding, installPos, &aiPlayer_, true, snapPoint_)){
				
				priority_ = FLT_INF;
				universe()->unitGrid.Scan(pos.x, pos.y, radius_ + max(orientBound.x, orientBound.y), *this);
				if(priority_ > bestPriority){
					if(!closeToEnemy_){
						bestPosition_ = installPos;
						foundPosition_ = true;
						show(installPos, universe()->worldPlayer()->unitColor());
						bestPriority = priority_;
					}
					if(closeToEnemy_ && checkEnemyDirection(pos, installPos, bestAngle)){
						bestPosition_ = installPos;
						foundPosition_ = true;
						show(installPos, universe()->worldPlayer()->unitColor());
						bestPriority = priority_;
					}
				}
			}

			angle += stepAngle;
		}
	}
}

void PlaceScanOp::operator()(UnitBase* unit)
{
	// Учитываем только здания 
	const AttributeBase& attr = unit->attr();
	if(!attr.isBuilding())
		return;

	if(&attr == attribute_)
		priority_ = min(priority_, installPos.distance2(unit->position2D()));
}

void PlaceScanOp::show(Vect2f pos, const Color4c& color)
{
	if(showDebugPlayer.placeOp){
		Vect2f v1 = -installBound;
		v1 *= 0.5;
		v1 += pos;
		Vect2f v2 = v1;
		v2 += installBound;
		show_vector(To3D(v1),To3D(Vect2f(v1.x,v2.y)),To3D(v2),To3D(Vect2f(v2.x,v1.y)),color);
		XBuffer buf;
		buf <= priority_;
		show_text(To3D(installPos), buf, color);
	}
}



