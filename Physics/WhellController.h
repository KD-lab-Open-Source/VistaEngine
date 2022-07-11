#ifndef __WHELL_CONTROLLER_H__
#define __WHELL_CONTROLLER_H__

///////////////////////////////////////////////////////////////
//
//    class WhellController
//
///////////////////////////////////////////////////////////////

class WhellController
{
public:
	WhellController(const WheelDescriptorList& wheelList, cObject3dx* model, cObject3dx* modelLogic);
	bool valid() { return whellRadius_ > FLT_EPS; }
	void quant(float angle, float linearVelocity);
	Vect3f placeToGround(const Se3f& pose, float& groundZ, bool waterAnalysis);

private:
	///////////////////////////////////////////////////////////
	//
	//    class Wheel
	//
	///////////////////////////////////////////////////////////

	class Wheel
	{
	public:
		Wheel(int nodeIndex) : nodeIndex_(nodeIndex) {}
		void interpolate(const Se3f& r, cObject3dx* model_)
		{
			nodeInterpolator_ = r;
			nodeInterpolator_(model_, nodeIndex_);
		}

	private:
		int nodeIndex_;
		InterpolatorNodeTransform nodeInterpolator_;
	};

	cObject3dx* model_;
	vector<Vect3f> positions_;
	vector<Wheel> wheels_;
	vector<Wheel> steeringWheels_;
	float whellAngle_;
	float angle_;
	float whellRadius_;
};

#endif // __WHELL_CONTROLLER_H__