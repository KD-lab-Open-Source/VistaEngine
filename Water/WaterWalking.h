#ifndef __WATER_WALKING_H_INCLUDED__
#define __WATER_WALKING_H_INCLUDED__

#include <deque>

class UnitReal;
class cEffect;

class cWaterPlume : public BaseGraphObject
{
public:
	cWaterPlume(UnitReal* unit, const WaterPlumeAttribute& attribute);
	~cWaterPlume();
	void Animate(float dt);
	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
	const MatXf& GetPosition() const { return pose; }
	
	struct ParticleUnDir
	{
		Vect3f pos;
		float phase;
		float max_size;
	};

	typedef deque<ParticleUnDir> ParticlesUnDir;

	struct UnitNode
	{
		int index;
		Vect3f position;
		float radius;
		float distance;
		float delta;
		cEffect* effect;
		bool onWater;
		UnitNode();
		~UnitNode();
		void update(const Vect3f& pos, bool enable, float waterZ, ParticlesUnDir& unDirPartls);
	};

	typedef vector<UnitNode> NodeContainer;

	NodeContainer& nodes() { return nodes_; }
	bool verifyZ() const { return verifyZ_; }
	ParticlesUnDir& unDirPartls() { return unDirPartls_; }
	void setRate(float rate_) { rate = rate_; }
	void setPosition(const Vect3f& position) { pose.trans() = position; }
private:
	MatXf pose;
	float radius;

	NodeContainer nodes_;
	bool verifyZ_;
	float rate;

	ParticlesUnDir unDirPartls_;
	cTexture* textureCyrcle;
	float frequency;
	float dt_;
};

#endif
