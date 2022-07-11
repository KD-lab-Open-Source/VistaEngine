#ifndef __ROAD_H__
#define __ROAD_H__

#include "XMath\xmath.h"
#include "Render\3dx\umath.h"

#include "worldFileDispatcher.h"
#include "procedurMap.h"
#include "terra.h"

struct VertexI {
	int x, y, z;
	int u, v;
	VertexI() {}
	VertexI(int x_, int y_, int z_) {x = x_; y = y_; z = z_;}
	VertexI(const Vect3f& v) {x = round(v.x); y = round(v.y); z = round(v.z); }
	VertexI(const Vect2f v, float _z) {
		x = round(v.x); y = round(v.y); z = round(_z); 
	}
	typedef int int3[3];
	VertexI(const int3& v) {x = v[0]; y = v[1]; z = v[2];}
};

struct sNode : public sNodePrimaryData{
	void serialize(Archive& ar);

	//Vect3f pos;
	Vect2f leftPnt;
	Vect2f rightPnt;
	//Vect2f leftPntEdge;
	//Vect2f rightPntEdge;
	//float width;
	//float angle;
	float lenght2Prev;
	float lenght2Next;
	vector<Vect3f> spline2NextLeft;
	vector<Vect3f> spline2NextRight;
	vector<Vect3f> spline2NextLeftEdge;
	vector<Vect3f> spline2NextRightEdge;
	vector<float> spline2NextSectionLenght;
	vector<Vect3f> spline2Next;
	sNode(){
		setPos(Vect3f::ZERO);
		width=0;
		angleOrWidtEdge=0;
		lenght2Prev=0;
	}
	sNode(const Vect3f& _pos, const float _width, const float _angleOrWidtEdge){
		setPos(_pos);
		width=_width;
		angleOrWidtEdge=_angleOrWidtEdge;
		lenght2Prev=0;
	}
	sNode(const sNodePrimaryData& primaryData){
		setPos(primaryData.pos);
		width=primaryData.width;
		angleOrWidtEdge=primaryData.angleOrWidtEdge;
		lenght2Prev=0;
	}
	void setPos(const Vect3f& _pos){
		pos=_pos;
		leftPnt=rightPnt=pos;
		//leftPntEdge=rightPntEdge=pos;
	}
};


class RoadTool {
public:
	RoadTool();
	void serialize(Archive& ar);
	//const string& _bitmapFileName, const string& _bitmapVolFileName, const string& _edgeBitmapFName, const string& _edgeVolBitmapFName, 
	list<sNode> nodeArr;
	typedef list<sNode>::iterator TypeNodeIterator;
	sRoadPMO::eTexturingMetod texturingMetod;
	sRoadPMO::ePutMetod putMetod;
	int roadDetailTex;
	int edgeDetailTex;
	struct TextureSetting {
		string bitmapFileName;
		string bitmapVolFileName;
		string edgeBitmapFName;
		string edgeVolBitmapFName;
		bool flag_onlyTextured;
		unsigned char alphaRoad;
		unsigned char alphaRoadSide;
		TextureSetting() { flag_onlyTextured=false; alphaRoad=255; alphaRoadSide=255; }	
		void set( const string& _bitmapFileName, const string& _bitmapVolFileName, 
		  const string& _edgeBitmapFName, const string& _edgeVolBitmapFName,
		  bool _flag_onlyTextured, unsigned char _alphaRoad, unsigned char _alphaRoadSide) {
			bitmapFileName=_bitmapFileName; bitmapVolFileName=_bitmapVolFileName;
            edgeBitmapFName=_edgeBitmapFName; edgeVolBitmapFName=_edgeVolBitmapFName;
            flag_onlyTextured=_flag_onlyTextured;
            alphaRoad=_alphaRoad; alphaRoadSide=_alphaRoadSide;
		}
		void serialize(Archive& ar);
	};
	void init(const vector<sNodePrimaryData>& primaryNodeArr, sRoadPMO::eTexturingMetod _texturingMetod, sRoadPMO::ePutMetod _putMetod, const RoadTool::TextureSetting& txSet);
	void buildRoad();
protected:
	void recalcNodeOrientation(TypeNodeIterator ni);
	sRect recalcSpline(TypeNodeIterator ni);
	void putSpline(TypeNodeIterator ni, int numPass);
	void putStrip(vector<sPolygon>& poligonArr, vector<VertexI> iPntArr, const BitmapDispatcher::Bitmap* texture, const BitmapDispatcher::Bitmap* vtexture, int numPass, int alpha, int detTex);

	bool addNode(const Vect3f& worldCoord, const Vect2i& scrCoord, int roadWidth, float edgeAngle);
	void recalcHeightNodeLinear();
	void recalcHeightNodeParabolic(int maxSphericHeight);
	void changeAllRoadWidthAndAngle(int newWidth, float newAngle);

	float getRoadLenghtAndRecalcNodeLengt();

	//bitmap option
	TextureSetting textureSetting;
};


#endif //__ROAD_H__
