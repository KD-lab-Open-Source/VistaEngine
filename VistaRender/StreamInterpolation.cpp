#include "StdAfx.h"
#include "StreamInterpolation.h"
#include "XMath\SafeMath.h"
#include "Render\src\NParticle.h"
#include "Render\3dx\Node3dx.h"
#include "Render\3dx\Simply3dx.h"

#ifndef _FINAL_VERSION_
#define CALL_STATISTIC
#endif

#ifdef CALL_STATISTIC
#include "Util\xtl\StaticMap.h"
#include "Util\Win32\DebugSymbolManager.h"
struct TimerContainer
{
	TimerContainer() : name(0), timer(0) {}
	char* name;
	TimerData* timer;
};
#endif

float StreamInterpolator::factor_;

MTSection streamLock;
StreamInterpolator streamLogicInterpolator;
StreamInterpolator streamLogicCommand;
StreamInterpolator streamLogicPostCommand;

StreamInterpolator::StreamInterpolator()
: stream_(256, 1)
{
}

StreamInterpolator::~StreamInterpolator()
{
	xassert(stream_.tell() == 0);
}

StreamInterpolator& StreamInterpolator::set(InterpolateFunction func)
{
	MTL();
	stream_ < flagValue;
	stream_.write(func);
	return *this;
}

StreamInterpolator& StreamInterpolator::set(InterpolateFunction func,BaseGraphObject* obj)
{
	MTL();
	xassert(obj);
	stream_ < flagValue;
	stream_.write(func);
	stream_.write(obj);
	return *this;
}

void StreamInterpolator::put(StreamInterpolator& s)
{
	stream_.write(s.stream_, s.stream_.tell());
}


void StreamInterpolator::clear()
{
	stream_.set(0);
}

void StreamInterpolator::process(float factor)
{
	start_timer_auto();

#ifdef CALL_STATISTIC
	static __declspec(thread) StaticMap<int, TimerContainer>* timerMap = 0;
	if(!timerMap)
		timerMap = new StaticMap<int, TimerContainer>;
#endif

	factor_ = factor;
	int size = stream_.tell();
	stream_.set(0);
	static InterpolateFunction funcPrev;
	while(stream_.tell() < size){
		int flag;
		stream_ > flag;
		xassert(flag == flagValue);
		InterpolateFunction func;
		stream_.read(func);

#ifdef CALL_STATISTIC
		TimerContainer& data = timerMap->operator []((int)func);
		if(data.timer == 0){
			string name;
			debugSymbolManager->getProcName(func, name);
			//xassert(!name.empty());
			data.name = new char[name.size()+1];
			strcpy(data.name, name.c_str());
			data.timer = new TimerData(data.name);
		}
		data.timer->start();
#endif

		func(stream_);

#ifdef CALL_STATISTIC
		data.timer->stop();
#endif

		funcPrev = func;
	}

	xassert(stream_.tell() == size);
}

/////////////////////////////////////////////////////////////

void fParticleRateInterpolation(XBuffer& stream)
{
	cEffect* eff;
	stream.read(eff);
	float f[2];
	stream.read(f, sizeof(f));
	float rate=f[0]*(1 - StreamInterpolator::factor())+f[1]*StreamInterpolator::factor();
	eff->SetParticleRate(rate);
}

void fSe3fInterpolation(XBuffer& stream)
{
	BaseGraphObject* cur;
	stream.read(cur);
	Se3f p[2];
	stream.read(p, sizeof(p));
	Se3f s;
	s.interpolate(p[0], p[1], StreamInterpolator::factor());
	cur->SetPosition(s);
}

void fNodeTransformClear(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	int node_index;
	stream.read(node_index);
	model->RestoreUserTransform(node_index);
}

void fNodeTransformCopy(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	int node_index;
	stream.read(node_index);
	model->CopyUserTransformToSecondAnimation(node_index);
}

void fNodeTransformInterpolation(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	int node_index;
	stream.read(node_index);
	Se3f p[2];
	stream.read(p, sizeof(p));

	Se3f s;
	s.interpolate(p[0], p[1], StreamInterpolator::factor());
	model->SetUserTransform(node_index, s);
}

void fPhaseCommand(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	float p;
	stream.read(p);
	int group;
	stream.read(group);
	model->SetAnimationGroupPhase(group, p);
}

void fPhaseInterpolation(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	float p[2];
	stream.read(p, sizeof(p));
	int group;
	stream.read(group);
	static float eps1=1+FLT_EPS;
	float phase = cycle(p[0] + getDist(p[1], p[0], eps1)*StreamInterpolator::factor(), eps1);
	model->SetAnimationGroupPhase(group, phase);
}

void fPhaseCommandFade(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	Vect2f p;
	stream.read(p);
	int group;
	stream.read(group);
	model->GetInterpolation()->SetAnimationGroupPhase(group, p.x);
	model->GetInterpolation()->SetAnimationGroupInterpolation(group, p.y);
}

void fPhaseInterpolationFade(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	Vect2f p[2];
	stream.read(p, sizeof(p));
	int group;
	stream.read(group);
	static float eps1=1+FLT_EPS;
	float phase = cycle(p[0].x + getDist(p[1].x, p[0].x, eps1)*StreamInterpolator::factor(), eps1);
	model->GetInterpolation()->SetAnimationGroupPhase(group, phase);

	float fade = p[0].y * (1 - StreamInterpolator::factor()) + p[1].y * StreamInterpolator::factor();
	model->GetInterpolation()->SetAnimationGroupInterpolation(group, fade);
}

void fChainInterpolation(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	int p[2];
	stream.read(p, sizeof(p));
	model->SetAnimationGroupChain(p[1], p[0]);
	float phase;
	stream.read(phase);
	model->SetAnimationGroupPhase(p[1], phase);
}

void fChainInterpolationFade(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	int p[2];
	stream.read(p, sizeof(p));
	model->GetInterpolation()->SetAnimationGroupChain(p[1], p[0]);
	float phase;
	stream.read(phase);
	model->GetInterpolation()->SetAnimationGroupPhase(p[1], phase);
	stream.read(phase);
	model->GetInterpolation()->SetAnimationGroupInterpolation(p[1], phase);
}

void fVisibilityGroupInterpolation(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	VisibilityGroupIndex p;
	stream.read(p);
	model->SetVisibilityGroup(p);
}

void fCommandSetVisibilityGroupOfSet(XBuffer& stream)
{
	cObject3dx* model;
	stream.read(model);
	VisibilityGroupIndex group;
	stream.read(group);
	VisibilitySetIndex set;
	stream.read(set);
	model->SetVisibilityGroup(group, set);
}

void fAngleInterpolation(XBuffer& stream)
{
	cObject3dx* node;
	stream.read(node);
	float p[2];
	stream.read(p, sizeof(p));
	eAxis axis;
	stream.read(axis);

	static float M_PI2=2*M_PI;
	float angle=cycle(p[0] + getDist(p[1], p[0], M_PI2)*StreamInterpolator::factor(), M_PI2);
	//
	//MatXf mat = node->GetPosition();
	//mat.rot() *= Mat3f(angle, axis);
	//node->SetPosition(mat);
	//node->SetPosition(node->GetPosition().rot().set(angle, axis));
}

void fColorInterpolation(XBuffer& stream)
{
	cObject3dx* node;
	stream.read(node);
	sColorInterpolate p[2];
	stream.read(p, sizeof(p));

	Color4f colors[3];
	for(int i = 0; i < 3; i++)
		colors[i].interpolate(p[0].colors[i],p[1].colors[i],StreamInterpolator::factor());

	node->SetColorOld(&colors[0],&colors[1],&colors[2]);
}

void fCommandSetPose(XBuffer& stream)
{
	BaseGraphObject* cur;
	stream.read(cur);
	Se3f s;
	stream.read(s);
	cur->SetPosition(s);
}

void fCommandRelease(XBuffer& stream)
{
	UnknownClass* obj;
	stream.read(obj);
	obj->Release();
}

void fCommandSetCycle(XBuffer& stream)
{
	cEffect* eff;
	stream.read(eff);
	bool f;
	stream.read(f);
	eff->setCycled(f);
}

void fCommandSetAutoDeleteAfterLife(XBuffer& stream)
{
	cEffect* eff;
	stream.read(eff);
	bool f;
	stream.read(f);
	eff->SetAutoDeleteAfterLife(f);
}

void fCommandSetParticleRate(XBuffer& stream)
{
	cEffect* eff;
	stream.read(eff);
	float f;
	stream.read(f);
	eff->SetParticleRate(f);
}

void fCommandSimplyOpacity(XBuffer& stream)
{
	cSimply3dx* model;
	stream.read(model);
	float opacity;
	stream.read(opacity);
	model->SetOpacity(opacity);
}

void fCommandSetTarget(XBuffer& stream)
{
	cEffect* eff;
	stream.read(eff);

	int size;
	stream.read(size);
	Vect3f center;
	stream.read(center);
	
	vector<Vect3f> strike_ends(size);
	for(int i = 0; i < size; ++i)
		stream.read(strike_ends[i]);	
	
	eff->SetTarget(center, strike_ends);
}

void fCommandSetTarget1(XBuffer& stream)
{
	cEffect* eff;
	stream.read(eff);

	Vect3f begin;
	stream.read(begin);
	Vect3f end;
	stream.read(end);
	
	eff->SetTarget(begin, end);
}

void fBeamInterpolation(XBuffer& stream)
{
	cEffect* eff;
	stream.read(eff);

	Vect3f begin;
	stream.read(begin);
	
	Vect3f end;
	stream.read(end);

	Vect3f begin_prev;
	stream.read(begin_prev);
	
	Vect3f end_prev;
	stream.read(end_prev);

	Vect3f v0, v1;
	v0.interpolate(begin_prev, begin, StreamInterpolator::factor());
	v1.interpolate(end_prev, end, StreamInterpolator::factor());

	eff->SetTarget(v0, v1);
}

void fCommandSetIgnored(XBuffer& stream)
{
	BaseGraphObject* cur;
	stream.read(cur);
	bool ignored;
	stream.read(ignored);
	if(ignored)
		cur->setAttribute(ATTRUNKOBJ_IGNORE);
	else
		cur->clearAttribute(ATTRUNKOBJ_IGNORE);
}

void fCommandSetScale(XBuffer& stream)
{
	c3dx* cur;
	stream.read(cur);
	float scale;
	stream.read(scale);
	cur->SetScale(scale);
}

void fCommandSetUserTransform(XBuffer& stream)
{
	cObject3dx* cur;
	stream.read(cur);
	int node;
	stream.read(node);
	Se3f pose;
	stream.read(pose);
	cur->SetUserTransform(node, pose);
}

void fHideByFactor(XBuffer& stream)
{
	BaseGraphObject* cur;
	stream.read(cur);
	float factor;
	stream.read(factor);
	if(StreamInterpolator::factor() > factor)
		cur->SetPosition(Se3f::ID);
		//cur->setAttribute(ATTRUNKOBJ_IGNORE);
}

void fCommandSetUnderwaterSiluet(XBuffer& stream)
{
	cObject3dx* cur;
	stream.read(cur);
	bool enabled;
	stream.read(enabled);
	if(enabled)
		cur->setAttribute(ATTR3DX_UNDERWATER);
	else
		cur->clearAttribute(ATTR3DX_UNDERWATER);
}

void attachSmart(BaseGraphObject* object)
{
	if(!MT_IS_GRAPH()){
		object->setAttribute(ATTRUNKOBJ_IGNORE);
		streamLogicCommand.set(fCommandSetIgnored, object) << false;
	}
	object->Attach();
}
