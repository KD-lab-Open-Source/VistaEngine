#ifndef __LIGHTING_H_INCLUDED__
#define __LIGHTING_H_INCLUDED__

#include <list>
#include "Render\inc\IVisGenericInternal.h"

class LightingParameters
{
public:
	LightingParameters();

	void serialize(Archive& ar);

	float generate_time;
	string texture_name;
	float strip_width_begin;
	float strip_width_time;
	float strip_length;
	float fade_time;
	float lighting_amplitude;
};

class RENDER_API Lighting : public BaseGraphObject
{
public:
	Lighting();
	~Lighting();
	void Update(const Vect3f& pos_begin, const vector<Vect3f>& pos_end);
	void SetParameters(LightingParameters& param_);

	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
	void Animate(float dt);

	bool IsLive();

	void StopGenerate(bool generate) {stopGenerate = generate;}

	const MatXf& GetPosition() const { return global_pos; }

	const Color4f& color() const { return color_; }
	void setColor(const Color4f& color){ color_ = color; }

protected:
	void Generate(Vect3f pos_begin,Vect3f pos_end,Camera* camera);
	float time;
	MatXf global_pos;
	Vect3f pos_begin;
	Vect3f pos_delta;
	vector<Vect3f> pos_end;
	LightingParameters param;
	bool stopGenerate;

	cTexture* pTexture;
	struct OneStrip
	{
		Vect3f pos;
		Vect3f n;
		float u;
	};

	struct OneLight
	{
		Vect3f pos_begin,pos_end;
		vector<Vect3f> position;
		vector<OneStrip> strip_list;
		float time;
		OneLight();
		~OneLight();

		void Generate(Vect3f pos_begin,Vect3f pos_end,Camera* camera,Lighting* parent);
		void Draw(Camera* camera,Lighting* parent);
		void Animate(float dt);
		void GenerateInterpolate(vector<float>& pos,int size,float amplitude);
		float get(vector<float>& p,float t);
		void Update(Vect3f pos_delta);
		void BuildStrip(Camera* camera,Lighting* parent);
	};
	list<OneLight*> lights;

	struct PreGenerate
	{
		Vect3f pos_begin;
		Vect3f pos_end;
	};
	vector<PreGenerate> pre_generate;

	Color4f color_;
};

#endif
