#include "stdafx.h"
#include "ExportMesh.h"
#include "DebugDlg.h"
ExportMesh::ExportMesh(Saver& saver_,const char* name_)
:saver(saver_),name(name_)
{
	pobject=NULL;
	inode_current=-1;
}

void ExportMesh::Export(IVisMesh* pobject_,int inode)
{
	inode_current=inode;

	pobject=pobject_;

	//pobject->SetCreateOptimizedNormalList();//3DMax 7

	//pobject->InitializeData();

	if(pobject->GetNumberVerts()<=0)
	{
		Msg("Error: %s cannot have vertex.\n",name);
		return;
	}

	if(pobject->GetNumberFaces()<=0)
	{
		Msg("Error: %s cannot have faces.\n",name);
		return;
	}

	set<IVisMaterial*> materials;
	int n = pobject->GetNumberFaces();
	for (int i=0; i<n; i++)
	{
		IVisMaterial* mat = pobject->GetMaterialFromFace(i);
		materials.insert(mat);
	}
	set<IVisMaterial*>::iterator it;
	for (it= materials.begin(); it!=materials.end(); ++it)
	{
		IVisMaterial*  material = *it;
		saver.push(C3DX_MESH);
			saver.push(C3DX_MESH_HEAD);
				saver<<inode;
				const char* material_name=material->GetName();
				int mat_index=pRootExport->FindMaterialIndex(material);
				if(mat_index<0)
				{
					Msg("Error: %s cannot have material.\n",name);
				}

				saver<<mat_index;
			saver.pop();

			saver.push(C3DX_MESH_NAME);
				saver<<name;
			saver.pop();

			ExportOneMesh(material);

		saver.pop();
	}
}

struct RePoints
{
	struct ATTR
	{
		Point2 texel;
		Point3 normal;
		Point3 face_norm;
		Point2 texel2;
	};

	struct SKIN
	{
		float weight;
		int   inode;
	};

	vector<ATTR> attr;
	vector<SKIN> skin;
	int new_base_index;

	RePoints():new_base_index(0)
	{
	}

	bool texel_eq(Point2& t1,Point2 & t2)
	{
		float eps=1e-4f;
		return fabsf(t1.x-t2.x)<eps && fabsf(t1.y-t2.y)<eps;
	}

	bool normal_eq(Point3& t1,Point3 & t2)
	{
		float eps=1e-4f;
		return fabsf(t1.x-t2.x)<eps && fabsf(t1.y-t2.y)<eps && fabsf(t1.z-t2.z)<eps;
	}

	void AddUnicalInt(Point2 texel,Point3 normal,Point3 face_norm,Point2 texel2)
	{
		for(int i=0;i<attr.size();i++)
			if(texel_eq(attr[i].texel,texel))
			if(texel_eq(attr[i].texel2,texel2))
			{
				if(normal_eq(attr[i].normal,normal))
				{
					attr[i].face_norm+=face_norm;
					return;
				}
			}
		ATTR a;
		a.texel=texel;
		a.normal=normal;
		a.face_norm=face_norm;
		a.texel2=texel2;
		attr.push_back(a);
	}

	int FindNewPoint(Point2 texel,Point3 normal,Point2 texel2)
	{
		xassert(attr.size());

		for(int i=0;i<attr.size();i++)
		{
			if(texel_eq(attr[i].texel,texel) && normal_eq(attr[i].normal,normal) &&
				texel_eq(attr[i].texel2,texel2)
				)
				return new_base_index+i;
		}

		xassert(0);
		return new_base_index;
	}
};

struct SortByWeight
{
	bool operator ()(const RePoints::SKIN& p0,const RePoints::SKIN& p1)const
	{
		return !(p0.weight<p1.weight);
	}
};

void ExportMesh::DeleteSingularPolygon(vector<FaceEx>& faces)
{
	int deleted=0;
	for(int i=0;i<faces.size();i++)
	{ // удаление вырожденных треугольников
		FaceEx &p=faces[i];
		if(p.vert[0]==p.vert[1] || 
		   p.vert[0]==p.vert[2] || 
		   p.vert[1]==p.vert[2])
		{
			faces.erase(faces.begin()+i);
			i--;
			deleted++;
		}
	}

	if(dbg_show_info_polygon)
		Msg("info: %s deleted=%i\n",name,deleted);
}

bool ExportMesh::ExportOneMesh(IVisMaterial* mtr)
{
	set<int> errorVertex;
	const int uv_range = 100;
	/*
	Mesh
	int getNumMaps() 
	BOOL mapSupport(int mp) 
	int getNumMapVerts(int mp) 
	UVVert *mapVerts(int mp)  
	TVFace *mapFaces(int mp) 
	*/
	vector<RePoints> pnt(pobject->GetNumberVerts());
	vector<FaceEx> faces;

	bool is_uv=false;
	bool is_uv2=false;

	if(pobject->IsMapSupport(1) && pobject->GetNumMapVerst(1)>0)
	{
		is_uv=true;
	}

	if(pobject->IsMapSupport(2) && pobject->GetNumMapVerst(2)>0)
	{
		is_uv2=true;
	}



	{
		int num_faces=pobject->GetNumberFaces();
		for(int i=0;i<num_faces;i++)
		{
			if (pobject->GetMaterialFromFace(i) == mtr)
			{
				faces.push_back(pobject->GetFace(i));
			}
		}
	}

	for(int i=0;i<3;i++)
		igame_to_mesh_polygon_index[i]=i;
	//if(!faces.empty())
	//{
	//	FaceEx& f=faces[0];
	//	Face& face=mesh->faces[f.meshFaceIndex];

	//	for(int i=0;i<3;i++)
	//	{
	//		for(int j=0;j<3;j++)
	//		if(face.v[j]==f.vert[i])
	//		{
	//			//face[igame_to_mesh_polygon_index[i]]==f.vert[i]
	//			igame_to_mesh_polygon_index[i]=j;
	//			break;
	//		}
	//	}

	//	xassert(igame_to_mesh_polygon_index[0]!=igame_to_mesh_polygon_index[1]);
	//	xassert(igame_to_mesh_polygon_index[0]!=igame_to_mesh_polygon_index[2]);
	//	xassert(igame_to_mesh_polygon_index[1]!=igame_to_mesh_polygon_index[2]);
	//}

	DeleteSingularPolygon(faces);//Потом включить

	bool uv_not_match=false;
	for(int iface=0;iface<faces.size();iface++)
	{
		FaceEx& f=faces[iface];
		UVFace tex_face1;
		UVFace tex_face2;
		if(is_uv)
			tex_face1 = pobject->GetMapVertex(1,f);
		if(is_uv2)
			tex_face2 = pobject->GetMapVertex(2,f);

		Point3 face_normal;
		{//calc face normal

			Point3 pos0=pobject->GetVertex(f.vert[0]);
			Point3 pos1=pobject->GetVertex(f.vert[1]);
			Point3 pos2=pobject->GetVertex(f.vert[2]);
			face_normal=(pos0-pos1)^(pos1-pos2);//CROSS PRODUCT
			face_normal=face_normal.Normalize();
		}

		for(int j=0;j<3;j++)
		{
			//xassert(f.v[j]<pnt.size());
			//xassert(f.norm[j]<pobject->GetNumberOfNormals());
			Point2 tex(0.0f,0.0f),tex2(0.0f,0.0f);
			if(is_uv)
			{
				if(IS_SPECIAL(tex_face1.v[j].x) || IS_SPECIAL(tex_face1.v[j].y) ||
					tex_face1.v[j].x > uv_range || tex_face1.v[j].x < -uv_range ||
					tex_face1.v[j].y > uv_range || tex_face1.v[j].y < -uv_range)
				{
					Msg("Warning: Кривые uv координаты у вершины %d объект %s\n",f.vert[j],name);
					tex_face1.v[j].x = tex_face1.v[j].y = tex_face1.v[j].z = 0;
				}
				tex.Set(tex_face1.v[j].x,tex_face1.v[j].y);
			}

			if(is_uv2)
			{
				if(IS_SPECIAL(tex_face2.v[j].x) || IS_SPECIAL(tex_face2.v[j].y) ||
					tex_face2.v[j].x > uv_range || tex_face2.v[j].x < -uv_range ||
					tex_face2.v[j].y > uv_range || tex_face2.v[j].y < -uv_range)
				{
					set<int>::iterator it;
					it = errorVertex.find(f.vert[j]);
					if(it == errorVertex.end())
					{
						errorVertex.insert(f.vert[j]);
						Msg("Warning: Кривые uv2 координаты у вершины %d объект %s\n",f.vert[j],name);
					}
					tex_face2.v[j].x = tex_face2.v[j].y = tex_face2.v[j].z = 0;
				}
				//TVFace& tface=tex_faces2[f.meshFaceIndex];
				//UVVert& t=tex_verts2[tface.t[igame_to_mesh_polygon_index[j]]];
				//tex2.Set(t.x,t.y);
				tex2.Set(tex_face2.v[j].x,tex_face2.v[j].y);
			}

			Point3 norm=pobject->GetNormal(f.index,f.vert[j]);
			pnt[f.vert[j]].AddUnicalInt(tex,norm,face_normal,tex2);
		}
	}

	if(uv_not_match)
		Msg("Error: %s. UV коордионаты не совпадают.\n",name);

	int num_bones_vertex[max_bones+2];//Количество вертексов с таким количеством bones
	for(int ibone=0;ibone<max_bones+2;ibone++)
		num_bones_vertex[ibone]=0;

	int n_vertex=0;
	for(int ipnt=0;ipnt<pnt.size();ipnt++)
	{
		n_vertex+=pnt[ipnt].attr.size();
	}

	//int nxmin=480,nxmax=492;
	//float pxmin=0,pxmax=0;

	bool is_skin=false;
	if(pobject->GetSkin())
	{
		is_skin=true;
		IVisSkin* pskin=pobject->GetSkin();
		//int num_skin_verts=pskin->GetNumOfSkinnedVerts();
		//int num_verts=pnt.size();
		//xassert(num_verts<=num_skin_verts);
		for(ipnt=0;ipnt<pnt.size();ipnt++)
		{
			RePoints& p=pnt[ipnt];
			int nbones=pskin->GetNumberBones(ipnt);

			//{
			//	Point3 pos=pobject->GetVertex(ipnt);
			//	if(ipnt==nxmin || ipnt==nxmax)
			//	{
			//		int k=0;
			//	}
			//}

			//IGameSkin::VertexType vt=pskin->GetVertexType(ipnt);
			//xassert(vt!=IGameSkin::IGAME_UNKNOWN);
			if(nbones==0)
			{
				p.skin.resize(1);
				RePoints::SKIN& s=p.skin[0];
				s.weight=1;
				s.inode=inode_current;
			}else
			{
				p.skin.resize(nbones);
				float sum_weight=0;
				for(int ibone=0;ibone<nbones;ibone++)
				{
					RePoints::SKIN& s=p.skin[ibone];
					s.weight=pskin->GetWeight(ipnt,ibone);
					if(IS_SPECIAL(s.weight))
					{
						s.weight = 0.1f;
						Msg("Error: В объекте \"%s\" у вершины %d все веса нулевые\n",pobject->GetNode()->GetName(),ipnt);
					}
					IVisNode* pnode=pskin->GetIBone(ipnt,ibone);
					//const char* nodename=pnode->GetName();
					//INode *  pnode_max=pskin->GetBone (ipnt,ibone); 
					//const char* nodename_max=pnode_max->GetName();

					s.inode=pRootExport->FindNodeIndex(pnode);
					if(s.inode<0)
					{
						Msg("Error: %s не прилинкованна к group center\n",pnode->GetName());
						return false;
					}
					xassert(s.weight>=0.0f && s.weight<=1.001f);
					sum_weight+=s.weight;
				}

				if(sum_weight<0.999f)
				{
					nbones++;
					p.skin.resize(nbones);

					RePoints::SKIN& s=p.skin[nbones-1];
					s.weight=1-sum_weight;
					s.inode=inode_current;
					sum_weight+=s.weight;
				}

				xassert(fabsf(sum_weight-1)<1e-3f);
				sort(p.skin.begin(),p.skin.end(),SortByWeight());

				{
					int max_bones=pRootExport->GetMaxWeights();//4;
					if(p.skin.size()>max_bones)
					{
						p.skin.resize(max_bones);
					}

					{
						float sum=0;
						for(ibone=0;ibone<p.skin.size();ibone++)
						{
							sum+=p.skin[ibone].weight;
						}

						xassert(sum>0.1f);
						xassert(sum<=1.001f);
						sum=1/sum;
						for(ibone=0;ibone<p.skin.size();ibone++)
						{
							p.skin[ibone].weight*=sum;
						}
					}
				}
			}

			{
				int mbones=min(nbones,max_bones+1);
				num_bones_vertex[mbones]++;
			}

		}

	}

	int n_polygon=faces.size();
	vector<sVertex> new_vertex(n_vertex);
	vector<sPolygon> new_polygon(n_polygon);

	{
		int cur_vertex=0;
		for(ipnt=0;ipnt<pnt.size();ipnt++)
		{
			RePoints& p=pnt[ipnt];
			p.new_base_index=cur_vertex;
			for(int j=0;j<p.attr.size();j++,cur_vertex++)
			{
				sVertex& v=new_vertex[cur_vertex];

				Point3 pos=pobject->GetVertex(ipnt);
				Point3 norm=p.attr[j].normal;//Нормали однако возвращает не всегда правильные
				//Point3 face_normal=p.attr[j].face_norm;
				//face_normal=face_normal.Normalize();
				//if(norm%face_normal<0)// DOT PRODUCT
				//	norm=-norm;

				v.pos.x=pos.x;
				v.pos.y=pos.y;
				v.pos.z=pos.z;

				v.norm.x=norm.x;
				v.norm.y=norm.y;
				v.norm.z=norm.z;

				if(is_uv)
				{
					Point2 tex=p.attr[j].texel;
					v.uv.x=tex.x;
					v.uv.y=1-tex.y;
				}else
				{
					v.uv.x=v.uv.y=0;
				}

				if(is_uv2)
				{
					Point2 tex=p.attr[j].texel2;
					v.uv2.x=tex.x;
					v.uv2.y=1-tex.y;
				}else
				{
					v.uv2.x=v.uv2.y=0;
				}

				for(int ibone=0;ibone<max_bones;ibone++)
				{
					v.bones[ibone]=0;
					v.bones_inode[ibone]=-1;
				}

				int nbones=min(max_bones,p.skin.size());
				for(ibone=0;ibone<nbones;ibone++)
				{
					v.bones[ibone]=p.skin[ibone].weight;
					v.bones_inode[ibone]=p.skin[ibone].inode;
				}
			}
		}

		xassert(n_vertex==cur_vertex);
	}

	for(int i=0;i<n_polygon;i++)
	{
		FaceEx& f=faces[i];
		UVFace tex_face1 = pobject->GetMapVertex(1,f);
		UVFace tex_face2 = pobject->GetMapVertex(2,f);

		for(int j=0;j<3;j++)
		{
			Point2 tex(0.0f,0.0f),tex2(0.0f,0.0f);
			if(is_uv)
			{
				if(IS_SPECIAL(tex_face1.v[j].x) || IS_SPECIAL(tex_face1.v[j].y) ||
					tex_face1.v[j].x > uv_range || tex_face1.v[j].x < -uv_range ||
					tex_face1.v[j].y > uv_range || tex_face1.v[j].y < -uv_range)
				{
					tex_face1.v[j].x = tex_face1.v[j].y = tex_face1.v[j].z = 0;
				}
				tex.Set(tex_face1.v[j].x,tex_face1.v[j].y);
			}

			if(is_uv2)
			{
				if(IS_SPECIAL(tex_face2.v[j].x) || IS_SPECIAL(tex_face2.v[j].y) ||
					tex_face2.v[j].x > uv_range || tex_face2.v[j].x < -uv_range ||
					tex_face2.v[j].y > uv_range || tex_face2.v[j].y < -uv_range)
				{
					tex_face2.v[j].x = tex_face2.v[j].y = tex_face2.v[j].z = 0;
				}
				tex2.Set(tex_face2.v[j].x,tex_face2.v[j].y);
			}

			Point3 norm=pobject->GetNormal(f.index,f.vert[j]);
			new_polygon[i].p[j]=pnt[f.vert[j]].FindNewPoint(tex,norm,tex2);
		}
	}

	//	SortPolygon(&new_polygon[0],n_polygon);

	if(new_vertex.size()>65535)
	{
		Msg("Error: %s %i vertex. Количество вершин в объекте слишком велико.\n",name,new_vertex.size());
		return false;
	}

	saver.push(C3DX_MESH_VERTEX);
	saver<<new_vertex.size();
	for(i=0;i<new_vertex.size();i++)
	{
		Vect3f pos=new_vertex[i].pos;
		RightToLeft(pos);
		saver<<pos;
	}
	saver.pop();

	saver.push(C3DX_MESH_NORMAL);
	saver<<new_vertex.size();
	for(i=0;i<new_vertex.size();i++)
	{
		Vect3f norm=new_vertex[i].norm;
		RightToLeft(norm);
		saver<<norm;
	}
	saver.pop();

	if(is_uv)
	{
		saver.push(C3DX_MESH_UV);
		saver<<new_vertex.size();
		for(i=0;i<new_vertex.size();i++)
		{
			saver<<new_vertex[i].uv;
		}
		saver.pop();
	}

	if(is_uv2)
	{
		saver.push(C3DX_MESH_UV2);
		saver<<new_vertex.size();
		for(i=0;i<new_vertex.size();i++)
		{
			saver<<new_vertex[i].uv2;
		}
		saver.pop();
	}

	if(is_skin)
	{
		saver.push(C3DX_MESH_SKIN);
		saver<<new_vertex.size();
		for(i=0;i<new_vertex.size();i++)
		{
			sVertex& v=new_vertex[i];
			for(int ibone=0;ibone<max_bones;ibone++)
			{
				xassert(v.bones[ibone]>=0 && v.bones[ibone]<1.1f);
				saver<<v.bones[ibone];
				saver<<v.bones_inode[ibone];
			}
		}
		saver.pop();
	}

	saver.push(C3DX_MESH_FACES);
	saver<<(int)new_polygon.size();
	for(i=0;i<new_polygon.size();i++)
	{
		sPolygon p=new_polygon[i];
		//		swap(p.p[0],p.p[1]);
		for(int j=0;j<3;j++)
		{
			xassert(p.p[j]>=0 && p.p[j]<65535);
			WORD pp=p.p[j];
			saver<<pp;
		}
	}
	saver.pop();

	if(dbg_show_info_polygon)
	{
		Msg("%s polygon=%i vertex=%i\n",name,n_polygon,n_vertex);
		if(is_skin)
		{
			Msg("%s bones",name);
			for(i=0;i<max_bones+2;i++)
			{
				Msg(" %i=%i",i,num_bones_vertex[i]);
			}
			Msg("\n");
		}
	}
	return true;

}

int ExportMesh::NumCashed(sPolygon* polygon,int n_polygon)
{
	const max_cache_size=8;
	int cache_size=0;
	list<int> cache;

	int kgood=0;
	int kmiss=0;
	for(int i=0;i<n_polygon;i++)
	{
		sPolygon& cur=polygon[i];
		for(int k=0;k<3;k++)
		{
			int poly=cur.p[k];
			list<int>::iterator it;
			bool in=false;
			FOR_EACH(cache,it)
			{
				if(*it==poly)
				{
					in=true;
					break;
				}
			}

			if(in)
				kgood++;
			else
				kmiss++;
		}

		for(k=0;k<3;k++)
		{
			int poly=cur.p[k];
			list<int>::iterator it;
			bool in=false;
			FOR_EACH(cache,it)
			{
				if(*it==poly)
				{
					in=true;
					break;
				}
			}

			if(in)
			{
				swap(*it,cache.back());
			}else
			{
				cache.push_back(poly);
				cache_size++;
				if(cache_size>max_cache_size)
					cache.pop_front();
			}
		}
	}

	xassert(kgood+kmiss==n_polygon*3);
	return kmiss;
}
//*
void ExportMesh::SortPolygon(sPolygon* polygon,int n_polygon)
{
	//Не забыть попробовать сортировку по большему количесту точек.
	int prev_cashed=NumCashed(polygon,n_polygon);
	int sorted=0;

	for(int i=1;i<n_polygon;i++)
	{
		int joptimal=-1;
		int koptimal=-1;
		sPolygon& prev=polygon[i-1];
		for(int j=i;j<n_polygon;j++)
		{
			sPolygon& cur=polygon[j];
			int k=0;
			if(cur.p[0]==prev.p[0])k++;
			if(cur.p[1]==prev.p[0])k++;
			if(cur.p[2]==prev.p[0])k++;
			if(cur.p[1]==prev.p[1])k++;
			if(cur.p[2]==prev.p[1])k++;
			if(cur.p[2]==prev.p[2])k++;
			if(k>koptimal)
			{
				joptimal=j;
				koptimal=k;
				if(k>1)
					break;
			}
		}

		if(joptimal!=i)
		{
			sPolygon tmp=polygon[i];
			polygon[i]=polygon[joptimal];
			polygon[joptimal]=tmp;
			sorted++;
		}
	}

	int cur_cashed=NumCashed(polygon,n_polygon);

	if(dbg_show_info_polygon)
		Msg("info: sorted=%i prev=%i cur=%i\n",sorted,prev_cashed,cur_cashed);
}
/*/
void ExportMesh::SortPolygon(sPolygon* polygon,int n_polygon)
{
	//Не забыть попробовать сортировку по большему количесту точек.
	int prev_cashed=NumCashed(polygon,n_polygon);
	int sorted=0;

	const max_cache_size=8;
	int cache_size=0;
	list<int> cache;

	for(int i=0;i<n_polygon;i++)
	{
		int joptimal=-1;
		int koptimal=-1;

		int jmax=n_polygon;//min(n_polygon,i+500);
		for(int j=i;j<jmax;j++)
		{
			sPolygon& cur=polygon[j];

			int in_cache=0;

			for(int k=0;k<3;k++)
			{
				int poly=cur.p[k];
				list<int>::iterator it;
				bool in=false;
				FOR_EACH(cache,it)
				{
					if(*it==poly)
					{
						in_cache++;
						break;
					}
				}
			}

			if(in_cache>koptimal)
			{
				joptimal=j;
				koptimal=in_cache;
				if(in_cache>2)
					break;
			}
		}

		if(joptimal!=i)
		{
			sPolygon tmp=polygon[i];
			polygon[i]=polygon[joptimal];
			polygon[joptimal]=tmp;
			sorted++;
		}

		sPolygon cur=polygon[i];

		for(int k=0;k<3;k++)
		{
			int poly=cur.p[k];
			list<int>::iterator it;
			bool in=false;
			FOR_EACH(cache,it)
			{
				if(*it==poly)
				{
					in=true;
					break;
				}
			}

			if(in)
			{
				swap(*it,cache.back());
			}else
			{
				cache.push_back(poly);
				cache_size++;
				if(cache_size>max_cache_size)
				{
					cache_size--;
					cache.pop_front();
				}
			}
		}
	}

	int cur_cashed=NumCashed(polygon,n_polygon);

	if(dbg_show_info_polygon)
		Msg("info: sorted=%i prev=%i cur=%i\n",sorted,prev_cashed,cur_cashed);
}
/**/
