#include "stdafxTr.h"
#include "procedurMap.h"
#include "Serialization.h"

#include "road.h"

BEGIN_ENUM_DESCRIPTOR(PMOperationID, "PMOperatinID")
REGISTER_ENUM(PMO_ID_NONE, "ничего");
REGISTER_ENUM(PMO_ID_TOOLZER, "toolzer");
REGISTER_ENUM(PMO_ID_SQUARE_TOOLZER, "PMO_ID_SQUARE_TOOLZER");
REGISTER_ENUM(PMO_ID_GEO, "geo");
REGISTER_ENUM(PMO_ID_BLUR, "PMO_ID_BLUR");
REGISTER_ENUM(PMO_ID_COLORPIC, "PMO_ID_COLORPIC");
REGISTER_ENUM(PMO_ID_BITGEN, "PMO_ID_BITGEN");
REGISTER_ENUM(PMO_ID_ROAD, "PMO_ID_ROAD");
END_ENUM_DESCRIPTOR(PMOperationID)

REGISTER_CLASS(sBasePMOperation, sToolzerPMO, "sToolzerPMO");
REGISTER_CLASS(sBasePMOperation, sSquareToolzerPMO, "sSquareToolzerPMO");
REGISTER_CLASS(sBasePMOperation, sGeoPMO, "sGeoPMO");
REGISTER_CLASS(sBasePMOperation, sBlurPMO, "sBlurPMO");
REGISTER_CLASS(sBasePMOperation, sColorPicPMO, "sColorPicPMO");
REGISTER_CLASS(sBasePMOperation, sBitGenPMO, "sBitGenPMO");
REGISTER_CLASS(sBasePMOperation, sRoadPMO, "sRoadPMO");

//typedef WrapPMO<sToolzerPMO> WrapPMOsToolzerPMO;
//REGISTER_CLASS(sBasePMOperation, WrapPMOsToolzerPMO, "sToolzerPMO");
//typedef WrapPMO<sSquareToolzerPMO> WrapPMOsSquareToolzerPMO;
//REGISTER_CLASS(sBasePMOperation, WrapPMOsSquareToolzerPMO, "sSquareToolzerPMO");
//typedef WrapPMO<sGeoPMO> WrapPMOsGeoPMO;
//REGISTER_CLASS(sBasePMOperation, WrapPMOsGeoPMO, "sGeoPMO");
//typedef WrapPMO<sBlurPMO> WrapPMOsBlurPMO;
//REGISTER_CLASS(sBasePMOperation, WrapPMOsBlurPMO, "sBlurPMO");
//typedef WrapPMO<sColorPicPMO> WrapPMOsColorPicPMO;
//REGISTER_CLASS(sBasePMOperation, WrapPMOsColorPicPMO, "sColorPicPMO");
//typedef WrapPMO<sBitGenPMO> WrapPMOsBitGenPMO;
//REGISTER_CLASS(sBasePMOperation, WrapPMOsBitGenPMO, "sBitGenPMO");
//typedef WrapPMO<sRoadPMO> WrapPMOsRoadPMO;
//REGISTER_CLASS(sBasePMOperation, WrapPMOsRoadPMO, "sRoadPMO");


void sToolzerPMO::serialize(Archive& ar) 
{
	ar.serialize(x, "x", 0);
	ar.serialize(y, "y", 0);
	ar.serialize(rad, "rad", 0);
	ar.serialize(smth, "smth", 0);
	ar.serialize(dh, "dh", 0);
	ar.serialize(smode, "smode", 0);
	ar.serialize(eql, "eql", 0);
	ar.serialize(filterHeigh, "filterHeigh", 0);
	ar.serialize(rndVal, "rndVal", 0);
}

void sGeoPMO::serialize(Archive& ar) 
{
	ar.serialize(x, "x", 0);
	ar.serialize(y, "y", 0);
	ar.serialize(sx, "sx", 0);
	ar.serialize(sy, "sy", 0);
	ar.serialize(maxGGAlt, "maxGGAlt", 0);
	ar.serialize(powerCellSize, "powerCellSize", 0);
	ar.serialize(powerShift, "powerShift", 0);
	ar.serialize(geoNetMesh, "geoNetMesh", 0);
	ar.serialize(noiseLevel, "noiseLevel", 0);
	ar.serialize(borderForm, "borderForm", 0);
	ar.serialize(inverse, "inverse", 0);
}

void sBlurPMO::serialize(Archive& ar) 
{
	ar.serialize(x, "x", 0);
	ar.serialize(y, "y", 0);
	ar.serialize(rad, "rad", 0);
	ar.serialize(intensity, "intensity", 0);
}


sColorPicPMO::sColorPicPMO() 
{
	pmoID=PMO_ID_COLORPIC;
	minFH=0;	//for updating old format
	maxFH=256<<VX_FRACTION;
}

void sColorPicPMO::serialize(Archive& ar) 
{
	ar.serialize(x, "x", 0);
	ar.serialize(y, "y", 0);
	ar.serialize(rad, "rad", 0);
	ar.serialize(cntrAlpha, "cntrAlpha", 0);
	ar.serialize(bitmapIDX, "bitmapIDX", 0);
	ar.serialize(minFH, "minFH", 0);
	ar.serialize(maxFH, "maxFH", 0);
}

void sBitGenPMO::serialize(Archive& ar) 
{
	ar.serialize(x, "x", 0);
	ar.serialize(y, "y", 0);
	ar.serialize(rad, "rad", 0);

	int BGM_ID=bitGenMetodID;
	ar.serialize(BGM_ID, "BGM_ID", 0);
	bitGenMetodID=(eBitGenMetodID) BGM_ID;
	ar.serialize(rndVal, "rndVal", 0);
	ar.serialize(maxHeight, "maxHeight", 0);
	ar.serialize(expPower, "expPower", 0);
	ar.serialize(kRoughness, "kRoughness", 0);
	if(bitGenMetodID!=BGMID_MPD)
		kRoughness=0;

	ar.serialize(filterHeigh, "filterHeigh", 0);
}

void sNodePrimaryData::serialize(Archive& ar)
{
	ar.serialize(pos, "pos", 0);
	ar.serialize(width, "width", 0);
	if(!ar.serialize(angleOrWidtEdge, "angleOrWidtEdge", 0)){
		ar.serialize(angleOrWidtEdge, "angle", 0);
	}
}
void sRoadPMO::serialize(Archive& ar)
{
	ar.serialize(nodePrimaryDataArr, "nodePrimaryDataArr", 0);
	ar.serialize(bitmapFileName, "bitmapFileName", 0);
	ar.serialize(bitmapVolFileName, "bitmapVolFileName", 0);
	ar.serialize(edgeBitmapFName, "edgeBitmapFName", 0);
	ar.serialize(edgeVolBitmapFName, "edgeVolBitmapFName", 0);
	ar.serialize(flag_onlyTextured, "flag_onlyTextured", 0);
	ar.serialize(texturingMetod, "texturingMetod", 0);
	ar.serialize(putMetod, "putMetod", 0);
}

/*
void ElementPMO::serialize(Archive& ar) 
{
	PMOperationID ID = basePMO.pmoID;
	ar.serialize(ID, "ID", 0);
	basePMO.pmoID = ID;
	switch(basePMO.pmoID){
	case PMO_ID_NONE:
		break;
	case PMO_ID_TOOLZER:
		ar.serialize(toolzerPMO, "toolzerPMO", 0);
		break;
	case PMO_ID_SQUARE_TOOLZER:
		ar.serialize(squareToolzerPMO, "squareToolzerPMO", 0);
		break;
	case PMO_ID_GEO:
		ar.serialize(geoPMO, "geoPMO", 0);
		break;
	case PMO_ID_BLUR:
		ar.serialize(blurPMO, "blurPMO", 0);
		break;
	case PMO_ID_COLORPIC:
		ar.serialize(colorPicPMO, "colorPicPMO", 0);
		break;
	case PMO_ID_BITGEN:
		ar.serialize(bitGenPMO, "bitGenPMO", 0);
		break;
	}
}*/



void vrtMap::playPMOperation(void)
{
	vMap.procMapOp=0;
	flag_record_operation=false;
	//ContainerPMO_Old::iterator p;
	//for(p=containerPMO_Old.begin(); (p!=containerPMO_Old.end()) && (p!=getCurNewPMO()); p++){
	//	ElementPMO& o = *p;
	//	switch(o.basePMO.pmoID){
	//	case PMO_ID_NONE:
	//		break;
	//	case PMO_ID_TOOLZER:
	//		deltaZone(o.toolzerPMO);
	//		break;
	//	case PMO_ID_SQUARE_TOOLZER: 
	//		squareDeltaZone(o.squareToolzerPMO);
	//		break;
	//	case PMO_ID_GEO:
	//		geoGeneration(o.geoPMO);
	//		break;
	//	case PMO_ID_BLUR:
	//		gaussFilter(o.blurPMO.x, o.blurPMO.y, o.blurPMO.rad, o.blurPMO.intensity);
	//		break;
	//	case PMO_ID_COLORPIC:
	//		drawBitmapCircle(o.colorPicPMO.x, o.colorPicPMO.y, o.colorPicPMO.rad, o.colorPicPMO.cntrAlpha, o.colorPicPMO.bitmapIDX);
	//		break;
	//	case PMO_ID_BITGEN:
	//		switch(o.bitGenPMO.bitGenMetodID){
	//		case BGMID_EXP:
	//			drawBitMetod(o.bitGenPMO.x, o.bitGenPMO.y, o.bitGenPMO.rad, sBitGenMetodExp(o.bitGenPMO.expPower, o.bitGenPMO.maxHeight, o.bitGenPMO.rndVal), o.bitGenPMO.filterHeigh);
	//			break;
	//		case BGMID_PN:
	//			drawBitMetod(o.bitGenPMO.x, o.bitGenPMO.y, o.bitGenPMO.rad, sBitGenMetodPN(o.bitGenPMO.expPower, o.bitGenPMO.maxHeight, o.bitGenPMO.rndVal), o.bitGenPMO.filterHeigh);
	//			break;
	//		case BGMID_MPD:
	//			drawBitMetod(o.bitGenPMO.x, o.bitGenPMO.y, o.bitGenPMO.rad, sBitGenMetodMPD(o.bitGenPMO.expPower, o.bitGenPMO.maxHeight, o.bitGenPMO.rndVal, o.bitGenPMO.kRoughness), o.bitGenPMO.filterHeigh);
	//			break;

	//		}
	//		//drawExp3(o.bitGenExp3PMO.x, o.bitGenExp3PMO.y, o.bitGenExp3PMO.rad, o.bitGenExp3PMO.maxDHeigh, o.bitGenExp3PMO.filterHeigh);
	//		break;
	//	}
	//}
	ContainerPMO::iterator p;
	for(p=containerPMO.begin(); (p!=containerPMO.end()) && (p!=getCurNewPMO()); p++){
		(*p)->actOn();
	}

	flag_record_operation=true;
}

void sToolzerPMO::actOn()
{
	//if(dh)
    vMap.deltaZone(*this);
}

void sSquareToolzerPMO::actOn()
{
	vMap.squareDeltaZone(*this);
}

void sGeoPMO::actOn()
{
	geoGeneration(*this);
}

void sBlurPMO::actOn()
{
	vMap.gaussFilter(x, y, rad, intensity);
}

void sColorPicPMO::actOn()
{
	vMap.drawBitmapCircle(x, y, rad, cntrAlpha, bitmapIDX, minFH, maxFH);
}

void sBitGenPMO::actOn()
{
	switch(bitGenMetodID){
	case BGMID_EXP:
		vMap.drawBitMetod(x, y, rad, sBitGenMetodExp(expPower, maxHeight, rndVal), filterHeigh);
		break;
	case BGMID_PN:
		vMap.drawBitMetod(x, y, rad, sBitGenMetodPN(expPower, maxHeight, rndVal), filterHeigh);
		break;
	case BGMID_MPD:
		vMap.drawBitMetod(x, y, rad, sBitGenMetodMPD(expPower, maxHeight, rndVal, kRoughness), filterHeigh);
		break;
	}
}

sRoadPMO::sRoadPMO(const list<struct sNode>& nodeArr, const string& _bitmapFileName, const string& _bitmapVolFileName, const string& _edgeBitmapFName, const string& _edgeVolBitmapFName, bool _flag_onlyTextured, eTexturingMetod _texturingMetod, ePutMetod _putMetod)
{
	pmoID=PMO_ID_ROAD;
	bitmapFileName=_bitmapFileName;
	bitmapVolFileName=_bitmapVolFileName;
	edgeBitmapFName=_edgeBitmapFName;
	edgeVolBitmapFName=_edgeVolBitmapFName;
	flag_onlyTextured=_flag_onlyTextured;
	nodePrimaryDataArr.reserve(nodeArr.size());
	list<sNode>::const_iterator p;
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++){
		nodePrimaryDataArr.push_back((sNodePrimaryData)(*p));
	}
	texturingMetod=_texturingMetod;
	putMetod=_putMetod;
}

void sRoadPMO::actOn()
{
	RoadTool roadTool;
	roadTool.init(nodePrimaryDataArr, bitmapFileName, bitmapVolFileName, edgeBitmapFName, edgeVolBitmapFName, texturingMetod, putMetod);
	roadTool.buildRoad(flag_onlyTextured);
}
