#ifndef __SHOW_HEAD_H_INCLUDED__
#define __SHOW_HEAD_H_INCLUDED__
#include "Handle.h"

#include "PlayOgg.h"

class ShowHead
{

	enum ShowHeadMode
	{	
		ShowHead_Silence,
		ShowHead_Play,
	};

	cScene* scene;
	cCamera* camera;
	sRectangle4f rect;
	float phase;
	float phasePrev;
	float phaseInterpolation;
	string model_name;
	bool needDraw;

	cObject3dx* head;
	float current_chain_len;
	float prev_chain_len;
	int ix_main_group;

	int ix_camera;			// index camera node
	int ix_light;			// index light node

	string main_chain_;
	int id_curren_chain;
	int id_prev_chain;
	bool playing;
	bool cycled_;
	Vect2f focus_;
	bool interpolated_;

protected:
	void MoveCamera();
public:
	ShowHead();
	~ShowHead();

	void init();
	void release();

	void SetWndPos(const sRectangle4f& pos);
	bool LoadHead(const char* model_name, const char* main_chain, sColor4c skin_color);
	void resetHead();
	bool Play(bool cycled);
	bool IsPlay();
	bool NeedDraw() const { return needDraw; }
	void ToggleDraw(bool state){ needDraw = state; }
	void SetMainChain(const char* main_chain);

	void quant(float dt);
	void draw();

	void SetChain(cAnimationChain* chain);
};

extern Singleton<ShowHead> showHead;

#endif
