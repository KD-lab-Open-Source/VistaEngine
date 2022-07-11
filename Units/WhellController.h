#ifndef __WHELL_CONTROLLER_H_INCLUDED__
#define __WHELL_CONTROLLER_H_INCLUDED__

class UnitReal;

class WheelDescriptor {
public:

	WheelDescriptor() : frontWheel(false) {}

	Logic3dxNode nodeLogic;
	Object3dxNode nodeGraphics;
	bool frontWheel;

	void serialize(Archive& ar) {
		ar.serialize(nodeLogic, "nodeLogic", "Имя логической ноды");
		ar.serialize(nodeGraphics, "nodeGraphics", "Имя графической ноды");
		ar.serialize(frontWheel, "frontWheel", "Переднее колесо");
	}
};

typedef vector<WheelDescriptor> WheelDescriptorList;

class WhellController {
public:
	
	WhellController(UnitReal& unit): unit_(unit), angle_(0), whellRadius(0), whellAngle(0), update_(false) { }

	bool init(const WheelDescriptorList& wheelList);
	void setAngle(float angle) { angle_ = angle; }
	void addVelocity(float linearVelocity);
	void interpolationQuant();

	Vect3f placeToGround(const Se3f& pose, float& groundZ, bool waterAnalysis);

private:

	UnitReal& unit_;
	vector<int> nodeIndexesFront;
	vector<Se3f> nodeOffsetsFront;
	vector<int> nodeIndexesBack;
	vector<Se3f> nodeOffsetsBack;
	vector<Vect3f> wheelPositions;

	vector<Se3f> nodeOrientationPrevFront;
	vector<Se3f> nodeOrientationPrevBack;

	float whellAngle;
	float angle_;
	float whellRadius;
	bool update_;
};

#endif
