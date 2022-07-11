#include "stdafx.h"
#include "Runtime3D.h"
#include "..\Render\src\FileImage.h"
#include "..\UserInterface\bubles\blobs.h"
#include "..\UserInterface\bubles\cell.h"
#include <algorithm>

class Demo3D:public Runtime3D
{
public:
	Demo3D();
	~Demo3D();
	virtual void Init();
	virtual void Quant();
protected:
	cFont* pFont;
	cBlobs blobs;

	Vect2f poses[500];
	Vect2f factors[500];
	float angle;

	vector<kdCell> cellList;

	vector<kdCell> cellList_;

};

Runtime3D* CreateRuntime3D()
{
	return new Demo3D;
}

Demo3D::Demo3D()
{
	pFont=NULL;
	angle=0;
}

Demo3D::~Demo3D()
{
	RELEASE(pFont);
}

void Demo3D::Init()
{
	pFont=gb_VisGeneric->CreateFont("Scripts\\Resource\\fonts\\Arial.font",20);
	terRenderDevice->SetDefaultFont(pFont);

	blobs.Init(terRenderDevice->GetSizeX(),terRenderDevice->GetSizeY());
	blobs.SetTexture("Scripts\\Resource\\Textures\\Twirl.tga");
	blobs.CreateBlobsTexture(32);

	for(int i=0; i < 500; i++) {
		poses[i].x = rand()%600 + 100;
		poses[i].y = rand()%600 + 100;
		factors[i].x = xm_random_generator.frand()*7.0f;
		factors[i].y = xm_random_generator.frand()*7.0f;
	}
/*
	// Расположение блобов.
	unsigned char* buf;
	int sizeX;
	int sizeY;
	int bpp;
	LoadTGA("test1.tga", sizeX, sizeY, buf, bpp);

	for(int i=0;i<60;i++)
		for(int j=0;j<60;j++){
			if(buf[(i* 60 + j)* 3 + 1 ] > 128) {
				Vect2f startPose(xm_random_generator.fabsRnd(250,350), xm_random_generator.fabsRnd(250,350));
//				kdCell newCell(Vect2f(j,i)*10 + startPose, Vect2f(j,i)*10 + Vect2f(300,300), 0);
				kdCell newCell(Vect2f(j,i)*9 + Vect2f(300,293), Vect2f(j,i)*9 + Vect2f(300,300), 0);
				cellList_.push_back(newCell);
			}
			if(buf[(i* 60 + j)* 3 + 2 ] > 128) {
				Vect2f startPose(xm_random_generator.fabsRnd(250,350), xm_random_generator.fabsRnd(250,350));
//				kdCell newCell(Vect2f(j,i)*10 + startPose, Vect2f(j,i)*10 + Vect2f(300,300), 1);
				kdCell newCell(Vect2f(j,i)*9 + Vect2f(300,293),Vect2f(j,i)*9 + Vect2f(300,300), 1);
				cellList_.push_back(newCell);
			}
		}

//	cellList[0].start();*/

//	for(int i = 0; i < gb_RenderDevice->GetSizeX(); i+=10)
//		for(int j = 0; j < gb_RenderDevice->GetSizeY(); j+=10) {
	for(int j = 0; j < gb_RenderDevice->GetSizeY() + 10; j+=10)
		for(int i = 0; i < gb_RenderDevice->GetSizeX() + 10; i+=10){
			Vect2f pos(i, gb_RenderDevice->GetSizeY() - j);
			kdCell newCell(pos + Vect2f(0, xm_random_generator.fabsRnd(5, 10)), pos, 1);
			cellList_.push_back(newCell);
		}

//	random_shuffle(cellList_.begin(), cellList_.end());
//	cellList_[0].start();
}

float time = 0;
float newTime = 0;
bool isWork = true;

void Demo3D::Quant()
{
	float dt = 0.2f;

	__super::Quant();
	terRenderDevice->Fill(128,160,135,0);
	terRenderDevice->BeginScene();
	blobs.BeginDraw();

	int sizeX = (gb_RenderDevice->GetSizeX() + 10) / 10;
	int sizeY = (gb_RenderDevice->GetSizeY() + 10) / 10;
/*
	if(!isWork) {
		int i = 0;
		int j = 0;
		for(j = 0; j < sizeY; j++){
			if(!cellList_[j * sizeX + i].isWork()){
				for(i = 0; i < sizeX; i++){
					cellList_[j * sizeX + i].start();
				}
				break;
			}
		}
		if(j == sizeY)
			isWork = true;
	}

		for(i=0;i<cellList_.size();i++){
			if(!cellList_[i].isWork()){
				int q = xm_random_generator.fabsRnd(1, 1);
				for(int w=0; w<q; w++)
					cellList_[i + w].start();
				break;
			}
		}

		if(i == cellList_.size())
			isWork = true;

	}
*/

	for(int i=0;i<cellList_.size();i++){
		if(cellList_[i].isWork()) {
			cellList_[i].setAnchor(Vect2f(mouse_pos.x,mouse_pos.y), dt);
			cellList_[i].quant(dt);
			blobs.Draw(cellList_[i].position().xi(),cellList_[i].position().yi(), cellList_[i].colorIndex());
		}
	}
	
	blobs.EndDraw();
	blobs.DrawBlobsShader(0,0,1.0f);

	__super::DrawFps(256,0);
	terRenderDevice->EndScene();
	terRenderDevice->Flush();
}
