#ifndef __SOURCE_TER_TOOL_H_INCLUDED__
#define __SOURCE_TER_TOOL_H_INCLUDED__

#include "SourceEffect.h"
#include "..\Terra\terTools.h"

struct SourceTerTool : SourceEffect{
	TerToolBase* pCurTerTool;
	TerToolReference terToolReference;
	int period; //0-не периодичный
	bool flag_autoKill;
	bool flag_dispersion;
	bool flag_end;
	DurationTimer sleepTimer;
	SourceTerTool()
	: SourceEffect() {
		pCurTerTool=0;
		period=0;
		flag_autoKill=true;
		flag_dispersion=false;
		flag_end=false;
		pose_ = Se3f::ID;
	}
	SourceTerTool(const SourceTerTool& donor)
	: SourceEffect(donor) {
		pCurTerTool=0;
		terToolReference=donor.terToolReference;
		period=donor.period;
		flag_autoKill=donor.flag_autoKill;
		flag_dispersion=donor.flag_dispersion;
		flag_end=donor.flag_end;
		pose_ = Se3f::ID;
	}
	SourceBase* clone() const {
		return new SourceTerTool(*this);
	}
	~SourceTerTool() {
		release();
	}
	void release(){
		if(pCurTerTool){
			delete pCurTerTool; pCurTerTool=0;
		}
	}
	//bool killRequest() { //virtual
	//	if(!flag_autoKill)
	//		return true;
	//	else {
	//		if(flag_end)
	//			return true;
	//		else 
	//			return false;
	//	}
	//}
	bool isUseKillTimer() const { //virtual
		return (!flag_autoKill); } //false

	void generateToolserPosition(){
		//QuatF rot = orientation();
		//float angle = G2R(logicRNDfabsRnd(-angle_disp_ / 2, angle_disp_ / 2));
		//rot.postmult(QuatF(angle, Vect3f(0, 0, 1)));
		QuatF rot;
		if(flag_dispersion)
			rot = QuatF(logicRNDfabsRnd(2*M_PI), Vect3f(0, 0, 1));
		else 
			rot = orientation();
		
		pCurTerTool->setPosition(Se3f(rot, position()), true);
	}
	
	void quant();

	void serialize(Archive& ar);
	SourceType type()const{return SOURCE_TERROOL;}

protected:
	void start();
	void stop();
};

#endif
