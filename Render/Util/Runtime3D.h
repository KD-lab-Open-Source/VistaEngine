#pragma once

class cScene;
class Camera;
class cInterfaceRenderDevice;

class Runtime3D
{
public:

	Runtime3D();
	virtual ~Runtime3D();

	virtual bool init(int xScr,int yScr,bool bwindow);
	virtual void finit();

	virtual void createScene();
	virtual void destroyScene();
	
	virtual void quant();
	virtual void DrawFps(int x,int y);
	virtual void onWheel(bool wheelUp){}
	virtual void KeyDown(int key){}

	virtual void OnLButtonDown(){ leftButtonPressed_ = true; }
	virtual void OnLButtonUp(){ leftButtonPressed_ = false; }
	virtual void OnRButtonDown(){ rightButtonPressed_ = true; }
	virtual void OnRButtonUp(){ rightButtonPressed_ = false; }

	bool leftButtonPressed() const { return leftButtonPressed_; }
	bool rightButtonPressed() const { return rightButtonPressed_; }

	const Vect2i& mousePos() const { return mousePos_; }
	Vect2f mousePosRelative() const;
	void setMousePos(const Vect2i& pos){ mousePos_ = pos; }
	
	void setCameraPosition(const MatXf& matrix);

	cScene* scene() const { return scene_; }

protected:
	Vect2i mousePos_;
	bool leftButtonPressed_;
	bool rightButtonPressed_;

	Camera* camera_;
	cScene* scene_;
};

Runtime3D* CreateRuntime3D();

extern cInterfaceRenderDevice *renderDevice;

