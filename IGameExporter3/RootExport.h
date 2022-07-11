#pragma once

#include "MoveEmblem.h"
#include "IVisExporter.h"
#include "Interpolate.h"

class Exporter : Static3dxFile, public ShareHandleBase
{
public:
	enum Lod {
		LOD0,
		LOD1,
		LOD2
	};

	Exporter(Interface* maxInterface, const char* filename);
	~Exporter();

	bool loadData(const char* filename);
	void export(const char* filename);

	IVisNode* Find(const char* name);

	int GetNumFrames();

	int FindMaterialIndex(IVisMaterial* mat);
	IVisMaterial* FindMaterial(const char* name);

	int GetMaxWeights(){return maxWeights;};
	void SetMaxWeights(int w){xassert(w>=1 && w<=4);maxWeights=w;};

	int GetMaterialNum(){return materials.size();}
	IVisMaterial* GetMaterial(int i){ return materials[i];}

	int findAnimationGroupIndex(const char* nodeName) const;//индекс в animation_group. -1 - не найдено

	void serialize(Archive& ar);

	AnimationGroups animationGroups_;
	StaticAnimationChains animationChains_;
	StaticVisibilitySets visibilitySets_;

	vector<sLogo> logos;

	int startTime() const { return startTime_; }
	int endTime() const { return endTime_; }
	int toTime(int frame) const { return frame*ticksPerFrame_; }
	int toFrame(int time) const { return (time - startTime_)/ticksPerFrame_; }

	int GetRoolMaterialCount() {return materials_.size();}
	IVisMaterial* GetRootMaterial(int i) {return materials_[i];}
	int GetRootNodeCount();
	IVisNode* GetRootNode(int n);
	IVisNode* FindNode(INode* node);

	void ProgressStart(const char* title=0);
	void ProgressEnd();
	void ProgressUpdate(const char* title=0);
	void AddProgressCount(int cnt);

	float relative_position_delta;
	float position_delta;
	float rotation_delta;
	float scale_delta;

	bool show_info_polygon;
	
protected:
	int maxWeights;
	Lod lod_;
	bool textLog_;
	bool dontExport_;

	int startTime_, endTime_, ticksPerFrame_;
	IVisMaterials materials;
	vector<string> duplicate_material_names;

	sBox6f bound_;
	float boundRadius_;

	string backupName_;
	string fileName_;

	void exportStatic3dx(bool logic);
	void exportLOD(int lod);
	void exportMaterials(Static3dxBase* object);
	bool exportMaterial(StaticMaterial& staticMaterial, IVisMaterial* mat);
	void exportOpacity(StaticMaterialAnimation& chain, IVisMaterial* mat, int interval_begin, int interval_size, bool cycled);
	void exportUV(Interpolator3dxUV& uv, IVisMaterial* mat, IVisTexmap* texmap, int interval_begin, int interval_size, bool cycled);

	IVisNode* FindRecursive(IVisNode* pGameNode,const char* name);

	void BuildMaterialList();
	void CheckMaterialInAnimationGroup();

	IVisMaterial* GetIMaterial(Mtl* mtl);

	Interface* interface_;
	IVisMaterials materials_;
	IVisNodes rootNodes_;

	int maxCount_;
	int curCount_;

	friend IVisNode;
	friend class AnimationGroupsSerializer;
};

extern class Exporter* exporter;

void RightToLeft(Matrix3& m);

inline Vect3f RightToLeft(const Vect3f& v)
{
	return Vect3f(v.x, -v.y, v.z);
}

inline Vect3f convert(const Point3& p)
{
	return Vect3f(p.x, p.y, p.z);
}

inline MatXf convert(const Matrix3& m)
{
	MatXf out;
	for(int i=0;i<3;i++)
		out.rot()[i] = convert(m.GetColumn3(i));

	out.trans() = convert(m.GetRow(3));
	return out;
}

inline Color4f convert(const Color& c)
{
	return Color4f(c.r, c.g, c.b);
}

class GameExporterError {};
