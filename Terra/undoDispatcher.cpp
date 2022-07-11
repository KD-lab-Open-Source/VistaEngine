#include "stdafxTr.h"

#include "vmap.h"

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
	//xassert(supBuf);
	if(!isRecordingPMO()) return;
	if(pPMO!=0){
		//if(getCurNewPMO()!=containerPMO.end())
		//	containerPMO.erase(getCurNewPMO(), containerPMO_Old.end()); //Удаление не нужных элементов
		deleteFromContainer(containerPMO, getCurNewPMO(), containerPMO.end());

		containerPMO.push_back(pPMO);
		setCurNewPMO(containerPMO.end());
	}
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
	curPreCA->prepare4Operation(_chArea, containerPMO.size(), saveVx, saveRGB, pPMO);
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
				pVxA[cnt]=vxaBuf[off];
			if(pSup){
				unsigned long c=supBuf[off];
				pSup[cnt*SIZE_SUPA_CELL+0]=c;
				pSup[cnt*SIZE_SUPA_CELL+1]=c>>8;
				pSup[cnt*SIZE_SUPA_CELL+2]=c>>16;
			}
			cnt++;
		}
	}
	curPreCA->putMultiRegion(region_);
	curPreCA++;// теперь указывает на окончание
}

void vrtMap::UndoDispatcher_PutPreChangedArea(list<sRect>& _chAreaList, bool saveVx, bool saveRGB, sBasePMOperation* pPMO)
{
	if(!isRecordingPMO()) return;
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
	curPreCA->prepare4Operation(_chAreaList, containerPMO.size(), saveVx, saveRGB, pPMO);
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
					pVxA[cnt]=vxaBuf[off];
				if(pSup){
					unsigned long c=supBuf[off];
					pSup[cnt*SIZE_SUPA_CELL+0]=c;
					pSup[cnt*SIZE_SUPA_CELL+1]=c>>8;
					pSup[cnt*SIZE_SUPA_CELL+2]=c>>16;
				}
				cnt++;
			}
		}
	}
	curPreCA->putMultiRegion(region_);
	curPreCA++;// теперь указывает на окончание
}

//void vrtMap::UndoDispatcher_KillLastChange(sBasePMOperation* pPMO)
//{
//	if(getCurNewPMO()!=containerPMO.end()){
//		//containerPMO.erase(getCurNewPMO(), containerPMO.end()); //Удаление не нужных элементов
//		deleteFromContainer(containerPMO, getCurNewPMO(), containerPMO.end());
//	}
//	if(!containerPMO.empty()){
//		delete containerPMO.back();
//		containerPMO.pop_back();
//	}
//	containerPMO.push_back(pPMO);
//	setCurNewPMO(containerPMO.end());
//
//	if( preCAs.begin() ==preCAs.end()) return; //если пустой список то возврат
//	if(curPreCA==preCAs.begin()) return;//если уже нет изменений
//
//	curPreCA--;//Теперь указывет на последний элемент
//
//	unsigned short* pVxA;
//	unsigned char* pSup;
//	sRect* pCurRect;
//	int cf=0;
//	while(curPreCA->getBuf(cf++, pCurRect, pVxA, pSup)){
//		int i, j, cnt=0;
//		for(i=0; i<pCurRect->sy; i++){
//			for(j=0; j<pCurRect->sx; j++){
//				int off=offsetBuf(XCYCL(pCurRect->x+j), YCYCL(pCurRect->y+i));
//				if(pVxA)
//					pVxA[cnt]=vxaBuf[off];
//				if(pSup){
//					unsigned long c=supBuf[off];
//					pSup[cnt*SIZE_SUPA_CELL+0]=c;
//					pSup[cnt*SIZE_SUPA_CELL+1]=c>>8;
//					pSup[cnt*SIZE_SUPA_CELL+2]=c>>16;
//				}
//				cnt++;
//			}
//		}
//		regRender(pCurRect->x, pCurRect->y, XCYCL(pCurRect->xr()), YCYCL(pCurRect->yb()), TypeCh_Texture|TypeCh_Height );
//	}
//	region_->lock();
//	*region_= *curPreCA->mRegion;
//	region_->unlock();
//	
//	curPreCA++;// теперь указывает end
//}

void vrtMap::UndoDispatcher_Undo()
{

	if( preCAs.begin() ==preCAs.end()) return; //если пустой список то возврат
	if(curPreCA==preCAs.begin()) return;//если уже нет изменений

	curPreCA--;//Теперь указывет на последний элемент
	if( curPreCA->pmo!=0){
		if(containerPMO.empty() || getCurNewPMO()==containerPMO.begin() )
            xassert(0&& "Невозможное ундо");;
		curNewPMO--;
		int iddx=curPreCA->idxPMO;
		xassert( (curNewPMO+1) == curPreCA->idxPMO );
	}
	xassert(curNewPMO>=0);

	///xassert(curPreCA->idxPMO==containerPMO.size());
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
				//pVxAOut[cnt]=vxaBuf[off];
				//pSurOut[cnt]=SurBuf[off];
				if(pVxAOut)
					pVxAOut[cnt]=vxaBuf[off];
				if(pSupOut){
					unsigned long c=supBuf[off];
					pSupOut[cnt*SIZE_SUPA_CELL+0]=c;
					pSupOut[cnt*SIZE_SUPA_CELL+1]=c>>8;
					pSupOut[cnt*SIZE_SUPA_CELL+2]=c>>16;
				}
				cnt++;
			}
		}
	}
	curPreCA->putMultiRegion(region_);
	unsigned short* pVxAIn;
	unsigned char* pSupIn;
	cf=0;
	while(backupPreChArea.getBuf(cf++, pCurRect, pVxAIn, pSupIn)){
		int i, j, cnt=0;
		for(i=0; i<pCurRect->sy; i++){
			for(j=0; j<pCurRect->sx; j++){
				int off=offsetBuf(XCYCL(pCurRect->x+j), YCYCL(pCurRect->y+i));
				//vxaBuf[off]=pVxAIn[cnt];
				//SurBuf[off]=pSurIn[cnt];
				if(pVxAIn)
					vxaBuf[off]=pVxAIn[cnt];
				if(pSupIn){
					unsigned long c=pSupIn[cnt*SIZE_SUPA_CELL+0] | (pSupIn[cnt*SIZE_SUPA_CELL+1]<<8) | 
						(pSupIn[cnt*SIZE_SUPA_CELL+2]<<16);
					supBuf[off]=c;
				}
				cnt++;
			}
		}
		regRender(pCurRect->x, pCurRect->y, XCYCL(pCurRect->xr()), YCYCL(pCurRect->yb()), TypeCh_Texture|TypeCh_Height );
	}
	region_->lock();
	*region_= *backupPreChArea.mRegion;
	region_->unlock();

	//curPreCA++;// теперь указывает на окончание
}

void vrtMap::UndoDispatcher_Redo()
{

	if( preCAs.begin() ==preCAs.end()) return; //если пустой список то возврат
	if(curPreCA==preCAs.end()) return;//если уже нет изменений

	if( curPreCA->pmo!=0){
		if(containerPMO.empty() || getCurNewPMO()==containerPMO.end() )
            xassert(0&& "Невозможное редо");;
		curNewPMO++;
		xassert( (curNewPMO) == curPreCA->idxPMO );
	}
	xassert(curNewPMO <= containerPMO.size());

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
				//pVxAOut[cnt]=vxaBuf[off];
				//pSurOut[cnt]=SurBuf[off];
				if(pVxAOut)
					pVxAOut[cnt]=vxaBuf[off];
				if(pSupOut){
					unsigned long c=supBuf[off];
					pSupOut[cnt*SIZE_SUPA_CELL+0]=c;
					pSupOut[cnt*SIZE_SUPA_CELL+1]=c>>8;
					pSupOut[cnt*SIZE_SUPA_CELL+2]=c>>16;
				}

				cnt++;
			}
		}
	}
	curPreCA->putMultiRegion(region_);
	unsigned short* pVxAIn;
	unsigned char* pSupIn;
	cf=0;
	while(backupPreChArea.getBuf(cf++, pCurRect, pVxAIn, pSupIn)){
		int i, j, cnt=0;
		for(i=0; i<pCurRect->sy; i++){
			for(j=0; j<pCurRect->sx; j++){
				int off=offsetBuf(XCYCL(pCurRect->x+j), YCYCL(pCurRect->y+i));
				//vxaBuf[off]=pVxAIn[cnt];
				//SurBuf[off]=pSurIn[cnt];
				if(pVxAIn)
					vxaBuf[off]=pVxAIn[cnt];
				if(pSupIn){
					unsigned long c=pSupIn[cnt*SIZE_SUPA_CELL+0] | (pSupIn[cnt*SIZE_SUPA_CELL+1]<<8) | 
						(pSupIn[cnt*SIZE_SUPA_CELL+2]<<16);
					supBuf[off]=c;
				}

				cnt++;
			}
		}
		regRender(pCurRect->x, pCurRect->y, XCYCL(pCurRect->xr()), YCYCL(pCurRect->yb()), TypeCh_Texture|TypeCh_Height );
	}
	region_->lock();
	*region_= *backupPreChArea.mRegion;
	region_->unlock();


	curPreCA++;
}

void vrtMap::UndoDispatcher_KillAllUndo()
{
	preCAs.erase(preCAs.begin(), preCAs.end());
	curPreCA=preCAs.begin();
	//containerPMO.clear();
	deleteFromContainer(containerPMO, containerPMO.begin(), containerPMO.end());
	setCurNewPMO(containerPMO.begin());
}

bool vrtMap::UndoDispatcher_IsUndoExist()
{
	if(curPreCA!=preCAs.begin()) return true;
	else return false;
}

bool vrtMap::UndoDispatcher_IsRedoExist()
{
	if(curPreCA!=preCAs.end()) return true;
	else return false;
}
