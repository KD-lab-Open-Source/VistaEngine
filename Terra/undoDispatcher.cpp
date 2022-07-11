#include "stdafxTr.h"


static const int MAX_SIZE_UNDO_REDO_BUFFER=64*1024*1024;//4096*4096;
int UNDO_REDO_BUFFER_SIZE=0;

template <class A, class B> void deleteFromContainer(A& container, B begIt, B endIt)
{
	//A::iterator p;
	//for( p=begIt; p!=endIt; ++p)
	//	delete *p;
	container.erase(begIt, endIt);
}

void vrtMap::clearContainerPMO()
{
	deleteFromContainer(containerPMO, containerPMO.begin(), containerPMO.end());
}

void vrtMap::UndoDispatcher_PutPreChangedArea(sRect& _chArea, bool saveVx, bool saveRGB, sBasePMOperation* pPMO)//xL, int yT, int xR, int yB
{
	//xassert(SupBuf);
	if(!flag_record_operation) return;
	if(pPMO!=0){
		//if(getCurNewPMO()!=containerPMO.end())
		//	containerPMO.erase(getCurNewPMO(), containerPMO_Old.end()); //Удаление не нужных элементов
		deleteFromContainer(containerPMO, getCurNewPMO(), containerPMO.end());

		containerPMO.push_back(pPMO);
		setCurNewPMO(containerPMO.end());
	}
	//xL=XCYCL(xL);
	//xR=XCYCL(xR);
	//yT=YCYCL(yT);
	//yB=YCYCL(yB);
	//int sx, sy;
	//if(xL==xR){ sx=H_SIZE; xL=0;}
	//else sx=XCYCL(xR-xL);
	//if(yT==yB){ sy=V_SIZE; yT=0;}
	//else sy=XCYCL(yB-yT);
	//if(sx > H_SIZE) sx=H_SIZE;
	//if(sy > V_SIZE) sy=V_SIZE;
	//int size=sx*sy;
	int size=_chArea.sx*_chArea.sy;

	if(curPreCA!=preCAs.end()) 
		preCAs.erase(curPreCA, preCAs.end()); //Удаление не нужных элементов
	//Проверка на переполнение буфера и удаление 1-х элементов
	while((UNDO_REDO_BUFFER_SIZE+size) > MAX_SIZE_UNDO_REDO_BUFFER){
		//if(preCAs.begin()==preCAs.end())ErrH.Abort("Список пустой а UNDO_REDO_BUFFER_SIZE==", XERR_USER, UNDO_REDO_BUFFER_SIZE);
		if(preCAs.empty()){
			xassert("Very larg changed area!");
			break;
		}
		preCAs.erase(preCAs.begin());
	}
	preCAs.push_back(sPreChangedArea());
	curPreCA=preCAs.end();
	curPreCA--;//Теперь указывет на последний элемент
	curPreCA->prepare4Operation(_chArea, containerPMO.size(), saveVx, saveRGB);
	xassert(_chArea.sx <=H_SIZE && _chArea.sy<=V_SIZE );
	unsigned short* pVxA;
	unsigned char* pSup;
	sRect* pCurRect;
	curPreCA->getBuf(0, pCurRect, pVxA, pSup);
	int i, j, cnt=0;
	for(i=0; i<pCurRect->sy; i++){
		for(j=0; j<pCurRect->sx; j++){
			int off=offsetBuf(XCYCL(pCurRect->x+j), YCYCL(pCurRect->y+i));
			if(pVxA)
				pVxA[cnt]=VxABuf[off];
			if(pSup){
				unsigned long c=SupBuf[off];
				pSup[cnt*SIZE_SUPA_CELL+0]=c;
				pSup[cnt*SIZE_SUPA_CELL+1]=c>>8;
				pSup[cnt*SIZE_SUPA_CELL+2]=c>>16;
			}
			cnt++;
		}
	}
	//curPreCA->x=xL;
	//curPreCA->y=yT;
	curPreCA++;// теперь указывает на окончание
}

void vrtMap::UndoDispatcher_PutPreChangedArea(list<sRect>& _chAreaList, bool saveVx, bool saveRGB, sBasePMOperation* pPMO)
{
	if(!flag_record_operation) return;
	if(pPMO!=0){
		//if(getCurNewPMO()!=containerPMO_Old.end()) 
		//	containerPMO_Old.erase(getCurNewPMO(), containerPMO_Old.end()); //Удаление не нужных элементов
		deleteFromContainer(containerPMO, getCurNewPMO(), containerPMO.end());

		containerPMO.push_back(pPMO);
		setCurNewPMO(containerPMO.end());
	}
	int size=0;
	list<sRect>::iterator p;
	for(p=_chAreaList.begin(); p!=_chAreaList.end(); p++){
		size+=p->sx*p->sy;
	}

	if(curPreCA!=preCAs.end()) 
		preCAs.erase(curPreCA, preCAs.end()); //Удаление не нужных элементов
	//Проверка на переполнение буфера и удаление 1-х элементов
	while((UNDO_REDO_BUFFER_SIZE+size) > MAX_SIZE_UNDO_REDO_BUFFER){
		//if(preCAs.begin()==preCAs.end())ErrH.Abort("Список пустой а UNDO_REDO_BUFFER_SIZE==", XERR_USER, UNDO_REDO_BUFFER_SIZE);
		if(preCAs.empty()){
			xassert("Very larg changed area!");
			break;
		}
		preCAs.erase(preCAs.begin());
	}
	preCAs.push_back(sPreChangedArea());
	curPreCA=preCAs.end();
	curPreCA--;//Теперь указывет на последний элемент
	curPreCA->prepare4Operation(_chAreaList, containerPMO.size(), saveVx, saveRGB);
	unsigned short* pVxA;
	unsigned char* pSup;
	sRect* pCurRect;
	int cf;
	for(p=_chAreaList.begin(), cf=0 ; p!=_chAreaList.end(); p++, cf++){
		xassert(p->sx <=H_SIZE && p->sy<=V_SIZE );
		bool resultGetBuf=curPreCA->getBuf(cf, pCurRect, pVxA, pSup);
		xassert(resultGetBuf);
		int i, j, cnt=0;
		for(i=0; i<p->sy; i++){
			for(j=0; j<p->sx; j++){
				int off=offsetBuf(XCYCL(p->x+j), YCYCL(p->y+i));
				if(pVxA)
					pVxA[cnt]=VxABuf[off];
				if(pSup){
					unsigned long c=SupBuf[off];
					pSup[cnt*SIZE_SUPA_CELL+0]=c;
					pSup[cnt*SIZE_SUPA_CELL+1]=c>>8;
					pSup[cnt*SIZE_SUPA_CELL+2]=c>>16;
				}
				cnt++;
			}
		}
	}
	//curPreCA->x=xL;
	//curPreCA->y=yT;
	curPreCA++;// теперь указывает на окончание
}

void vrtMap::UndoDispatcher_KillLastChange(sBasePMOperation* pPMO)
{
	if(getCurNewPMO()!=containerPMO.end()){
		//containerPMO.erase(getCurNewPMO(), containerPMO.end()); //Удаление не нужных элементов
		deleteFromContainer(containerPMO, getCurNewPMO(), containerPMO.end());
	}
	if(!containerPMO.empty()){
		delete containerPMO.back();
		containerPMO.pop_back();
	}
	containerPMO.push_back(pPMO);
	setCurNewPMO(containerPMO.end());

	if( preCAs.begin() ==preCAs.end()) return; //если пустой список то возврат
	if(curPreCA==preCAs.begin()) return;//если уже нет изменений

	curPreCA--;//Теперь указывет на последний элемент

	unsigned short* pVxA;
	unsigned char* pSup;
	sRect* pCurRect;
	int cf=0;
	while(curPreCA->getBuf(cf++, pCurRect, pVxA, pSup)){
		int i, j, cnt=0;
		for(i=0; i<pCurRect->sy; i++){
			for(j=0; j<pCurRect->sx; j++){
				int off=offsetBuf(XCYCL(pCurRect->x+j), YCYCL(pCurRect->y+i));
				if(pVxA)
					pVxA[cnt]=VxABuf[off];
				if(pSup){
					unsigned long c=SupBuf[off];
					pSup[cnt*SIZE_SUPA_CELL+0]=c;
					pSup[cnt*SIZE_SUPA_CELL+1]=c>>8;
					pSup[cnt*SIZE_SUPA_CELL+2]=c>>16;
				}
				cnt++;
			}
		}
		regRender(pCurRect->x, pCurRect->y, XCYCL(pCurRect->xr()), YCYCL(pCurRect->yb()), TypeCh_Texture|TypeCh_Height );
	}

	curPreCA++;// теперь указывает end
}

void vrtMap::UndoDispatcher_Undo(void)
{
	if( containerPMO.empty() || getCurNewPMO()==containerPMO.begin() )
		xassert(0&& "Невозможное ундо");
	curNewPMO--;
	xassert(curNewPMO>=0);

	if( preCAs.begin() ==preCAs.end()) return; //если пустой список то возврат
	if(curPreCA==preCAs.begin()) return;//если уже нет изменений

	curPreCA--;//Теперь указывет на последний элемент
	///xassert(curPreCA->idxPMO==containerPMO.size());
	int iddx=curPreCA->idxPMO;
	xassert( (curNewPMO+1) == curPreCA->idxPMO );
	sPreChangedArea backupPreChArea(*curPreCA);
	unsigned short* pVxAOut;
	unsigned char* pSupOut;
	sRect* pCurRect;
	int cf=0;
	while(curPreCA->getBuf(cf++, pCurRect, pVxAOut, pSupOut)){
		int i, j, cnt=0;
		for(i=0; i<pCurRect->sy; i++){
			for(j=0; j<pCurRect->sx; j++){
				int off=offsetBuf(XCYCL(pCurRect->x+j), YCYCL(pCurRect->y+i));
				//pVxAOut[cnt]=VxABuf[off];
				//pSurOut[cnt]=SurBuf[off];
				if(pVxAOut)
					pVxAOut[cnt]=VxABuf[off];
				if(pSupOut){
					unsigned long c=SupBuf[off];
					pSupOut[cnt*SIZE_SUPA_CELL+0]=c;
					pSupOut[cnt*SIZE_SUPA_CELL+1]=c>>8;
					pSupOut[cnt*SIZE_SUPA_CELL+2]=c>>16;
				}
				cnt++;
			}
		}
	}
	unsigned short* pVxAIn;
	unsigned char* pSupIn;
	cf=0;
	while(backupPreChArea.getBuf(cf++, pCurRect, pVxAIn, pSupIn)){
		int i, j, cnt=0;
		for(i=0; i<pCurRect->sy; i++){
			for(j=0; j<pCurRect->sx; j++){
				int off=offsetBuf(XCYCL(pCurRect->x+j), YCYCL(pCurRect->y+i));
				//VxABuf[off]=pVxAIn[cnt];
				//SurBuf[off]=pSurIn[cnt];
				if(pVxAIn)
					VxABuf[off]=pVxAIn[cnt];
				if(pSupIn){
					unsigned long c=pSupIn[cnt*SIZE_SUPA_CELL+0] | (pSupIn[cnt*SIZE_SUPA_CELL+1]<<8) | 
						(pSupIn[cnt*SIZE_SUPA_CELL+2]<<16);
					SupBuf[off]=c;
				}
				cnt++;
			}
		}
		regRender(pCurRect->x, pCurRect->y, XCYCL(pCurRect->xr()), YCYCL(pCurRect->yb()), TypeCh_Texture|TypeCh_Height );
	}

	//curPreCA++;// теперь указывает на окончание
}

void vrtMap::UndoDispatcher_Redo(void)
{
	if( containerPMO.empty() || getCurNewPMO()==containerPMO.end() )
		xassert(0&& "Невозможное редо");
	curNewPMO++;
	xassert(curNewPMO <= containerPMO.size());

	if( preCAs.begin() ==preCAs.end()) return; //если пустой список то возврат
	if(curPreCA==preCAs.end()) return;//если уже нет изменений

	xassert( (curNewPMO) == curPreCA->idxPMO );

	sPreChangedArea backupPreChArea(*curPreCA);
	unsigned short* pVxAOut;
	unsigned char* pSupOut;
	sRect* pCurRect;
	int cf=0;
	while(curPreCA->getBuf(cf++, pCurRect, pVxAOut, pSupOut)){
		int i, j, cnt=0;
		for(i=0; i<pCurRect->sy; i++){
			for(j=0; j<pCurRect->sx; j++){
				int off=offsetBuf(XCYCL(pCurRect->x+j), YCYCL(pCurRect->y+i));
				//pVxAOut[cnt]=VxABuf[off];
				//pSurOut[cnt]=SurBuf[off];
				if(pVxAOut)
					pVxAOut[cnt]=VxABuf[off];
				if(pSupOut){
					unsigned long c=SupBuf[off];
					pSupOut[cnt*SIZE_SUPA_CELL+0]=c;
					pSupOut[cnt*SIZE_SUPA_CELL+1]=c>>8;
					pSupOut[cnt*SIZE_SUPA_CELL+2]=c>>16;
				}

				cnt++;
			}
		}
	}
	unsigned short* pVxAIn;
	unsigned char* pSupIn;
	cf=0;
	while(backupPreChArea.getBuf(cf++, pCurRect, pVxAIn, pSupIn)){
		int i, j, cnt=0;
		for(i=0; i<pCurRect->sy; i++){
			for(j=0; j<pCurRect->sx; j++){
				int off=offsetBuf(XCYCL(pCurRect->x+j), YCYCL(pCurRect->y+i));
				//VxABuf[off]=pVxAIn[cnt];
				//SurBuf[off]=pSurIn[cnt];
				if(pVxAIn)
					VxABuf[off]=pVxAIn[cnt];
				if(pSupIn){
					unsigned long c=pSupIn[cnt*SIZE_SUPA_CELL+0] | (pSupIn[cnt*SIZE_SUPA_CELL+1]<<8) | 
						(pSupIn[cnt*SIZE_SUPA_CELL+2]<<16);
					SupBuf[off]=c;
				}

				cnt++;
			}
		}
		regRender(pCurRect->x, pCurRect->y, XCYCL(pCurRect->xr()), YCYCL(pCurRect->yb()), TypeCh_Texture|TypeCh_Height );
	}

	curPreCA++;
}

void vrtMap::UndoDispatcher_KillAllUndo(void)
{
	preCAs.erase(preCAs.begin(), preCAs.end());
	curPreCA=preCAs.begin();
	//containerPMO.clear();
	deleteFromContainer(containerPMO, containerPMO.begin(), containerPMO.end());
	setCurNewPMO(containerPMO.begin());
}

bool vrtMap::UndoDispatcher_IsUndoExist(void)
{
	if(curPreCA!=preCAs.begin()) return true;
	else return false;
}

bool vrtMap::UndoDispatcher_IsRedoExist(void)
{
	if(curPreCA!=preCAs.end()) return true;
	else return false;
}
