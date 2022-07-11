
extern int UNDO_REDO_BUFFER_SIZE; //Текущий размер буфера в SurMap пикселах
const static int SIZE_SUPA_CELL=3;

struct sPreChangedArea {
	//short x, y;
	//short sx, sy;
	vector<sRect> simpleChAreaList;
	unsigned int idxPMO;
	unsigned short* vxaa;
	//unsigned char* sura;
	unsigned char* supa;
	int sizeArea;
	sPreChangedArea(void){
		vxaa=0; //sura=0;
		supa=0;
		sizeArea=0;
	}
	sPreChangedArea(const sPreChangedArea& donor){
		simpleChAreaList=donor.simpleChAreaList;
		idxPMO=donor.idxPMO;
		sizeArea=donor.sizeArea;
		vxaa=0; supa=0;
		if(sizeArea){
			if(donor.vxaa){
				vxaa=new unsigned short[sizeArea];
				memcpy(vxaa, donor.vxaa, sizeArea*sizeof(vxaa[0]));
			}
			if(donor.supa){
				supa=new unsigned char[sizeArea*SIZE_SUPA_CELL];
				memcpy(supa, donor.supa, sizeArea*SIZE_SUPA_CELL);
			}
		}
		else {
			vxaa=0; supa=0;
		}
	}
	void prepare4Operation(sRect& _chArea, unsigned int _idxPMO, bool saveVx, bool saveRGB){
		xassert(simpleChAreaList.empty()); //Должен быть пустым !
		simpleChAreaList.push_back(_chArea);
		sizeArea=_chArea.sx*_chArea.sy;
		UNDO_REDO_BUFFER_SIZE+=sizeArea;
		xassert(vxaa==0);
		xassert(supa==0);
		//vxaa=new unsigned short[sizeArea];
		//sura=new unsigned char[sizeArea];
		idxPMO=_idxPMO;
		if(saveVx)
			vxaa=new unsigned short[sizeArea];
		if(saveRGB)
			supa=new unsigned char[sizeArea*SIZE_SUPA_CELL];
	};
	void prepare4Operation(list<sRect>& _chAreaList, unsigned int _idxPMO, bool saveVx, bool saveRGB){
		xassert(simpleChAreaList.empty()); //Должен быть пустым !
		simpleChAreaList.insert(simpleChAreaList.begin(), _chAreaList.begin(), _chAreaList.end());//.insert(simpleChAreaList.begin, _chAreaList);
		sizeArea=0;
		vector<sRect>::iterator p;
		for(p=simpleChAreaList.begin(); p!=simpleChAreaList.end(); p++){
			sizeArea+=p->sx*p->sy;
		}
		UNDO_REDO_BUFFER_SIZE+=sizeArea;
		xassert(vxaa==0);
		xassert(supa==0);
		//vxaa=new unsigned short[sizeArea];
		//sura=new unsigned char[sizeArea];
		idxPMO=_idxPMO;
		if(saveVx)
			vxaa=new unsigned short[sizeArea];
		if(saveRGB)
			supa=new unsigned char[sizeArea*SIZE_SUPA_CELL];
	};
	bool getBuf(int idx, sRect*& pRect, unsigned short*& pVxA, unsigned char*& pSup){
		if(idx<simpleChAreaList.size()){
			int i;
			int off=0;
			for(i=0; i<idx; i++){
				off+=simpleChAreaList[i].sx*simpleChAreaList[i].sy;
			}
			pRect=&simpleChAreaList[idx];
			if(vxaa) pVxA=&vxaa[off];
			else pVxA=0;
			if(supa) pSup=&supa[off*SIZE_SUPA_CELL];
			else pSup=0;
			return true;
		}
		else
			return false;
	}
	~sPreChangedArea(void){
		UNDO_REDO_BUFFER_SIZE-=sizeArea;
		if(supa) delete [] supa;
		if(vxaa) delete [] vxaa;
	};
};
