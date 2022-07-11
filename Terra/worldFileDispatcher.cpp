#include "stdafxTr.h"
#include "worldFileDispatcher.h"
#include "Serialization.h"
#include "tgai.h"

BitmapDispatcher bitmapDispatcher;


BitmapDispatcher::Bitmap::Bitmap()
{
	bitmap = 0;
	uid = -1;
	flag_AlphaChanelPresent=false;
}

bool BitmapDispatcher::Bitmap::checkSize(unsigned int size)
{
	switch(size){
	case 4:
	case 8:
	case 16:
	case 32:
	case 64:
	case 128:
	case 256:
	case 512:
	case 1024:
	case 2048:
		return true;
	default:
		return true;
	}
}

void BitmapDispatcher::Bitmap::load(const char* name, int _uid)
{
	uid = _uid;
	TGAHEAD tgahead;
	if(tgahead.loadHeader(name)){
		if( (tgahead.PixelDepth!=24 && tgahead.PixelDepth!=32)  || tgahead.ImageType != 2 
			|| !checkSize(tgahead.Width) || !checkSize(tgahead.Height)) {
			//AfxMessageBox("Не поддерживаемый тип TGA (необходим 24bit не компрессованный, с размерами, кратными степени 2)");
			xassert(0&&"Не поддерживаемый тип TGA (необходим 32/24bit не компрессованный, с размерами, кратными степени 2)");
			bitmap=0;
		}
		else {
			size.x = tgahead.Width;
			size.y = tgahead.Height;
			mask.x = size.x - 1;
			mask.y = size.y - 1;
			bitmap = new unsigned long[size.x*size.y];
			//tgahead.load2RGBL(tgahead.Width, tgahead.Height, bitmap);
			if(tgahead.PixelDepth==24){
				tgahead.load2RGBL(size.x, size.y, bitmap);
				flag_AlphaChanelPresent=false;
			}
			else if(tgahead.PixelDepth==32){
				tgahead.load2buf((unsigned char*)bitmap, size.x*size.y*sizeof(unsigned long));
				flag_AlphaChanelPresent=true;
			}
		}
	}
}

BitmapDispatcher::Bitmap::~Bitmap()
{
	if(bitmap)
		delete[] bitmap; 
}

int BitmapDispatcher::getUID(const char* name)
{
	BitmapInfoTable::iterator i;
	int maxID=0; //for check
	for(i=bitmapInfoTable.begin(); i!=bitmapInfoTable.end(); i++){
		if(i->first == name){
			return i->second; //i - bitmapInfoTable.begin();
		}
		maxID=max(i->second, maxID);
	}
	//check!
	if(maxID >= firstFreeUID){
		xassert(0&& "UID truble!");
		firstFreeUID=maxID+1;
	}
	bitmapInfoTable.push_back(make_pair(name, firstFreeUID++));
	return bitmapInfoTable.back().second; //names_.size() - 1;
}

const char* BitmapDispatcher::getFNameByUID(unsigned int _uid)
{
	BitmapInfoTable::iterator i;
	for(i=bitmapInfoTable.begin(); i!=bitmapInfoTable.end(); i++){
		if(i->second==_uid)
			return i->first.c_str();
	}
	xassert(0&&"BitmapDispatcher: file by id not found");
	return 0;
}

const BitmapDispatcher::Bitmap* BitmapDispatcher::getBitmap(int _uid)
{
	list<Bitmap>::iterator i;
	for(i=bitmaps_.begin(); i!=bitmaps_.end(); i++){
		if(i->uid == _uid){
			//if(i->bitmap==0){
			//	i->load(names_[_uid].c_str(), index);
			//	if(!i->bitmap) 
			//		return 0;
			//}
			return i->bitmap ? &*i : 0;

		}
	}
	bitmaps_.push_front(Bitmap());
	if(bitmaps_.size() > bitmapsCashMax)
		bitmaps_.pop_back();
	const char* fname=getFNameByUID(_uid);
	if(fname){
		bitmaps_.front().load(fname, _uid);
		return bitmaps_.front().bitmap ? &bitmaps_.front() : 0;
	}
	return 0;
}

const BitmapDispatcher::Bitmap* BitmapDispatcher::getBitmap(const char* fname)
{
	return getBitmap(getUID(fname));
}

void BitmapDispatcher::serialize(Archive& ar) 
{
	ar.serialize(firstFreeUID, "firstFreeUID", 0);
	if( (!ar.serialize(bitmapInfoTable, "bitmapInfoTable", 0)) && ar.isInput() ){
		bitmapInfoTable.clear();
		vector<string> names;
		vector<string>::iterator p;
		ar.serialize(names, "names", 0);
		for(p=names.begin(); p!=names.end(); p++){
			bitmapInfoTable.push_back(make_pair(p->c_str(), p-names.begin()+1));
		}
		firstFreeUID=names.size()+1;
	}
	//check
	int maxID=0;
	BitmapInfoTable::iterator i;
	for(i=bitmapInfoTable.begin(); i!=bitmapInfoTable.end(); i++){
		maxID=max(i->second, maxID);
	}
	if(maxID >= firstFreeUID){
		xassert(0&& "UID truble!");
		firstFreeUID=maxID+1;
	}
}
