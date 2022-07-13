#ifndef _NORMALIZETGA_H_
#define _NORMALIZETGA_H_

class NormalizeTga
{
public:
	bool Normalize(struct sColor4c* data,int sizex,int sizey,int tile_size);
	bool SaveDDS(struct IDirect3DDevice9* pDevice,const char* filename);
protected:
	int sizex,sizey;
	int tile_size;
	struct sColor4c* in_data;

	int bright_sizex,bright_sizey;
	float* bright_data;
	float GetBrightTile(int x,int y)
	{
		return bright_data[(x%bright_sizex)+(y%bright_sizey)*bright_sizex];
	}

	float GetBrightInterpolate(int x,int y);
};


#endif _NORMALIZETGA_H_
