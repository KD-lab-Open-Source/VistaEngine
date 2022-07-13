
/*******************************************************************************
		3D graphical structs and constants
*******************************************************************************/

const SCREEN_LEN_X = 4;
const SCREEN_LEN_Y = 3;


#define NORMAL_LEN   7
#define NORMAL	((1 << NORMAL_LEN) - 5)

#define SIZE_OF_MODEL	116
#define SCALE_THRESHOLD 240

#define NEXT_VERTEX(curr)	(curr != end ? curr + 1 : beg)
#define PREV_VERTEX(curr)	(curr != beg ? curr - 1 : end)

#define C3D_VERSION_1	3
#define C3D_VERSION_3	8

#define DRAW_SX 	256
#define DRAW_SY 	256

#define DBL_EPS 1e-15
#define INT_INF 0x7FFFFFFF

#define SWAP(a,b)	{ *(int*)&a ^= int(b); *(int*)&b ^= int(a); *(int*)&a ^= int(b); }
#define SIGN(x) (x ? (x > 0 ? 1 : -1 ) : 0)
#define GOOD_SHR(x,n)	((x) < 0 ? -(-(x) + (1 << (n) - 1) >> (n)) : ((x) + (1 << (n) - 1) >> (n)))
#define DIV16(a,b)	  (a = ((a) << 16)/(b))
#define DIV16T(x,y)	((int)div_table_short[((x) & 0xFF) | (((y) << 8) & 0xFF00)] << 8)
#define MUL_CHAR(x,y)	     (mul_table[((x) & 0xFF) | (((y) << 8) & 0xFF00)])


enum {
	DRAW_DEFAULT,
	DRAW_LINE,
	DRAW_FLAT,
	DRAW_GOURAUD
};

struct COLORS_IDS{
	enum {
		ZERO_RESERVED,
		
		BODY,

		WINDOW,
		WHEEL,
		DEFENSE,
		WEAPON,
		TUBE,
		
		BODY_RED,
		BODY_BLUE,
		BODY_YELLOW,
		BODY_GRAY,
		YELLOW_CHARGED,
		
		MATERIAL_0,
		MATERIAL_1,
		MATERIAL_2,
		MATERIAL_3,
		MATERIAL_4,
		MATERIAL_5,
		MATERIAL_6,
		MATERIAL_7,
		
		BLACK,
		BODY_GREEN,
		
		SKYFARMER_KERNOBOO,
		SKYFARMER_PIPETKA,
		ROTTEN_ITEM,

		MAX_COLORS_IDS
		};
	};

/*******************************************************************************
			Vertex -> PolygonS link
	Vertex:    <ÄÄÄÄÄÄÄÄÄ¿	   ÚÄÄÄÄÄÄÄÄÄÄ> PolygonS:
	num_poly	     ÀÄÄÄÄÄÅÄÄÄÄÄ¿	num_vert
	polygons[0]  ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ	 ÀÄÄÄÄ	vertices[indices_in_poly[0]]
	indices_in_poly[0]

			PolygonS -> Vertex link
	Vertex:   <ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿  ÚÄ> PolygonS:
	num_poly		      ÚÄÄÅÄÄÙ	num_vert
	polygons[indices_in_vert[0]] ÄÙ  ÀÄÄÄÄ	vertices[0]
						indices_in_vert[0]
*******************************************************************************/

struct PolygonS;

struct Vertex {
	char x_8,y_8,z_8;
	unsigned char yscr_8;
	unsigned char xscr_8;
	unsigned char zscr_8;

	float x,y,z;
	Vector turned;
	float xscr,yscr;

	int flag;
	int index;
	int wheel_belonging;

	int num_poly;
	PolygonS** polygons;
	char* indices_in_poly;
	Vertex* reference;


		Vertex(double vx,double vy,double vz){ x_8 = round(vx); x = (float)vx; y_8 = round(vy); y = (float)vy; z_8 = round(vz); z = (float)vz; }
		Vertex(const DBV& v){ x_8 = round(v.x); x = (float)v.x; y_8 = round(v.y); y = (float)v.y; z_8 = round(v.z); z = (float)v.z; }
		Vertex& operator = (const DBV& v){ x_8 = round(v.x); x = (float)v.x; y_8 = round(v.y); y = (float)v.y; z_8 = round(v.z); z = (float)v.z; return *this; }
		operator DBV (){ return DBV(x,y,z); }

		Vertex(){}

	void convert();
	void fast_convert();
	void fast_convert_z();
	void convert_big();
};

struct Normal {
	char x,y,z;
	unsigned char I_8,n_power;

	int I;
	int flag;
	int index;
	Normal* reference;

		Normal(int xx,int yy,int zz){ x = (char)xx; y = (char)yy; z = (char)zz; }
		Normal(const Vector& v){ x = (char)v.x; y = (char)v.y; z = (char)v.z; }
		Normal& operator = (const Vector& v){ x = (char)v.x; y = (char)v.y; z = (char)v.z; return *this; }
		operator Vector (){ return Vector(x,y,z); }
		Normal(){}

	void calc_intensity();
	void fast_calc_intensity();
};

struct PolygonS {
	int num_vert;
	Vertex** vertices;
	Normal** normals;
	unsigned char color_id;
	char middle_x,middle_y,middle_z;
	Normal flat_normal;

	int flag;
	int index;

	char* indices_in_vert;

	float z_min,z_max;
	PolygonS *prev;
	PolygonS *next;

//	void put_in_list();
	void draw_line();
	void draw_flat();
	void draw_gouraud();
	void draw_gouraud_old();
	void draw_gouraud_decomposed_true_z();
	void draw_pixel();
	//void draw_gouraud_big();
	void draw_gouraud_big(unsigned short* GRAF_BUF, int GB_MAXX, int GB_MAXY);
	void height_project();
	float height(float x, float y);
	int lower_average(int xx,int yy,int zz,double& X0,double& Y0,double& dZ0,double& X1,double& Y1,double& dZ1);
	int upper_average(int xx,int yy,int zz,double& X0,double& Y0,double& dZ0,double& X1,double& Y1,double& dZ1);
	int check_terrain_type(int xx,int yy,int zz,int terrain_type);
	int check_mole_submerging(int xx,int yy,int zz);
	int lower_average_z(int x_offset,int y_offset,int z_offset,double& dZ,DBV& r,DBV& n);

	void compProjectionIntegrals();
	void compFaceIntegrals();
};


struct Model {
	int num_vert;
	Vertex* vertices;
	int num_norm;
	Normal* normals;
	int num_poly;
	PolygonS* polygons;
	PolygonS** sorted_polygons[3];

	int xmax,ymax,zmax;
	int xmin,ymin,zmin;
	int x_off,y_off,z_off;
	int rmax;
	int memory_allocation_method;
	
	double volume;
	DBV rcm;
	DBM J;


	Model();

	void loadASC(char* name);
	void loadASCinverted(char* name);
	void loadC3D(XBuffer& in);
	void loadC3Dvariable(XBuffer& in);
	void saveC3D(XStream& out);
	Vertex* add_vertices(int num_add);
	PolygonS* add_polygons(int num_add);
	void make_polygon_vertex_links();
	void triangle_to_quadrilateral();
	void calc_box(int xmin,int xmax,int ymin,int ymax,int zmin,int zmax,int delta);
	void join(Model& m);
	void free();

	void prepare(int central = 0);
	void move(Vector offset);
	void rotate(DBM R);
	void scale(double k,int dont_move = 0);
	void calc_polygons_middle();
	void set_colors(int color_id,int n_power,int wheel_belonging);
	void height_project();

	void make_normals();
	void make_normals_smoothing(int grad_threshould = 60);
	void weld(double grid_size);
	void split(double threshold);
	void morph(Model& shape,int z_low);
	DBV r_attract(DBV r0);
	void calc_bound(Model& shape);

	void draw(Vector R, const DBM& A_scl, const DBM& A_inv);
	void draw_child(const Vector& R,const DBM& A_c2p,const DBM& A_p2c);
	void pixel_draw();
	void z_buffering_draw();
	//void draw_big();
	void draw_big(int ViewX, int ViewY, float k_scale, unsigned short* GRAF_BUF, int GB_MAXX, int GB_MAXY);

	void pre_sorting();
	void sample(int i_sample);

	void VolumeIntegral(int only_upper_level = 0);
	double compVolumeIntegrals();
	};


/* ---------------------------- EXTERN SECTION ----------------------------- */

extern int draw_mode;
extern int draw_color;

extern DBM A_g2s;
extern Vector Refl, View, Light;
extern int ViewZ;//ViewX,ViewY,
extern int focus;
extern double focus_flt;
extern int TurnAngle;
extern int SlopeAngle;

extern int ScreenCX;
extern int ScreenCY;
extern int Xmax2;
extern int Ymax2;
extern int UcutLeft;
extern int UcutRight;
extern int VcutUp;
extern int VcutDown;

extern int page;
extern int counter;
extern DBM A_convert;

extern int CentrX;
extern int CentrY;
extern int CentrZ;

extern int u_cut_left;
extern int u_cut_right;
extern int v_cut_up;
extern int v_cut_down;

extern int flush_x;
extern int flush_y;
extern int flush_sx;
extern int flush_sy;

extern int poly_cnt;
extern int poly_number;
extern int poly_save;

extern int poly_count;

extern PolygonS* z_list_min;
extern PolygonS* z_list_max;
extern PolygonS* z_list_curr;
extern PolygonS* max_z_poly;
extern PolygonS* min_z_poly;

//void graph3d_init();
//void calc_screen(int zoom_percent);
void calc_screen(int zoom_percent, int MAX_X, int MAX_Y);
void calc_border(int& x1,int& x2,int& xc,int& sx2,int& flush_x,int& flush_sx,int xmax);


