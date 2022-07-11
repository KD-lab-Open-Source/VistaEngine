
#ifndef __PHYSICS_UTIL_H__
#define __PHYSICS_UTIL_H__

#include "crc.h"
#include "..\Render\Inc\umath.h"
#include "..\Render\Inc\RenderMT.h"
#include "MTSection.h"

/////////////////////////////////////////////////////////////////////////////////
//		Отладочный вывод 3D с кэшированием
/////////////////////////////////////////////////////////////////////////////////
class ShowDispatcher
{
	class Shape {
		enum Type { Point, Text, Text2D, Circle, Delta, Line, Triangle, Quadrangle, ConvexArray };
		Type type;
		sColor4c color;
		//union {
		//	struct { Vect3f point; float radius; };
		//	struct { Vect3f pointX; const char* text; };
		//	struct { Vect3f point1, point2; };
		//	struct { int n_points; Vect3f* points; };
		//	};
		
		union {
			struct { Vect3f point; float radius; };
			struct { Vect3f point1, point2; };
		};
		Vect3f pointX; string text;
		int n_points; vector<Vect3f> points;

		//static bool isArray(Type type) { return type == Triangle || type == Quadrangle || type == ConvexArray; }
	public:	
		void validate(const Vect3f& v){const float inf=20000; xassert(v.x>=-inf && v.x<inf && v.y>=-inf && v.y<inf && v.z>=-inf && v.z<inf);}

		Shape(const Vect3f& v, sColor4c color_) { validate(v);type = Point; point = v; color = color_; }
		//Shape(const Vect3f& v, const char* text_, sColor4c color_) { validate(v); type = Text; pointX = v; text = strdup(text_); color = color_; }
		Shape(const Vect3f& v, const char* text_, sColor4c color_) { validate(v); type = Text; pointX = v; text = text_; color = color_; }
		//Shape(const Vect2f& v, const char* text_, sColor4c color_) { type = Text2D; pointX = Vect3f(v.x, v.y, 0.f); text = strdup(text_); color = color_; }
		Shape(const Vect2f& v, const char* text_, sColor4c color_) { type = Text2D; pointX = Vect3f(v.x, v.y, 0.f); text = text_; color = color_; }
		Shape(const Vect3f& v, float radius_, sColor4c color_) { validate(v); type = Circle; point = v; radius = radius_; color = color_; }
		Shape(const Vect3f& v0, const Vect3f& v1, sColor4c color_, int line) { validate(v0);validate(v1);type = line ? Line : Delta; point1 = v0; point2 = v1; color = color_; }
		//Shape(const Vect3f& v0, const Vect3f& v1, const Vect3f& v2, sColor4c color_) { validate(v0);validate(v1);validate(v2);type = Triangle; points = new Vect3f[n_points = 3]; points[0] = v0; points[1] = v1; points[2] = v2; color = color_; }
		Shape(const Vect3f& v0, const Vect3f& v1, const Vect3f& v2, sColor4c color_) {
			validate(v0);validate(v1);validate(v2);
			type = Triangle;
			points.push_back(v0);
			points.push_back(v1);
			points.push_back(v2);
			color = color_;
			n_points = points.size();
		}
		//Shape(const Vect3f& v0, const Vect3f& v1, const Vect3f& v2, const Vect3f& v3, sColor4c color_) { validate(v0);validate(v1);validate(v2);type = Quadrangle; points = new Vect3f[n_points = 4]; points[0] = v0; points[1] = v1; points[2] = v2; points[3] = v3; color = color_; }
		Shape(const Vect3f& v0, const Vect3f& v1, const Vect3f& v2, const Vect3f& v3, sColor4c color_) {
			validate(v0);validate(v1);validate(v2);validate(v3);
			type = Quadrangle;
			points.push_back(v0);
			points.push_back(v1);
			points.push_back(v2);
			points.push_back(v3);
			color = color_;
			n_points = points.size();
		}
		//Shape(int n_points_, const Vect3f* points_, sColor4c color_) { type = ConvexArray; points = new Vect3f[n_points = n_points_]; memcpy(points, points_, sizeof(Vect3f)*n_points); color = color_; }
		Shape(int n_points_, const Vect3f* points_, sColor4c color_) {
			type = ConvexArray;
			for(int i = 0; i < n_points; ++i){
				points.push_back(points_[i]);
			}
			color = color_;
			n_points = points.size();
		}
		//Shape(const Shape& shape) 
		//{ 
		//	*this = shape; 
		//	if(isArray(type)){ 
		//		points = new Vect3f[n_points = shape.n_points]; 
		//		memcpy(points, shape.points, sizeof(Vect3f)*n_points); 
		//	} 
		//	else if(type == Text || type == Text2D)
		//		text = strdup(shape.text);
		//}
		//~Shape() { if(isArray(type)) delete points; else if(type == Text || type == Text2D) free((void*)text); }
		void show();
		void showConvex();
		};

	typedef vector<Shape> List;
	List shapes;
	List shapesGraphics_;
	bool need_font;
	MTSection lock_;

public:
	void draw();
	void clear();

	void point(const Vect3f& v, sColor4c color)
			{ MTL(); shapes.push_back(Shape(v, color)); }
	void text(const Vect3f& v, const char* text, sColor4c color)
			{ MTL(); shapes.push_back(Shape(v, text, color)); need_font = true; }
	void text2d(const Vect2f& v, const char* text, sColor4c color)
			{ MTL(); shapes.push_back(Shape(v, text, color)); need_font = true; }
	void circle(const Vect3f& v, float radius, sColor4c color)
			{ MTL(); shapes.push_back(Shape(v, radius, color)); }
	void line(const Vect3f &v0, const Vect3f &v1, sColor4c color)
			{ MTL(); shapes.push_back(Shape(v0, v1, color, 1)); }
	void delta(const Vect3f& v, const Vect3f& dv, sColor4c color)
			{ MTL(); shapes.push_back(Shape(v, dv, color, 0)); }
	void triangle(const Vect3f &v0, const Vect3f &v1, const Vect3f &v2, sColor4c color)
			{ MTL(); shapes.push_back(Shape(v0, v1, v2, color)); }
	void quadrangle(const Vect3f &v0, const Vect3f &v1, const Vect3f &v2, const Vect3f &v3, sColor4c color)
			{ MTL(); shapes.push_back(Shape(v0, v1, v2, v3, color)); }
	void convex(int n_points, const Vect3f* points, sColor4c color)
			{ MTL(); shapes.push_back(Shape(n_points, points, color));  }
};

extern ShowDispatcher show_dispatcher;

inline void show_vector(const Vect3f& vg, sColor4c color){ show_dispatcher.point(vg, color); }
inline void show_vector(const Vect3f& vg, float radius, sColor4c color){ show_dispatcher.circle(vg, radius, color); }
inline void show_vector(const Vect3f& vg, const Vect3f& delta, sColor4c color){ show_dispatcher.delta(vg, delta, color); }
inline void show_vector(const Vect3f &vg0, const Vect3f &vg1, const Vect3f &vg2, sColor4c color){ show_dispatcher.triangle(vg0, vg1, vg2, color); }
inline void show_vector(const Vect3f &vg0, const Vect3f &vg1, const Vect3f &vg2, const Vect3f &vg3, sColor4c color){ show_dispatcher.quadrangle(vg0, vg1, vg2, vg3, color); }
inline void show_convex(int n_points, const Vect3f* points, sColor4c color){ show_dispatcher.convex(n_points, points, color); }
inline void show_line(const Vect3f &vg0, const Vect3f &vg1, sColor4c color){ show_dispatcher.line(vg0, vg1, color); }
void show_terrain_line(const Vect2f& p1, const Vect2f& p2, sColor4c color); // Медленная очень.
inline void show_text(const Vect3f& vg, const char* text, sColor4c color){ show_dispatcher.text(vg, text, color); }
inline void show_text2d(const Vect2f& vg, const char* text, sColor4c color){ show_dispatcher.text2d(vg, text, color); }

extern sColor4c 
	 WHITE,
	 BLACK,
	 RED,
	 GREEN,
	 BLUE,
	 YELLOW,
	 MAGENTA,
	 CYAN,
	 X_COLOR,
	 Y_COLOR,
	 Z_COLOR;
sColor4c XCOL(sColor4c color, int intensity = 255, int alpha = 255);

/////////////////////////////////////////////////////////////////////////////////
//		Watch system
/////////////////////////////////////////////////////////////////////////////////
void add_watch(const char* var, const char* value);

XBuffer& watch_buffer();
template<class T> inline void watchBuffer(const T& t) { watch_buffer().init(); watch_buffer() <= t; watch_buffer() < char(0); }
inline void watchBuffer(const char* t) { watch_buffer().init(); watch_buffer() < t; watch_buffer() < char(0); }
inline void watchBuffer(const string& t) { watch_buffer().init(); watch_buffer() < t.c_str(); watch_buffer() < char(0); }

#define watch(var) { watchBuffer(var); add_watch(#var, watch_buffer()); }
#define watch_i(var, index) { watch_buffer().init(); watch_buffer() < #var < "[" <= index < "]"; string name(watch_buffer()); watchBuffer(var); add_watch(name.c_str(), watch_buffer()); }
#define watch_gi(var, index, group) { watch_buffer().init(); watch_buffer() < #group < "." < #var < "[" <= index <= "]"; string name(watch_buffer()); watchBuffer(var); add_watch(name.c_str(), watch_buffer()); }
void show_watch();

/////////////////////////////////////////////////////////////////////////////////
//		Determinacy Log
//
//  Log usage:
//
//		log_var(var);  
//		
//	or:	
//
//		#ifndef _FINAL_VERSION_
//		if(log_mode)
//			log_buffer <= var;
//		#endif
//
//	Command line arguments:
//
//	save_log [time_to_exit:seconds]
//	verify_log [append_log] [time_to_exit:seconds]
//
//	_FORCE_NET_LOG_ - to force network log under _FINAL_VERSION_
/////////////////////////////////////////////////////////////////////////////////
#define _FORCE_NET_LOG_
#if (!defined(_FINAL_VERSION_) || defined(_FORCE_NET_LOG_))
#define _DO_LOG_
#endif

extern bool net_log_mode;
extern XBuffer net_log_buffer;

#ifdef _DO_LOG_
#define log_var_aux(var, file, line) { if(net_log_mode) { watchBuffer(var); net_log_buffer < file < ", " <= line < "] " < #var < ": " < watch_buffer() < "\n"; } }
#define log_var(var) log_var_aux(var, __FILE__, __LINE__)
#define log_var_crc(address, size) { log_var(crc32((const unsigned char*)address, size, startCRC32)) }
#else
#define log_var_aux(var, file, line)
#define log_var(var)
#define log_var_crc(address, size)	
#endif


/////////////////////////////////////////////////////////////////////////////////
//		Smart Log
/////////////////////////////////////////////////////////////////////////////////
#ifdef _FINAL_VERSION_
	class VoidStream
	{
	public:
		VoidStream(const char* name = 0, unsigned flags = 0){}
		void open(const char* name, unsigned flags){}
		bool isOpen() const { return true; }
		void close(){}
		template<class T> 
		VoidStream& operator< (const T&) { return *this; }
		template<class T> 
		VoidStream& operator<= (const T&) { return *this; }
		template<class T> 
		VoidStream& operator<< (const T&) { return *this; }
	};

	typedef VoidStream LogStream;
#else
	typedef XStream LogStream;
#endif

extern LogStream fout;


/////////////////////////////////////////////////////////////////////////////////
//		Utils
/////////////////////////////////////////////////////////////////////////////////
//template<class T> // Для установки параметров
//bool check_command_line_parameter(const char* switch_str, T& parameter) { const char* s = check_command_line(switch_str); if(s){ parameter = atoi(s); return true; } else return false; }

// Vect2f  -> Vect3f convertions
Vect3f To3D(const Vect2f& pos);
inline Vect3f To3Dzero(const Vect2f& pos) { return Vect3f(pos.x, pos.y, 0); }
inline Vect3f to3D(const Vect2f& pos, float z) { return Vect3f(pos.x, pos.y, z); }
Vect3f clampWorldPosition(const Vect3f& pos, float radius);

#undef xassert_s
#ifndef _FINAL_VERSION_
#define xassert_s(exp, str) { string s = #exp; s += "\n"; s += str; xxassert(exp,s.c_str()); }
#else
#define xassert_s(exp, str) 
#endif

//--------------------------------------
extern RandomGenerator logicRnd;
extern RandomGenerator effectRND;//В графике используется graphRnd.

#ifndef _FINAL_VERSION_

inline int logicRNDi(int x, const char* file, int line)
{
	MTL();
	log_var_aux(logicRnd.get(), file, line);
	return logicRnd(x);
}
inline int logicRNDii(int min, int max, const char* file, int line)
{
	MTL();
	log_var_aux(logicRnd.get(), file, line);
	return logicRnd(min, max);
}
inline float logicRNDf(float val, const char* file, int line)
{
	MTL();
	log_var_aux(logicRnd.get(), file, line);
	return logicRnd.frnd(val);
}
inline float logicRNDff(float min, float max, const char* file, int line)
{
	MTL();
	log_var_aux(logicRnd.get(), file, line);
	return logicRnd.fabsRnd(min, max);
}
inline float logicRNDfa(const char* file, int line, float val = 1.f)
{
	MTL();
	log_var_aux(logicRnd.get(), file, line);
	return logicRnd.fabsRnd(val);
}

#define logicRND(x) logicRNDi(x, __FILE__, __LINE__)
#define logicRNDinterval(min, max) logicRNDii(min, max, __FILE__, __LINE__)
//-1..+1
#define logicRNDfrnd(val) logicRNDf(val, __FILE__, __LINE__)
//0..+1
#define logicRNDfrand() logicRNDfa(__FILE__, __LINE__)
#define logicRNDfabsRnd(val) logicRNDfa(__FILE__, __LINE__, val)
#define logicRNDfabsRndInterval(min, max) logicRNDff((min), (max), __FILE__, __LINE__)

#else // _FINAL_VERSION_

#define logicRND(x) logicRnd(x)
#define logicRNDinterval(min, max) logicRnd(min, max)
//-1..+1
#define logicRNDfrnd(val) logicRnd.frnd(val)
//0..+1
#define logicRNDfrand() logicRnd.fabsRnd(1.f)
#define logicRNDfabsRnd(val) logicRnd.fabsRnd(val)
#define logicRNDfabsRndInterval(min, max) logicRnd.fabsRnd(min, max)

#endif // _FINAL_VERSION_


#include "Range.h"

inline int logicRndInterval(const Rangei& range){
	return logicRnd(range.minimum(), range.maximum());
}

inline float logicRndInterval(const Rangef& range){
	return logicRnd.fabsRnd(range.minimum(), range.maximum());
}

//--------------------------------------

#include "..\Util\DebugPrm.h"


///////////////////////////////////////////////
//		Profiler
///////////////////////////////////////////////
#include "Profiler.h"

//--------------------------------------

bool isUnderEditor();

#endif // __PHYSICS_UTIL_H__
