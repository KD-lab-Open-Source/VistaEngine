#ifndef __WORLDFILEDISPATCHER_H__
#define __WORLDFILEDISPATCHER_H__

struct Bitmap32 {
	unsigned short sx,sy;
	unsigned long* pData;

	Bitmap32(){
		pData = 0;
	};
	void create(int _sx, int _sy){
		sx=_sx; sy=_sy;
		pData=new unsigned long[sx*sy];
	};
	void release(){
		if(pData) {
			delete [] pData; pData=0;
		}
	};
	~Bitmap32(){
		release();
	};
	inline unsigned long get(int x, int y){
		return pData[((y%sy)*sx + (x%sx))]; 
	}
};

class BitmapDispatcher
{
public:
	struct Bitmap {
		unsigned long* bitmap;
		Vect2i size;
		Vect2i mask;
		unsigned int uid;
		bool flag_AlphaChanelPresent;
	
		Bitmap();
		~Bitmap();
		void load(const char* name, int _uid);

		unsigned long operator()(const Vect2i& coord) const {
			return bitmap[(coord.x & mask.x) + (coord.y & mask.y)*size.x];
		}

		static bool checkSize(unsigned int size);
	};

	int getUID(const char* name);
	const Bitmap* getBitmap(int _uid);
	const Bitmap* getBitmap(const char* fname);
	const char* getFNameByUID(unsigned int _uid);

	BitmapDispatcher(){
		//firstFreeUID=1;
		//clearCache();
		release();
	}
	void serialize(Archive& ar);

	void clearCache(){
		bitmaps_.clear();
	}
	void release(){
		firstFreeUID=1;
		bitmapInfoTable.clear();
		clearCache();
	}
private:
	typedef vector<pair<string, unsigned int> > BitmapInfoTable;
	BitmapInfoTable bitmapInfoTable;
	unsigned int firstFreeUID;
	enum { bitmapsCashMax = 15 };
	list<Bitmap> bitmaps_;
};

extern BitmapDispatcher bitmapDispatcher;





#endif //__WORLDFILEDISPATCHER_H__
