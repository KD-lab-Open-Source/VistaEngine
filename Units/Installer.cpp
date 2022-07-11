#include "StdAfx.h"
#include "Installer.h"
#include "vmap.h"
#include "CameraManager.h"
#include "RenderObjects.h"
#include "BaseUnit.h"
#include "Universe.h"
#include "Water\CircleManager.h"
#include "IronBuilding.h"
#include "ScanningShape.h"
#include "Render\src\cZPlane.h"
#include "Render\src\Scene.h"
#include "Render\src\VisGeneric.h"

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
	requestPosition_ = Vect2f::ZERO;
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
			if(chain->animationGroup() >= 0)
				ObjectPoint->SetAnimationGroupChain(chain->animationGroup(), chain->chainIndex());
			else for(int i = 0; i < ObjectPoint->GetAnimationGroupNumber(); ++i)
				ObjectPoint->SetAnimationGroupChain(i, chain->chainIndex());
		}
		ObjectPoint->setAttribute(ATTR3DX_HIDE_LIGHTS);
		ObjectPoint->setAttribute(ATTRUNKOBJ_IGNORE);
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
	const AttributeBuilding* attribute_;
	int marker_;

public:
	ScanGroundLineBuffOp(int x0,int y0,int sx,int sy, char* buffer, const AttributeBuilding* attribute, int marker)
	{
		x0_ = x0;
		y0_ = y0;
		sx_ = sx;
		sy_ = sy;
		buffer_ = buffer;
		cnt = max = 0;
		building_ = false;
		attribute_ = attribute;
		marker_ = marker;
	}
	void operator()(int x1,int x2,int y)
	{
		xassert((x1 - x0_) >= 0 && (x2 - x0_) < sx_ && (y - y0_) >= 0 && (y - y0_) < sy_);

		max += x2 - x1 + 1;
		unsigned short* buf = vMap.gABuf + vMap.offsetGBufWorldC(0, y);
		char* pd = buffer_ + (y - y0_)*sx_ + x1 - x0_;
		while(x1 <= x2){
			unsigned short p = *(buf + vMap.XCYCLG(x1 >> kmGrid));
			if(!(p & ( GRIDAT_BUILDING | GRIDAT_BASE_OF_BUILDING_CORRUPT)) && attribute_->checkPointForInstaller(x1, y)){
				cnt++;
				(*pd) |= 1;
			}
			else if(p & GRIDAT_BUILDING)
				building_ = true;

			(*pd) |= marker_;
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

	float radius = attribute_->basementExtent.norm();

	requestPosition_ = Vect2f(
		clamp(position.x, radius, vMap.H_SIZE - radius),
		clamp(position.y, radius, vMap.V_SIZE - radius));

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
		ObjectPoint->clearAttribute(ATTRUNKOBJ_IGNORE);

		ObjectPoint->SetPosition(Se3f(QuatF(angle_, Vect3f::K), position_));
		ObjectPoint->SetOpacity(0.5f);
		if(valid())
			ObjectPoint->SetTextureLerpColor(Color4f(0,1.0f,0,0.5f));
		else
			ObjectPoint->SetTextureLerpColor(Color4f(1.0f,0,0,0.5f));
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
	int offset = 0;//GlobalAttributes::instance().installerOffset;
	OffsetX = x0 - offset;
	OffsetY = y0 - offset;

	int sx = x1 - x0 + 2 + 2*offset;
	int sy = y1 - y0 + 2 + 2*offset;

	if(!BaseBuff || BaseBuffSX < sx || BaseBuffSY < sy){
		if(BaseBuff)
			delete BaseBuff;
		BaseBuffSX = sx;
		BaseBuffSY = sy;
		BaseBuff = new char[BaseBuffSX * BaseBuffSY];
		InitTexture();
	}
	memset(BaseBuff,0,BaseBuffSX * BaseBuffSY);

	ScanGroundLineBuffOp line_op1(OffsetX, OffsetY, BaseBuffSX, BaseBuffSY, BaseBuff, attribute_, 4);
	for(int yy = 0; yy < BaseBuffSY; yy++)
		line_op1(OffsetX, OffsetX + BaseBuffSX - 1, OffsetY + yy);

	ScanGroundLineBuffOp line_op(OffsetX, OffsetY, BaseBuffSX, BaseBuffSY, BaseBuff, attribute_, 2);
	
	ScanningShape shape;
	shape.setPolygon(&points[0], 4);
	ScanningShape::const_iterator j;
	int y = shape.rect().y;
	FOR_EACH(shape.intervals(), j){
		line_op((*j).xl, (*j).xr, y);
		y++;
	}
	
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
		if((p->player() == player_ || p->player()->isWorld()) 
		  && p->attr().producedPlacementZone == attribute_.placementZone
		  && p->position2D().distance2(position_) < sqr(p->attr().producedPlacementZoneRadius + attribute_.placementZone->showRadius)
		  && p->isConstructed())
			universe()->circleManager()->addCircle(p->position2D(), p->attr().producedPlacementZoneRadius, player_->race()->placementZoneCircle);
	}

private:
	Vect2f position_;
	const AttributeBuilding& attribute_;
	Player* player_;
};

void BuildingInstaller::quant(Player* player, Camera* camera)
{
	MTG();
	if(ObjectPoint){
		MTAuto lock(universe()->unitGridLock);
		environmentAnalysis(player);
		if(attribute_->placementZone){
			ShowPlacementZoneOp op(*attribute_, position(), player);
			universe()->unitGrid.Scan(Vect2i(position()), AttributeBase::producedPlacementZoneRadiusMax() + attribute_->placementZone->showRadius, op);
		}
	}

	if(plane)
		plane->setAttribute(ATTRUNKOBJ_IGNORE);

	if(!(ObjectPoint && pTexture))
		return;

	if(!plane)
		plane = terScene->CreatePlaneObj();

	if(attribute_->installerLight && gb_VisGeneric->shadowEnabled()){
		plane->setAttribute(ATTRUNKOBJ_SHADOW);
		plane->setAttribute(ATTRUNKOBJ_IGNORE_NORMALCAMERA);
	}
	else{
		plane->clearAttribute(ATTRUNKOBJ_SHADOW);
		plane->clearAttribute(ATTRUNKOBJ_IGNORE_NORMALCAMERA);
	}

	int Pitch;
	BYTE* buf = (BYTE*)pTexture->LockTexture(Pitch);

	Color4c colors[8] = {
		Color4c(0, 0, 0, 0), // 0 - empty
		Color4c(0, 0, 0, 0), // 1 - empty
		Color4c(255,0,0,128), // 2 - bad
		(valid() ? Color4c(0,255,0,128) : Color4c(200,128,128,128)), // 3 - good
		Color4c(128,0,0,64), // 4 - bad external
		Color4c(0,128,0,64), // 5 - good external
		Color4c(255,0,0,128), // 6 - bad
		(valid() ? Color4c(0,255,0,128) : Color4c(200,128,128,128)), // 7 - good
	};

	char* p = BaseBuff;
	for(int i = 0;i < BaseBuffSY;i++){
		DWORD* c = (DWORD*)(buf+i*Pitch);
		for(int j = 0;j < BaseBuffSX;j++)
			*(c++) = colors[*(p++) & 7].RGBA();
	}

	pTexture->UnlockTexture();

	if(plane){
		plane->clearAttribute(ATTRUNKOBJ_IGNORE);
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
	return round(cycle(angle_, 2*M_PI)/(M_PI_4))*(M_PI_4); 
}
