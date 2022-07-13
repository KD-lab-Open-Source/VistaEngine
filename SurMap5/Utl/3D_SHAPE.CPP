#include "stdafx.h"


#include "3d_shape.h"

//#include "common.h"

#include "..\..\terra\terra.h"

#include "..\..\terra\auxInlineFuction.h"

//#pragma warning( disable : 4244 ) //warning при конверте int 2 float

//#ifdef _SURMAP_
//extern int DepthShow;
//#else
int DepthShow=0;
//#endif

#define COLOR_OFFSET	128
#define COLOR_SHIFT	3

int shape_control(int code);
void load_shape(char* name,int x,int y);
//void view_shape();
void view_shape(unsigned short* GRAF_BUF, int GB_MAXX, int GB_MAXY);

int shape_size;
int shape_shift;
//unsigned char* upper_buffer;
//unsigned char* lower_buffer;
unsigned short* upper_buffer;
unsigned short* lower_buffer;
int shape_x,shape_y;
int shape_x_avr;
int shape_y_avr;
int shape_z_avr;


int shape_offset;
double shape_scale_x = 1;
double shape_scale_y = 1;
double shape_scale_z = 1;

int catch_log;
DBM A_shape(Vect3f(1,-1,-1), Vect3f::ZERO);
Model shape;

const int PIx2 = 1 << 12;
const int PI = PIx2/2;


int shape_control(int code)
{
/*	if(!catch_log)
		return 0;

	switch(code){
		case VK_ESCAPE:
			catch_log = 0;
			break;

		case VK_SPACE:
			A_shape.set(Vect3f(1,-1,-1), Vect3f::ZERO);
			if(XKey.Pressed(VK_CONTROL))
				shape_scale_x = shape_scale_y = shape_scale_z = 1;
			break;

		case VK_RETURN:
			shape.height_project();
			catch_log = 0;
#ifdef _SURMAP_
			S3Danalyze();
#endif
			break;
	}

	float rotate_step;
	int move_step;
	int alt = XKey.Pressed(VK_CONTROL);
	double scale_step;
	if(XKey.Pressed(VK_SHIFT)){
		move_step = 20;
		rotate_step = M_PI/8;
		scale_step = 1.1;
		}
	else{
		move_step = 1;
		rotate_step = M_PI/256;
		scale_step = 1.001;
		}

	if(XKey.Pressed(VK_DELETE))
		A_shape = DBM(-rotate_step,Z_AXIS)*A_shape;
	if(XKey.Pressed(VK_NEXT))
		A_shape = DBM(rotate_step,Z_AXIS)*A_shape;

	if(XKey.Pressed(VK_HOME))
		A_shape = DBM(-rotate_step,X_AXIS)*A_shape;
	if(XKey.Pressed(VK_END))
		A_shape = DBM(rotate_step,X_AXIS)*A_shape;

	if(XKey.Pressed(VK_INSERT))
		A_shape = DBM(rotate_step,Y_AXIS)*A_shape;
	if(XKey.Pressed(VK_PRIOR))
		A_shape = DBM(-rotate_step,Y_AXIS)*A_shape;

	if(XKey.Pressed(VK_ADD) || XKey.Pressed(VK_OEM_PLUS)){
		if(!alt){
			shape_scale_x *= scale_step;
			shape_scale_y *= scale_step;
			}
		shape_scale_z *= scale_step;
		}
	if(XKey.Pressed(VK_SUBTRACT) || XKey.Pressed(VK_OEM_MINUS)){
		if(!alt){
			shape_scale_x /= scale_step;
			shape_scale_y /= scale_step;
			}
		shape_scale_z /= scale_step;
		}

	if(XKey.Pressed(VK_UP))
		if(alt)
			shape_scale_y *= scale_step;
		else
			shape.y_off -= move_step;
	if(XKey.Pressed(VK_DOWN))
		if(alt)
			shape_scale_y /= scale_step;
		else
			shape.y_off += move_step;
	if(XKey.Pressed(VK_LEFT))
		if(alt)
			shape_scale_x /= scale_step;
		else
			shape.x_off -= move_step;
	if(XKey.Pressed(VK_RIGHT))
		if(alt)
			shape_scale_x *= scale_step;
		else
			shape.x_off += move_step;

#ifdef _SURMAP_
	shape.x_off &= vMap.clip_mask_x;
	shape.y_off &= vMap.clip_mask_y;
#endif
*/
	return 1;
}

void load_shape(char* name,int x,int y)
{
	XStream c3d(name,XS_IN);
	XBuffer buf(c3d.size());
	c3d.read(buf.address(),c3d.size());
	c3d.close();
	shape.loadC3D(buf);

	shape.x_off = x;
	shape.y_off = y;
	shape.z_off = 128;

	catch_log = 1;
}
void release_shape(void)
{
	shape.free();
	if(upper_buffer){
		delete upper_buffer;
		delete lower_buffer;
		}
}
/*void view_shape(int ViewX, int ViewY, unsigned short* GRAF_BUF, int GB_MAXX, int GB_MAXY)
{
	if(!catch_log)
		return;
	shape.draw_big(ViewX, ViewY, GRAF_BUF, GB_MAXX, GB_MAXY);
}*/
void Model::draw_big(int ViewX, int ViewY, float k_scale, unsigned short* GRAF_BUF, int GB_MAXX, int GB_MAXY)
//void Model::draw_big()
{
	calc_screen(100, GB_MAXX, GB_MAXY);

	int x = getDistX(x_off,ViewX);
	int y = getDistY(y_off,ViewY);
	if(DepthShow){
		DBV R(x,y,0);
		R *= A_g2s;
		CentrX = round(R.x);
		CentrY = round(R.y);
		CentrZ = ViewZ + round(R.z);
		}
	else{
		float co = cos(TurnAngle*M_PI/PI);
		float si = sin(TurnAngle*M_PI/PI);
		CentrX = round(x*co - y*si);
		CentrY = round(x*si + y*co);
		CentrZ = ViewZ;
		}
	CentrX = round((float)CentrX*k_scale);
	CentrY = round((float)CentrY*k_scale);
	CentrZ = round((float)CentrZ*k_scale);

	A_convert = A_g2s*A_shape*DBM(shape_scale_x*k_scale,0,0,
					 0,shape_scale_y*k_scale,0,
					 0,0,shape_scale_z*k_scale);

	Refl = Light;
	Refl.z = -Refl.z;
	Refl.y = -Refl.y;
	A_shape.invXform(Refl);

	int direct;
	PolygonS** sort_poly;
	double vx = -A_convert[2][0];
	double vy = -A_convert[2][1];
	double vz = -A_convert[2][2];
	if(fabs(vz) >= fabs(vy))
		if(fabs(vz) >= fabs(vx)){
			sort_poly = sorted_polygons[Z_AXIS];
			direct = SIGN(vz);
			}
		else{
			sort_poly = sorted_polygons[X_AXIS];
			direct = -SIGN(vx);
			}
	else
		if(fabs(vx) > fabs(vy)){
			sort_poly = sorted_polygons[X_AXIS];
			direct = -SIGN(vx);
			}
		else{
			sort_poly = sorted_polygons[Y_AXIS];
			direct = -SIGN(vy);
			}

	for(register int i = 0;i < num_vert;i++)
		vertices[i].convert_big();

	for(i = 0;i < num_norm;i++)
		normals[i].fast_calc_intensity();

	if(direct > 0){
		for(i = 0;i < num_poly;i++)
			sort_poly[i] -> draw_gouraud_big(GRAF_BUF, GB_MAXX, GB_MAXY);
		}
	else{
		for(i = num_poly - 1;i >= 0;i--)
			sort_poly[i] -> draw_gouraud_big(GRAF_BUF, GB_MAXX, GB_MAXY);
		}
}
void Vertex::convert_big()
{
	turned = A_convert*DBV(x,y,z);
//	  int zz = CentrZ + turned.z;
//	  if(zz <= 0)
//		  zz = 1;
//	  xscr = (turned.x + CentrX)*focus/(zz) + ScreenCX;
//	  yscr = (turned.y + CentrY)*focus/(zz) + ScreenCY;
	xscr = turned.x + CentrX + ScreenCX;
	yscr = turned.y + CentrY + ScreenCY;
}
inline void Normal::fast_calc_intensity()
{
	int Ln = dot(Vector(*this), Refl);
	if(Ln < 0){
		I = 0;
		I_8 = 0;
		return;
		}
	if(Ln > (63 << 8))
		I = (63 << 8);
	else
		I = Ln;
	I_8 = I >> 7;
}
//void PolygonS::draw_gouraud_big()
void PolygonS::draw_gouraud_big(unsigned short* GRAF_BUF, int GB_MAXX, int GB_MAXY)
{
	#define NEXT_LEFT(p)	((p) == beg ? end : (p) - 1)
	#define NEXT_RIGHT(p)	((p) == end ? beg : (p) + 1)

	int xl,xr,al,ar,bl,br,d,where,Y;
	int cl,cr,ckl,ckr;
	int cf,cfk;
	int x1,x2,len;
	Vertex *lfv,*rfv,*ltv,*rtv;
	Vertex **lto,**rto;
	Vertex **beg,**curr,**up,**end;
	Normal *lfn,*rfn,*ltn,*rtn;
	unsigned short* vp;

	beg = up = curr = vertices;
	end = vertices + num_vert - 1;
	curr++;
	int t;
	int x_left = (*beg) -> xscr;
	int x_right = x_left;
	int y_up = (*beg) -> yscr;
	int y_down = y_up;
	for(;curr <= end;curr++){
		if(y_up > (t = (*curr) -> yscr)){
			up = curr;
			y_up = t;
			}
		if(y_down < t)
			y_down = t;
		if(x_left > (t = (*curr) -> xscr))
			x_left = t;
		if(x_right < t)
			x_right = t;
		}

	if(y_down < VcutUp || y_up > VcutDown ||
	   x_right < UcutLeft || x_left > UcutRight)
		return;

	rfv = lfv = *up;
	rfn = lfn = normals[up - beg];

	lto = NEXT_LEFT(up);
	ltv = *lto;
	ltn = normals[lto - beg];

	rto = NEXT_RIGHT(up);
	rtv = *rto;
	rtn = normals[rto - beg];

	Y = lfv -> yscr; xl = lfv -> xscr;
	al = ltv -> xscr - xl; bl = ltv -> yscr - Y;
	ar = rtv -> xscr - xl; br = rtv -> yscr - Y;
	xr = xl = (xl << 16) + (1 << 15);

	cl = lfn -> I_8;
	ckl = ltn -> I_8 - cl;
	ckr = rtn -> I_8 - cl;
	cr = cl <<= 7;

	if(bl){
		DIV16(al,bl);
		ckl = (ckl << 7)/bl;
		}
	else
		al <<= 16;

	if(br){
		DIV16(ar,br);
		ckr = (ckr << 7)/br;
		}
	else
		ar <<= 16;

	vp = GRAF_BUF + Y*GB_MAXX;//(unsigned short*)XGR_VIDEOBUF + Y*XGR_MAXX;
	while(1){
		if(bl > br){
			d = br;
			where = 0;
			}
		else{
			d = bl;
			where = 1;
			}
		while(d-- > 0){
			x1 = xl >> 16;
			x2 = xr >> 16;
			if(x1 > x2)
				return;
			if(x1 < x2){
				if(Y >= VcutUp && Y < VcutDown &&
				   x2 >= UcutLeft && x1 < UcutRight){
					len = x2 - x1;
					cf = cl;
					cfk = (cr - cl)/len;
					if(x1 < UcutLeft){
						cf += cfk*(UcutLeft - x1);
						x1 = UcutLeft;
						len = x2 - x1;
						}
					if(x2 >= UcutRight){
						x2 = UcutRight;
						len = x2 - x1;
						}
					vp += x1;
					while(len--){
						*(vp++) = COLOR_OFFSET + (cf >> COLOR_SHIFT + 7);
						cf += cfk;
						}
					vp -= x2;
					}
				}
			Y++;
			vp += GB_MAXX;//XGR_MAXX;

			xl += al;
			xr += ar;

			cl += ckl;
			cr += ckr;
			}
		if(where){
			if(ltv == rtv) return;
			lfv = ltv;
			lfn = ltn;
			lto = NEXT_LEFT(lto);
			ltv = *lto;
			ltn = normals[lto - beg];

			br -= bl;

			xl = lfv -> xscr;
			al = ltv -> xscr - xl;
			bl = ltv -> yscr - Y;
			xl = (xl << 16) + (1 << 15);

			cl = lfn -> I_8;
			ckl = ltn -> I_8 - cl;
			cl <<= 7;

			if(bl){
				DIV16(al,bl);
				ckl = (ckl << 7)/bl;
				}
			else{
				al <<= 16;
				}
			}
		else {
			if(rtv == ltv) return;
			rfv = rtv;
			rfn = rtn;
			rto = NEXT_RIGHT(rto);
			rtv = *rto;
			rtn = normals[rto - beg];

			bl -= br;

			xr = rfv -> xscr;
			ar = rtv -> xscr - xr;
			br = rtv -> yscr - Y;
			xr = (xr << 16) + (1 << 15);

			cr = rfn -> I_8;
			ckr = rtn -> I_8 - cr;
			cr <<= 7;

			if(br){
				DIV16(ar,br);
				ckr = (ckr << 7)/br;
				}
			else
				ar <<= 16;
			}
		}
}
/*******************************************************************************
			Height projection
*******************************************************************************/
const averaging_threshould = 0;//1 << VX_FRACTION - 1;
const LSCALE = 2;
const SCALE = 1 << LSCALE;

void Model::height_project()
{
	int x,x_min = INT_INF,x_max = -INT_INF;
	int y,y_min = INT_INF,y_max = -INT_INF;
	float z,z_min = (float)INT_INF,z_max = -(float)INT_INF;
	A_convert = A_shape*DBM(shape_scale_x,0,0,
				   0,shape_scale_y,0,
				   0,0,shape_scale_z);

	for(int i = 0;i < num_vert;i++){
		vertices[i].turned = A_convert*DBV(vertices[i].x,vertices[i].y,vertices[i].z);
		vertices[i].turned.z *= 1 << VX_FRACTION;
		if((x = vertices[i].turned.x) < x_min)
			x_min = x;
		if((x = vertices[i].turned.x) > x_max)
			x_max = x;
		if((y = vertices[i].turned.y) < y_min)
			y_min = y;
		if((y = vertices[i].turned.y) > y_max)
			y_max = y;
		if((z = -vertices[i].turned.z) < z_min)
			z_min = z;
		if((z = vertices[i].turned.z) > z_max)
			z_max = z;
		}

	shape_offset = max(x_max - x_min,y_max - y_min)/2 + 1;
	shape_size = shape_offset*2;
	shape_shift = BitSR(shape_size) + 1;

	shape_x = x_off + x_min - 1;
	shape_y = y_off + y_min;

	shape_x_avr = (x_max - x_min)/2;
	shape_y_avr = (y_max - y_min)/2;
	shape_z_avr = (z_max - z_min)/2 + 1;

	shape_size *= SCALE;
	shape_shift += LSCALE;
	for(i = 0;i < num_vert;i++){
		vertices[i].turned.x *= SCALE;
		vertices[i].turned.y *= SCALE;
		}
	x_min *= SCALE;
	y_min *= SCALE;

	upper_buffer = new unsigned short[shape_size << shape_shift];
	lower_buffer = new unsigned short[shape_size << shape_shift];
	memset(upper_buffer,0,(shape_size << shape_shift)*sizeof(unsigned short));
	memset(lower_buffer,MAX_VX_HEIGHT,(shape_size << shape_shift)*sizeof(unsigned short));

	for(i = 0;i < num_vert;i++){
		vertices[i].xscr = vertices[i].turned.x - x_min;
		vertices[i].yscr = vertices[i].turned.y - y_min;
		vertices[i].turned.z = -vertices[i].turned.z - z_min + 1;
		}

	for(i = 0;i < num_poly;i++)
		polygons[i].height_project();

	int size = shape_size >> LSCALE;
	int shift = shape_shift - LSCALE;
	unsigned short* up_buffer = new unsigned short[size << shift];
	unsigned short* lo_buffer = new unsigned short[size << shift];
	for(y = 0;y < size;y++)
		for(int x = 0;x < size;x++){
			int up = 0;
			int lo = 0;
			for(int i = 0;i < SCALE;i++)
				for(int j = 0;j < SCALE;j++){
					up += upper_buffer[x*SCALE + i + (y*SCALE + j << shape_shift)];
					lo += lower_buffer[x*SCALE + i + (y*SCALE + j << shape_shift)];
					}

			up_buffer[x + (y << shift)] = up + (1 << LSCALE - 1) >> 2*LSCALE;
			lo_buffer[x + (y << shift)] = lo + (1 << LSCALE - 1) >> 2*LSCALE;
			}
	swap(upper_buffer, up_buffer);
	swap(lower_buffer, lo_buffer);
	swap(shape_size, size);
	swap(shape_shift, shift);
	delete up_buffer;
	delete lo_buffer;
}

float PolygonS::height(float x, float y)
{
	float x0 = vertices[0] -> xscr;
	float y0 = vertices[0] -> yscr;
	float z0 = vertices[0] -> turned.z;

	float dx1 = vertices[1] -> xscr - x0;
	float dy1 = vertices[1] -> yscr - y0;
	float dz1 = vertices[1] -> turned.z - z0;

	float dx2 = vertices[2] -> xscr - x0;
	float dy2 = vertices[2] -> yscr - y0;
	float dz2 = vertices[2] -> turned.z - z0;
	
	float t9 = dx1*dz2;
	float t12 = dx1*dy2;
	float t14 = dx2*dz1;
	float t17 = dx2*dy1;
	float zzz = t12-t17;
	if(fabs(zzz) < 1e-12)
		throw 1;
	return -(x*dy1*dz2-x*dz1*dy2-x0*dy1*dz2+x0*dz1*dy2-t9*y+t9*y0-t12*z0+t14*y-t14*y0+t17*z0)/zzz;
}

void PolygonS::height_project()
{
#define NEXT_LEFT(p)	((p) == beg ? end : (p) - 1)
#define NEXT_RIGHT(p)	((p) == end ? beg : (p) + 1)

	Vertex *lfv,*rfv,*ltv,*rtv;
	Vertex **lto,**rto;
	Vertex **beg,**curr,**up,**end;

	beg = up = curr = vertices;
	end = vertices + num_vert - 1;
	curr++;
	for(;curr <= end;curr++)
		if((*up) -> yscr > (*curr) -> yscr)
			up = curr;

	rfv = lfv = *up;

	lto = NEXT_LEFT(up);
	ltv = *lto;

	rto = NEXT_RIGHT(up);
	rtv = *rto;

	int Y = round(lfv -> yscr); 
	float xl = lfv -> xscr;
	float al = ltv -> xscr - xl; 
	float bl = round(ltv -> yscr) - Y;
	float ar = rtv -> xscr - xl; 
	float br = round(rtv -> yscr) - Y;
	float xr = xl;

	float cl = lfv -> turned.z;
	float ckl = ltv -> turned.z - cl;
	float ckr = rtv -> turned.z - cl;
	float cr = cl;

	if(bl){
		al /= bl;
		ckl /= bl;
		}

	if(br){
		ar /= br;
		ckr /= br;
		}

	if(Y<0)Y=0;//ВНИМАНИЕ причина выхода в отрицательную обдасть не установлена (conus.c3d)
	unsigned short * up_vp = upper_buffer + (Y << shape_shift);
	unsigned short * lo_vp = lower_buffer + (Y << shape_shift);

	int where, d;
	while(1){
		if(bl > br){
			d = br;
			where = 0;
			}
		else{
			d = bl;
			where = 1;
			}
		while(d-- > 0){
			int x1 = round(xl);
			int x2 = round(xr);
			if(x1 != x2){
				int swap_log = 0;
				if(x1 > x2){
					SWAP(x1, x2);
					SWAP(cl, cr);
					swap_log = 1;
					}
				int len = x2 - x1;
				float cf = cl;
				float cfk = (cr - cl)/len;
				up_vp += x1;
				lo_vp += x1;
				while(len--){
					try {
						//int z = round(cf);
						int z = round(height(x1++,Y));
						if(z > 0 && z < 0xffFF){//MAX_VX_HEIGHT
							int u = *up_vp;
							if(abs(u - z) < averaging_threshould)
								*up_vp = u + z >> 1;
							else
								if(u < z)
									*up_vp = z;

							int l = *lo_vp;
							if(abs(l - z) < averaging_threshould)
								*lo_vp = l + z >> 1;
							else
								if(l > z)
									*lo_vp = z;
							}
						}
					catch(...){}
					cf += cfk;
					up_vp++;
					lo_vp++;
					}
				up_vp -= x2;
				lo_vp -= x2;
				if(swap_log)
					SWAP(cl,cr);
				}
			Y++;
			up_vp += 1 << shape_shift;
			lo_vp += 1 << shape_shift;

			xl += al;
			xr += ar;

			cl += ckl;
			cr += ckr;
			}
		if(where){
			if(ltv == rtv) return;
			lfv = ltv;
			lto = NEXT_LEFT(lto);
			ltv = *lto;

			br -= bl;

			xl = lfv -> xscr;
			al = ltv -> xscr - xl;
			bl = round(ltv -> yscr) - Y;

			cl = lfv -> turned.z;
			ckl = ltv -> turned.z - cl;

			if(bl){
				al /= bl;
				ckl /= bl;
				}
			}
		else {
			if(rtv == ltv) return;
			rfv = rtv;
			rto = NEXT_RIGHT(rto);
			rtv = *rto;

			bl -= br;

			xr = rfv -> xscr;
			ar = rtv -> xscr - xr;
			br = round(rtv -> yscr) - Y;

			cr = rfv -> turned.z;
			ckr = rtv -> turned.z - cr;

			if(br){
				ar /= br;
				ckr /= br;
				}
			}
		}
}

