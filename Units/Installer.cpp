#include "StdAfx.h"
#include "Installer.h"
#include "Terra.h"
#include "CameraManager.h"
#include "RenderObjects.h"
#include "BaseUnit.h"
#include "Universe.h"
#include "ScanPoly.h"
#include "ExternalShow.h"
#include "IronBuilding.h"

BuildingInstaller::BuildingInstaller()
{
	attribute_ = 0;
	ObjectPoint = 0;
	BaseBuff = 0;
	BaseBuffSX = BaseBuffSY = 0;
	pTexture = 0;
	plane = 0;
	multipleInstalling_ = false;
	snapPosition_ = Vect2f::ZERO;
	requestPosition_ = Vect3f::ZERO;
}

BuildingInstaller::~BuildingInstaller()
{
	Clear();
}

void BuildingInstaller::Clear()
{
	MTG();
	attribute_ = 0;

	RELEASE(ObjectPoint);

	valid_ = 0;
	multipleInstalling_ = false;

	delete BaseBuff;
	BaseBuff = 0;
	BaseBuffSX = BaseBuffSY = 0;

	RELEASE(plane);
	RELEASE(pTexture);
}

void BuildingInstaller::InitObject(const AttributeBuilding* attr, bool createHologram)
{
	MTG();
	xassert(attr); 

	Clear();
	
	attribute_ = attr; 
	valid_ = 0;
	buildingInArea_ = 0;

	if(createHologram){
		ObjectPoint = terScene->CreateObject3dxDetached(attr->modelName.c_str(), 0);
		ObjectPoint->SetScale(attr->boundScale);
		universe()->activePlayer()->setModelSkinColor(ObjectPoint);
		const AnimationChain* chain = attr->animationChain(CHAIN_HOLOGRAM, 0);
		if(chain){
			ObjectPoint->SetVisibilityGroup(chain->visibilityGroup());
			ObjectPoint->SetAnimationGroupChain(chain->animationGroup(), chain->chainIndex());
		}
		ObjectPoint->SetAttr(ATTR3DX_HIDE_LIGHTS);
		ObjectPoint->SetAttr(ATTRUNKOBJ_IGNORE);
		terScene->AttachObj(ObjectPoint);
	}
	
	angle_ = 0;
	position_ = Vect3f::ZERO;

	OffsetX = OffsetY = 0;
}

void BuildingInstaller::ConstructObject(Player* player, UnitInterface* builder)
{
	MTG();
	if(player)
		if(valid()){
			if(player->race()->startConstructionSound)
				player->race()->startConstructionSound->play(position_);
			UnitCommand command(COMMAND_ID_BUILDING_START, attribute_, Vect3f(position().x,position().y,angleDiscrete()), builder);
			if(isShiftPressed()){
				command.setShiftModifier();
				player->sendCommand(command);
				multipleInstalling_ = true;
			}
			else{
				player->sendCommand(command);
				Clear();
			}
		}
		else if(player->race()->unableToConstructSound)
			player->race()->unableToConstructSound->play(position_);

}

class ScanGroundLineBuffOp
{
	int cnt, max;
	int x0_, y0_, sx_, sy_;
	char* buffer_;
	bool building_;

public:
	ScanGroundLineBuffOp(int x0,int y0,int sx,int sy, char* buffer)
	{
		x0_ = x0;
		y0_ = y0;
		sx_ = sx;
		sy_ = sy;
		buffer_ = buffer;
		cnt = max = 0;
		building_ = false;
	}
	void operator()(int x1,int x2,int y)
	{
		xassert((x1 - x0_) >= 0 && (x2 - x0_) < sx_ && (y - y0_) >= 0 && (y - y0_) < sy_);

		max += x2 - x1 + 1;
		unsigned short* buf = vMap.GABuf + vMap.offsetGBufWorldC(0, y);
		char* pd = buffer_ + (y - y0_)*sx_ + x1 - x0_;
		while(x1 <= x2){
			unsigned short p = *(buf + vMap.XCYCLG(x1 >> kmGrid));
			//if((p & GRIDAT_LEVELED) && !(p & ( GRIDAT_BUILDING | GRIDAT_BASE_OF_BUILDING_CORRUPT)))
			if(!(p & ( GRIDAT_BUILDING | GRIDAT_BASE_OF_BUILDING_CORRUPT))){
				cnt++;
				(*pd) |= 2;
			}
			else if(p & GRIDAT_BUILDING)
				building_ = true;

			(*pd) |= 1;
			x1++;
			pd++;
		}
	}
	bool valid() 
	{ 
		xassert(cnt <= max && "Bad Max");
		return cnt == max; 
	}
	bool building() const { return building_; }
};

void BuildingInstaller::SetBuildPosition(const Vect2f& position, float angle)
{
	MTG();
	xassert(attribute_);

	if(multipleInstalling_ && !isShiftPressed()){
		Clear();
		return;
	}

	float radius = attribute_->boundRadius;
	
	requestPosition_ = Vect3f(
		clamp(position.x, radius, vMap.H_SIZE - radius),
		clamp(position.y, radius, vMap.V_SIZE - radius), 
		0);

	requestPosition_.z = attribute_->buildingPositionZ(requestPosition_);

	if(valid() && !snapPosition_.eq(Vect2f::ZERO)){
		position_.x = snapPosition_.x;
		position_.y = snapPosition_.y;
	}
	else {
		position_.x = position.x;
		position_.y = position.y;
	}

	position_ = Vect3f(
		clamp(position_.x, radius, vMap.H_SIZE - radius),
		clamp(position_.y, radius, vMap.V_SIZE - radius), 
		0);

    position_.z = attribute_->buildingPositionZ(position_);
	angle_ = angle;

	if(ObjectPoint){
		ObjectPoint->ClearAttr(ATTRUNKOBJ_IGNORE);

		ObjectPoint->SetPosition(Se3f(QuatF(angle_, Vect3f::K), position_));
		ObjectPoint->SetOpacity(0.5f);
		if(valid())
			ObjectPoint->SetTextureLerpColor(sColor4f(0,1.0f,0,0.5f));
		else
			ObjectPoint->SetTextureLerpColor(sColor4f(1.0f,0,0,0.5f));
	}
}

void BuildingInstaller::environmentAnalysis(Player* player)
{
	if(!ObjectPoint)
		return;

	Vect2i points[4];
	attribute_->calcBasementPoints(angleDiscrete(), position_, points);
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
	OffsetX = x0;
	OffsetY = y0;

	int sx = x1 - x0 + 2;
	int sy = y1 - y0 + 2;

	if(!BaseBuff || BaseBuffSX < sx || BaseBuffSY < sy){
		if(BaseBuff)
			delete BaseBuff;
		BaseBuffSX = sx;
		BaseBuffSY = sy;
		BaseBuff = new char[BaseBuffSX * BaseBuffSY];
		InitTexture();
	}
	memset(BaseBuff,0,BaseBuffSX * BaseBuffSY);

	ScanGroundLineBuffOp line_op(OffsetX, OffsetY, BaseBuffSX, BaseBuffSY, BaseBuff);
	scanPolyByLineOp(points, 4, line_op);
	valid_ = line_op.valid();
	buildingInArea_ = line_op.building();

	snapPosition_ = Vect2f::ZERO;
	if(valid_)
		valid_ = attribute_->checkBuildingPosition(requestPosition_, Mat2f(angleDiscrete()), player, true, snapPosition_);
}

void BuildingInstaller::InitTexture()
{
	RELEASE(pTexture);

	int dx = 1 << (BitSR(BaseBuffSX) + 1);
	int dy = 1 << (BitSR(BaseBuffSY) + 1);
	dx = dy = max(dx,dy);
	pTexture = gb_VisGeneric->CreateTexture(dx,dy,true);
	if(pTexture == 0)return;

	int Pitch;
	BYTE* buf = (BYTE*)pTexture->LockTexture(Pitch);
	for(int y=0;y<dy;y++)
	{
		DWORD* c = (DWORD*)(buf+y*Pitch);
		for(int x = 0; x < dx; x++,c++)
			*c = 0;
	}
	pTexture->UnlockTexture();
}

class ShowPlacementZoneOp
{
public:
	ShowPlacementZoneOp(const AttributeBuilding& attribute, const Vect2f& position, Player* player) 
		: attribute_(attribute), position_(position), player_(player)
	{}

	void operator()(UnitBase* p)
	{
		if((p->player() == player_ || p->player()->isWorld()) && 
			p->attr().producedPlacementZone == attribute_.placementZone){
			if(p->position2D().distance2(position_) < sqr(p->attr().producedPlacementZoneRadius + attribute_.placementZone->showRadius))
				if(p->attr().isObjective())
					safe_cast<UnitReal*>(p)->drawCircle(RADIUS_PLACEMENT_ZONE, p->position(), p->attr().producedPlacementZoneRadius, attribute_.placementZone->circleColor);
				else
					terCircleShowGraph(p->position(), p->attr().producedPlacementZoneRadius, attribute_.placementZone->circleColor);
		}
	}

private:
	Vect2f position_;
	const AttributeBuilding& attribute_;
	Player* player_;
};

void BuildingInstaller::quant(Player* player, cCamera* DrawNode)
{
	MTG();
	if(ObjectPoint && attribute_->placementZone){
		MTAuto lock(universe()->unitGridLock);
		environmentAnalysis(player);
		ShowPlacementZoneOp op(*attribute_, position(), player);
		universe()->unitGrid.Scan((const Vect2f&)position(), AttributeBase::producedPlacementZoneRadiusMax() + attribute_->placementZone->showRadius, op);
	}

	if(plane)
		plane->SetAttr(ATTRUNKOBJ_IGNORE);

	if(!(ObjectPoint && pTexture))
		return;

	if(!plane)
		plane = terScene->CreatePlaneObj();

	if(attribute_->installerLight && DrawNode && DrawNode->FindChildCamera(ATTRCAMERA_SHADOW) &&
		gb_VisGeneric->GetShadowType()!=SHADOW_MAP_SELF)
	{
		plane->SetAttr(ATTRUNKOBJ_SHADOW);
		plane->SetAttr(ATTRUNKOBJ_IGNORE_NORMALCAMERA);
	}
	else{
		plane->ClearAttr(ATTRUNKOBJ_SHADOW);
		plane->ClearAttr(ATTRUNKOBJ_IGNORE_NORMALCAMERA);
	}

	int Pitch;
	BYTE* buf = (BYTE*)pTexture->LockTexture(Pitch);

	sColor4c cempty(0,0,0,0);
	sColor4c cgood = valid() ? sColor4c(0,255,0,128) : sColor4c(200,128,128,128);
	sColor4c cbad(255,0,0,128);
	char* p = BaseBuff;
	for(int i = 0;i < BaseBuffSY;i++)
	{
		DWORD* c = (DWORD*)(buf+i*Pitch);
		for(int j = 0;j < BaseBuffSX;j++)
		{
			if((*p) & 1){
				if((*p) & 2)
					*c = cgood.RGBA();
				else
					*c = cbad.RGBA();
			}else
				*c = cempty.RGBA();
			p++;
			c++;
		}
	}

	pTexture->UnlockTexture();

	if(plane){
		plane->ClearAttr(ATTRUNKOBJ_IGNORE);
		pTexture->AddRef();
		plane->SetTexture(0,pTexture);
		plane->SetSize(Vect3f(BaseBuffSX,BaseBuffSY,-10));
		MatXf mat = MatXf::ID;
		mat.trans() = Vect3f(OffsetX, OffsetY, position_.z + 2);
		plane->SetPosition(mat);
		float du = BaseBuffSX/(float)pTexture->GetWidth();
		float dv = BaseBuffSY/(float)pTexture->GetHeight();
		plane->SetUV(0,0,du,dv);
	}
}

float BuildingInstaller::angleDiscrete() const 
{ 
	return round(cycle(angle_, 2*M_PI)/(M_PI/4))*(M_PI/4); 
}
