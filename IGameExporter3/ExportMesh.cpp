#include "stdafx.h"
#include "RootExport.h"


void IVisNode::exportMeshesRecursive(TempMeshes& meshes)
{
	if(isMesh()){
		IVisMesh* imesh = GetMesh();
		if(imesh->GetNumberVerts()<=0)
			Msg("Error: %s cannot have vertex.\n", GetName());
		else if(imesh->GetNumberFaces()<=0)
			Msg("Error: %s cannot have faces.\n", GetName());
		else{
			set<IVisMaterial*> materials;
			int n = imesh->GetNumberFaces();
			for(int i=0; i<n; i++){
				IVisMaterial* mat = imesh->GetMaterialFromFace(i);
				materials.insert(mat);
			}
			set<IVisMaterial*>::iterator it;
			for(it = materials.begin(); it != materials.end(); ++it){
				IVisMaterial* material = *it;
				TempMesh mesh = new cTempMesh3dx;
				meshes.push_back(mesh);

				mesh->inode = index(true);
				mesh->imaterial = exporter->FindMaterialIndex(material);
				mesh->visibilityAnimated = visibilityAnimated_;
				if(mesh->imaterial < 0)
					Msg("Error: %s cannot have material.\n", GetName());

				mesh->name = GetName();

				mesh->export(imesh, material, index(true));
			}
		}
	}

	IVisNodes::iterator ni;
	FOR_EACH(childNodes_, ni)
		(*ni)->exportMeshesRecursive(meshes);
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

struct sVertex
{
	Vect3f pos;
	Vect3f norm;
	Vect2f uv;
	Vect2f uv2;

	float bones[MAX_BONES];
	int bones_inode[MAX_BONES];
};

bool cTempMesh3dx::export(IVisMesh* pobject, IVisMaterial* mat, int inode)
{
	pobject->GetMAXMesh()->buildNormals();
	pobject->GetMAXMesh()->checkNormals(TRUE);

	set<int> errorVertex;
	const int uv_range = 100;
	vector<RePoints> pnt(pobject->GetNumberVerts());
	vector<FaceEx> faces;

	bool is_uv=false;
	bool is_uv2=false;

	if(pobject->IsMapSupport(1) && pobject->GetNumMapVerst(1)>0)
		is_uv=true;

	if(pobject->IsMapSupport(2) && pobject->GetNumMapVerst(2)>0)
		is_uv2=true;

	int num_faces=pobject->GetNumberFaces();
	for(int i=0;i<num_faces;i++)
		if (pobject->GetMaterialFromFace(i) == mat)
			faces.push_back(pobject->GetFace(i));

	bool uv_not_match=false;
	for(int iface=0;iface<faces.size();iface++){
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
				tex2.Set(tex_face2.v[j].x,tex_face2.v[j].y);
			}

			Point3 norm=pobject->GetNormal(f.index,f.vert[j]);
			pnt[f.vert[j]].AddUnicalInt(tex,norm,face_normal,tex2);
		}
	}

	if(uv_not_match)
		Msg("Error: %s. UV коордионаты не совпадают.\n",name);

	int num_bones_vertex[MAX_BONES+2];//Количество вертексов с таким количеством bones
	for(int ibone=0;ibone<MAX_BONES+2;ibone++)
		num_bones_vertex[ibone]=0;

	int n_vertex=0;
	for(int ipnt=0;ipnt<pnt.size();ipnt++)
		n_vertex+=pnt[ipnt].attr.size();

	bool is_skin=false;
	if(pobject->GetSkin()){
		is_skin=true;
		IVisSkin* pskin=pobject->GetSkin();
		for(ipnt=0;ipnt<pnt.size();ipnt++){
			RePoints& p=pnt[ipnt];
			int nbones=pskin->GetNumberBones(ipnt);

			if(nbones==0){
				p.skin.resize(1);
				RePoints::SKIN& s=p.skin[0];
				s.weight=1;
				s.inode=inode;
			}
			else{
				p.skin.resize(nbones);
				float sum_weight=0;
				for(int ibone=0;ibone<nbones;ibone++){
					RePoints::SKIN& s=p.skin[ibone];
					s.weight=pskin->GetWeight(ipnt,ibone);
					if(IS_SPECIAL(s.weight)){
						s.weight = 0.1f;
						Msg("Error: В объекте \"%s\" у вершины %d все веса нулевые\n",pobject->GetNode()->GetName(),ipnt);
					}
					IVisNode* pnode=pskin->GetIBone(ipnt,ibone);
					//const char* nodename=pnode->GetName();
					//INode *  pnode_max=pskin->GetBone (ipnt,ibone); 
					//const char* nodename_max=pnode_max->GetName();

					s.inode = pnode->index();
					if(s.inode<0){
						Msg("Error: %s не прилинкованна к group center\n",pnode->GetName());
						return false;
					}
					xassert(s.weight>=0.0f && s.weight<=1.001f);
					sum_weight+=s.weight;
				}

				if(sum_weight<0.999f){
					nbones++;
					p.skin.resize(nbones);

					RePoints::SKIN& s=p.skin[nbones-1];
					s.weight=1-sum_weight;
					s.inode=inode;
					sum_weight+=s.weight;
				}

				xassert(fabsf(sum_weight-1)<1e-3f);
				sort(p.skin.begin(),p.skin.end(),SortByWeight());

				int max_bones = exporter->GetMaxWeights();//4;
				if(p.skin.size()>max_bones)
					p.skin.resize(max_bones);

				float sum=0;
				for(int ibone=0;ibone<p.skin.size();ibone++)
					sum+=p.skin[ibone].weight;

				xassert(sum>0.1f);
				xassert(sum<=1.001f);
				sum=1/sum;
				for(int ibone=0;ibone<p.skin.size();ibone++)
					p.skin[ibone].weight*=sum;
			}

			int mbones=min(nbones,MAX_BONES+1);
			num_bones_vertex[mbones]++;
		}
	}

	int n_polygon=faces.size();
	vector<sVertex> new_vertex(n_vertex);
	vector<sPolygon> new_polygon(n_polygon);

	int cur_vertex=0;
	for(ipnt=0;ipnt<pnt.size();ipnt++){
		RePoints& p=pnt[ipnt];
		p.new_base_index=cur_vertex;
		for(int j=0;j<p.attr.size();j++,cur_vertex++){
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

			if(is_uv){
				Point2 tex=p.attr[j].texel;
				v.uv.x=tex.x;
				v.uv.y=1-tex.y;
			}
			else
				v.uv.x=v.uv.y=0;

			if(is_uv2){
				Point2 tex=p.attr[j].texel2;
				v.uv2.x=tex.x;
				v.uv2.y=1-tex.y;
			}
			else
				v.uv2.x=v.uv2.y=0;

			for(int ibone=0;ibone<MAX_BONES;ibone++){
				v.bones[ibone]=0;
				v.bones_inode[ibone]=-1;
			}

			int nbones=min(MAX_BONES,p.skin.size());
			for(int ibone=0;ibone<nbones;ibone++){
				v.bones[ibone]=p.skin[ibone].weight;
				v.bones_inode[ibone]=p.skin[ibone].inode;
			}
		}
	}

	xassert(n_vertex==cur_vertex);

	for(int i=0;i<n_polygon;i++){
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
			new_polygon[i][j]=pnt[f.vert[j]].FindNewPoint(tex,norm,tex2);
		}
	}

	if(new_vertex.size()>65535){
		Msg("Error: %s %i vertex. Количество вершин в объекте слишком велико.\n",name,new_vertex.size());
		return false;
	}

	vertex_pos.resize(new_vertex.size(), Vect3f::ZERO);
	vertex_norm.resize(new_vertex.size(), Vect3f::ZERO);
	for(i=0;i<new_vertex.size();i++){
		vertex_pos[i] = RightToLeft(new_vertex[i].pos);
		vertex_norm[i] = RightToLeft(new_vertex[i].norm);
	}

	if(is_uv){
		vertex_uv.resize(new_vertex.size(), Vect2f::ZERO);
		for(i=0;i<new_vertex.size();i++)
			vertex_uv[i] = new_vertex[i].uv;
	}

	if(is_uv2){
		vertex_uv2.resize(new_vertex.size(), Vect2f::ZERO);
		for(i=0;i<new_vertex.size();i++)
			vertex_uv2[i] = new_vertex[i].uv2;
	}

	if(is_skin){
		bones.resize(new_vertex.size(), cTempBone());
		for(i=0;i<new_vertex.size();i++){
			sVertex& v = new_vertex[i];
			for(int ibone=0;ibone<MAX_BONES;ibone++){
				xassert(v.bones[ibone]>=0 && v.bones[ibone]<1.1f);
				bones[i].weight[ibone] = v.bones[ibone];
				bones[i].inode[ibone] = v.bones_inode[ibone];
			}
		}
	}

	polygons.resize(new_polygon.size());
	for(i=0;i<new_polygon.size();i++){
		sPolygon p=new_polygon[i];
		for(int j=0;j<3;j++){
			xassert(p[j]>=0 && p[j]<65535);
			polygons[i][j] = p[j];
		}
	}

	deleteSingularPolygon();

	if(exporter->show_info_polygon){
		Msg("%s polygon=%i vertex=%i\n",name,n_polygon,n_vertex);
		if(is_skin){
			Msg("%s bones",name);
			for(i=0;i<MAX_BONES+2;i++)
				Msg(" %i=%i",i,num_bones_vertex[i]);
			Msg("\n");
		}
	}
	return true;
}

