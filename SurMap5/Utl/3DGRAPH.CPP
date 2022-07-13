#include "stdafx.h"
#include "general.h"


int draw_mode = DRAW_GOURAUD;
int x_prec = 2;

PolygonS* z_list_min=0;
PolygonS* z_list_max=0;
PolygonS* z_list_curr=0;
PolygonS* max_z_poly=0;
PolygonS* min_z_poly=0;

int poly_cnt=0;
int poly_number=-1;
int poly_save=0;

int focus=512;
double focus_flt=512.;

DBM A_convert;
DBM A_g2s;
//int ViewX = 0,ViewY = 0,
int ViewZ = 512;
Vector Refl(0,0,NORMAL),Light(NORMAL,0,2*NORMAL),View(0,0,NORMAL);
int TurnAngle=0;
int SlopeAngle;
int ScreenCX;
int ScreenCY;

int CentrX = 0;//XGR_MAXX/2;
int CentrY = 0;//XGR_MAXY/2;
int CentrZ = 0;
int Xmax2 = 0;//XGR_MAXX/2 - 80;
int Ymax2 = 0;//XGR_MAXY/2 - 80;

int UcutLeft = 0;
int UcutRight = 0;//XGR_MAXX;
int VcutUp = 0;
int VcutDown = 0;//XGR_MAXY;

int u_cut_left;
int u_cut_right;
int v_cut_up;
int v_cut_down;

int flush_x = 0;
int flush_y = 0;
int flush_sx = 0;//XGR_MAXX;
int flush_sy = 0;//XGR_MAXY;

double Ha,Va;
double Hb,Vb;
double Oc,Hc,Vc;

int n_power = 8;

/******************************************************************************
	 MEMORY HEAP Function & Variable
******************************************************************************/
MemoryHeap quant_heap("Quant mem heap overflow at addr");
MemoryHeap load_heap("Load mem heap overflow at addr");

MemoryHeap::MemoryHeap(char* message)
{
	offset = size = 0;
	overflow_message = message;
}
void MemoryHeap::alloc(int sz)
{
	if(size)
		ErrH.Abort("Multiple alloc for MemoryHeap");
	heap = new char[size = sz];
	offset = 0;
}

void MemoryHeap::end_alloc()
{
	if(size != offset)
		if(size < offset)
			ErrH.Abort("Defiáiency of locked memory");
		else
			ErrH.Abort("Abundance of locked memory");
	heap = 0;
	size = 0;
}

/******************************************************************************
	MAIN FUNCTION OF 3D GRAPH
******************************************************************************/
void graph3d_init()
{
	A_g2s = Mat3f::ID;
	Light.normalize(NORMAL);

	max_z_poly = ALLOC(1,PolygonS);
	min_z_poly = ALLOC(1,PolygonS);
	max_z_poly -> num_vert = min_z_poly -> num_vert = 0;
	max_z_poly -> z_max = max_z_poly -> z_min = (float)INT_INF;
	min_z_poly -> z_max = min_z_poly -> z_min = (float)-INT_INF;

	min_z_poly -> prev = max_z_poly -> next = 0;
	z_list_max = min_z_poly -> next = max_z_poly;
	z_list_curr = z_list_min = max_z_poly -> prev = min_z_poly;

	poly_cnt = 0;

	focus_flt = focus = 512;

	UcutLeft = 0;
	UcutRight = 0;//XGR_MAXX ? XGR_MAXX : 640;
	VcutUp = 0;
	VcutDown = 0;//XGR_MAXY ? XGR_MAXY : 480;
}

void calc_border(int& x1,int& x2,int& xc,int& sx2,int& flush_x,int& flush_sx,int xmax)
{
	if(x1 < 0)
		x1 = 0;
	if(x2 >= xmax)
		x2 = xmax - 1;
	xc = (x1 + x2)/2;
	sx2 = (x2 - x1)/2;
	x1 = xc - sx2;
	x2 = xc + sx2;
	flush_x = x1;
	flush_sx = x2 - x1 + 1;
	if(flush_sx % 4)
		flush_sx += 8 - flush_sx % 8;
}

void calc_screen(int zoom_percent, int MAX_X, int MAX_Y)
{
	if(zoom_percent > 0){
		Xmax2 = (MAX_X*zoom_percent)/200;
		Ymax2 = (MAX_Y*zoom_percent)/200;

//		  focus = (Xmax2*540)/180;
		focus = 512;

		ScreenCX = MAX_X/2;
		ScreenCY = MAX_Y/2;

		UcutLeft = ScreenCX - Xmax2;
		UcutRight = ScreenCX + Xmax2;
		VcutUp = ScreenCY - Ymax2;
		VcutDown = ScreenCY + Ymax2;

		calc_border(UcutLeft,UcutRight,ScreenCX,Xmax2,flush_x,flush_sx,MAX_X);
		calc_border(VcutUp,VcutDown,ScreenCY,Ymax2,flush_y,flush_sy,MAX_Y);
		}

	focus_flt = focus;

#ifndef _SURMAP_
//	ViewX = XGR_MAXX/2;
//	ViewY = XGR_MAXY/2;
	ViewZ = focus;
#endif
}

/*******************************************************************************
		PolygonS function
*******************************************************************************/
/*void PolygonS::put_in_list()
{
	Vertex* vl = vertices[0];
	Vertex* vc = vertices[1];
	Vertex* vr = vertices[2];
//	  int x0,y0,x1,y1,x2,y2;
//	  x0 = vl -> xscr; y0 = vl -> yscr;
//	  x1 = vc -> xscr; y1 = vc -> yscr;
//	  x2 = vr -> xscr; y2 = vr -> yscr;
//	  if((x0 - x1)*(y2 - y1) - (y0 - y1)*(x2 - x1) > 0)
//		  return;

	// SORTING
	z_max = (float)(CentrZ + max(vl -> turned.z,vc -> turned.z,vr -> turned.z));
	z_min = (float)(CentrZ + min(vl -> turned.z,vc -> turned.z,vr -> turned.z));

	z_max = (float)((vl -> turned.z + vc -> turned.z + vr -> turned.z)/3);

	if(z_list_curr -> z_max < z_max){
		while(z_list_curr -> z_max < z_max)
			z_list_curr = z_list_curr -> next;
		next = z_list_curr;
		prev = z_list_curr -> prev;
		prev -> next = this;
		z_list_curr -> prev = this;
		}
	else{
		while(z_list_curr -> z_max > z_max)
			z_list_curr = z_list_curr -> prev;
		prev = z_list_curr;
		next = z_list_curr -> next;
		next -> prev = this;
		z_list_curr -> next = this;
		}
}
*/
