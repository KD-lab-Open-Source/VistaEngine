
#define IMP_SPHERE	0
#define IMP_POLYGON	1
#define IMP_PEAK	2
#define IMP_MAX 	3

struct ImpassType;

void ImpassPrepare(void);
void ImpassSave(void);

extern ImpassType* ImpPattern[IMP_MAX];
extern int curImpIndex;
extern char* ImpItem[IMP_MAX];

struct ImpassType {
	int sx,sy,size;
	int level;
	int force;
	int mode;

	int radInt;
	int h;
	int n,phase;
	int inverse;
	int plain;
	int noiseLevel,noiseAmp;

	double rad;
	short* map;

		ImpassType(int _sx,int _sy,int _force = 0,int _mode = 0,int _level = 0,int _rad = 100,int _h = 256,int _n = 4,int _phase = 0,int _inverse = 0,int _plain = 0,int _noiseLevel = 0,int _noiseAmp = 0);
		~ImpassType(void){ delete map; }

	virtual void build(void){}

	void put(int x,int y);
	void change(int _sx,int _sy,int _force = 0,int _mode = 0,int _level = 0,int _rad = 100,int _h = 256,int _n = 4,int _phase = 0,int _inverse = 0,int _plain = 0,int _noiseLevel = 0,int _noiseAmp = 0);
	void init(void);
	void reinit(void);
	void read(XStream& ff);
	void write(XStream& ff);
	};

struct ImpSphere : ImpassType {

		ImpSphere(int _sx,int _sy,int _force = 0,int _mode = 0,int _level = 0,int _rad = 100,int _h = 256,int _n = 4,int _phase = 0,int _inverse = 0,int _plain = 0,int _noiseLevel = 0,int _noiseAmp = 0);

	virtual void build(void);
	};

struct ImpPolygon : ImpassType {

		ImpPolygon(int _sx,int _sy,int _force = 0,int _mode = 0,int _level = 0,int _rad = 100,int _h = 256,int _n = 4,int _phase = 0,int _inverse = 0,int _plain = 0,int _noiseLevel = 0,int _noiseAmp = 0);

	virtual void build(void);
	};

struct ImpPeak : ImpassType {

		ImpPeak(int _sx,int _sy,int _force = 0,int _mode = 0,int _level = 0,int _rad = 100,int _h = 256,int _n = 4,int _phase = 0,int _inverse = 0,int _plain = 0,int _noiseLevel = 0,int _noiseAmp = 0);

	virtual void build(void);
	};

extern void PutImpPoligon(int x, int y, int _sx,int _sy,int _force,int _mode,int _level,int _rad,int _h,int _n,int _phase,int _inverse,int _plain,int _noiseLevel,int _noiseAmp);
extern void PutImpSphere(int x, int y, int _sx,int _sy,int _force,int _mode,int _level,int _rad,int _h,int _n,int _phase,int _inverse,int _plain,int _noiseLevel,int _noiseAmp);
