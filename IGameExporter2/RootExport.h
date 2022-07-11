#pragma once
typedef map<IGameNode*,int> MAP_NODE;

#include "AnimationData.h"
#include "MoveEmblem.h"

#include "Interpolate.h"
#include "CacheNodeAnimation.h"

typedef VectTemplate<6> VectUV;
typedef VectTemplate<3> VectPosition;
typedef VectTemplate<4> VectRotation;
typedef VectTemplate<1> VectScale;

class VisibilityChain{
public:
    struct Node{
        float interval_begin;
        float interval_size;
        bool value;
		bool operator!=(const Node& rhs) const{
			return !operator==(rhs);
		}
		bool operator==(const Node& rhs) const{
			return interval_begin == rhs.interval_begin &&
				interval_size == rhs.interval_size &&
				value == rhs.value;
		}
    };

	bool operator!=(const VisibilityChain& rhs) const{
		return !operator==(rhs);
	}
	bool operator==(const VisibilityChain& rhs) const{
		if(nodes_.size() != rhs.nodes_.size())
			return false;
		for(int i = 0; i < nodes_.size(); ++i){
			if(!(nodes_[i] == rhs.nodes_[i]))
				return false;
		}
		return true;
	}
	size_t size() const{ return nodes_.size(); }
	typedef std::vector<Node> Nodes;

    VisibilityChain(const vector<bool>& visibility, int interval_size, bool cycled);
	void SaveFixed(Saver& saver) const;
    void Save(Saver& saver) const;
private:
    Nodes nodes_;
};


typedef InterpolatePosition<VectPosition> PositionChain;
typedef InterpolatePosition<VectRotation> RotationChain;
typedef InterpolatePosition<VectScale> ScaleChain;
typedef InterpolatePosition<VectUV>      UVChain;

typedef std::list<PositionChain>   PositionChains;
typedef std::list<RotationChain>   RotationChains;
typedef std::list<ScaleChain>      ScaleChains;
typedef std::list<VisibilityChain> VisibilityChains;
typedef std::list<UVChain>         UVChains;

class ChainsBlock{
public:
    ChainsBlock();
    ~ChainsBlock();

	void save(Saver& saver) const;

	int put(PositionChain& chain);
	int put(RotationChain& chain);
	int put(ScaleChain& chain);
	int put(VisibilityChain& chain);
	int put(UVChain& chain);

	PositionChains   positionChains_;
	RotationChains   rotationChains_;
	ScaleChains      scaleChains_;
	VisibilityChains visibilityChains_;
	UVChains		 uvChains_;

    int positionCounter_;
    int rotationCounter_;
    int scaleCounter_;
    int visibilityCounter_;
	int uvCounter_;
};



struct NextLevelNode
{
	IGameNode* node;
	IGameNode* pParent;
	int current;//inode
	int parent;//iparent
	bool is_nodelete;//Не удаляется при оптимизации
	vector<IGameNode*> additional;

	NextLevelNode()
	{
		node=NULL;
		pParent=NULL;
		current=-1;
		parent=-1;
		is_nodelete=false;
	}

	Matrix3 GetLocalTM(int max_time);
	Matrix3 GetWorldTM(int max_time);
};

class RootExport
{
public:
	RootExport();
	~RootExport();
	void Init(IGameScene * pIgame);

	bool LoadData(const char* filename);
	void Export(const char* filename);
	void ExportLOD(const char* filename,int lod);

	IGameNode* Find(const char* name);

	int GetNumFrames();

	int FindMaterialIndex(IGameMaterial* mat);
	IGameMaterial* FindMaterial(const char* name);

	int FindNodeIndex(IGameNode* node);
	IGameNode* GetNode(int index)
	{
		xassert(index>=0 && index<all_nodes.size());
		return all_nodes[index].node;
	}

	int GetMaxWeights(){return max_weights;};
	void SetMaxWeights(int w){xassert(w>=1 && w<=4);max_weights=w;};

	int GetMaterialNum(){return materials.size();}
	IGameMaterial* GetMaterial(int i){ return materials[i];}
public:
	IGameScene * pIgame;
	AnimationData animation_data;
	vector<sLogo> logos;

	MAP_NODE nondelete_node;
	MAP_NODE logic_node;
	MAP_NODE bound_node;

	CacheNodeAnimation cache;

	int ToMaxTime(int frame);
	bool IsLogicNodeBound(IGameNode* node);

	int GetBaseFrame();
	int GetBaseFrameMax(){return ToMaxTime(GetBaseFrame());};

	void ExportLODHelpers(Saver& saver);
protected:
	void Export(bool logic);
	FileSaver saver_;

	enum EXPORT_LOGIC
	{
		EXPORT_GROUP=0,
		EXPORT_LOGIC_CENTER=1,
		EXPORT_LOGIC_IN_GROUP=2,
	};
	EXPORT_LOGIC export_logic;
	int max_weights;

	TimeValue time_start,time_end,time_frame;
	vector<IGameMaterial*> materials;
	vector<string> duplicate_material_names;
	IGameNode* node_base;

	MAP_NODE node_map;
	vector<NextLevelNode> all_nodes;

	void SaveNodes(Saver& saver, ChainsBlock& chains_block);

	void ExportNode(Saver& saver, NextLevelNode& n, ChainsBlock& block);
	void ExportMatrix(Saver& saver, NextLevelNode& n,int interval_begin,int interval_size,bool cycled, ChainsBlock& chain);

	IGameNode* FindRecursive(IGameNode* pGameNode,const char* name);

	bool IsIgnore(IGameNode* pGameNode,bool root);
	void SaveMaterials(Saver& saver, ChainsBlock& chains_block);
	void CalcAllNodes();
	void CalcBoundBox();

	void CalcNodeMapOptimize();
	void CalcNodeMapEasy();
	void CalcNodeAnimate(MAP_NODE& nondelete,bool is_logic);
	void CalcNodeHaveMesh();
	bool CalcNodeAnimate(NextLevelNode& n,int interval_begin,int interval_size,bool cycled);
	void CalcNodeMapLogic();
	bool ParentInEnemyanimationGroup(const NextLevelNode& n);

	void SaveBasement(Saver& saver);
	void SaveMeshes(Saver& saver);
	void SaveLights(Saver& saver, ChainsBlock& chains_block);

	struct sLogicNodeBound
	{
		int inode;
		IGameMesh* pobject;
	};

	void FindLogicBounds(IGameMesh*& pLogicBound,vector<sLogicNodeBound>& node_bounds);
	void SaveLogicBounds(Saver& saver,IGameMesh* pLogicBound,vector<sLogicNodeBound>& node_bounds);
	void SaveLogicBoundRoot(Saver& saver,IGameMesh* pobject);
	void SaveLogicBoundLocal(Saver& saver,IGameMesh* pobject,int inode);


	void LoadLogos(CLoadDirectory it);
	void SaveLogos(Saver& saver);

	void LoadNonDeleteNode(CLoadIterator it);
	void SaveNonDeleteNode(Saver& saver);
	void LoadLogicNode(CLoadIterator it);
	void SaveLogicNode(Saver& saver);

	void LoadMapNode(CLoadIterator it,MAP_NODE& nodes,const char* error_message);
	void SaveMapNode(Saver& saver,MAP_NODE& nodes,int idx);

	void BuildMaterialList();
	void CheckMaterialInAnimationGroup();

	friend struct NextLevelNode;
};

extern class RootExport* pRootExport;

void RightToLeft(Matrix3& m);

bool IsNodeMesh(IGameNode* node);
bool IsNodeLight(IGameNode* node);
