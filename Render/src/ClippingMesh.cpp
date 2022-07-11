#include "stdafxrd.h"
#include "ClippingMesh.h"
#include "D3DRender.h"
#include "cCamera.h"

ClippingMesh::ClippingMesh(float zmax)
{
	zmax_ = zmax;
	epsilon=1e-4f;
}

int ClippingMesh::clip(const Plane& clipplane)
{
	//vertex processing
	int positive=0,negative=0;
	for(int i=0;i<V.size();i++){
		CVertex& v=V[i];
		if(v.visible){
			v.distance=clipplane.distance(v.point);
			if(v.distance>=epsilon)
				positive++;
			else if(v.distance<=-epsilon){
				negative++;
				v.visible=false;
			}
			else
				v.distance=0;
		}
	}

	if(negative==0) //no clipping
		return +1;

	if(positive==0)//all clipped
		return -1;
	
	//Edge processing
	for(int i=0;i<E.size();i++){
		CEdge& e=E[i];
		if(e.visible){
			float d0=V[e.vertex[0]].distance,d1=V[e.vertex[1]].distance;
			if(d0<=0 && d1<=0){
				CEdge::Faces::iterator it;
				FOR_EACH(e.faces,it){
					int j=*it;
					//set<int>::iterator ite=F[j].edge.find(i);
					//F[j].edge.erase(ite);
					F[j].edges.remove(i);
					if(F[j].edges.empty())
						F[j].visible=false;

				}

				e.visible=false;
				continue;
			}

			if(d0>=0 && d1>=0)
				continue;
			
			//edge is split
			float t=d0/(d0-d1);

			CVertex intersect;
			intersect.point=(1-t)*V[e.vertex[0]].point+t*V[e.vertex[1]].point;
			int index=V.size();
			V.push_back(intersect);
			if(d0>0)
				e.vertex[1]=index;
			else
				e.vertex[0]=index;
		}
	}

	//face processing
	CFace closeFace;
//	closeFace.plane=clipplane;
	int findex=F.size();

	for(int i=0;i<F.size();i++){
		CFace& f=F[i];
		if(f.visible){
			CFace::Edges::iterator it;
			FOR_EACH(f.edges,it){
				CEdge& e=E[*it];
				V[e.vertex[0]].occurs=0;
				V[e.vertex[1]].occurs=0;
			}

			int start,final;
			if(GetOpenPolyline(f,start,final)){
				CEdge closeEdge;
				int eindex=E.size();
				closeEdge.vertex[0]=start;
				closeEdge.vertex[1]=final;
				closeEdge.faces.add(i);
				f.edges.add(eindex);

				//Code to closing polyhedron
				closeEdge.faces.add(findex);
				closeFace.edges.add(eindex);
				
				//
				E.push_back(closeEdge);
			}
		}
	}

	F.push_back(closeFace);

	return 0;
}


bool ClippingMesh::GetOpenPolyline(const CFace& face,int& start,int& final)
{
	CFace::Edges::const_iterator it;
	FOR_EACH(face.edges,it){
		CEdge& e=E[*it];
		V[e.vertex[0]].occurs++;
		V[e.vertex[1]].occurs++;
	}

	start=-1;
	final=-1;
	
	FOR_EACH(face.edges,it){
		CEdge& e=E[*it];
		int i0=e.vertex[0],i1=e.vertex[1];
		if(V[i0].occurs==1){
			if(start==-1)
				start=i0;
			else if(final==-1)
				final=i0;
		}
		if(V[i1].occurs==1){
			if(start==-1)
				start=i1;
			else if(final==-1)
				final=i1;
		}
	}

	return start!=-1;
}


void ClippingMesh::BuildPolygon(APolygons& p)
{//Самый простой метод, некоторые точки могут не использоваться.
	p.points.resize(V.size());
	int i;
	for(i=0;i<V.size();i++)
		p.points[i]=V[i].point;

	p.faces_flat.clear();
	for(i=0;i<F.size();i++)
	if(F[i].visible)
	{
		int num_vertex_index=p.faces_flat.size();
		p.faces_flat.push_back(0);

		CFace& face=F[i];

		CFace::Edges::iterator it;
		CFace::Edges edge=face.edges;

		int begin,old;
		{
			it=edge.begin();
			CEdge& e=E[*it];
			begin=e.vertex[0];
			old=e.vertex[1];
			p.faces_flat.push_back(begin);
			p.faces_flat.push_back(old);
			edge.erase(it);
		}

		while(!edge.empty())
		{
			if(begin==old)
			{
				xassert(0);
				break;
			}

			bool berase=false;
			CFace::Edges::iterator it;
			FOR_EACH(edge,it)
			{
				CEdge& e=E[*it];
				if(e.vertex[0]==old || e.vertex[1]==old)
				{
					if(e.vertex[0]==old)
						old=e.vertex[1];
					else
						old=e.vertex[0];

					p.faces_flat.push_back(old);

					edge.erase(it);
					berase=true;
					break;
				}
			}
			xassert(berase);
		}

		int nvi=p.faces_flat.size()-num_vertex_index-1;
		p.faces_flat[num_vertex_index]=nvi;
	}
}

//////////////////////////////////////////////////////////
void ClippingMesh::createBox(Vect3f& vmin,Vect3f& vmax)
{
	V.clear();
	E.clear();
	F.clear();

	V.resize(8);
	V[0].point.set(vmin.x,vmin.y,vmin.z);
	V[1].point.set(vmax.x,vmin.y,vmin.z);
	V[2].point.set(vmin.x,vmax.y,vmin.z);
	V[3].point.set(vmax.x,vmax.y,vmin.z);
	
	V[4].point.set(vmin.x,vmin.y,vmax.z);
	V[5].point.set(vmax.x,vmin.y,vmax.z);
	V[6].point.set(vmin.x,vmax.y,vmax.z);
	V[7].point.set(vmax.x,vmax.y,vmax.z);

	E.resize(12);
	
	struct
	{
		int v0,v1;
		int f0,f1;
	} edgedata[]=
	{
		{0,1,	0,1},
		{1,3,	0,2},
		{3,2,	0,3},
		{2,0,	0,4},

		{4,5,	5,1},
		{5,7,	5,2},
		{7,6,	5,3},
		{6,4,	5,4},

		{0,4,	4,1},
		{1,5,	1,2},
		{3,7,	2,3},
		{2,6,	3,4},
	};

	int i;
	for(i=0;i<12;i++)
	{
		CEdge& e=E[i];
		e.vertex[0]=edgedata[i].v0;
		e.vertex[1]=edgedata[i].v1;
		e.faces.add(edgedata[i].f0);
		e.faces.add(edgedata[i].f1);
	}
	
	F.resize(6);
	for(i=0;i<E.size();i++)
	{
		CFace::Edges::iterator it;
		FOR_EACH(E[i].faces,it)
			F[*it].edges.add(i);
	}
}


void ClippingMesh::draw()
{
	for(int i=0;i<E.size();i++)
		if(E[i].visible){
			int iv0 = E[i].vertex[0];
			int iv1 = E[i].vertex[1];
			Vect3f v0,v1;
			v0 = V[iv0].point;
			v1 = V[iv1].point;
			gb_RenderDevice->DrawLine(v0,v1,Color4c(0,0,0,255));
		}
}

void ClippingMesh::calcBoundTransformed(const Mat3f& m, sBox6f& box)
{
	box.invalidate();
	for(int i=0;i<E.size();i++)
		if(E[i].visible){
			for(int j=0;j<2;j++){
				int iv = E[i].vertex[j];
				box.addPoint(m*V[iv].point);
			}
		}
}

void ClippingMesh::calcBoundTransformed(const Mat4f& m, sBox6f& box)
{
	box.invalidate();
	for(int i=0;i<E.size();i++)
		if(E[i].visible){
			for(int j=0;j<2;j++){
				int iv = E[i].vertex[j];
				Vect3f vx;
				m.xformCoord(V[iv].point, vx);
				box.addPoint(vx);
			}
		}
}

void ClippingMesh::fillVisPoly(BYTE *buf,Vect2f* vert,int vert_size,int VISMAP_W,int VISMAP_H)
{
	MTG(); 
	//Временно для редактора xassert(!MT_IS_LOGIC());
	if(vert_size==0)return;
	const int VISMAP_W_MAX=128,VISMAP_H_MAX=128;
	xassert(VISMAP_W<=VISMAP_W_MAX && VISMAP_H<=VISMAP_H_MAX);
	float lx[VISMAP_W_MAX], rx[VISMAP_H_MAX];
	int i, y, ytop, ybot;

	// find top/bottom y
	ytop = floor(vert[0].y);
	ybot = ceil(vert[0].y);
	for(i=1;i<vert_size;i++)
	{
		float y=vert[i].y;
		if (y < ytop) ytop = floor(y);
		if (y > ybot) ybot = ceil(y);
	}

	for (i = 0; i < VISMAP_H; i++)
	{
		lx[i] = VISMAP_W-1;
		rx[i] = 0;
	}

	// render edges
	for (i = 0; i < vert_size; i++)
	{
		int i2=(i+1>=vert_size)?0:i+1;
		float x1, x2, y1, y2, t;
		x1=vert[i].x;  y1=vert[i].y;
		x2=vert[i2].x; y2=vert[i2].y;

		if (y1 > y2) { t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t; }

		int iy1 = (int)y1, iy2 = (int)y2;
		if(iy1>iy2)continue;

		float dy = (y2 == y1) ? 1 : (y2 - y1);
		float dy1 =1/dy;
		for (y = max(iy1, 0); y <= min(iy2, VISMAP_H-1); y++)
		{
			float ix1 = x1 + (y-y1) * (x2-x1) * dy1;
			float ix2 = x1 + (y-y1+1) * (x2-x1) * dy1;
			if (y == iy1) ix1 = x1;
			if (y == iy2) ix2 = x2;
			lx[y] = min(min(lx[y], ix1), ix2);
			rx[y] = max(max(rx[y], ix1), ix2);
		}
	}

	// fill the buffer
	for (y = max(0, ytop); y <= min(ybot, VISMAP_H-1); y++)
	{
		if (lx[y] > rx[y]) continue;
		int x1 = (int)max((float)floor(lx[y]), 0.0f);
		int x2 = (int)min((float)ceil(rx[y]), (float)VISMAP_W);
		if(x1>=x2)continue;
		memset(buf + y*VISMAP_W + x1, 1, x2-x1);
	}
}

void ClippingMesh::createTileBox(Camera* camera, Vect2i TileNumber,Vect2i TileSize)
{
	float dx=TileNumber.x*TileSize.x;
	float dy=TileNumber.y*TileSize.y;

	createBox(Vect3f(0,0,0),Vect3f(dx,dy, zmax_));

	for(int i=0;i<camera->GetNumberPlaneClip3d();i++)
		clip(camera->GetPlaneClip3d(i));
}

void ClippingMesh::calcVisMap(Camera* camera, Vect2i TileNumber,Vect2i TileSize,BYTE* visMap,bool clear)
{
	createTileBox(camera,TileNumber,TileSize);

	APolygons poly;
	BuildPolygon(poly);
	if(clear)
		memset(visMap, 0, TileNumber.x*TileNumber.y);

	for(int i=0;i<poly.faces_flat.size();)
	{
		int inp_size=poly.faces_flat[i];
		int* inp=&poly.faces_flat[i+1];
		i+=inp_size+1;
		static vector<Vect2f> points;
		points.resize(inp_size);
		for(int j=0;j<inp_size;j++)
		{
			Vect3f& p=poly.points[inp[j]];
			points[j].x=p.x/TileSize.x;
			points[j].y=p.y/TileSize.y;
		}
		
		fillVisPoly(visMap, &points[0],points.size(),TileNumber.x,TileNumber.y);
	}

	if(false)
		draw();
}

void ClippingMesh::calcVisBox(Camera* camera, Vect2i TileNumber,Vect2i TileSize,const Mat4f& direction,sBox6f& box)
{
	createTileBox(camera,TileNumber,TileSize);
	calcBoundTransformed(direction, box);
}

void drawTestGrid(unsigned char* grid, const Vect2i& size)
{
	int minx=14, miny=639;

	Color4c visible(0, 255, 0, 80);
	Color4c unvisible(255, 255, 255, 80);

	int mul = size.x > 32 ? 4 : 8;

	for(int y=0;y < size.y; y++)
		for(int x=0;x < size.x; x++)
			gb_RenderDevice3D->DrawRectangle(x*mul + minx, y*mul + miny, mul, mul, grid[y*size.x+x] ? visible : unvisible);
}

