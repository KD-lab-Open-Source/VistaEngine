#ifndef __TRANSPARENT_TRACKING_H_INCLUDED__
#define __TRANSPARENT_TRACKING_H_INCLUDED__

class UnitBase;

class TransparentTracking
{
public:
	TransparentTracking();
	~TransparentTracking();
	void track(Camera* camera);

private:
	Vect2i gridSize_;
	typedef vector<UnitBase*> Objects;
	struct Node
	{
		Objects objects;
	};
	Node* grid_;
	bool gridFilled_;

	bool calcBound(c3dx* obj, Camera* camera, Vect2i& leftTop, Vect2i& rightBottom);
};


#endif
