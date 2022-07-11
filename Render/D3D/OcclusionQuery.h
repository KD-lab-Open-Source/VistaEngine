#ifndef __OCCLUSION_QUERY_H_INCLUDED__
#define __OCCLUSION_QUERY_H_INCLUDED__

class Camera;
/*
VisibleCount()
IsVisible()
 должны вызываться только на следующий кадр. То есть ДО Test
*/
	  
class cOcclusionQuery
{
	IDirect3DQuery9* pQuery;
	bool draw;
	int testedCount_;
public:
	cOcclusionQuery();
	~cOcclusionQuery();

	bool Init();
	void Done();

	bool IsInit(){return pQuery!=0;}
	void Test(const Vect3f& pos);
	void Test(const Vect3f* point, int numPoints);
	void Clear() { draw = false; }

	int TestedCount() const{ return testedCount_; }
	int VisibleCount();
	bool IsVisible();

	void Begin();
	void End();
};

#endif
