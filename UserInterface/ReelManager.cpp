#include "StdAfx.h"
#include "ReelManager.h"
#include "RenderObjects.h"
#include "SoundApp.h"
#include "PlayOgg.h"
#include "GameShell.h"
#include "VistaRender\postEffects.h"
#include "Units\GlobalAttributes.h"
#include "bubles\blobs.h"
#include "bubles\cell.h"
#include "Render\Src\cCamera.h"
#include "Render\Src\TexLibrary.h"
#include "Render\D3D\D3DRender.h"
#include "Render\src\NParticle.h"
#include "Render\src\Scene.h"
#include "Render\src\VisGeneric.h"


float SPLASH_FADE_IN_TIME = 800; 
float SPLASH_FADE_OUT_TIME = 800;
float SPLASH_REEL_ABORT_DISABLED_TIME = 5600;

Vect2i mousePosition()
{
	if(gb_RenderDevice){
		POINT pt;
		GetCursorPos(&pt);
		if(ScreenToClient(gb_RenderDevice->GetWindowHandle(), &pt))
			return Vect2i(pt.x, pt.y);
	}

	return Vect2i(0, 0);
}

extern OggPlayer gb_Music;
//extern GameShell* gameShell;
//
ReelManager::ReelManager() {
	visible = false;
	pos.set(0, 0);
	size.set(0, 0);
	sizeType = FULL_SCREEN;
	bgTexture = NULL;
}
ReelManager::~ReelManager() {
	RELEASE(bgTexture);
}

void ReelManager::showModal(const char* binkFileName, const char* soundFileName, bool stopBGMusic, int alpha) {
#ifndef _DEMO_
	return;
	if (stopBGMusic) {
		gb_Music.stop();
//		bink->SetVolume(max(terMusicVolume, terSoundVolume));
//	} else {
//		bink->SetVolume(0);
	}

	string soundPath = soundFileName ? soundFileName : "";

	if (!soundPath.empty() && soundPath != "empty" && stopBGMusic) {
		int ret = gb_Music.play(soundFileName);
		xassert(ret);
	}

	gb_RenderDevice->Fill(0, 0, 0, 0);
	gb_RenderDevice->BeginScene();
	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();

	if (soundFileName && strlen(soundFileName) && stopBGMusic) {
		gb_Music.stop();
	}
	hide();
#endif
}

void ReelManager::showPictureModal(const char* pictureFileName, int stableTime) {

	cTexture* pictureTexture = gb_VisGeneric->CreateTexture( pictureFileName );

	int screenWidth = gb_RenderDevice->GetSizeX();
	int screenHeight = gb_RenderDevice->GetSizeY();
/*
	bgTexture = gb_VisGeneric->CreateTextureScreen();
	if (!bgTexture)	{
		xassertStr(0, "Cannot create background texture to play bink file");
		return;
	}
*/
	startTime = xclock();

	float maxTime = SPLASH_FADE_IN_TIME + stableTime + SPLASH_FADE_OUT_TIME;
	visible = true;
	MSG msg;
	while (isVisible()) {
		if ( PeekMessage(&msg, 0, 0, 0, PM_REMOVE) ) {
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		} else {
			double timeElapsed = xclock() - startTime;
			if (timeElapsed > SPLASH_REEL_ABORT_DISABLED_TIME) {
				gameShell->reelAbortEnabled = true;
			}
			if ( stableTime < 0 || timeElapsed < maxTime ) {
				gb_RenderDevice->Fill(0, 0, 0, 0);
				gb_RenderDevice->BeginScene();
/*
				if (bgTexture) {
					gb_RenderDevice->DrawSprite(0, 0, screenWidth, screenHeight, 0, 0,
												screenWidth / (float)bgTexture->GetWidth(),
												screenHeight / (float)bgTexture->GetHeight(),
												bgTexture);
				}
*/
				int alpha = 0;
				if (timeElapsed <= SPLASH_FADE_IN_TIME) {
					alpha = round(255.0f * timeElapsed / SPLASH_FADE_IN_TIME);
				} else if (stableTime < 0 || timeElapsed <= (SPLASH_FADE_IN_TIME + stableTime)) {
					alpha = 255;
				} else {
					alpha = round(255.0f * (maxTime - timeElapsed) / SPLASH_FADE_OUT_TIME);
				}
				float dy = screenHeight / (float)screenWidth;
				gb_RenderDevice->DrawSprite(0, 0, screenWidth, screenHeight, 0, 0,
											1, dy, pictureTexture, Color4c(255, 255, 255, alpha));

				gb_RenderDevice->EndScene();
				gb_RenderDevice->Flush();
			} else {
				break;
			}
		}
	}
	gb_RenderDevice->Fill(0, 0, 0, 0);
	gb_RenderDevice->BeginScene();
	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();

	RELEASE(pictureTexture);
//	RELEASE(bgTexture);

	hide();
}
class Skate
{
public:
	Skate(cScene* scene, Camera* camera)
	{
		scene_ = scene;
		camera_ = camera;
		id_swim = -1;
		id_activated = -1;
		id_drink = -1;
		id_current = -1;
		id_stand = -1;
		movePhase = phase = phaseSelect = 0;
		screenSize.x = gb_RenderDevice->GetSizeX();
		screenSize.y = gb_RenderDevice->GetSizeY();
		scale = 10;
		model = NULL;
		model_kdv = NULL;
		chain_len = 0.5f;
		select_chain_len = 1.f;
		isDone_ = false;
		startMouseTimer_ = false;
		mouseTimer_ = 0;
		oldMousePos_.set(0,0);
		intersect_ = false;
		loopSoundSwim_ = loopSoundStop_ = loopSoundRise_ = false;
		startSoundSwim_ = false;
		startSoundStop_ = false;
		startSoundRise_ = false;

	}
	~Skate()
	{
		fishSwim_.Stop(true);
		fishStop_.Stop(true);
		fishRise_.Stop(true);
	}
	bool isDone() {return isDone_;}
	void setFadeParameters(float fishAlpha, float logoAlpha, float fadeTime)
	{
		fishAlpha_ = 1-fishAlpha;
		logoAlpha_ = logoAlpha;
		fadeTime_ = fadeTime;
		select_chain_len = 1.f/fadeTime_;
	}
	bool Init(const char* modelName, const char* logoName,const SoundAttribute* swimAttr,const SoundAttribute* stopAttr,const SoundAttribute* riseAttr)
	{
		if(!modelName || !modelName[0])
			return false;
		model = scene_->CreateObject3dx(modelName, NULL, GlobalAttributes::instance().enableAnimationInterpolation);
		if(!model)
			return false;
		if(logoName && logoName[0])
			model_kdv = scene_->CreateObject3dx(logoName, NULL, GlobalAttributes::instance().enableAnimationInterpolation);

		id_swim = model->GetChainIndex("swim");
		id_activated = model->GetChainIndex("fish_includ");
		id_drink = model->GetChainIndex("drink");
		id_stand = model->GetChainIndex("KDV_stand");
		id_current = id_swim;
		modelPos = MatXf(Mat3f::ID,Vect3f::ID);
		model->SetAnimationGroupChain(0,id_current);
		path.GetOrCreateKey(0)->Val() = Vect3f(-400,0,0);
		path.GetOrCreateKey(1)->Val() = Vect3f(0,0,0);
		modelPos.trans() = path.Get(0);
		model->SetPosition(modelPos);
		model->SetAnimationGroupChain(1,"KDV_include");
		if(model_kdv)
		{
			model_kdv->SetPosition(modelPos);
			model_kdv->SetChain("KDV_include");
			model_kdv->SetPhase(1);
		}
		StaticAnimationChain* chain = model->GetChain(model->GetChainIndex("KDV_include"));
		xassert(chain);
		select_chain_len = 1.0f/chain->time;
		chain = model->GetChain(id_swim);
		xassert(chain);
		chain_len = 1.0f/chain->time;
		fishSwim_.Init(swimAttr);
		fishStop_.Init(stopAttr);
		fishRise_.Init(riseAttr);
		if(swimAttr) loopSoundSwim_ = swimAttr->cycled();
		if(stopAttr) loopSoundStop_ = stopAttr->cycled();
		if(riseAttr) loopSoundRise_ = riseAttr->cycled();

		return true;

	}
	void Animate(float dt)
	{
		if(!model)
			return;

		phase += dt*chain_len;
		if(phase>1)
			if(movePhase==1&&id_current==id_drink)
			{
				phase = 1;
				isDone_=true;
			}
			else
				phase -= 1;

		Vect3f pos = path.Get(movePhase);
		modelPos.trans() = pos;
		model->SetPosition(modelPos);
		if(model_kdv)
			model_kdv->SetPosition(modelPos);
		Vect2i mouse_pos = mousePosition();
		Vect2f pos_in(mouse_pos.x/(float)screenSize.x-0.5f,
			mouse_pos.y/(float)screenSize.y-0.5f);

		Vect3f pos0,dir,pos1;
		camera_->GetWorldRay(pos_in,pos0,dir);

		pos1 = pos0+dir*9999.9f;

		if(model->IntersectBound(pos0,pos1)&&movePhase!=1)
		{
			intersect_ = true;
			if(oldMousePos_==mouse_pos)
			{
				if(!startMouseTimer_)
				{
					startMouseTimer_ = true;
					mouseTimer_ = 0;
				}
			}else
				startMouseTimer_ = false;

		}else
		{
			startMouseTimer_ = false;
			intersect_ = false;
		}
		if(startMouseTimer_)
		{
			mouseTimer_ += dt*0.2f;
		}
		oldMousePos_ = mouse_pos;
		movePhase += dt*0.2f;
		if(movePhase>1)
			movePhase = 1;
		if(mouseTimer_>1 || !intersect_)
		{
			phaseSelect -= dt*select_chain_len;
			if(phaseSelect<0)
				phaseSelect = 0;
		}else
		{
			phaseSelect += dt*select_chain_len;
			if(phaseSelect>1)
				phaseSelect = 1;
		}
		if(movePhase==1)
		{
			if(model_kdv)
			{
				RELEASE(model_kdv);
			}
			if(id_current != id_drink&&!isDone_)
			{
				model->SetAnimationGroupChain(0,id_drink);
				model->SetAnimationGroupChain(1,id_drink);
				id_current = id_drink;
				phase = 0;
				StaticAnimationChain* chain = model->GetChain(id_drink);
				xassert(chain);
				chain_len = 1.0f/15.0f;
				fishRise_.Play();
			}
			if(isDone_&&id_stand != -1&&id_current != id_stand)
			{
				model->SetAnimationGroupChain(0,id_stand);
				model->SetAnimationGroupChain(1,id_stand);
				id_current = id_stand;
				phase = 0;
				StaticAnimationChain* chain = model->GetChain(id_stand);
				xassert(chain);
				chain_len = 1.0f/chain->time;
				fishRise_.Stop();
				startSoundRise_ = false;
			}
		}

		if(movePhase != 1)
		{
			if(!intersect_||mouseTimer_>1)
			{
				if(!startSoundSwim_)
				{
					fishSwim_.Play(loopSoundSwim_);
					startSoundSwim_ = true;
				}
				fishStop_.Stop();
				startSoundStop_ = false;
			}else
			{
				if(!startSoundStop_)
				{
					startSoundStop_ = true;
					fishStop_.Play(loopSoundStop_);
				}
				fishSwim_.Stop();
				startSoundSwim_ = false;
			}

		}else
		if(id_current==id_drink)
		{
			if(!startSoundRise_)
			{
				startSoundRise_ = true;
				fishRise_.Play(loopSoundRise_);
			}
			startSoundStop_ = startSoundSwim_ = false;
			fishStop_.Stop();
			fishSwim_.Stop();
		}

		model->SetAnimationGroupPhase(0,phase);
		model->SetOpacity(1-fishAlpha_*phaseSelect);
		if(model_kdv)
			model_kdv->SetOpacity(phaseSelect*logoAlpha_);
		if(movePhase==1)
			model->SetAnimationGroupPhase(1,phase);
	}
	void Release()
	{
		RELEASE(model);
		RELEASE(model_kdv);
	}
	cObject3dx* getModel() {return model;}

	bool isDrink() { return id_current == id_drink || id_current == id_stand; }
	float getPhase() { return (id_current==id_drink)?phase:1; }

private:
	cObject3dx* model;
	cObject3dx* model_kdv;
	int id_swim;
	int id_activated;
	int id_drink;
	int id_current;
	int id_stand;
	cScene* scene_;
	Camera* camera_;
	float movePhase;
	float phase;
	float phaseSelect;
	MatXf modelPos;
	KeysPos path;
	Vect2i screenSize;
	float scale;
	float chain_len;
	float select_chain_len;
	bool isDone_;
	bool startMouseTimer_;
	float mouseTimer_;
	Vect2i oldMousePos_;
	bool intersect_;
	SNDSound fishSwim_;
	SNDSound fishStop_;
	SNDSound fishRise_;
	bool loopSoundSwim_;
	bool loopSoundStop_;
	bool loopSoundRise_;
	bool startSoundSwim_;
	bool startSoundStop_;
	bool startSoundRise_;
	float fishAlpha_;
	float logoAlpha_;
	float fadeTime_;
};

void ReelManager::showLogoModal(LogoAttributes& logoAttributes, const cBlobsSetting& blobsSetting, int stableTime,SoundLogoAttributes& soundAttributes)
{
	int oldTextureDetail = gb_VisGeneric->GetTextureDetailLevel();
	gb_VisGeneric->SetTextureDetailLevel(0);
	int screenWidth = gb_RenderDevice->GetSizeX();
	int screenHeight = gb_RenderDevice->GetSizeY();
	int endPhase = 0;
	int endPhasePrev = 0;

	float phaseFade = 0.f;
	float fadeInTime = 1000.f/SPLASH_FADE_IN_TIME;
	float fadeOutTime = 1000.f/SPLASH_FADE_OUT_TIME;
	float phaseStay = 0;

	cScene* scene = gb_VisGeneric->CreateScene();
	xassert(scene);
	scene->SetSunDirection(Vect3f(1,0,0));
	scene->SetSunColor(Color4f(1,1,1,1),Color4f(1,1,1,1),Color4f(1,1,1,1));
	Camera* camera = scene->CreateCamera();
	xassert(camera);
	camera->setAttribute(ATTRCAMERA_PERSPECTIVE|ATTRCAMERA_CLEARZBUFFER);
	cObject3dx* dno = NULL;	
	if(!logoAttributes.groundName.empty())
		dno = scene->CreateObject3dx(logoAttributes.groundName.c_str(), NULL);
	int id_stand = -1;
	int id_rise = -1;
	int id_current = -1;
	float chain_len = 1;
	SNDSound bkgSound;
	SNDSound waterOut_;
	bool waterOutStartPlay = false;
	SNDSound mouseMove_;
	bkgSound.Init(soundAttributes.backGround_);
	waterOut_.Init(soundAttributes.waterOut_);
	mouseMove_.Init(soundAttributes.mouseMove_);
	if(dno)
	{
		id_stand = dno->GetChainIndex("stand");
		id_rise = dno->GetChainIndex("rise");
		id_current = id_rise;
		dno->SetChain(id_current);
		StaticAnimationChain* chain = dno->GetChain(id_stand);
		xassert(chain);
		chain_len = 1.0f/chain->time;
	}
	//dno->SetPosition(MatXf(Mat3f::ID,offset));
	float dnoMovePhase = 0;
	Skate skate(scene,camera);
	skate.Init(logoAttributes.fishName.c_str(),logoAttributes.logoName.c_str(),
		soundAttributes.fishSwim_,
		soundAttributes.fishStop_,
		soundAttributes.fishRise_);
	skate.setFadeParameters(logoAttributes.fishAlpha,logoAttributes.logoAlpha,logoAttributes.fadeTime);
	if(skate.getModel())
	{
		float focus = 1;
		if(skate.getModel()->GetFov()!=0)
		{
			float fov = skate.getModel()->GetFov();
			focus = 1.0f/(2*tan(fov*0.5f));
		}else
			focus = 1.0f/sqrt(2.f);

		int ix_camera = skate.getModel()->FindNode("Camera01");
		int ix_light = skate.getModel()->FindNode("FDirect01");
		if (ix_camera!=-1)
		{
			MatXf cam(skate.getModel()->GetNodePosition(ix_camera));

			Mat3f rot(Vect3f(1,0,0), M_PI);
			cam.rot()=cam.rot()*rot;
			cam.Invert();

			camera->SetPosition(cam);
		}else
		{
			Mat3f matr(0,1,0,
					   0,0,1,
					   1,0,0);
			camera->SetPosition(MatXf(matr,Vect3f(0,0,700)));
		}
		if (ix_light!=-1)
		{
			Se3f me = skate.getModel()->GetNodePosition(ix_light);
			MatXf m(me);
			Mat3f rot(Vect3f(1,0,0), M_PI);
			m.rot()=rot*m.rot();
			m.Invert();
			scene->SetSunDirection(m.rot().zrow());
		}
		camera->SetFrustumPositionAutoCenter(sRectangle4f(0,0,1,1),focus);
	}else
	{
		Mat3f matr(0,1,0,
			0,0,1,
			1,0,0);
		camera->SetPosition(MatXf(matr,Vect3f(0,0,700)));
		camera->SetFrustumPositionAutoCenter(sRectangle4f(0,0,1,1),1.0f/sqrt(2.f));
	}
	
	cTexture* backTexture = GetTexLibrary()->CreateRenderTexture(screenWidth,screenHeight,TEXTURE_RENDER32);
	float logoPhase=0;

	// Init

	cBlobs blobs;
	blobs.Init(screenWidth, screenHeight);
	//blobs.SetTexture(backTexture);
	blobs.CreateBlobsTexture(64);

	RandomGenerator rand_;

	vector<kdCell> cellList_;
	for(int j = screenHeight + 20; j > 0; j-=20)
		for(int i = 0; i < screenWidth + 20; i+=20) {
			Vect2f pos(i,j);
			kdCell newCell(pos + Vect2f(0,rand_.fabsRnd(20,40)), pos, 1);
			cellList_.push_back(newCell);
		}


	float clockPrev = startTime = xclock();
	float maxTime = SPLASH_FADE_IN_TIME + stableTime + SPLASH_FADE_OUT_TIME;
	visible = true;
	MSG msg;
	bool isWork = false;
	float dnoPhase = 0;
	bool bkgLoop = false;
	bool waterLoop = false;
	bool mouseLoop = false;
	if((const SoundAttribute*)soundAttributes.backGround_)
		bkgLoop = ((const SoundAttribute*)soundAttributes.backGround_)->cycled();
	if((const SoundAttribute*)soundAttributes.waterOut_)
		waterLoop = ((const SoundAttribute*)soundAttributes.waterOut_)->cycled();
	if((const SoundAttribute*)soundAttributes.mouseMove_)
		mouseLoop = ((const SoundAttribute*)soundAttributes.mouseMove_)->cycled();
	bkgSound.Play(bkgLoop);
	sndSystem.StartFade(true);
	Vect2i oldMousePos = mousePosition();
	while (isVisible()) {
			sndSystem.Update();
			double curTime = xclock();
			if(curTime < clockPrev)
				curTime = clockPrev;
			double timeElapsed = curTime - startTime;
			float dt = float(curTime - clockPrev)/65.0f;
			if(dt>0.1f)
				dt = 0.1f;

			float delta = float(curTime - clockPrev)*0.001f;
			if(delta>0.1f)
				delta = 0.1f;
			clockPrev = curTime;
			dnoPhase += delta * chain_len;
			if(dnoPhase >=1)
				dnoPhase -= 1;
			if (timeElapsed > SPLASH_REEL_ABORT_DISABLED_TIME) {
				gameShell->reelAbortEnabled = true;
			}
			if(!isWork) {
				int i = 0;
				for(i=0;i<cellList_.size(); i++){
					if(!cellList_[i].isWork()){
						for(int w=0; w<3; w++) 
							if((i + w) < cellList_.size())
								cellList_[i + w].start();
						break;
					}
				}

				if(i == cellList_.size()) {
					isWork = true;
					random_shuffle(cellList_.begin(), cellList_.end());
				}
				dnoMovePhase = float(i)/(cellList_.size());
			}
			if(isWork)
			{
				skate.Animate(delta);
				if(dno)
				{
					if(id_current != id_stand)
					{
						id_current = id_stand;
						dno->SetChain(id_current);
					}
					dno->SetPhase(dnoPhase);
				}
			}else
			{
				if(dno)
					dno->SetPhase(dnoMovePhase);
			}
			gb_RenderDevice->Fill(0, 0, 0, 0);
			gb_RenderDevice->BeginScene();
			scene->Draw(camera);
			IDirect3DSurface9 *pDestSurface=NULL;
			RDCALL(backTexture->GetDDSurface(0)->GetSurfaceLevel(0,&pDestSurface));
			RDCALL(gb_RenderDevice3D->D3DDevice_->StretchRect(
				gb_RenderDevice3D->backBuffer_,NULL,pDestSurface,NULL,D3DTEXF_LINEAR));
			RELEASE(pDestSurface);

			blobs.BeginDraw();

			Vect2i mouse_pos = mousePosition();
			if(mouse_pos == oldMousePos)
			{
				mouseMove_.Stop();
			}else
			{
				if(!mouseMove_.IsPlayed())
					mouseMove_.Play(mouseLoop);
			}
			oldMousePos = mouse_pos;
			for(int i=0;i<cellList_.size();i++){
				if(cellList_[i].isWork()) {
					cellList_[i].setAnchor(Vect2f(mouse_pos.x,mouse_pos.y), dt);
					cellList_[i].quant(dt);
					blobs.Draw(cellList_[i].position().xi(),cellList_[i].position().yi(), cellList_[i].colorIndex(), cellList_[i].colorPhase);
				}
			}
			
			blobs.EndDraw(blobsSetting);
			
			float workOut = false;
			if(skate.isDrink()) {
				for(int onCell = endPhasePrev; onCell <= endPhase; onCell++)
					if(onCell < cellList_.size()) {
						Vect2f curPos = cellList_[onCell].position();
						Vect2f endPos(screenWidth / 2, screenHeight / 2);
						Vect2f startPos = (curPos - endPos);
						cellList_[onCell].endPosition() = Vect2f(screenWidth / 2, screenHeight / 2);
						cellList_[onCell].setVelocity(Vect2f(startPos.y, - startPos.x) * 0.3f);
					}

				endPhasePrev = endPhase;
				endPhase = skate.getPhase() * cellList_.size();
				
				workOut = true;
				for(int i=0; i<cellList_.size(); i++) {
					if(cellList_[i].position().eq(Vect2f(screenWidth / 2, screenHeight / 2), 60.0f))
						cellList_[i].stop();

					if(cellList_[i].isWork())
						workOut = false;

					if(timeElapsed > 60000.0)
						workOut = true;
				}

				if(!waterOutStartPlay)
				{
					waterOut_.Play(waterLoop);
					waterOutStartPlay = true;
				}
			}
/*				for(int i=0; i<cellList_.size(); i++) {
					Vect2f curPos = cellList_[i].position();
					Vect2f endPos(screenWidth / 2, screenHeight / 2);
					Vect2f startPos = (curPos - endPos);
					float len = startPos.norm() - rand_.fabsRnd(1,10) * dt;
					if(len < 20)
						cellList_[i].stop();
					else
						startPos.normalize(len);

					cellList_[i].endPosition() = cellList_[i].position() = endPos + startPos;
					startPos.normalize(rand_.fabsRnd(20.0f, 40.0f));
					cellList_[i].setVelocity(Vect2f(startPos.y, - startPos.x));
				}*/

			//float phase = 1.0f;
			//if(timeElapsed < SPLASH_FADE_IN_TIME)
			//	phase = float(timeElapsed / SPLASH_FADE_IN_TIME);
			//else if(timeElapsed > maxTime - SPLASH_FADE_OUT_TIME)
			//	phase = float((maxTime - timeElapsed) / SPLASH_FADE_OUT_TIME);

			if(fadeInTime>FLT_EPS)
			{
				phaseFade += delta*fadeInTime;
				if(phaseFade > 1)
				{
					phaseFade = 1;
					fadeInTime = 0;
				}
			}else
			if(fadeOutTime>FLT_EPS && workOut)
			{
				phaseFade -= delta*fadeOutTime;
				if(phaseFade < 0)
				{
					phaseFade = 0;
					fadeOutTime = 0;
					hide();
				}
			}

			if(gb_RenderDevice3D->IsPS20())
				blobs.DrawBlobsShader(0, 0, phaseFade, backTexture, blobsSetting);
			/**/
/*
			char s[512];
			char* p = s;
			*p = char(0);
			static FPS fps;
			fps.quant();
			float fpsmin,fpsmax;
			fps.GetFPSminmax(fpsmin,fpsmax);
			p+=sprintf(p,"FPS=% 3.1f min=% 3.1f max=% 3.1f\n",fps.GetFPS(),fpsmin,fpsmax);
			gb_RenderDevice->OutText(0, 20, s, Color4f(1, 1, 1, 1));
*/
			gb_RenderDevice->EndScene();
			gb_RenderDevice->Flush();

			if ( PeekMessage(&msg, 0, 0, 0, PM_REMOVE) ) {
				TranslateMessage( &msg );
				DispatchMessage( &msg );
		}

//		}
	}
	bkgSound.Stop(true);
	waterOut_.Stop(true);
	mouseMove_.Stop(true);
	sndSystem.StartFade(false);
	gb_RenderDevice->Fill(0, 0, 0, 0);
	gb_RenderDevice->BeginScene();
	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();
	//RELEASE(logoTexture);
	//RELEASE(backg);
	RELEASE(backTexture);
	RELEASE(dno);
	skate.Release();
	RELEASE(camera);
	RELEASE(scene);
	gb_VisGeneric->SetTextureDetailLevel(oldTextureDetail);
}
