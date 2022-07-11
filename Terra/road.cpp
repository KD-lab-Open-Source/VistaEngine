#include "stdafxTr.h"

#include "Serialization.h"
#include "XPrmArchive.h"

#include "..\UTIL\DebugUtil.h"
#include "road.h"

const float DEFAULT_EDGE_WIDTH=20.f;

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(sRoadPMO, eTexturingMetod, "TexturingMetod")
REGISTER_ENUM_ENCLOSED(sRoadPMO, TM_AlignWidthAndHeight, "AlignWidthAndHeight")
REGISTER_ENUM_ENCLOSED(sRoadPMO, TM_AlignOnlyWidth, "AlignOnlyWidth")
REGISTER_ENUM_ENCLOSED(sRoadPMO, TM_1to1, "1to1")
END_ENUM_DESCRIPTOR_ENCLOSED(sRoadPMO, eTexturingMetod)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(sRoadPMO, ePutMetod, "PutMetod")
REGISTER_ENUM_ENCLOSED(sRoadPMO, PM_ByMaxHeight, "ByMaxHeight")
REGISTER_ENUM_ENCLOSED(sRoadPMO, PM_ByRoadHeight, "ByRoadHeight")
END_ENUM_DESCRIPTOR_ENCLOSED(sRoadPMO, ePutMetod)

RoadTool::RoadTool()
{
	texturingMetod=sRoadPMO::TM_AlignWidthAndHeight;
	putMetod=sRoadPMO::PM_ByMaxHeight;
}

void sNode::serialize(Archive& ar)
{
	ar.serialize(pos, "pos", 0);
	ar.serialize(width, "width", 0);
	ar.serialize(angleOrWidtEdge, "angleOrWidtEdge", 0);
}

void RoadTool::serialize(Archive& ar)
{
	ar.serialize(nodeArr, "nodeArr", 0);
	ar.serialize(bitmapFileName, "bitmapFileName", 0);
	ar.serialize(bitmapVolFileName, "bitmapVolFileName", 0);
	ar.serialize(edgeBitmapFName, "edgeBitmapFName", 0);
	ar.serialize(edgeVolBitmapFName, "edgeVolBitmapFName", 0);
	ar.serialize(texturingMetod, "texturingMetod", 0);
	ar.serialize(putMetod, "putMetod", 0);
}

bool RoadTool::addNode(const Vect3f& worldCoord, const Vect2i& scrCoord, int roadWidth, float edgeAngle)
{
	nodeArr.push_back(sNode(worldCoord, roadWidth, edgeAngle));
	return 0;
}


void RoadTool::init(const vector<sNodePrimaryData>& primaryNodeArr, const string& _bitmapFileName, const string& _bitmapVolFileName, const string& _edgeBitmapFName, const string& _edgeVolBitmapFName, sRoadPMO::eTexturingMetod _texturingMetod, sRoadPMO::ePutMetod _putMetod)
{
	bitmapFileName=_bitmapFileName;
	bitmapVolFileName=_bitmapVolFileName;
	edgeBitmapFName=_edgeBitmapFName;
	edgeVolBitmapFName=_edgeVolBitmapFName;
	//nodeArr.reserve(primaryNodeArr.size());
	vector<sNodePrimaryData>::const_iterator p;
	for(p=primaryNodeArr.begin(); p!=primaryNodeArr.end(); p++){
		nodeArr.push_back((sNode)(*p));
	}
	texturingMetod=_texturingMetod;
	putMetod=_putMetod;
}

static int crc32TMP1;
static int crc32TMP2;
static int crc32TMP3;
static int crc32TMP4;
static int vrtxICntrs;
void RoadTool::buildRoad(bool flag_onlyTextured)
{
	crc32TMP1=startCRC32;
	crc32TMP2=startCRC32;
	crc32TMP3=startCRC32;
	crc32TMP4=startCRC32;
	vrtxICntrs=0;

	TypeNodeIterator p;
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++)
		recalcNodeOrientation(p);
	list<sRect> chArLst;
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++){
		sRect result=recalcSpline(p);
		if(!result.isEmpty()) chArLst.push_back(result);
	}
	if(vMap.flag_record_operation){
		sRoadPMO* pRoadPMO=new sRoadPMO(nodeArr, bitmapFileName, bitmapVolFileName, edgeBitmapFName, edgeVolBitmapFName, flag_onlyTextured, texturingMetod, putMetod);
		//pRoadPMO->bitmapFileName=bitmapFileName;
		//pRoadPMO->bitmapVolFileName=bitmapVolFileName;
		//pRoadPMO->edgeBitmapFName=edgeBitmapFName;
		//pRoadPMO->edgeVolBitmapFName=edgeVolBitmapFName;
		//pRoadPMO->nodePrimaryDataArr.reserve(nodeArr.size());
		//pRoadPMO->flag_onlyTextured=flag_onlyTextured;
		//for(TypeNodeIterator p=nodeArr.begin(); p!=nodeArr.end(); p++){
		//	pRoadPMO->nodePrimaryDataArr.push_back((sNodePrimaryData)(*p));
		//}
		vMap.UndoDispatcher_PutPreChangedArea(chArLst, 1,1, pRoadPMO);
	}
	if(putMetod==sRoadPMO::PM_ByRoadHeight)
		for(p=nodeArr.begin(); p!=nodeArr.end(); p++){
			getRoadLenghtAndRecalcNodeLengt();
			putSpline(p, 1, flag_onlyTextured); //1-pass
		}
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++){
		if(putMetod!=sRoadPMO::PM_ByRoadHeight)
			getRoadLenghtAndRecalcNodeLengt();
		putSpline(p,0, flag_onlyTextured); //1-pass
	}
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++)
		putSpline(p,2, flag_onlyTextured); //2-pass
}

void RoadTool::recalcHeightNodeLinear()
{
	float lenght=getRoadLenghtAndRecalcNodeLengt();
	double k;
	if(lenght > FLT_EPS)
		k=(nodeArr.back().pos.z-nodeArr.front().pos.z)/lenght;
	else k=0;
	TypeNodeIterator p;
	p=nodeArr.begin();
	++p;
	lenght=0;
	for(p; p!=nodeArr.end(); ++p){
		lenght+=p->lenght2Prev;
		p->pos.z=nodeArr.front().pos.z+k*lenght;
	}
}

void RoadTool::recalcHeightNodeParabolic(int maxSphericHeight)
{
	Vect3f midPnt=nodeArr.front().pos + (nodeArr.back().pos-nodeArr.front().pos)/2;
	float L=nodeArr.front().pos.distance(nodeArr.back().pos);
	float h=(float)(maxSphericHeight);//*(1<<VX_FRACTION);
	float R=(L*L + 4*h*h)/(8*h);
	//QuatF Vect3f
	Vect3f firstV=nodeArr.front().pos-midPnt;
	Vect3f rotate90V(firstV.y, -firstV.x, 0);//повернутый на 90градусов
	Vect3f vect2centerSph;
	vect2centerSph.cross(firstV, rotate90V);
	vect2centerSph.Normalize();
	vect2centerSph*=(R-h);
	Vect3f centerSphers=midPnt+vect2centerSph;

	//Для окружность
	Vect3f N;
	N.cross(nodeArr.front().pos-centerSphers, nodeArr.back().pos-centerSphers);

	TypeNodeIterator p;
	p=nodeArr.begin();
	++p;
	for(p; p!=nodeArr.end(); ++p){
		float z, z2;
		//Для Честная сфера
		//Vect3f curPnt=p->pos-centerSphers;
		//Для окружность
		Vect3f cp=p->pos-centerSphers;
		float x,y;
		x = (-N.y*N.x*cp.y + sqr(N.y)*cp.x - N.z*N.x*cp.z + sqr(N.z)*cp.x)/(sqr(N.x)+sqr(N.y)+sqr(N.z));
		y = (sqr(N.x)*cp.y - N.z*N.y*cp.z + sqr(N.z)*cp.y - N.x*N.y*cp.x)/(sqr(N.x)+sqr(N.y)+sqr(N.z));
		Vect3f curPnt(x,y,0);

		z2 = R*R - sqr(curPnt.x) - sqr(curPnt.y);
		if(z2 > 0) z=sqrtf(z2); 
		else z=0;
		z=z+centerSphers.z;
		if(z < 0) z=0;
		if(z > MAX_VX_HEIGHT_WHOLE) z=MAX_VX_HEIGHT_WHOLE;
		p->pos.z=z;
	}

}



void RoadTool::recalcNodeOrientation(TypeNodeIterator ni)
{
	if(nodeArr.empty()) return;
	TypeNodeIterator prevni=ni;
	Vect2f normal=Vect2f::ZERO;
	if(prevni!=nodeArr.begin()){
		prevni--;
		normal=Vect2f(ni->pos.y - prevni->pos.y, prevni->pos.x - ni->pos.x );
	}
	TypeNodeIterator nextni=ni;
	nextni++;
	if(nextni!=nodeArr.end()){
		normal+=Vect2f(nextni->pos.y - ni->pos.y, ni->pos.x - nextni->pos.x);
	}
	if(normal.norm2() > FLT_EPS){
        normal.normalize(1.f);
		ni->leftPnt =Vect2f(ni->pos) - normal*ni->width;
		ni->rightPnt=Vect2f(ni->pos) + normal*ni->width;
		//float ctan=sin(ni->angleOrWidtEdge)/cos(ni->angleOrWidtEdge);
		//float fullWidth=ni->width + ni->pos.z*ctan;
		//ni->leftPntEdge = Vect2f(ni->pos) - normal*fullWidth;
		//ni->rightPntEdge= Vect2f(ni->pos) + normal*fullWidth;
	}
	else{
		ni->leftPnt =Vect2f(ni->pos);
		ni->rightPnt=Vect2f(ni->pos);
		//ni->leftPntEdge = Vect2f(ni->pos);
		//ni->rightPntEdge= Vect2f(ni->pos);
	}

}

void RoadTool::changeAllRoadWidthAndAngle(int newWidth, float newAngle)
{
	TypeNodeIterator p;
	for(p=nodeArr.begin(); p!=nodeArr.end(); ++p){
		p->width=newWidth;
		p->angleOrWidtEdge=newAngle;
	}
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++)
		recalcNodeOrientation(p);
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++)
		recalcSpline(p);
}

float RoadTool::getRoadLenghtAndRecalcNodeLengt()
{
	float lenght=0;
	if(!nodeArr.empty()){
		nodeArr.front().lenght2Prev=0;
		TypeNodeIterator m,p;
		p=m=nodeArr.begin();
		++p;
		for(p; p!=nodeArr.end(); ++p, ++m){
			p->lenght2Prev= p->pos.distance(m->pos);// p->pos.distance(m->pos);
			lenght+=p->lenght2Prev;
		}
	}
	return lenght;
}


sRect RoadTool::recalcSpline(TypeNodeIterator ni)
{
	sRect retval;

	ni->spline2NextLeft.clear();
	ni->spline2NextRight.clear();
	ni->spline2NextLeftEdge.clear();
	ni->spline2NextRightEdge.clear();
	ni->spline2NextSectionLenght.clear();
	ni->spline2Next.clear();

	TypeNodeIterator begi=ni;
	TypeNodeIterator endi=begi;
	endi++;
	if(endi==nodeArr.end())
		return retval;//У посленей ноды нет сплайна
	Vect3f P1L,P2L,P3L,P4L;
	Vect3f P1R,P2R,P3R,P4R;
	//Vect2f P1LE,P4LE;
	//Vect2f P1RE,P4RE;
	Vect3f P1,P2,P3,P4;
	P2=begi->pos;
	P3=endi->pos;
	P2L=Vect3f(begi->leftPnt.x, begi->leftPnt.y, begi->pos.z);
	P2R=Vect3f(begi->rightPnt.x, begi->rightPnt.y, begi->pos.z);
	P3L=Vect3f(endi->leftPnt.x, endi->leftPnt.y, endi->pos.z);
	P3R=Vect3f(endi->rightPnt.x, endi->rightPnt.y, endi->pos.z);
	//Vect2f& P2LE=begi->leftPntEdge;
	//Vect2f& P3LE=endi->leftPntEdge;
	//Vect2f& P2RE=begi->rightPntEdge;
	//Vect2f& P3RE=endi->rightPntEdge;
	const float MAX_K_DIST=30.f; //Коэффициент расстояния после которого перестает действовать HermitSpline
	if(begi!=nodeArr.begin()){
		begi--;
		P1=begi->pos;
		float kDist=P2.distance2(P1)/max(1.f, P2.distance2(P3));
		if(kDist > MAX_K_DIST){
			kDist = MAX_K_DIST;
			Vect3f t=P1-P2;
			t.Normalize();
			P1=P2-t*kDist;
		}
		P1L=Vect3f(begi->leftPnt.x, begi->leftPnt.y, begi->pos.z);
		P1R=Vect3f(begi->rightPnt.x, begi->rightPnt.y, begi->pos.z);
		//P1LE=begi->leftPntEdge;
		//P1RE=begi->rightPntEdge;
		begi++; //Используется дальше
	}
	else {
		P1=P2;
		P1L=P2L;
		P1R=P2R;
		//P1LE=P2LE;
		//P1RE=P2RE;
	}
	endi++;
	if(endi!=nodeArr.end()){
		P4=endi->pos;
		float kDist=P3.distance2(P4)/max(1.f, P3.distance2(P2));
		if(kDist > MAX_K_DIST){
			kDist = MAX_K_DIST;
			Vect3f t=P4-P3;
			t.Normalize();
			P4=P2-t*kDist;
		}
		P4L=Vect3f(endi->leftPnt.x, endi->leftPnt.y, endi->pos.z);
		P4R=Vect3f(endi->rightPnt.x, endi->rightPnt.y, endi->pos.z);
		//P4LE=endi->leftPntEdge;
		//P4RE=endi->rightPntEdge;
	}
	else {
		P4=P3;
		P4L=P3L;
		P4R=P3R;
		//P4LE=P3LE;
		//P4RE=P3RE;
	}
	endi--; // //Используется дальше

	//int lenghL=round(Vect2f(P2L).distance(Vect2f(P3L)));
	//int lenghR=round(Vect2f(P2R).distance(Vect2f(P3R)));
	//int lenght=lenghL > lenghR ? lenghL : lenghR;
	//lenght/=2;//для более гладких полигонов
	int lenght=round(Vect2f(endi->pos).distance(Vect2f(begi->pos)));
	lenght/=4;
	float dt;
	if(lenght)
		dt=1.f/(float)lenght;
	else dt=0;

	ni->spline2NextLeft.clear();
	ni->spline2NextRight.clear();
	ni->spline2NextLeftEdge.clear();
	ni->spline2NextRightEdge.clear();
	ni->spline2NextSectionLenght.clear();
	ni->spline2Next.clear();

	ni->lenght2Next=0;
	if(lenght){
		ni->spline2NextLeft.reserve(lenght+1);
		ni->spline2NextRight.reserve(lenght+1);
		ni->spline2NextLeftEdge.reserve(lenght+1);
		ni->spline2NextRightEdge.reserve(lenght+1);
		ni->spline2NextSectionLenght.reserve(lenght+1);
		ni->spline2Next.reserve(lenght+1);


		const float MIN_LENGHT_SECTION=1.f;
		float t=0;
		int i;
		for(i=0; i<=lenght; i++){
			float curWidth= begi->width + (float)(endi->width - begi->width)*i/lenght;
			float curAngleOrWidth=begi->angleOrWidtEdge + (float)(endi->angleOrWidtEdge - begi->angleOrWidtEdge)*i/lenght;
			Vect3f pnt=HermitSpline(t, P1, P2,P3,P4);
			Vect3f pl=HermitSpline(t, P1L,P2L,P3L,P4L);
			Vect3f pr=HermitSpline(t, P1R,P2R,P3R,P4R);
			//pl.z*=(1<<VX_FRACTION);
			//pr.z*=(1<<VX_FRACTION);
			//ni->spline2NextLeft.push_back(pl);
			//ni->spline2NextRight.push_back(pr);
			Vect2f normal=Vect2f::ZERO;
			Vect3f prevpnt=pnt;
			if(i!=0){
				prevpnt=HermitSpline(t-dt, P1, P2,P3,P4);
				normal=Vect2f(pnt.y - prevpnt.y, prevpnt.x - pnt.x );
			}
			//if(i!=lenght-1){
				Vect3f nextpnt=HermitSpline(t+dt, P1, P2,P3,P4);
				normal+=Vect2f(nextpnt.y - pnt.y, pnt.x - nextpnt.x);
			//}
			float lenghtSection=Vect2f(nextpnt).distance(Vect2f(pnt));
			//if(lenghtSection < MIN_LENGHT_SECTION ) continue;
			ni->lenght2Next+=lenghtSection;
			ni->spline2NextSectionLenght.push_back(lenghtSection);
			ni->spline2Next.push_back(pnt);
			if(normal.norm2() > FLT_EPS){
				normal.normalize(1.f);
				ni->spline2NextLeft.push_back(to3D(Vect2f(pnt) - normal*curWidth, pnt.z));
				retval.addBound(Vect2i(ni->spline2NextLeft.back()));
				ni->spline2NextRight.push_back(to3D(Vect2f(pnt) + normal*curWidth, pnt.z));
				retval.addBound(Vect2i(ni->spline2NextRight.back()));
				float fullWidth=curWidth;
				float lez=0, rez=0;
				Vect2f le,re;
				if(putMetod==sRoadPMO::PM_ByMaxHeight){
					fullWidth += pnt.z * (sin(curAngleOrWidth)/cos(curAngleOrWidth)); //pnt.z*ctan(curAngle)
					le=Vect2f(pnt) - normal*fullWidth;
					re=Vect2f(pnt) + normal*fullWidth;
				}
				else if(putMetod==sRoadPMO::PM_ByRoadHeight){
					fullWidth += curAngleOrWidth*180.f/M_PI;//DEFAULT_EDGE_WIDTH;
					le=Vect2f(pnt) - normal*fullWidth;
					re=Vect2f(pnt) + normal*fullWidth;
					//lez=vMap.GetApproxAlt(round(le.x),round(le.y));
					//rez=vMap.GetApproxAlt(round(re.x),round(re.y));
					lez=vMap.GetAltC(round(le.x),round(le.y))*VOXEL_DIVIDER;
					rez=vMap.GetAltC(round(re.x),round(re.y))*VOXEL_DIVIDER;
				}
				ni->spline2NextLeftEdge.push_back( Vect3f(le,lez) );
				retval.addBound(Vect2i(le));
				ni->spline2NextRightEdge.push_back( Vect3f(re, rez) );
				retval.addBound(Vect2i(re));
			}
			else {
				ni->spline2NextLeft.push_back(pnt);
				ni->spline2NextRight.push_back(pnt);
				ni->spline2NextLeftEdge.push_back( pnt );
				ni->spline2NextRightEdge.push_back( pnt );
				retval.addBound(Vect2i(pnt));
			}

			//Vect2f ple=HermitSpline(t, P1LE,P2LE,P3LE,P4LE);
			//Vect2f pre=HermitSpline(t, P1RE,P2RE,P3RE,P4RE);
			//ni->spline2NextLeftEdge.push_back(ple);
			//ni->spline2NextRightEdge.push_back(pre);
			t+=dt;
		}
		ni->spline2NextLeft.pop_back();
		ni->spline2NextRight.pop_back();
		ni->spline2NextLeftEdge.pop_back();
		ni->spline2NextRightEdge.pop_back();
	}

	return retval;
}

void alphaBlending(unsigned long& src, unsigned long& dst)
{
	short alpha=src>>24;
	short alpha2=255-alpha;
	unsigned char r1,g1,b1;
	unsigned char r2,g2,b2;
	r1=*((unsigned char*)(&src)+2);
	g1=*((unsigned char*)(&src)+1);
	b1=*((unsigned char*)(&src)+0);
	r2=*((unsigned char*)(&dst)+2);
	g2=*((unsigned char*)(&dst)+1);
	b2=*((unsigned char*)(&dst)+0);

	short r= ( r1*alpha + r2*alpha2) >>8;
	short g= ( g1*alpha + g2*alpha2) >>8;
	short b= ( b1*alpha + b2*alpha2) >>8;

	//return (r<<16) | (g<<8) | b;
	dst=(r<<16) | (g<<8) | b;
}


#define SUBPIXEL
#define SUBTEXEL
///#define roundFIntF0(a) (((a)+(1<<15)>>16)
//#define roundFIntF0(a) ((a)>=0 ? (a)>>16 : (-((-a)>>16)) )
#define roundFIntF0(a) ((a)>>16)

#define accurRoundFIntF0(a) ((a)>=0 ? ((a)+(1<<15))>>16 : (-(((-a)+(1<<15))>>16)) )
const unsigned char PRECISION_FINT=16;
const unsigned char PRECISION05_FINT=8;
#define cvrtFIntF8(a) ((a+(1<<7))>>8)
///#define ceilFIntF16(a) (a>0 ? ((a+0xffFF)&0xFFff0000) : ((a|0xffFF)+1) )
///#define ceilFIntF0(a) ((a+0xffFF)>>16)
//#define ceilFIntF16(a) ((a)>=0 ? (((a)+0xffFF)&0xFFff0000) : (-((-(a)+0xffFF)&0xFFff0000)) )
//#define ceilFIntF0(a) ((a)>=0 ? (((a)+0xffFF)>>16) : (-((-(a)+0xffFF)>>16)) )
#define ceilFIntF16(a) (((a)+0xffFF)&0xFFff0000) 
#define ceilFIntF0(a) (((a)+0xffFF)>>16)

void RoadTool::putStrip(vector<sPolygon>& poligonArr, vector<VertexI> iPntArr, const BitmapDispatcher::Bitmap* texture, const BitmapDispatcher::Bitmap* vtexture, int numPass, bool flag_onlyTextured )
{
	xassert(vMap.SupBuf);
	unsigned long* pBitmap=0;
	Vect2i bitmapSize=Vect2i(1,1);
	bool flag_AlphaChanelPresent=false;
	if(texture){
		pBitmap=texture->bitmap;
		bitmapSize=texture->size;
		flag_AlphaChanelPresent=texture->flag_AlphaChanelPresent;
	}
	unsigned long* pBitmapV=0;
	Vect2i bitmapSizeV=Vect2i(1,1);
	if(vtexture){
		pBitmapV=vtexture->bitmap;
		bitmapSizeV=vtexture->size;
	}

	int iminX=0;
	int iminY=0;
	int imaxX=vMap.H_SIZE;
	int imaxY=vMap.V_SIZE;

	vector<sPolygon>::iterator p;
	for(p=poligonArr.begin(); p!=poligonArr.end(); p++){

		const VertexI* a = &iPntArr[p->p1]; // Для сортировки по Y.
		const VertexI* b = &iPntArr[p->p2];
		const VertexI* c = &iPntArr[p->p3];

		if(a->y > b->y) swap(a, b);
		if(a->y > c->y) swap(a, c);
		if(b->y > c->y) swap(b, c);

/*		//new
		int x_start, x_end;
		int dx_start, dx_end, dz1_start, dz1_end;
		int z1_start, z1_end;
		int z1, dz1;
		int u_start, u_end, v_start, v_end;
		int du_start, du_end, dv_start, dv_end;
		int u, v, du, dv;

		// считаем по самой длинной линии (т.е. проходящей через вершину B)
		double k;
		int divisor;
		divisor=(c->y - a->y);
		if(accurRoundFIntF0(divisor)) k = ((double)(b->y - a->y)) / (double)divisor;
		else k=0;
		x_start = a->x  + round((double)(c->x - a->x)*k);
		z1_start = a->z + round((double)(c->z - a->z)*k);
		u_start = a->u + round((double)(c->u - a->u)*k);
		v_start = a->v + round((double)(c->v - a->v)*k);
		x_end = b->x;
		z1_end = b->z;
		u_end = b->u;
		v_end = b->v;
		divisor= x_start - x_end;
		if(accurRoundFIntF0(divisor)) {
			dz1 = round((double)(z1_start - z1_end)/(double)divisor *(double)(1<<16));
			du = round((double)(u_start - u_end)/(double)divisor *(double)(1<<16));
			dv = round((double)(v_start - v_end)/(double)divisor *(double)(1<<16));
		}
		else { dz1=0; du=0; dv=0; }

		x_start = a->x;
		z1_start = a->z;
		u_start = a->u;
		v_start = a->v;
		divisor=(c->y - a->y);
		if(accurRoundFIntF0(divisor)){
			dx_start = round((double)(c->x - a->x) / (double)divisor *(double)(1<<16)); 
			dz1_start = round((double)(c->z - a->z) / (double)divisor *(double)(1<<16)); 
			du_start = round((double)(c->u - a->u) / (double)divisor *(double)(1<<16)); 
			dv_start = round((double)(c->v - a->v) / (double)divisor *(double)(1<<16)); 
		}
		else { dx_start=0; dz1_start=0; du_start=0; dv_start=0; }
#ifdef SUBPIXEL
		int tmp;
		tmp = (accurRoundFIntF0(a->y)<<16) - a->y;
		x_start += (dx_start>>8) * (tmp>>8); //Норма
		z1_start += (dz1_start>>8) * (tmp>>8); //Норма
		u_start += (du_start>>8) * (tmp>>8); //Норма
		v_start += (dv_start>>8) * (tmp>>8); //Норма
#endif

		int current_sx, current_sy;
		//1-я часть полигона
		if(accurRoundFIntF0(b->y) - accurRoundFIntF0(a->y) ){
			x_end = a->x;
			z1_end = a->z;
			u_end = a->u;
			v_end = a->v;
			divisor= b->y - a->y;
			if(accurRoundFIntF0(divisor)){ 
				dx_end = round((double)(b->x - a->x) / (double)divisor *(double)(1<<16));
                dz1_end = round((double)(b->z - a->z) / (double)divisor *(double)(1<<16));
				du_end = round((double)(b->u - a->u) / (double)divisor *(double)(1<<16));
				dv_end = round((double)(b->v - a->v) / (double)divisor *(double)(1<<16));
			}
			else {dx_end =0; dz1_end =0; du_end=0; dv_end=0; }
#ifdef SUBPIXEL
			tmp = (accurRoundFIntF0(a->y)<<16) - (a->y);
			x_end += (dx_end>>8) * (tmp>>8); //Норма
			z1_end += (dz1_end>>8) * (tmp>>8); //Норма
			u_end += (du_end>>8) * (tmp>>8); //Норма
			v_end += (dv_end>>8) * (tmp>>8); //Норма
#endif
			for (current_sy = accurRoundFIntF0(a->y); current_sy < accurRoundFIntF0(b->y); current_sy++) {
				if((current_sy) >= imaxY ) break; ///!!!!
				// x_start должен находиться левее x_end
				//if(x_start > curMaxX) x_start=curMaxX;
				//if(x_end > curMaxX) x_end=curMaxX;
				//if(x_start < curMinX) x_start=curMinX;
				//if(x_end < curMinX) x_end=curMinX;
				int ceilxend;
				if (x_start > x_end) {
					current_sx = accurRoundFIntF0(x_end);
					z1 = z1_end;
					u = u_end;
					v = v_end;
					ceilxend=accurRoundFIntF0(x_start);
				} else {
					current_sx = accurRoundFIntF0(x_start);
					z1 = z1_start;
					u = u_start;
					v = v_start;
					ceilxend=accurRoundFIntF0(x_end);
				}
#ifdef SUBTEXEL
				//tmp = ceil(x) - x;
				int tmp = (accurRoundFIntF0(current_sx)<<16) - current_sx;
				z1 += (dz1>>8)* (tmp>>8);
				u += (du>>8)* (tmp>>8);
				v += (dv>>8)* (tmp>>8);
#endif
				// текстурируем строку
				if((current_sy) >= 0 ) {
					while(current_sx <= ceilxend) {
					// используем z-буфер для определения видимости текущей точки
						if( (current_sx<imaxX) && (current_sx >= 0)) {
							register int bufoff=vMap.offsetBuf(current_sx,current_sy);//(current_sy)*voxelBitmap.sx + current_sx;
							if(flag_onlyTextured){
								int vox=vMap.GetAlt(bufoff);
								if(pBitmap){
									if(v>=0 && u >=0 ){
										unsigned int offBitmap=(accurRoundFIntF0(u))%bitmapSize.x+(accurRoundFIntF0(v))%bitmapSize.y*bitmapSize.x;
										vMap.SetAtr(bufoff, VmAt_Nrml_Dam);
										vMap.SupBuf[bufoff]= pBitmap[offBitmap];
									}
									//else vMap.SupBuf[bufoff]=0xffFFff;
								}
								if(pBitmapV){
									if(v>=0 && u >=0 ){
										unsigned int offBitmap=(accurRoundFIntF0(u))%bitmapSize.x+(accurRoundFIntF0(v))%bitmapSize.y*bitmapSize.x;
										vox+= (*((unsigned char*)(&pBitmapV[offBitmap])+2)+ *((unsigned char*)(&pBitmapV[offBitmap]+1)) + *((unsigned char*)(&pBitmapV[offBitmap])) )>>2;
										if(vox>MAX_VX_HEIGHT)
											vox=MAX_VX_HEIGHT;
									}
								}
								vMap.PutAlt(bufoff,vox);
							}
							else if( vMap.GetAlt(bufoff) <= (z1+(1<<(15-VX_FRACTION)))>>(16-VX_FRACTION)){ // (voxelBitmap.pRaster[bufoff]<= z1>>(16-VX_FRACTION) )
								//voxelBitmap.pRaster[bufoff] = z1>>(16-VX_FRACTION);
								int vox=(z1+(1<<(15-VX_FRACTION)))>>(16-VX_FRACTION);
								if(pBitmap){
									if(v>=0 && u >=0 ){
										unsigned int offBitmap=(accurRoundFIntF0(u))%bitmapSize.x+(accurRoundFIntF0(v))%bitmapSize.y*bitmapSize.x;
										vMap.SetAtr(bufoff, VmAt_Nrml_Dam);
										vMap.SupBuf[bufoff]= pBitmap[offBitmap];
									}
									//else vMap.SupBuf[bufoff]=0xffFFff;
								}
								if(pBitmapV){
									if(v>=0 && u >=0 ){
										unsigned int offBitmap=(accurRoundFIntF0(u))%bitmapSize.x+(accurRoundFIntF0(v))%bitmapSize.y*bitmapSize.x;
										vox+= (*((unsigned char*)(&pBitmapV[offBitmap])+2)+ *((unsigned char*)(&pBitmapV[offBitmap]+1)) + *((unsigned char*)(&pBitmapV[offBitmap])) )>>2;
										if(vox>MAX_VX_HEIGHT)
											vox=MAX_VX_HEIGHT;
									}
								}
								vMap.PutAlt(bufoff,vox);
							}
						}
						z1 += dz1;
						u += du;
						v += dv;
						current_sx++;
					}
				}

				// сдвигаем начальные и конечные значения x/u/v/(1/z)
				x_start += dx_start;
				z1_start += dz1_start;
				x_end += dx_end;
				z1_end += dz1_end;
				u_start += du_start;
				v_start += dv_start;
				u_end += du_end;
				v_end += dv_end;
			}
		}

		//2-я часть полигона
		x_end = b->x;
		z1_end = b->z;
		u_end = b->u;
		v_end = b->v;
		divisor= c->y - b->y;
		if(accurRoundFIntF0(divisor)){
			dx_end = round((double)(c->x - b->x) / (double)divisor *(double)(1<<16));
            dz1_end = round((double)(c->z - b->z) / (double)divisor *(double)(1<<16));
			du_end = round((double)(c->u - b->u) / (double)divisor *(double)(1<<16));
			dv_end = round((double)(c->v - b->v) / (double)divisor *(double)(1<<16));
		}
		else{ dx_end=0; dz1_end=0; du_end=0; dv_end=0; }
#ifdef SUBPIXEL
		tmp = (accurRoundFIntF0(b->y)<<16) - (b->y);
		x_end += (dx_end>>8) * (tmp>>8); //Норма
		z1_end += (dz1_end>>8) * (tmp>>8); //Норма
		u_end += (du_end>>8) * (tmp>>8); //Норма
		v_end += (dv_end>>8) * (tmp>>8); //Норма
#endif
		// построчная отрисовка грани
		for (current_sy = accurRoundFIntF0(b->y); current_sy <= accurRoundFIntF0(c->y); current_sy++) {
			if((current_sy) >= imaxY ) break;
			// x_start должен находиться левее x_end
			//if(x_start > curMaxX) x_start=curMaxX;
			//if(x_end > curMaxX) x_end=curMaxX;
			//if(x_start < curMinX) x_start=curMinX;
			//if(x_end < curMinX) x_end=curMinX;
			int ceilxend;
			if (x_start > x_end) {
				current_sx = accurRoundFIntF0(x_end);
				z1 = z1_end;
				u = u_end;
				v = v_end;
				ceilxend=accurRoundFIntF0(x_start);
			} else {
				current_sx = accurRoundFIntF0(x_start);
				z1 = z1_start;
				u = u_start;
				v = v_start;
				ceilxend=accurRoundFIntF0(x_end);
			}
#ifdef SUBTEXEL
			//tmp = ceil(x) - x;
			int tmp = (accurRoundFIntF0(current_sx)<<16) - current_sx;
			z1 += (dz1>>8)* (tmp>>8);
			u += (du>>8)* (tmp>>8);
			v += (dv>>8)* (tmp>>8);
#endif
			// текстурируем строку
			if((current_sy) >= 0 ) {
				while(current_sx <= ceilxend) {
					// используем z-буфер для определения видимости текущей точки
					if( (current_sx<imaxX) && (current_sx >= 0)) {
						register int bufoff=vMap.offsetBuf(current_sx,current_sy);//(current_sy)*voxelBitmap.sx + current_sx;
						if(flag_onlyTextured){
							int vox=vMap.GetAlt(bufoff);
							if(pBitmap){
								if(v>=0 && u >=0 ){
									unsigned int offBitmap=(accurRoundFIntF0(u))%bitmapSize.x+(accurRoundFIntF0(v))%bitmapSize.y*bitmapSize.x;
									vMap.SetAtr(bufoff, VmAt_Nrml_Dam);
									vMap.SupBuf[bufoff]= pBitmap[offBitmap];
								}
								//else vMap.SupBuf[bufoff]=0xffFFff;
							}
							if(pBitmapV){
								if(v>=0 && u >=0 ){
									unsigned int offBitmap=(accurRoundFIntF0(u))%bitmapSize.x+(accurRoundFIntF0(v))%bitmapSize.y*bitmapSize.x;
									vox+= (*((unsigned char*)(&pBitmapV[offBitmap])+2)+ *((unsigned char*)(&pBitmapV[offBitmap]+1)) + *((unsigned char*)(&pBitmapV[offBitmap])) )>>2;
									if(vox>MAX_VX_HEIGHT)
										vox=MAX_VX_HEIGHT;
								}
							}
							vMap.PutAlt(bufoff,vox);
						}
						else if( vMap.GetAlt(bufoff) <= (z1+(1<<(15-VX_FRACTION)))>>(16-VX_FRACTION) ){ // (voxelBitmap.pRaster[bufoff]<= z1>>(16-VX_FRACTION) )
							//voxelBitmap.pRaster[bufoff] = z1>>(16-VX_FRACTION);
							int vox=(z1+(1<<(15-VX_FRACTION)))>>(16-VX_FRACTION);
							if(pBitmap){
								if(v>=0 && u >=0 ){
									unsigned int offBitmap=(accurRoundFIntF0(u))%bitmapSize.x+(accurRoundFIntF0(v))%bitmapSize.y*bitmapSize.x;
									vMap.SetAtr(bufoff, VmAt_Nrml_Dam);
									vMap.SupBuf[bufoff]= pBitmap[offBitmap];
								}
								//else vMap.SupBuf[bufoff]=0xffFFff;
							}
							if(pBitmapV){
								if(v>=0 && u >=0 ){
									unsigned int offBitmap=(accurRoundFIntF0(u))%bitmapSize.x+(accurRoundFIntF0(v))%bitmapSize.y*bitmapSize.x;
									vox+= (*((unsigned char*)(&pBitmapV[offBitmap])+2)+ *((unsigned char*)(&pBitmapV[offBitmap]+1)) + *((unsigned char*)(&pBitmapV[offBitmap])) )>>2;
									if(vox>MAX_VX_HEIGHT)
										vox=MAX_VX_HEIGHT;
								}
							}
							vMap.PutAlt(bufoff,vox);
						}
					}
					z1 += dz1;
					u += du;
					v += dv;
					current_sx++;
				}
			}

			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			z1_start += dz1_start;
			x_end += dx_end;
			z1_end += dz1_end;
			u_start += du_start;
			v_start += dv_start;
			u_end += du_end;
			v_end += dv_end;
		}

*/
////////////////////////////////

		//old
		//if(roundFIntF0(c->y - a->y)<=0) continue;
		int curMaxX=a->x;
		if(curMaxX<b->x)curMaxX=b->x;
		if(curMaxX<c->x)curMaxX=c->x;
		int curMinX=a->x;
		if(curMinX>b->x)curMinX=b->x;
		if(curMinX>c->x)curMinX=c->x;
		int curMaxZ=a->z;
		if(curMaxZ<b->z)curMaxZ=b->z;
		if(curMaxZ<c->z)curMaxZ=c->z;

		int current_sx, current_sy;

		//float
		int tmp, k, x_start, x_end;
		int dx_start, dx_end, dz1_start, dz1_end;
		int z1_start, z1_end;
		int x, z1, dz1;
		int u_start, u_end, v_start, v_end;
		int du_start, du_end, dv_start, dv_end;
		int u, v, du, dv;

		int length;
		//unsigned short *dest;

		// посчитаем du/dsx, dv/dsx, d(1/z)/dsx
		// считаем по самой длинной линии (т.е. проходящей через вершину B)
		int divisor;
		//divisor=cvrtFIntF8(c->y - a->y);
		//if(divisor) k = (b->y - a->y) / divisor;
		divisor=(c->y - a->y);
		if(roundFIntF0(divisor)) k = ((__int64)(b->y - a->y)<<8) / divisor;// F8
		else k=0;
		x_start = a->x + (cvrtFIntF8(c->x - a->x))*(k);
		z1_start = a->z + (cvrtFIntF8(c->z - a->z))*(k);
		u_start = a->u + (cvrtFIntF8(c->u - a->u))*(k);
		v_start = a->v + (cvrtFIntF8(c->v - a->v))*(k);
		x_end = b->x;
		z1_end = b->z;
		u_end = b->u;
		v_end = b->v;
		//divisor=cvrtFIntF8(x_start - x_end);
		//if(divisor) dz1 = ((z1_start - z1_end)/divisor) <<8;
		divisor= x_start - x_end;
		if(roundFIntF0(divisor)) {
			dz1 = (((__int64)(z1_start - z1_end)<<16)/divisor);
			du = (((__int64)(u_start - u_end)<<16)/divisor);
			dv = (((__int64)(v_start - v_end)<<16)/divisor);
		}
		else { dz1=0; du=0; dv=0; }
		//int tdivisor=divisor;
		//int tz1_start=z1_start;
		//int tz1_end=z1_end;

		x_start = a->x;
		z1_start = a->z;
		u_start = a->u;
		v_start = a->v;
		//divisor=cvrtFIntF8(c->y - a->y);
		//if(divisor){
		//	dx_start = ((c->x - a->x) / divisor) <<8; 
		//	dz1_start = ((c->z - a->z) / divisor) <<8; 
		//}
		divisor=(c->y - a->y);
		if(roundFIntF0(divisor)){
			dx_start = ((__int64)(c->x - a->x)<<16) / divisor; 
			dz1_start = ((__int64)(c->z - a->z)<<16) / divisor; 
			du_start = ((__int64)(c->u - a->u)<<16) / divisor; 
			dv_start = ((__int64)(c->v - a->v)<<16) / divisor; 
		}
		else { dx_start=0; dz1_start=0; du_start=0; dv_start=0; }
#ifdef SUBPIXEL
		//tmp = ceil(a->sy) - a->sy;
		tmp = ceilFIntF16(a->y) - a->y;
		x_start += (dx_start>>8) * (tmp>>8); //Норма
		z1_start += (dz1_start>>8) * (tmp>>8); //Норма
		u_start += (du_start>>8) * (tmp>>8); //Норма
		v_start += (dv_start>>8) * (tmp>>8); //Норма
#endif

		//if(roundFIntF0(b->y - a->y) ==0) goto loc_scip01;
		if (ceilFIntF16(b->y) > ceilFIntF16(a->y)) {
			//tmp = ceil(a->sy) - a->sy;
			tmp = ceilFIntF16(a->y) - (a->y);
			x_end = a->x;
			z1_end = a->z;
			u_end = a->u;
			v_end = a->v;
			//divisor=cvrtFIntF8(b->y - a->y);
			//if(divisor){ 
			//	dx_end = ((b->x - a->x) / divisor) <<8;
			//	dz1_end = ((b->z - a->z) / divisor) <<8;
			//}
			divisor=b->y - a->y;
			if(roundFIntF0(divisor)){ 
				dx_end = ((__int64)(b->x - a->x)<<16) / divisor;
                dz1_end = ((__int64)(b->z - a->z)<<16) / divisor;
				du_end = ((__int64)(b->u - a->u)<<16) / divisor;
				dv_end = ((__int64)(b->v - a->v)<<16) / divisor;
			}
			else {dx_end =0; dz1_end =0; du_end=0; dv_end=0; }
		} else {
			//tmp = ceil(b->sy) - b->sy;
			tmp = ceilFIntF16(b->y) - b->y;			//???????????
			x_end = b->x;
			z1_end = b->z;
			u_end = b->u;
			v_end = b->v;
			//divisor=cvrtFIntF8(c->y - b->y);
			//if(divisor){
			//	dx_end = ((c->x - b->x) / divisor)<<8;
			//	dz1_end = ((c->z - b->z)) / divisor)<<8;
			//}
			divisor=c->y - b->y;
			if(roundFIntF0(divisor)){
				dx_end = ((__int64)(c->x - b->x)<<16) / divisor;
                dz1_end = ((__int64)(c->z - b->z)<<16) / divisor;
				du_end = ((__int64)(c->u - b->u)<<16) / divisor;
				dv_end = ((__int64)(c->v - b->v)<<16) / divisor;
			}
			else{ dx_end=0; dz1_end=0; du_end=0; dv_end=0; }
		}
#ifdef SUBPIXEL
		x_end += (dx_end>>8) * (tmp>>8); //Норма
		z1_end += (dz1_end>>8) * (tmp>>8); //Норма
		u_end += (du_end>>8) * (tmp>>8); //Норма
		v_end += (dv_end>>8) * (tmp>>8); //Норма
#endif

////////////////////////////////
//loc_scip01:;
		// построчная отрисовка грани
		for (current_sy = ceilFIntF0(a->y); current_sy <= ceilFIntF0(c->y); current_sy++) {
			if((current_sy) >= imaxY ) break;
			//if((current_sy-minY) < 0 ) break;//continue;
			if (current_sy == ceilFIntF0(b->y)) {
				//if(roundFIntF0(c->y - b->y)==0) goto loc_scip02;
				x_end = b->x;
				z1_end = b->z;
				u_end = b->u;
				v_end = b->v;
				//divisor=cvrtFIntF8(c->y - b->y);
				//if(divisor){
				//	dx_end = ((c->x - b->x) / divisor)<<8;
				//	dz1_end = ((c->z - b->z)) / divisor)<<8;
				//}
				//else { dx_end=0; dz1_end=0; }
				divisor = c->y - b->y;
				if(roundFIntF0(divisor)){
					dx_end = ((__int64)(c->x - b->x)<<16) / divisor;
					dz1_end = ((__int64)(c->z - b->z)<<16) / divisor;
					du_end = ((__int64)(c->u - b->u)<<16) / divisor;
					dv_end = ((__int64)(c->v - b->v)<<16) / divisor;
				}
				else{ dx_end=0; dz1_end=0; du_end=0; dv_end=0; }
#ifdef SUBPIXEL
				//tmp = ceil(b->sy) - b->sy;
				tmp = ceilFIntF16(b->y) - b->y;
				x_end += (dx_end>>8) * (tmp>>8); //Норма
				z1_end += (dz1_end>>8) * (tmp>>8); //Норма
				u_end += (du_end>>8) * (tmp>>8); //Норма
				v_end += (dv_end>>8) * (tmp>>8); //Норма
#endif
			}

			// x_start должен находиться левее x_end
			if(x_start > curMaxX) x_start=curMaxX;
			if(x_end > curMaxX) x_end=curMaxX;
			if(x_start < curMinX) x_start=curMinX;
			if(x_end < curMinX) x_end=curMinX;
			if (x_start > x_end) {
			  x = x_end;
			  z1 = z1_end;
			  u = u_end;
			  v = v_end;
			  //length = ceil(x_start) - ceil(x_end);
			  length = ceilFIntF0(x_start) - ceilFIntF0(x_end);
			} else {
			  x = x_start;
			  z1 = z1_start;
              u = u_start;
			  v = v_start;
			  //length = ceil(x_end) - ceil(x_start);
			  length = ceilFIntF0(x_end) - ceilFIntF0(x_start);
			}

			// текстурируем строку
			//current_sx = (int)ceil(x)-minX;
			current_sx = ceilFIntF0(x);
	
			if((current_sy) >= 0 ) if (length) {
#ifdef SUBTEXEL
				//tmp = ceil(x) - x;
				tmp = ceilFIntF16(x) - x;
				z1 += (dz1>>8)* (tmp>>8);
				u += (du>>8)* (tmp>>8);
				v += (dv>>8)* (tmp>>8);
#endif
				while (length--) {
				// используем z-буфер для определения видимости текущей точки
					//xassert(current_sx <= ceilFIntF0(a->x)-iminX+10 || current_sx <= ceilFIntF0(b->x)-iminX+10 || current_sx <= ceilFIntF0(c->x)-iminX+10);
					//xassert(current_sx >= ceilFIntF0(a->x)-iminX-10 || current_sx >= ceilFIntF0(b->x)-iminX-10 || current_sx >= ceilFIntF0(c->x)-iminX-10);
					//xassert(z1 <= a->z+(20<<16) || z1 <= b->z+(20<<16) || z1 <= c->z+(20<<16));
					//xassert(z1 >= a->z-(20<<16) || z1 >= b->z-(20<<16) || z1 >= c->z-(20<<16));
					if(z1 > curMaxZ) z1=curMaxZ;
					int cu=ceilFIntF0(u)%bitmapSize.x;//(==bitmapSizeV.x)
					if(cu <0 ) cu+=bitmapSize.x;
					int cv=ceilFIntF0(v)%bitmapSize.y;//(==bitmapSizeV.y)
					if(cv <0 ) cv+=bitmapSize.y;
					if( (current_sx<imaxX) && (current_sx >= 0)) {
						register int bufoff=vMap.offsetBuf(current_sx,current_sy);//(current_sy)*voxelBitmap.sx + current_sx;
						if(flag_onlyTextured){
							int vox=vMap.GetAlt(bufoff);
							if(pBitmap){
								unsigned int offBitmap=cu + cv*bitmapSize.x;
								vMap.SetAtr(bufoff, VmAt_Nrml_Dam);
								if(!flag_AlphaChanelPresent)
									vMap.SupBuf[bufoff]= pBitmap[offBitmap];
								else 
									alphaBlending(pBitmap[offBitmap], vMap.SupBuf[bufoff]);
							}
							if(pBitmapV){
								unsigned int offBitmap=cu + cv*bitmapSizeV.x;
								vox+= (*((unsigned char*)(&pBitmapV[offBitmap])+2)+ *((unsigned char*)(&pBitmapV[offBitmap]+1)) + *((unsigned char*)(&pBitmapV[offBitmap])) )>>2;
								if(vox>MAX_VX_HEIGHT)
									vox=MAX_VX_HEIGHT;
							}
							vMap.PutAlt(bufoff,vox);
						}
						else if(numPass==1){
							vMap.PutAlt(bufoff,0);
						}
						else if( vMap.GetAlt(bufoff) <= z1>>(16-VX_FRACTION)){
							int vox=0;
							if(numPass==0)
                                vox=z1>>(16-VX_FRACTION);
							else if(numPass==2){
								int xx,yy;
								for(yy=-1; yy<=1; yy++)
									for(xx=-1; xx<=1; xx++)
										vox+=vMap.GetAlt(vMap.offsetBufC(current_sx+xx,current_sy+yy));
								vox/=9;
							}
							if(pBitmap){
								unsigned int offBitmap=cu + cv*bitmapSize.x;
								vMap.SetAtr(bufoff, VmAt_Nrml_Dam);
								if(!flag_AlphaChanelPresent)
									vMap.SupBuf[bufoff]= pBitmap[offBitmap];
								else 
									alphaBlending(pBitmap[offBitmap], vMap.SupBuf[bufoff]);
							}
							if(pBitmapV){
								unsigned int offBitmap=cu + cv*bitmapSizeV.x;
								vox+= (*((unsigned char*)(&pBitmapV[offBitmap])+2)+ *((unsigned char*)(&pBitmapV[offBitmap]+1)) + *((unsigned char*)(&pBitmapV[offBitmap])) )>>2;
								if(vox>MAX_VX_HEIGHT)
									vox=MAX_VX_HEIGHT;
							}
							vMap.PutAlt(bufoff,vox);
						}
					}
					z1 += dz1;
					u += du;
					v += dv;
					current_sx++;
				}
			}

			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			z1_start += dz1_start;
			x_end += dx_end;
			z1_end += dz1_end;
			u_start += du_start;
			v_start += dv_start;
			u_end += du_end;
			v_end += dv_end;
		}
//loc_scip02:;


	}
}

void RoadTool::putSpline(TypeNodeIterator ni, int numPass, bool flag_onlyTextured)
{
	//Bitmap !!!
	const BitmapDispatcher::Bitmap* texture=bitmapDispatcher.getBitmap(bitmapFileName.c_str());
	const BitmapDispatcher::Bitmap* vtexture=bitmapDispatcher.getBitmap(bitmapVolFileName.c_str());
	if(texture && vtexture && texture->size!=vtexture->size){
		xassert(0&&"Bitmap and bitmapV is not equal !");
	}
	const BitmapDispatcher::Bitmap* textureEdge=bitmapDispatcher.getBitmap(edgeBitmapFName.c_str());
	const BitmapDispatcher::Bitmap* vtextureEdge=bitmapDispatcher.getBitmap(edgeVolBitmapFName.c_str());
	if(textureEdge && vtextureEdge && textureEdge->size!=vtextureEdge->size){
		xassert(0&&"Edge Bitmap and bitmapV is not equal !");
	}

	int maxTetxure_V_Coordinate=0;
	float kVScale;
	if(texture){
		maxTetxure_V_Coordinate=texture->size.y-1;
		kVScale=(float)texture->size.y/2.f;
	}
	else if(vtexture){
		maxTetxure_V_Coordinate=vtexture->size.y-1;
		kVScale=(float)vtexture->size.y/2.f;
	}
	else {
		maxTetxure_V_Coordinate=0;
		kVScale=1.f;
	}
	//const float kVScale=(float)texture->size.y/2.f;

	if(ni->spline2NextLeft.empty()) return; //проверка ni < (nodeArr.end()-1)
	xassert(ni->spline2NextLeft.size()==ni->spline2NextRight.size());
	int sizeSpline=ni->spline2NextLeft.size();
	vector<sPolygon> poligonArr;
	vector<VertexI> iPntArr;
	vector<VertexI> iPntArr2;
	poligonArr.reserve(sizeSpline*2);
	iPntArr.reserve(sizeSpline*2);
	int i=0;
	TypeNodeIterator m=ni;
	float curLengtTrackF=0;
	while(m!=nodeArr.begin()) { 
		float cEndWidth=kVScale/m->width;
		m--; 
		float cBegWidth=kVScale/m->width;
		if(texturingMetod==sRoadPMO::TM_AlignWidthAndHeight)
			curLengtTrackF+=(0.5f*(cBegWidth+cEndWidth)*m->lenght2Next);
		else 
			curLengtTrackF+=m->lenght2Next;
	}
	m=ni; m++; // m - node end spline
	float curWidth=ni->width;
	const float dWidth=(m->width - ni->width)/ni->lenght2Next;//(float)ni->spline2NextLeft.size();
	int curU;
	for(i=0; i<sizeSpline; i++){
		curU=round( fmod(curLengtTrackF, (float)(SHRT_MAX+1))*(float)(1<<16) );
		Vect3f& lp=ni->spline2NextLeft[i];
		Vect3f& rp=ni->spline2NextRight[i];
		iPntArr.push_back(VertexI(lp*(1<<16)));
		iPntArr.back().u=curU;
		if(texturingMetod==sRoadPMO::TM_1to1)
			iPntArr.back().v=round((kVScale-curWidth)*(1<<16));
		else 
			iPntArr.back().v=0;
		iPntArr.push_back(VertexI(rp*(1<<16)));
		iPntArr.back().u=curU;
		if(texturingMetod==sRoadPMO::TM_1to1)
			iPntArr.back().v=round((kVScale+curWidth)*(1<<16));
		else
			iPntArr.back().v=maxTetxure_V_Coordinate<<16;
		int idx=i*2;
		poligonArr.push_back(sPolygon());
		poligonArr.back().set(idx, idx+2, idx+3); //(idx-2, idx, idx+1);
		poligonArr.push_back(sPolygon());
		poligonArr.back().set(idx, idx+1, idx+3); //(idx-2, idx-1, idx+1);
		curWidth+=ni->spline2NextSectionLenght[i]*dWidth;
		if(texturingMetod==sRoadPMO::TM_AlignWidthAndHeight)
			curLengtTrackF+=ni->spline2NextSectionLenght[i]*kVScale/curWidth;
		else 
			curLengtTrackF+=ni->spline2NextSectionLenght[i];
	}
	ni++;
	if(ni!=nodeArr.end() && !ni->spline2NextLeft.empty() ){ //Внимание - используется i !
		curU=round( fmod(curLengtTrackF, (float)(SHRT_MAX+1))*(float)(1<<16) );
		Vect3f& lp=ni->spline2NextLeft[0];
		Vect3f& rp=ni->spline2NextRight[0];
		iPntArr.push_back(VertexI(lp*(1<<16)));
		iPntArr.back().u=curU;
		if(texturingMetod==sRoadPMO::TM_1to1)
			iPntArr.back().v=round((kVScale-curWidth)*(1<<16));
		else 
			iPntArr.back().v=0;
		iPntArr.push_back(VertexI(rp*(1<<16)));
		iPntArr.back().u=curU;
		if(texturingMetod==sRoadPMO::TM_1to1)
			iPntArr.back().v=round((kVScale+curWidth)*(1<<16));
		else
			iPntArr.back().v=maxTetxure_V_Coordinate<<16;
	}
	else {
		iPntArr.push_back(iPntArr.back());
		iPntArr.push_back(iPntArr.back());
	}
	if(numPass==0)
		putStrip(poligonArr, iPntArr, texture, vtexture, numPass, flag_onlyTextured);
	else 
		putStrip(poligonArr, iPntArr, 0, 0, numPass, flag_onlyTextured);

	ni--;

	//Edge
	//maxTetxure_V_Coordinate= textureEdge ? textureEdge->size.y-1 : 0;
	if(textureEdge)
		maxTetxure_V_Coordinate=textureEdge->size.y-1;
	else if(vtextureEdge)
		maxTetxure_V_Coordinate=vtextureEdge->size.y-1;
	else 
		maxTetxure_V_Coordinate=0;

	poligonArr.clear();
	iPntArr.clear();
	poligonArr.reserve(sizeSpline*2);
	iPntArr.reserve(sizeSpline*2);
	iPntArr2.reserve(sizeSpline*2);

	m=ni;
	curLengtTrackF=0;
	while(m!=nodeArr.begin()) {
		m--;
		curLengtTrackF+=m->lenght2Next;
	}
	for(i=0; i<sizeSpline; i++){
		//left edge
		curU=round( fmod(curLengtTrackF, (float)(SHRT_MAX+1))*(float)(1<<16) );
		Vect3f& lp=ni->spline2NextLeftEdge[i];
		Vect3f& rp=ni->spline2NextLeft[i];
		iPntArr.push_back(VertexI(lp*(1<<16))); 
		iPntArr.back().u=curU;
		iPntArr.back().v=0;
		iPntArr.push_back(VertexI(rp*(1<<16)));
		iPntArr.back().u=curU;
		iPntArr.back().v=maxTetxure_V_Coordinate<<16;
		//right edge
		Vect3f& lp2=ni->spline2NextRight[i];
		Vect3f& rp2=ni->spline2NextRightEdge[i];
		iPntArr2.push_back(VertexI(lp2*(1<<16))); 
		iPntArr2.back().u=curU;
		iPntArr2.back().v=maxTetxure_V_Coordinate<<16;
		iPntArr2.push_back(VertexI(rp2*(1<<16)));
		iPntArr2.back().u=curU;
		iPntArr2.back().v=0;
		//poligons можно не заполнять
		int idx=i*2;
		poligonArr.push_back(sPolygon());
		poligonArr.back().set(idx, idx+2, idx+3); //(idx-2, idx, idx+1);
		poligonArr.push_back(sPolygon());
		poligonArr.back().set(idx, idx+1, idx+3); //(idx-2, idx-1, idx+1);
		curLengtTrackF+=ni->spline2NextSectionLenght[i];
	}
	ni++;
	if(ni!=nodeArr.end() && !ni->spline2NextLeft.empty() ){ //Внимание - используется i !
		curU=round( fmod(curLengtTrackF, (float)(SHRT_MAX+1))*(float)(1<<16) );
		//left edge
		Vect3f& lp=ni->spline2NextLeftEdge[0];
		Vect3f& rp=ni->spline2NextLeft[0];
		iPntArr.push_back(VertexI(lp*(1<<16))); 
		iPntArr.back().u=curU;
		iPntArr.back().v=0;
		iPntArr.push_back(VertexI(rp*(1<<16)));
		iPntArr.back().u=curU;
		iPntArr.back().v=maxTetxure_V_Coordinate<<16;
		//right edge
		Vect3f& lp2=ni->spline2NextRight[0];
		Vect3f& rp2=ni->spline2NextRightEdge[0];
		iPntArr2.push_back(VertexI(lp2*(1<<16))); 
		iPntArr2.back().u=curU;
		iPntArr2.back().v=maxTetxure_V_Coordinate<<16;
		iPntArr2.push_back(VertexI(rp2*(1<<16)));
		iPntArr2.back().u=curU;
		iPntArr2.back().v=0;

	}
	else {
		iPntArr.push_back(iPntArr.back());
		iPntArr.push_back(iPntArr.back());
		iPntArr2.push_back(iPntArr2.back());
		iPntArr2.push_back(iPntArr2.back());
	}
	if(numPass==0)
		putStrip(poligonArr, iPntArr, textureEdge, vtextureEdge, 0, flag_onlyTextured);
	else 
		putStrip(poligonArr, iPntArr, 0, 0, numPass, flag_onlyTextured);


	if(numPass==0)
		putStrip(poligonArr, iPntArr2, textureEdge, vtextureEdge, 0, flag_onlyTextured);
	else 
		putStrip(poligonArr, iPntArr2, 0, 0, numPass, flag_onlyTextured);

	ni--;

	vMap.WorldRender();
}

