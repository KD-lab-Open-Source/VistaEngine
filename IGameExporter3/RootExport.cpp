#include "StdAfx.h"
#include "RootExport.h"
#include "Interpolate.h"
#include "ExportMesh.h"
#include "ExportLight.h"
#include "ExportMaterial.h"
#include "ExportBasement.h"
#include "DebugDlg.h"
#include "..\render\inc\umath.h"
const float relative_position_delta=1e-4f;;
const float rotation_delta=2e-4f;
const float scale_delta=1e-3f;//При scale существенно отличном от 1, тоже будет давать неточные результаты.
sBox6f bound_box;
float position_delta=relative_position_delta;


void RightToLeft(Matrix3& m)
{
	Point3 p=m.GetRow(0);
	p.y=-p.y;
	m.SetRow(0,p);

	p=m.GetRow(1);
	p.x=-p.x;
	p.z=-p.z;
	m.SetRow(1,p);

	p=m.GetRow(2);
	p.y=-p.y;
	m.SetRow(2,p);

	p=m.GetRow(3);
	p.y=-p.y;
	m.SetRow(3,p);
}

RootExport::RootExport()
{
	pIgame=NULL;
	node_base=NULL;
	max_weights=1;
}

RootExport::~RootExport()
{
}

void RootExport::Init(IVisExporter * pIgame_)
{
	pIgame=pIgame_;
	time_start = pIgame->GetStartTime();
	time_end=pIgame->GetEndTime();
	time_frame=pIgame->GetTicksPerFrame();
	cache.Build(pIgame);
	BuildMaterialList();
}

bool RootExport::LoadData(const char* filename)
{
	CLoadDirectoryFile s;
	if(!s.Load(filename))
		return false;
	while(CLoadData* ld=s.next())
	switch(ld->id)
	{
	case C3DX_ANIMATION_HEAD:
		animation_data.Load(ld);
		break;
	case C3DX_GRAPH_OBJECT:
		{
			CLoadDirectory dir(ld);
			{
				while(CLoadData* ld=dir.next())
				switch(ld->id)
				{
				case C3DX_ANIMATION_HEAD:
					animation_data.Load(ld);
					break;
				}
			}
			break;
		}
	case C3DX_MAX_WEIGHTS:
		{
			CLoadIterator it(ld);
			it>>max_weights;
		}
		break;
	case C3DX_NON_DELETE_NODE:
		LoadNonDeleteNode(ld);
		break;
	case C3DX_LOGIC_BOUND_NODE_LIST:
		LoadMapNode(ld,bound_node,"Не могу найти ноду %s. Она перестанет быть logic node bound.\n");
		break;
	case C3DX_LOGIC_NODE:
		LoadLogicNode(ld);
		break;
	case C3DX_LOGOS:
		LoadLogos(ld);
		break;
	}

	ShowConsole(NULL);
	return true;
}

int RootExport::GetNumFrames()
{
	return (time_end-time_start)/time_frame+1;
}

void RootExport::Export(const char* filename)
{
	CheckMaterialInAnimationGroup();
	if(!saver_.Init(filename))
	{
		Msg("Не могу открыть файл: %s",filename);
		return;
	}

	saver_.push(C3DX_MAX_WEIGHTS);
	saver_<<max_weights;
	saver_.pop();
	SaveNonDeleteNode(saver_);
	SaveLogicNode(saver_);
	SaveMapNode(saver_,bound_node,C3DX_LOGIC_BOUND_NODE_LIST);
	SaveLogos(saver_);

	Export(true);
	Export(false);

	ExportLODHelpers(saver_);//Good table nodes
}

void RootExport::Export(bool export_logic_)
{
	node_base=NULL;
	node_map.clear();
	all_nodes.clear();

	export_logic=export_logic_?EXPORT_LOGIC_CENTER:EXPORT_GROUP;

	if(export_logic)
	{
		bool is=false;
		for(int loop = 0; loop <pIgame->GetRootNodeCount();loop++)
		{
			IVisNode* pGameNode = pIgame->GetRootNode(loop);
			if(IsIgnore(pGameNode,true))
				continue;
			is=true;
		}
		if(!is)
		{//logic object not found
			export_logic=EXPORT_LOGIC_IN_GROUP;
		}
	}


	CalcAllNodes();
	CalcBoundBox();

	switch(export_logic)
	{
	case EXPORT_LOGIC_CENTER:
		CalcNodeMapEasy();
		break;
	case EXPORT_GROUP:
		{
		CalcNodeAnimate(nondelete_node,false);
		CalcNodeMapOptimize();
		CalcNodeHaveMesh();
		CalcNodeMapOptimize();
		break;
		}
	case EXPORT_LOGIC_IN_GROUP:
		CalcNodeAnimate(logic_node,true);
		CalcNodeMapOptimize();
		CalcNodeMapLogic();
		CalcNodeMapOptimize();
		break;
	}

	IVisMesh* pLogicBound=NULL;
	vector<sLogicNodeBound> node_bounds;
	if(export_logic)
		FindLogicBounds(pLogicBound,node_bounds);
	if(export_logic==EXPORT_LOGIC_IN_GROUP)
	{
		if(all_nodes.size()<=1 && pLogicBound==NULL && node_bounds.empty())
			return;
	}


	saver_.push(export_logic?C3DX_LOGIC_OBJECT:C3DX_GRAPH_OBJECT);
	animation_data.Save(saver_);

	{
		MemorySaver memory_saver;

		ChainsBlock chains_block;

		if(!export_logic)
			SaveMaterials(memory_saver, chains_block);
		SaveNodes(memory_saver, chains_block);

		if(!export_logic)
		{
			SaveMeshes(memory_saver);
			SaveLights(memory_saver, chains_block);
			SaveCamera(memory_saver);
		}

		chains_block.save(saver_);
		saver_.write(memory_saver.buffer(), memory_saver.length());
	}

	SaveBasement(saver_);
	if(export_logic)
		SaveLogicBounds(saver_,pLogicBound,node_bounds);
	saver_.pop();
}


bool RootExport::IsIgnore(IVisNode* node,bool root)
{
	if(node->IsTarget())
		return true;
	const char* name=node->GetName();

	if(root)
	{
		if(export_logic==EXPORT_LOGIC_CENTER)
		{
			if(strstr(name,"logic")==NULL)
				return true;

		}else
		{
			if(strstr(name,"group")==NULL)
				return true;
		}
	}

	if(strcmp(name,"_base_")==0)
		return true;
	return false;
}

void RootExport::CalcAllNodes()
{
	all_nodes.clear();
	int current_node=0;
	vector<NextLevelNode> cur_level;
	vector<NextLevelNode> next_level;
	for(int loop = 0; loop <pIgame->GetRootNodeCount();loop++)
	{
		IVisNode* pGameNode = pIgame->GetRootNode(loop);
		if(IsIgnore(pGameNode,true))
			continue;

		NextLevelNode n;
		n.node=pGameNode;
		n.current=current_node;
		n.parent=-1;
		n.pParent=NULL;
		cur_level.push_back(n);
		all_nodes.push_back(n);
		current_node++;
	}

	if(cur_level.size()>1)
	{
		Msg("Error: Несколько глобальных объектов\n");
	}

	if(!export_logic && cur_level.empty())
	{
		Msg("Error: Нет group center\n");
	}
	
	while(!cur_level.empty())
	{
		vector<NextLevelNode>::iterator it_node;
		FOR_EACH(cur_level,it_node)
		{
			NextLevelNode& n=*it_node;

			for(int count=0;count<n.node->GetChildNodeCount();count++)
			{
				IVisNode * pChildNode = n.node->GetChildNode(count);
				
				if(strcmp(pChildNode->GetName(),"_base_")==0)
				{
					node_base=pChildNode;
					continue;
				}

				if(IsIgnore(pChildNode,false))
					continue;


				NextLevelNode next;
				next.node=pChildNode;
				next.current=current_node;
				next.parent=n.current;
				next.pParent=n.node;
				next_level.push_back(next);
				all_nodes.push_back(next);
				current_node++;
			}
		}
		cur_level.swap(next_level);
		next_level.clear();
	}

	for(int inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		const char* name=n.node->GetName();
		const char* parent_name=NULL;
		IVisNode* parent_parent=NULL;
		if(n.pParent)
		{
			parent_name=n.pParent->GetName();
			parent_parent=n.pParent->GetParentNode();
		}
	}
}

void RootExport::CalcNodeMapEasy()
{
	node_map.clear();
	for(int inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		xassert(n.current==inode);
		node_map[n.node]=n.current;
	}
}

void RootExport::CalcNodeMapOptimize()
{
	/*
	Удаляем !is_nodelete ноды и пересчитываем 
	NextLevelNode::current,NextLevelNode::parent
	*/
	node_map.clear();
	if(all_nodes.empty())
		return;
	vector<NextLevelNode> deleted_node,good_node;
	vector<int> remap_node;
	remap_node.resize(all_nodes.size());
	for(int inode=0;inode<all_nodes.size();inode++)
		remap_node[inode]=-1;

	remap_node[0]=0;
	good_node.push_back(all_nodes[0]);

	for(inode=1;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		if(n.is_nodelete || !n.pParent)
		{
			remap_node[inode] = (int)good_node.size();
			good_node.push_back(n);
		}else
		{
			//Дать ссылку на первый анимированный parent
			remap_node[inode]=remap_node[n.parent];
			deleted_node.push_back(n);
		}
	}

	for(inode=0;inode<good_node.size();inode++)
	{
		NextLevelNode& n=good_node[inode];
		n.current=remap_node[n.current];
		if(n.parent>=0)
		{
			n.parent=remap_node[n.parent];
			xassert(n.parent>=0 && n.parent<inode);
			n.pParent=good_node[n.parent].node;
		}
	}

	for(inode=0;inode<deleted_node.size();inode++)
	{
		NextLevelNode& n=deleted_node[inode];
		xassert(n.parent>=0 && n.parent<remap_node.size());
		int inew=remap_node[n.parent];
		good_node[inew].additional.push_back(n.node);
		if(!n.additional.empty())
		{
			for(int iadd=0;iadd<n.additional.size();iadd++)
				good_node[inew].additional.push_back(n.additional[iadd]);
			n.additional.clear();
		}
	}

	all_nodes=good_node;

	for(int inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		xassert(n.current==inode);
		node_map[n.node]=n.current;
		n.is_nodelete=false;
		for(int imesh=0;imesh<n.additional.size();imesh++)
		{
			IVisNode* node=n.additional[imesh];
			node_map[node]=n.current;
		}
	}
}

/*
Нельзя удалять также ноды, которые находятся на стыке разных анимационных групп,
чтобы не получилось, что суммируется анимация из разных групп.
Соответственно нужен формализм, где скопом определяется, какие ноды .

1. Нельзя удалять ноды, если они помечены как логические/неудаляемые.
2. Нельзя удаляить ноды у которых есть источники света/эффекты.
3. Нельзя удалять ноды, если к ним по иерархии привязанны меши либо ноды, входящии в другую анимационную группу.

Соотвественно такой алгоритм:
   1. Сначала удаляем неанимированные ноды.
   2. Вторым проходом смотрим - если к нодам не привязанно мешей, источников света либо нод, 
      которые входят в другую анимационную группу, то можно их удалить.

---------------------------------Попытка номер 2
CalcNodeAnimate
CalcNodeMapLogic

Первым проходом удаляем все неанимированные ноды, которые не помеченны как неудаляемые/логические.
Вторым проходом - удаляем анимированные ноды, к которые 
	не расположенны на границе разных анимационных групп, не помеченны как неудаляемые/логические.
    в случае графики - не привязанны меши.

*/

void RootExport::CalcNodeMapLogic()
{
	int num_not_animate=0;
	for(int inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		n.is_nodelete=false;
		const char* name=n.node->GetName();
		if(strstr(name,"logic"))
		{
			n.is_nodelete=true;
		}

		MAP_NODE::iterator it_non_del=logic_node.find(n.node);
		if(it_non_del!=logic_node.end())
		{
			n.is_nodelete=true;
		}

		if(ParentInEnemyanimationGroup(n))
		{
			all_nodes[n.parent].is_nodelete=true;
		}

		//Bad stat, see previons if
		if(n.pParent)
		{
			if(!n.is_nodelete)
			{
				if(dbg_show_info_delete_node)
					Msg("INFO: Not animated node=%s\n",n.node->GetName());
				num_not_animate++;
			}
		}else
		{
			if(dbg_show_info_delete_node)
				Msg("INFO: Top node=%s\n",n.node->GetName());
		}
	}

	if(dbg_show_info_delete_node)
		Msg("INFO: Not logic nodes=%i , all_nodes=%i\n",num_not_animate,all_nodes.size());
}

void RootExport::CalcNodeAnimate(MAP_NODE& nondelete,bool is_logic)
{
	int num_not_animate=0;
	for(int inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		n.is_nodelete=false;
		const char* name=n.node->GetName();

		MAP_NODE::iterator it = nondelete.find(n.node);
		if (it != nondelete.end())
		{
			n.is_nodelete=true;
		}else
		{
			if(strcmp(name,"logic bound")!=0 && !IsLogicNodeBound(n.node))
			{
				bool scale_error=false;
				for(int iac=0;iac<animation_data.animation_chain.size();iac++)
				{
					AnimationChain& ac=animation_data.animation_chain[iac];
					if(CalcNodeAnimate(n,ac.begin_frame,max(ac.end_frame-ac.begin_frame+1,1),ac.cycled,scale_error))
					{
						n.is_nodelete=true;
					}
				}

				if(scale_error)
					Msg("Node %s. Не поддерживается анизотропный scale\n",n.node->GetName());
			}
		}

		if(n.pParent)
		{
			if(!n.is_nodelete)
			{
				if(dbg_show_info_delete_node)
					Msg("INFO: Not animated node=%s\n",n.node->GetName());
				num_not_animate++;
			}
		}else
		{
			if(dbg_show_info_delete_node)
				Msg("INFO: Top node=%s\n",n.node->GetName());
		}
	}

	if(dbg_show_info_delete_node)
		Msg("INFO: Not animated nodes=%i , all_nodes=%i\n",num_not_animate,all_nodes.size());
}

bool RootExport::CalcNodeAnimate(NextLevelNode& n,int interval_begin,int interval_size,bool cycled,bool& scale_error)
{
	IVisNode * node=n.node;
	IVisNode* pParent=n.pParent;

	typedef VectTemplate<3> VectPosition;
	typedef VectTemplate<4> VectRotation;
	typedef VectTemplate<1> VectScale;
	typedef InterpolatePosition<VectPosition> IPosition;
	typedef InterpolatePosition<VectRotation> IRotation;
	typedef InterpolatePosition<VectScale> IScale;
	vector<VectPosition> position;
	vector<VectRotation> rotation;
	vector<VectScale> scale;
	IPosition ipos(0);
	IRotation irot(TypeCorrectCycleQuat);
	IScale iscale(0);

	static char eff[]="effect:";
	bool export_visibility=false;
	const char* node_name=node->GetName();
	if(strncmp(node_name,eff,sizeof(eff)-1)==0)
		export_visibility=true;
	vector<bool> visibility;
	if(export_visibility)
		return true;//Оставить ноды у эффектов

	if(n.node->GetType() == LIGHT_TYPE || n.node->GetType() == CAMERA_TYPE)
	{
		return true;//Оставлять ноды у источников света.
	}
	bool anisotropic_scale=false;
	for(int icurrent=-1;icurrent<interval_size;icurrent++)
	{
		AffineParts ap;
		int current_max_time=ToMaxTime(icurrent+interval_begin);
		if(icurrent==-1)
			current_max_time=0;//Так как в SetStaticFrame(time) всегда time==0

		Matrix3 m=n.GetLocalTM(current_max_time);

		if(!pParent)
		{
			m.NoRot();
			m.NoScale();
		}

		decomp_affine(m, &ap);
		Quat q = ap.q;
		//		Msg("  q=%f %f %f %f\n",q.x,q.y, q.z, q.w);
		//		Msg("  pos=%f %f %f\n",ap.t.x,ap.t.y, ap.t.z);
		VectPosition pos;
		pos[0]=ap.t.x;
		pos[1]=ap.t.y;
		pos[2]=ap.t.z;

		position.push_back(pos);

		VectRotation rot,rot_inv;
		rot[0]=ap.q.x;
		rot[1]=ap.q.y;
		rot[2]=ap.q.z;
		rot[3]=ap.q.w;
		//Матрица вращения может скачком менять знак.
		//Необходимо препятствовать этому явлению.
		if(rotation.empty())
		{
			rotation.push_back(rot);
		}else
		{
			VectRotation rot_prev=rotation.back();
			rot_inv[0]=-ap.q.x;
			rot_inv[1]=-ap.q.y;
			rot_inv[2]=-ap.q.z;
			rot_inv[3]=-ap.q.w;
			float d=rot_prev.distance(rot);
			float d_inv=rot_prev.distance(rot_inv);
			if(d<d_inv)
				rotation.push_back(rot);
			else
				rotation.push_back(rot_inv);
		}

		VectScale s;
		float mids=(ap.k.x+ap.k.y+ap.k.z)/3;
		float deltas=fabsf(mids-ap.k.x)+fabsf(mids-ap.k.y)+fabsf(mids-ap.k.z);
		if(ap.f<0 && mids>0)
		{
			mids=-mids;
		}

		s[0]=mids;
		scale.push_back(s);

		if(deltas>1e-3f)
			anisotropic_scale=true;


		if(export_visibility)
		{
			float visible=node->GetVisibility(current_max_time);
			visibility.push_back(visible>0.5f);
		}
	}

	ipos.InterpolateFakeConst(position,position_delta,cycled);
	irot.InterpolateFakeConst(rotation,rotation_delta,cycled);
	iscale.InterpolateFakeConst(scale,scale_delta,cycled);

	bool no_pos=ipos.out_data.size()==1 && ipos.out_data[0].itpl==ITPL_CONSTANT;
	bool no_rot=irot.out_data.size()==1 && irot.out_data[0].itpl==ITPL_CONSTANT;
	bool no_scale=iscale.out_data.size()==1 && iscale.out_data[0].itpl==ITPL_CONSTANT;

	if(!no_scale && anisotropic_scale)
		scale_error=true;
	return !(no_pos && no_rot && no_scale);
}

void RootExport::SaveNodes(Saver& saver, ChainsBlock& chains_block)
{
	// float  INode::GetVisibility(TimeValue t,Interval *valid=NULL)
	{//test duplicate
		typedef map<string,IVisNode*> ObjectMap;
		ObjectMap omap;
		for(int inode=0;inode<all_nodes.size();inode++)
		{
			NextLevelNode& n=all_nodes[inode];
			string name=n.node->GetName();
			ObjectMap::iterator it=omap.find(name);
			if(it!=omap.end())
			{
				Msg("Error: Node %s. несколько объектов с таким именем.\n",n.node->GetName());	
			}else
			{
				omap[name]=n.node;
			}
		}
	}


	saver.push(C3DX_NODES);
	for(int inode=0;inode<all_nodes.size();inode++){
		NextLevelNode& n=all_nodes[inode];
		ExportNode(saver, n, chains_block);
	}
	saver.pop();
}

bool IsNodeMesh(IVisNode* node)
{
	const char* name=node->GetName();
	if(name[0]=='B' && name[1]=='i' && name[2]=='p')
		return false;
	if(strcmp(name,"logic bound")==0)
		return false;
	if(pRootExport->IsLogicNodeBound(node))
		return false;

	if(node->IsBone())
		return false;

	if(node->GetType() == MESH_TYPE)
	{
		return true;
	}
	return false;
}

bool IsNodeLight(IVisNode* node)
{
	const char* name=node->GetName();
	if(name[0]=='B' && name[1]=='i' && name[2]=='p')
		return false;
	if(strcmp(name,"logic bound")==0)
		return false;
	if(pRootExport->IsLogicNodeBound(node))
		return false;

	if(node->GetType() == LIGHT_TYPE)
	{
		return true;
	}
	return false;
}
bool IsNodeCamera(IVisNode* node)
{
	string name=node->GetName();
	if(name != "Camera01")
		return false;
	if(node->GetType() == CAMERA_TYPE)
		return true;
	return false;
}

void RootExport::SaveMeshes(Saver& saver)
{
//для проверки просто эту строчку вставлял	pIgame->SetTime(0);
	saver.push(C3DX_MESHES);
	for(int inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		for(int imesh=-1;imesh<(int)n.additional.size();imesh++)
		{
			IVisNode* node=n.node;
			if(imesh!=-1)
				node=n.additional[imesh];

			if (IsNodeMesh(node)) 
			{
				ExportMesh mesh(saver,node->GetName());
				mesh.Export(node->GetMesh(),n.current);
			}
		}
	}
	saver.pop();
}

void RootExport::SaveLights(Saver& saver, ChainsBlock& chains_block)
{
	saver.push(C3DX_LIGHTS);
	for(int inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		if(IsNodeLight(n.node))
		{
			ExportLight light(saver,n.node->GetName());
			light.Export(n.node->GetLight(0),n.current, chains_block);
		}
	}
	saver.pop();
}
void RootExport::SaveCamera(Saver& saver)
{
	for(int inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		if(IsNodeCamera(n.node))
		{
			IVisCamera* camera = n.node->GetCamera();
			if(!camera)
				return;
			saver.push(C3DX_CAMERA);
				saver.push(C3DX_CAMERA_HEAD);
				saver<<n.current;
				saver<<camera->GetFov();
				saver.pop();
			saver.pop();
			return;
		}
	}
}

void SaveMatrix(Saver& s ,Matrix3& m)
{
	for(int i=0;i<3;i++)
	{
		Point3 p=m.GetColumn3(i);
		s<<p.x;
		s<<p.y;
		s<<p.z;
	}

	{
		Point3 p=m.GetRow(3);
		s<<p.x;
		s<<p.y;
		s<<p.z;
	}
}

void RootExport::ExportNode(Saver& saver, NextLevelNode& n, ChainsBlock& chains)
{
	saver.push(C3DX_NODE);

	const char* name=n.node->GetName();
	{
		saver.push(C3DX_NODE_HEAD);
		saver<<n.node->GetName();
		saver<<n.current;
		saver<<n.parent;
		saver.pop();
	}

//	Msg("Node: %i %i %s\n",inode,iparent,pnode->GetName());

	for(int i=0;i<animation_data.animation_chain.size();i++)
	{
		AnimationChain& ac=animation_data.animation_chain[i];
		saver.push(C3DX_NODE_CHAIN);

		{
			saver.push(C3DX_NODE_CHAIN_HEAD);
			saver<<i;
			saver<<ac.name;
			saver.pop();
		}

		ExportMatrix(saver, n,ac.begin_frame,max(ac.end_frame-ac.begin_frame+1,1),ac.cycled, chains);
		saver.pop();
	}
	{
		saver.push(C3DX_NODE_INIT_MATRIX);
		Matrix3 m=n.GetWorldTM(GetBaseFrameMax());
		if(n.pParent==0)
		{
			m.NoScale();
			m.NoRot();
		}

		RightToLeft(m);

		SaveMatrix(saver,m);
		saver.pop();
	}
	saver.pop();

	//Msg("Готова нода %s, с цепочкой: %i, %i, %i  (animation_data.animation_chain.size() = %i)\n",
	//	 name, chains.position.size(), chains.rotation.size(), chains.scale.size(), animation_data.animation_chain.size());
}

int RootExport::ToMaxTime(int frame)
{
	int out=frame*time_frame;//-time_start;
//	xassert(out>=time_start && out<=time_end);
	return out;
}

void RootExport::ExportMatrix(Saver& saver, NextLevelNode& n,int interval_begin,int interval_size,bool cycled, ChainsBlock& block)
{
	IVisNode * node=n.node;
	IVisNode* pParent=n.pParent;
	typedef VectTemplate<3> VectPosition;
	typedef VectTemplate<4> VectRotation;
	typedef VectTemplate<1> VectScale;
	typedef InterpolatePosition<VectPosition> IPosition;
	typedef InterpolatePosition<VectRotation> IRotation;
	typedef InterpolatePosition<VectScale> IScale;
	vector<VectPosition> position;
	vector<VectRotation> rotation;
	vector<VectScale> scale;
	IPosition ipos(0);
	IRotation irot(TypeCorrectCycleQuat);
	IScale iscale(0);
	const char* node_name=node->GetName();
	vector<bool> visibility;

	bool is_2=true;
	/*
	if(n.parent>=0)
	{
	NextLevelNode& parent=all_nodes[n.parent];
	is_2=parent.parent<0;
	}else
	is_2=false;
	/*/
	is_2=n.parent==0;
	/**/
	bool save=false;
	if(false && strcmp(node_name,"Bip02 R Thigh")==0)
	{
		save=true;
	}

	for(int icurrent=0;icurrent<interval_size;icurrent++)
	{
		AffineParts ap;

		int current_max_time=ToMaxTime(icurrent+interval_begin);

		Matrix3 m=n.GetLocalTM(current_max_time);
		if(is_2)
		{
			Matrix3 mparent=pRootExport->cache.GetWorldTM(pParent,current_max_time);
			mparent.NoTrans();
			m=m*mparent;
		}

		RightToLeft(m);

		if(!pParent)
		{
			m.NoRot();
			m.NoScale();
		}

		decomp_affine(m, &ap);
		Quat q = ap.q;
		if(save)
			Msg("  {%.6f, %.6f, %.6f, %.6f,},\n",q.x,q.y, q.z, q.w);
		//		Msg("  pos=%f %f %f\n",ap.t.x,ap.t.y, ap.t.z);
		VectPosition pos;
		pos[0]=ap.t.x;
		pos[1]=ap.t.y;
		pos[2]=ap.t.z;

		position.push_back(pos);

		VectRotation rot,rot_inv;
		rot[0]=ap.q.x;
		rot[1]=ap.q.y;
		rot[2]=ap.q.z;
		rot[3]=ap.q.w;
		//Матрица вращения может скачком менять знак.
		//Необходимо препятствовать этому явлению.
		if(rotation.empty())
		{
			rotation.push_back(rot);
		}else
		{
			VectRotation rot_prev=rotation.back();
			rot_inv[0]=-ap.q.x;
			rot_inv[1]=-ap.q.y;
			rot_inv[2]=-ap.q.z;
			rot_inv[3]=-ap.q.w;
			float d=rot_prev.distance(rot);
			float d_inv=rot_prev.distance(rot_inv);
			if(d<d_inv)
				rotation.push_back(rot);
			else
				rotation.push_back(rot_inv);
		}

		VectScale s;
		float mids=(ap.k.x+ap.k.y+ap.k.z)/3;
		float deltas=fabsf(mids-ap.k.x)+fabsf(mids-ap.k.y)+fabsf(mids-ap.k.z);
		if(ap.f<0 && mids>0)
		{
			mids=-mids;
		}

		//if(save)Msg("  s=%.3f\n",mids);

		s[0]=mids;
		scale.push_back(s);

		{
			float visible=node->GetVisibility(current_max_time);
			visibility.push_back(visible>0.5f);
		}
	}

	ipos.Interpolate(position,position_delta,cycled);
	int position_index = block.put(ipos);

	/*
	saver.push(C3DX_NODE_POSITION);
	if(!ipos.Save(saver))
	Msg("Error: Node %s. ITPL_UNKNOWN POSITION\n",node->GetName());
	saver.pop();
	*/

	saver.push(C3DX_NODE_POSITION_INDEX);
	saver<<position_index;
	saver<<int(ipos.size());
	saver.pop();

	if(save)
	{
		int k=0;
	}

	irot.Interpolate(rotation,rotation_delta,cycled);
	int rotation_index = block.put(irot);

	if(save)
	{
		//for(int icurrent=0;icurrent<interval_size;icurrent++)
		//{
		//	VectRotation r=irot.GetTestValue(icurrent);
		//	Msg(" qi=%.3f %.3f %.3f %.3f\n",r[0],r[1], r[2], r[3]);
		//}
		for(int iout=0;iout<irot.out_data.size();iout++)
		{
			IRotation::One& o=irot.out_data[iout];
			Msg("%i begin=%f, size=%f\n",iout, float(o.interval_begin) / float(irot.interval_size),
				float(o.interval_size) / float(irot.interval_size));
			{
				for(int i=0;i<4;i++)
				{
					Msg("  %i %f %f %f %f\n",i,o.a0[i],o.a1[i],o.a2[i],o.a3[i]);
				}
			}
		}
	}

	/*
	saver.push(C3DX_NODE_ROTATION);
	if(!irot.Save(saver))
	Msg("Error: Node %s. ITPL_UNKNOWN ROTATION\n",node->GetName());
	saver.pop();
	*/

	saver.push(C3DX_NODE_ROTATION_INDEX);
	saver<<rotation_index;
	saver<<int(irot.size());
	saver.pop();

	iscale.Interpolate(scale,scale_delta,cycled);
	int scale_index = block.put(iscale);

	/*
	saver.push(C3DX_NODE_SCALE);
	if(!iscale.Save(saver))
	Msg("Error: Node %s. ITPL_UNKNOWN SCALE\n",node->GetName());
	saver.pop();
	*/

	saver.push(C3DX_NODE_SCALE_INDEX);
	saver<<scale_index;
	saver<<int(iscale.size());
	saver.pop();

	VisibilityChain ivis(visibility, interval_size, cycled);
	int visibility_index = block.put(ivis);

	/*
	saver.push(C3DX_NODE_VISIBILITY);
	ivis.Save(saver);
	saver.pop();
	*/

	saver.push(C3DX_NODE_VISIBILITY_INDEX);
	saver<<visibility_index;
	saver<<int(ivis.size());
	saver.pop();


	/* для отладки {{{*/
	if(dbg_show_position)
	{
		for(int i=0;i<ipos.out_data.size();i++)
		{
			IPosition::One& o=ipos.out_data[i];
			if(o.itpl==ITPL_SPLINE || o.itpl==ITPL_LINEAR)
			{
				if(o.itpl==ITPL_SPLINE)
					Msg("SPLINE");
				else
					Msg("LINEAR");
				Msg(" %i %i x=(%f %f %f %f) y=(%f %f %f %f) z=(%f %f %f %f)\n",
					o.interval_begin,o.interval_size,
					o.a0[0],o.a1[0],o.a2[0],o.a3[0],
					o.a0[1],o.a1[1],o.a2[1],o.a3[1],
					o.a0[2],o.a1[2],o.a2[2],o.a3[2]
					);
			}else
				if(o.itpl==ITPL_CONSTANT)
				{
					Msg("CONST %i %i p=%f %f %f\n",o.interval_begin,o.interval_size,
						o.a0[0],o.a0[1],o.a0[2]);
				}else
				{
					Msg("Error: Node %s. UNKN %i %i\n",o.interval_begin,o.interval_size);
				}
		}
	}

	if(dbg_show_scale)
	{
		for(int i=0;i<iscale.out_data.size();i++)
		{
			IScale::One& o=iscale.out_data[i];
			if(o.itpl==ITPL_SPLINE || o.itpl==ITPL_LINEAR)
			{
				if(o.itpl==ITPL_SPLINE)
					Msg("SPLINE");
				else
					Msg("LINEAR");
				Msg(" %i %i s=(%f %f %f %f)\n",
					o.interval_begin,o.interval_size,o.a0[0],o.a1[0],o.a2[0],o.a3[0]);
			}else
				if(o.itpl==ITPL_CONSTANT)
				{
					Msg("CONST %i %i s=%f\n",o.interval_begin,o.interval_size,o.a0[0]);
				}else
				{
					Msg("Error: UNKN %i %i\n",o.interval_begin,o.interval_size);
				}
		}
	}

	if(dbg_show_rotation)
	{
		for(int i=0;i<irot.out_data.size();i++)
		{
			IRotation::One& o=irot.out_data[i];
			if(o.itpl==ITPL_SPLINE || o.itpl==ITPL_LINEAR)
			{
				if(o.itpl==ITPL_SPLINE)
					Msg("SPLINE");
				else
					Msg("LINEAR");
				Msg(" %i %i x=(%f %f %f %f) y=(%f %f %f %f) z=(%f %f %f %f) w=(%f %f %f %f)\n",
					o.interval_begin,o.interval_size,
					o.a0[0],o.a1[0],o.a2[0],o.a3[0],
					o.a0[1],o.a1[1],o.a2[1],o.a3[1],
					o.a0[2],o.a1[2],o.a2[2],o.a3[2],
					o.a0[3],o.a1[3],o.a2[3],o.a3[3]
					);
			}else
				if(o.itpl==ITPL_CONSTANT)
				{
					Msg("CONST %i %i p=%f %f %f %f\n",o.interval_begin,o.interval_size,
						o.a0[0],o.a0[1],o.a0[2],o.a0[3]);
				}else
				{
					Msg("Error: Node %s. UNKN %i %i\n",o.interval_begin,o.interval_size);
				}
		}
	}/*}}}*/
}

IVisNode* RootExport::Find(const char* name)
{
	for(int loop = 0; loop <pIgame->GetRootNodeCount();loop++)
	{
		IVisNode * pGameNode = pIgame->GetRootNode(loop);
		IVisNode * out=FindRecursive(pGameNode,name);
		if(out)
			return out;
	}

	return NULL;
}

IVisNode* RootExport::FindRecursive(IVisNode* pGameNode,const char* name)
{
	if(strcmp(pGameNode->GetName(),name)==0)
		return pGameNode;
	for(int count=0;count<pGameNode->GetChildNodeCount();count++)
	{
		IVisNode * pChildNode = pGameNode->GetChildNode(count);
		IVisNode * out=FindRecursive(pChildNode,name);
		if(out)
			return out;
	}

	return NULL;
}

void RootExport::BuildMaterialList()
{
	materials.clear();
	int size=pIgame->GetRoolMaterialCount();
	for(int i=0;i<size;i++)
	{
		IVisMaterial *mat=pIgame->GetRootMaterial(i);
		if (mat->IsMultiType())
		{
			for(int j=0; j<mat->GetSubMaterialCount(); j++)
			{
				IVisMaterial *sub_mat=  mat->GetSubMaterial(j);
				materials.push_back(sub_mat);
			}
		}else
		{
			materials.push_back(mat);
		}
	}

	
	for(int i=0;i<materials.size();i++)
	{//duplicate material
		IVisMaterial* mat_i=materials[i];
		const char* mat_name_i=mat_i->GetName();
		for(int j=i+1;j<materials.size();j++)
		{
			IVisMaterial* mat_j=materials[j];
			const char* mat_name_j=mat_j->GetName();
			if(strcmp(mat_name_i,mat_name_j)==0)
			{
				Msg("Error: Материал с именем %s встречается несколько раз.\n",mat_name_i);
			}
		}
	}

	if(!duplicate_material_names.empty())
	{
		vector<IVisMaterial*> materials_correct(duplicate_material_names.size(),NULL);
		for(int i=0;i<materials.size();i++)
		{
			IVisMaterial* mat=materials[i];
			const char* mat_name=mat->GetName();
			int idx;
			for(idx=0;idx<duplicate_material_names.size();idx++)
				if(duplicate_material_names[idx]==mat_name)
					break;
			if(idx<duplicate_material_names.size())
			{
				materials_correct[idx]=mat;
			}else
			{
				Msg("Error: Материал %s не найден в оригинальном файле.\n",mat_name);
				throw GameExporterError();
			}
		}
		materials=materials_correct;
	}

	for(int i=0;i<materials.size();i++)
	{//duplicate material
		IVisMaterial* mat_i=materials[i];
		if(mat_i==NULL)
			continue;
		const char* mat_name_i=mat_i->GetName();
		string diffname_i,fillcolorname_i;
		int num_tex = mat_i->GetTexmapCount();
		for(int t=0; t<num_tex; t++)
		{
			if(mat_i->GetTexmap(t)->GetSlot() == ID_DI)
				diffname_i = GetFileName(mat_i->GetTexmap(t)->GetBitmapFileName());
			if(mat_i->GetTexmap(t)->GetSlot() == ID_FI)
				fillcolorname_i = GetFileName(mat_i->GetTexmap(t)->GetBitmapFileName());
		}
		for(int j=i+1;j<materials.size();j++)
		{
			IVisMaterial* mat_j=materials[j];
			if(mat_j==NULL)
				continue;
			const char* mat_name_j=mat_j->GetName();
			string diffname_j,fillcolorname_j;
			int num_tex = mat_j->GetTexmapCount();
			for(int t=0; t<num_tex; t++)
			{
				if(mat_j->GetTexmap(t)->GetSlot() == ID_DI)
					diffname_j = GetFileName(mat_j->GetTexmap(t)->GetBitmapFileName());
				if(mat_j->GetTexmap(t)->GetSlot() == ID_FI)
					fillcolorname_j = GetFileName(mat_j->GetTexmap(t)->GetBitmapFileName());
			}
			if(diffname_i == diffname_j && fillcolorname_i != fillcolorname_j)
			{
				Msg("Error: Материалы (\"%s\", \"%s\") имеют одинаковую diffuse текстуру и разную текстру fiter color.\n",mat_name_i, mat_name_j);
			}
		}
	}


}

void RootExport::CheckMaterialInAnimationGroup()
{
	for(int imat=0;imat<materials.size();imat++)
	{
		IVisMaterial* mat=materials[imat];
		if(mat==NULL)
			continue;
		bool found=false;
		for(int iag=0;iag<animation_data.animation_group.size();iag++)
		{
			AnimationGroup& ag=animation_data.animation_group[iag];
			for(int im=0;im<ag.materials.size();im++)
			{
				if(mat==ag.materials[im])
				{
					found=true;
					break;
				}
			}
		}

		if(!found)
		{
			Msg("Материал %s не входит ни в одну анимационную группу.\n",mat->GetName());
		}
	}
}

void RootExport::SaveMaterials(Saver& saver, ChainsBlock& chains_block)
{
	saver.push(C3DX_MATERIAL_GROUP);
	materials.clear();
	int size=pIgame->GetRoolMaterialCount();
	for(int i=0;i<size;i++)
	{
		IVisMaterial *mat=pIgame->GetRootMaterial(i);
		if (mat->IsMultiType())
		{
			for(int j=0; j<mat->GetSubMaterialCount(); j++)
			{
				IVisMaterial *sub_mat=  mat->GetSubMaterial(j);
				materials.push_back(sub_mat);
				ExportMaterial m(saver);
				m.Export(saver, sub_mat, chains_block);
			}
		}else
		{
			materials.push_back(mat);
			ExportMaterial m(saver);
			m.Export(saver, mat, chains_block);
		}
	}
	saver.pop();
}

int RootExport::FindMaterialIndex(IVisMaterial* mat)
{
	for(int i=0;i<materials.size();i++)
	if(materials[i]==mat)
		return i;

	return -1;
}

int RootExport::FindNodeIndex(IVisNode* node)
{
	MAP_NODE::iterator it=node_map.find(node);
	if(it!=node_map.end())
	{
		xassert(it->first==node);
		return it->second;
	}
	return -1;
}

void RootExport::SaveBasement(Saver& saver)
{
	if(!all_nodes.empty() && node_base)
	{
		//find root
		NextLevelNode* root=&all_nodes[0];
		const char* root_name=root->node->GetName();
		xassert(root->pParent==NULL);

		//
		if(node_base->GetType() == MESH_TYPE)
		{
			ExportBasement base(saver,node_base->GetName());
			Matrix3 m_root=root->GetWorldTM(GetBaseFrameMax());
			m_root.NoRot();
			m_root.NoScale();
			base.Export(node_base->GetMesh(),node_base,m_root);
		}else
		{
			Msg("Объект с именем _base_ должен состоять из треугольников.\n");
		}
	}
}

void RootExport::FindLogicBounds(IVisMesh*& pLogicBound,vector<sLogicNodeBound>& node_bounds)
{
	pLogicBound=NULL;
	node_bounds.clear();
	for(int inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		for(int imesh=-1;imesh<(int)n.additional.size();imesh++)
		{
			IVisNode* node=n.node;
			if(imesh!=-1)
				node=n.additional[imesh];

			const char* name=node->GetName();
			if(strcmp(name,"logic bound")==0)
			{
				if(!(node->GetType() == MESH_TYPE))
				{
					Msg("Объект с именем logic bound должен состоять из треугольников.\n");
					break;
				}
				pLogicBound=node->GetMesh();
			}

			if(IsLogicNodeBound(node))
			{
				if(!(node->GetType() == MESH_TYPE))
				{
					Msg("Логический bound (%s) должен состоять из треугольников. \n", name);
				}else
				{
					sLogicNodeBound s;
					s.pobject=node->GetMesh();
					s.inode=inode;
					node_bounds.push_back(s);
				}
			}
		}
	}
}

void RootExport::SaveLogicBounds(Saver& saver,IVisMesh* pLogicBound,vector<sLogicNodeBound>& node_bounds)
{
	if(pLogicBound)
		SaveLogicBoundRoot(saver,pLogicBound);

	vector<sLogicNodeBound>::iterator it;
	FOR_EACH(node_bounds,it)
	{
		SaveLogicBoundLocal(saver,it->pobject,it->inode);
	}
}


void RootExport::SaveLogicBoundRoot(Saver& saver,IVisMesh* pobject)
{
	NextLevelNode* root=&all_nodes[0];

	sBox6f box;
	box.SetInvalidBox();
	Matrix3 m_root=root->GetLocalTM(GetBaseFrameMax());
	m_root.NoScale();
	m_root.NoRot();
	Matrix3 inv_root=Inverse(m_root);
	int num_vertex=pobject->GetNumberVerts();
	for(int i=0;i<num_vertex;i++)
	{
		Point3 pos0=pobject->GetVertex(i);
		Point3 pos=pos0*inv_root;
		Vect3f p(pos.x,pos.y,pos.z);
		RightToLeft(p);
		box.AddBound(p);
	}

	saver.push(C3DX_LOGIC_BOUND);
	saver<<(int)0;
	saver<<box.min;
	saver<<box.max;
	saver.pop();
}

void RootExport::SaveLogicBoundLocal(Saver& saver,IVisMesh* pobject,int inode)
{
	NextLevelNode* parent=&all_nodes[inode];

	sBox6f box;
	box.SetInvalidBox();
	Matrix3 m_root=parent->GetWorldTM(GetBaseFrameMax());
	Matrix3 inv_root=Inverse(m_root);
	int num_vertex=pobject->GetNumberVerts();
	for(int i=0;i<num_vertex;i++)
	{
		Point3 pos0=pobject->GetVertex(i);
		Point3 pos=pos0*inv_root;
		Vect3f p(pos.x,pos.y,pos.z);
		RightToLeft(p);
		box.AddBound(p);
	}

	saver.push(C3DX_LOGIC_BOUND_LOCAL);
	saver<<inode;
	saver<<box.min;
	saver<<box.max;
	saver.pop();
}

IVisMaterial* RootExport::FindMaterial(const char* name)
{
	for(int i=0;i<materials.size();i++)
	{
		IVisMaterial *mat=materials[i];
		if(mat==NULL)
			continue;
		const char* mat_name=mat->GetName();
		if(strcmp(name,mat_name)==0)
			return mat;
	}

	return NULL;
}

void RootExport::LoadMapNode(CLoadIterator it,MAP_NODE& nodes,const char* error_message)
{
	vector<string> non_delete;
	it>>non_delete;
	nodes.clear();

	vector<string>::iterator itn;
	FOR_EACH(non_delete,itn)
	{
		string& node_name=*itn;
		IVisNode* node=Find(node_name.c_str());
		if(node)
			nodes[node]=1;
		else
			Msg(error_message,node_name.c_str());
	}
}

void RootExport::SaveMapNode(Saver& saver,MAP_NODE& nodes,int idx)
{
	vector<string> non_delete;
	MAP_NODE::iterator itnode;
	FOR_EACH(nodes,itnode)
	{
		string name=itnode->first->GetName();
		non_delete.push_back(name);
	}

	saver.push(idx);
	saver<<non_delete;
	saver.pop();
}


void RootExport::LoadNonDeleteNode(CLoadIterator it)
{
	LoadMapNode(it,nondelete_node,"Не могу найти ноду %s. Она перестанет быть неудаляемой.\n");
}

void RootExport::SaveNonDeleteNode(Saver& saver)
{
	SaveMapNode(saver,nondelete_node,C3DX_NON_DELETE_NODE);
}

void RootExport::LoadLogicNode(CLoadIterator it)
{
	LoadMapNode(it,logic_node,"Не могу найти ноду %s. Она перестанет быть логической.\n");
}

void RootExport::SaveLogicNode(Saver& saver)
{
	SaveMapNode(saver,logic_node,C3DX_LOGIC_NODE);
}

Matrix3 NextLevelNode::GetLocalTM(int max_time)
{
	if(!pParent)
	{
		Matrix3 cur=pRootExport->cache.GetWorldTM(node,max_time);
		return cur;
	}
	Matrix3 m=pRootExport->cache.GetWorldTM(node,max_time);
	Matrix3 mparent=pRootExport->cache.GetWorldTM(pParent,max_time);
	mparent.Invert();
	return m*mparent;
}

Matrix3 NextLevelNode::GetWorldTM(int max_time)
{
	Matrix3 cur=pRootExport->cache.GetWorldTM(node,max_time);
	if(parent==-1)
	{
		cur.NoRot();
		cur.NoScale();
	}
	return cur;
}

void RootExport::CalcNodeHaveMesh()
{
	if(all_nodes.size()<1)
		return;
	all_nodes[0].is_nodelete=true;
	for(int inode=1;inode<all_nodes.size();inode++)
	{
		NextLevelNode& cur_node=all_nodes[inode];
		cur_node.is_nodelete=false;
	}

	for(inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& cur_node=all_nodes[inode];
		if(ParentInEnemyanimationGroup(cur_node))
		{
			all_nodes[cur_node.parent].is_nodelete=true;
		}


		for(int imesh=-1;imesh<(int)cur_node.additional.size();imesh++)
		{
			IVisNode* node=cur_node.node;
			const char* name=node->GetName();
			MAP_NODE::iterator it_non_del=nondelete_node.find(node);
			if(it_non_del!=nondelete_node.end())
			{
				cur_node.is_nodelete=true;
			}
			if(imesh!=-1)
				node=cur_node.additional[imesh];

			if(node->GetType() == LIGHT_TYPE)
			{
				cur_node.is_nodelete = true;
			}else
				if(node->GetType() == MESH_TYPE)
				{
					IVisMesh* mesh = node->GetMesh();
					if(!mesh->GetSkin())
					{
						cur_node.is_nodelete=true;
					}else
					{
						IVisSkin* skin = mesh->GetSkin();
						int num_skin_verts=skin->GetVertexCount();
						for(int ipnt=0;ipnt<num_skin_verts;ipnt++)
						{
							int nbones=skin->GetNumberBones(ipnt);

							if(nbones==0)
							{
								cur_node.is_nodelete=true;
							}else
							{
								for(int ibone=0;ibone<nbones;ibone++)
								{
									IVisNode* pnode=skin->GetIBone(ipnt,ibone);
									int inode=pRootExport->FindNodeIndex(pnode);
									if(inode<0)
									{
										Msg("Error: %s не прилонкованна к group center\n",pnode->GetName());
										return;
									}
									xassert(inode>=0 && inode<all_nodes.size());
									all_nodes[inode].is_nodelete=true;
								}
							}
						}
					}
				}
		}
	}

	if(dbg_show_info_delete_node)
		for(inode=0;inode<all_nodes.size();inode++)
		{
			NextLevelNode& cur_node=all_nodes[inode];
			if(!cur_node.is_nodelete)
				Msg("INFO: Not have mesh node=%s\n",cur_node.node->GetName());
		}

}

void RootExport::LoadLogos(CLoadDirectory it)
{
	while(CLoadData* ld=it.next())
	switch(ld->id)
	{
	case C3DX_LOGO_COORD:
		CLoadIterator li(ld);
		sLogo logo;
		li>>logo.TextureName;
		li>>logo.rect.min;
		li>>logo.rect.max;
		li>>logo.angle;
		logos.push_back(logo);
		break;
	}
}

void RootExport::SaveLogos(Saver& saver)
{
	if (logos.size()>0)
	{
		saver.push(C3DX_LOGOS);
		int size=logos.size();
		for(int i=0;i<size;i++)
		{
			saver.push(C3DX_LOGO_COORD);
			saver<<logos[i].TextureName;
			saver<<logos[i].rect.min;
			saver<<logos[i].rect.max;
			saver<<logos[i].angle;
			saver.pop();
		}
		saver.pop();
	}
}


void RootExport::CalcBoundBox()
{
	bound_box.SetInvalidBox();
	for(int inode=0;inode<all_nodes.size();inode++)
	{
		NextLevelNode& n=all_nodes[inode];
		for(int imesh=-1;imesh<(int)n.additional.size();imesh++)
		{
			IVisNode* node=n.node;
			if(imesh!=-1)
				node=n.additional[imesh];

			Matrix3 mparent=n.GetWorldTM(GetBaseFrameMax());
			Point3 max_pos=mparent.GetTrans();
			Vect3f pos(max_pos.x,max_pos.y,max_pos.z);

			bound_box.AddBound(pos);


			if (IsNodeMesh(node)) 
			{
				IVisMesh* mesh=node->GetMesh();
				int  num_verts=mesh->GetNumberVerts();
				for(int ivertex=0;ivertex<num_verts;ivertex++)
				{
					Point3  max_pos=mesh->GetVertex(ivertex); 
					Vect3f pos(max_pos.x,max_pos.y,max_pos.z);
					bound_box.AddBound(pos);
				}
			}
		}
	}

	float radius=bound_box.GetRadius();
	position_delta=radius*relative_position_delta;
}

bool RootExport::ParentInEnemyanimationGroup(const NextLevelNode& n)
{
	if(n.pParent==NULL)
		return false;
	int idx_node=animation_data.FindAnimationGroupIndex(n.node);
	int idx_parent=animation_data.FindAnimationGroupIndex(n.pParent);
	return idx_node!=idx_parent;
}

bool RootExport::IsLogicNodeBound(IVisNode* node)
{
	const char* name=node->GetName();
	if(strstr(name,"logic node bound"))
		return true;
	MAP_NODE::iterator it = bound_node.find(node);
	if (it != bound_node.end())
		return true;
	return false;
}

int RootExport::GetBaseFrame()
{
	return 0;//Баг гдето, см CalcNodeAnimate icurrent=-1 - это к этому же затычка.
	if(animation_data.animation_chain.empty())
		return 0;
	AnimationChain& ac=animation_data.animation_chain[0];
	return ac.begin_frame;
}
