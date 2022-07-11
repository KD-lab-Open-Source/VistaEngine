#ifndef _NORMALIZETGA_H_
#define _NORMALIZETGA_H_

#include "Render\src\Texture.h"

class TextureMiniDetail : public cTexture
{
public:
	TextureMiniDetail(const char* textureName, int tileSize);
	bool reload();

protected:
	int tileSize_;
	Color4c* in_data;

	int brightSizeX_,brightSizeY_;
	float* brightData_;

	float GetBrightTile(int x,int y){
		return brightData_[(x%brightSizeX_)+(y%brightSizeY_)*brightSizeX_];
	}

	float GetBrightInterpolate(int x,int y);
	bool normalize();
	bool buildDDS();
};


#endif _NORMALIZETGA_H_
