#include "stdafxTr.h"

#include <time.h>
#include <process.h>
#include "quantizer.h"
//for serilization
#include "Serialization.h"
#include "XPrmArchive.h"

bool ColorQuantizer::prepare4PutColor(int _nMaxColors)
{
	nMaxColors=_nMaxColors;
	if (nMaxColors > 256 || nMaxColors < 8) return false;

	// initialization variables
	release();
	nLeafs = 0;	// counter of leaf nodes in octree
	pHead= new sOctreeNode(1, NULL, nLeafs);	// head of list with reducible non-leaf nodes (dummy)
	pHead->nLevel = 0;
	pRoot= new sOctreeNode(7, NULL, nLeafs);	// root of octree

	// Put tree root in reducible list. Root will always be the last node in the list,
	// and also functions as list sentinel.
	pHead->pNextNode = pRoot;
	pRoot->pPrevNode = pHead;

	return true;
}
void ColorQuantizer::putColor(int rgb)
{
	unsigned char r = (rgb&0xff0000)>>16;
	unsigned char g = (rgb&0xff00)>>8;
	unsigned char b =  rgb&0xff;

	pRoot->SetColor(r, g, b, pHead, nLeafs);
						// ...add color to tree, make new leaf if necessary.

	while(nLeafs > nMaxColors) { // If in the process too many leaves are created...
		// ... then, one by one, pick a non-leaf node...
		sOctreeNode * pReducible = pHead->pNextNode;
		xassert(pReducible);

		if(!pReducible->pNextNode) break;
		pReducible->Remove();	// ...remove it from the list of reducibles...
		pReducible->GetQuantized(nLeafs);	// ...and reduce it to a leaf.
	}
}
bool ColorQuantizer::postProcess()
{
	// We now have the important colors in the pic. Put them in a palette
	setPaletteSize(nMaxColors);

	unsigned int index = 0;
	pRoot->FillPalette(pPalette, index);	// Let octree fill the palette

	xassert(index == nLeafs);
	return index == nLeafs;
}

unsigned char ColorQuantizer::GetIdxColor(unsigned int rgb)
{
	unsigned char r = (rgb&0xff0000)>>16;
	unsigned char g = (rgb&0xff00)>>8;
	unsigned char b =  rgb&0xff;
	return pRoot->GetPaletteIndex(r, g, b);
}

//////////////////////////////////

int ColorQuantizer::quantizeOctree(unsigned long* pSource, unsigned char* pDest, Vect2s bitmapSize, int _nMaxColors)
{
	int result = 0;

	//Original version
	//nMaxColors=_nMaxColors;
	//if (nMaxColors > 256 || nMaxColors < 8) return false;

	//UINT nLeafs = 0;	// counter of leaf nodes in octree

	//sOctreeNode head(1, NULL, nLeafs);	// head of list with reducible non-leaf nodes (dummy)
	//head.nLevel = 0;
	//sOctreeNode root(7, NULL, nLeafs);	// root of octree

	//// Put tree root in reducible list. Root will always be the last node in the list,
	//// and also functions as list sentinel.
	//head.pNextNode = & root;
	//root.pPrevNode = & head;

	//// First loop: gather color information, put it in tree leaves
	//for(int y = 0; y < bitmapSize.y; y++)	{
	//	unsigned long * psrc = pSource + bitmapSize.x*y;
	//	for(int x = 0; x < bitmapSize.x; x++) {
	//		unsigned char r = (psrc[x]&0xff0000)>>16;
	//		unsigned char g = (psrc[x]&0xff00)>>8;
	//		unsigned char b = psrc[x]&0xff;

	//		root.SetColor(r, g, b, & head, nLeafs);
	//							// ...add color to tree, make new leaf if necessary.

	//		while(nLeafs > nMaxColors) { // If in the process too many leaves are created...
	//			// ... then, one by one, pick a non-leaf node...
	//			sOctreeNode * pReducible = head.pNextNode;
	//			xassert(pReducible);

	//			if(!pReducible->pNextNode) break;
	//			pReducible->Remove();	// ...remove it from the list of reducibles...
	//			pReducible->GetQuantized(nLeafs);	// ...and reduce it to a leaf.
	//		}
	//	}
	//}
	///////////////////////
	//// We now have the important colors in the pic. Put them in a palette
	//setPaletteSize(nMaxColors);
	//unsigned int index = 0;
	//root.FillPalette(pPalette, index);	// Let octree fill the palette
	//xassert(index == nLeafs);

	//test version
	if(!prepare4PutColor(_nMaxColors)) return false;

	for(int y = 0; y < bitmapSize.y; y++)	{
		unsigned long * psrc = pSource + bitmapSize.x*y;
		for(int x = 0; x < bitmapSize.x; x++) 
			putColor(psrc[x]);
	}
	if(!postProcess()) return false;

	result=ditherFloydSteinberg(pSource, pDest, bitmapSize);
/*
	// Second loop: fill indexed bitmap
	for (int y = 0; y < bitmapSize.y; y++) {	// For each row...
		unsigned long * psrc = pSource + bitmapSize.x*y;
		for (int x = 0; x < bitmapSize.x; x++) {	// ...for each pixel...
			// ...gather color information from source bitmap...
			unsigned char r = (psrc[x]&0xff0000)>>16;
			unsigned char g = (psrc[x]&0xff00)>>8;
			unsigned char b = psrc[x]&0xff;

			// ...let octree calculate index...
			unsigned char index = root.GetPaletteIndex(r, g, b);

			// ...and put index in the destination bitmap.
			*pDest++ = index;
		}
	}
*/
	return result;
}

int ColorQuantizer::ditherFloydSteinberg(unsigned long* pSource, unsigned char* pDest, Vect2s bitmapSize)
{
	xassert(bitmapSize.x>0);
	bool statisticArr[256];
	memset(&statisticArr[0], '\0', sizeof(statisticArr));
	const int FS_SCALE=1024;	/* Floyd-Steinberg scaling factor */
	const int SIZE_FS_ERR_ARR=bitmapSize.x+2;
	long* thisRErr=new long[SIZE_FS_ERR_ARR];
    long* nextRErr=new long[SIZE_FS_ERR_ARR];
    long* thisGErr=new long[SIZE_FS_ERR_ARR];
    long* nextGErr=new long[SIZE_FS_ERR_ARR];
    long* thisBErr=new long[SIZE_FS_ERR_ARR];
    long* nextBErr=new long[SIZE_FS_ERR_ARR];
    ///srand((int)(time(0)^getpid()));
	int x;
    for(x=0; x < SIZE_FS_ERR_ARR; x++) {
        //thisRErr[x] = rand()%(FS_SCALE*2) - FS_SCALE;
        //thisGErr[x] = rand()%(FS_SCALE*2) - FS_SCALE;
        //thisBErr[x] = rand()%(FS_SCALE*2) - FS_SCALE;
        thisRErr[x] = XRnd(FS_SCALE*2) - FS_SCALE;
        thisGErr[x] = XRnd(FS_SCALE*2) - FS_SCALE;
        thisBErr[x] = XRnd(FS_SCALE*2) - FS_SCALE;
        /* (random errors in [-1 .. 1]) */
    }
    bool flag_FSDirection=1;

	for (int y = 0; y < bitmapSize.y; y++) {	// For each row...
		unsigned long * psrc = pSource + bitmapSize.x*y;
		unsigned char * pdst = pDest + bitmapSize.x*y;
		memset(nextRErr, '\0', SIZE_FS_ERR_ARR*sizeof(nextRErr[0]));
		memset(nextGErr, '\0', SIZE_FS_ERR_ARR*sizeof(nextGErr[0]));
		memset(nextBErr, '\0', SIZE_FS_ERR_ARR*sizeof(nextBErr[0]));
		int limitX;
        if(flag_FSDirection ) {
            x=0;
			limitX=bitmapSize.x;
		}
		else {
			x=bitmapSize.x-1;
			limitX=-1;
		}

		do{	long sr = (psrc[x]&0xff0000)>>16;
			long sg = (psrc[x]&0xff00)>>8;
			long sb = psrc[x]&0xff;
            // Use Floyd-Steinberg errors to adjust actual color
            sr+=thisRErr[x+1]/FS_SCALE;
            sg+=thisGErr[x+1]/FS_SCALE;
            sb+=thisBErr[x+1]/FS_SCALE;
			sr=clamp(sr, 0, 255);
			sg=clamp(sg, 0, 255);
			sb=clamp(sb, 0, 255);

            register int r1, g1, b1, r2, g2, b2;
            register long dist, newdist;
            r1 = sr;
            g1 = sg;
            b1 = sb;
            dist = 2000000000;
			int ind=0;
            for(int i = 0; i < paletteSize; i++) {
                r2 = pPalette[i].r;
                g2 = pPalette[i].g;
                b2 = pPalette[i].b;
                newdist = (r1-r2)*(r1-r2) + (g1-g2)*(g1-g2) + (b1-b2)*(b1-b2);
                if(newdist < dist) {
                    ind = i;
                    dist = newdist;
                }
            }
            // Propagate Floyd-Steinberg error terms
			long err;
            if(flag_FSDirection){
                err=(sr-(long)pPalette[ind].r)*FS_SCALE;
                thisRErr[x+2] += (err*7)/16;
                nextRErr[x  ] += (err*3)/16;
                nextRErr[x+1] += (err*5)/16;
                nextRErr[x+2] += (err  )/16;
                err=(sg-(long)pPalette[ind].g)*FS_SCALE;
                thisGErr[x+2] += (err*7)/16;
                nextGErr[x  ] += (err*3)/16;
                nextGErr[x+1] += (err*5)/16;
                nextGErr[x+2] += (err  )/16;
                err=(sb-(long)pPalette[ind].b)*FS_SCALE;
                thisBErr[x+2] += (err*7)/16;
                nextBErr[x  ] += (err*3)/16;
                nextBErr[x+1] += (err*5)/16;
                nextBErr[x+2] += (err  )/16;
            } 
			else {
                err=(sr-(long)pPalette[ind].r)*FS_SCALE;
                thisRErr[x  ] += (err*7)/16;
                nextRErr[x+2] += (err*3)/16;
                nextRErr[x+1] += (err*5)/16;
                nextRErr[x  ] += (err  )/16;
                err=(sg-(long)pPalette[ind].g)*FS_SCALE;
                thisGErr[x  ] += (err*7)/16;
                nextGErr[x+2] += (err*3)/16;
                nextGErr[x+1] += (err*5)/16;
                nextGErr[x  ] += (err  )/16;
                err=(sb-(long)pPalette[ind].b)*FS_SCALE;
                thisBErr[x  ] += (err*7)/16;
                nextBErr[x+2] += (err*3)/16;
                nextBErr[x+1] += (err*5)/16;
                nextBErr[x  ] += (err  )/16;
            }
			pdst[x]=ind;
			statisticArr[ind]=1;
            if(flag_FSDirection)
                x++;
            else 
				x--;
		}while(x!=limitX);
		swap(thisRErr, nextRErr);
		swap(thisGErr, nextGErr);
		swap(thisBErr, nextBErr);
        flag_FSDirection=!flag_FSDirection;
	}
	delete thisRErr;
    delete nextRErr;
    delete thisGErr;
    delete nextGErr;
    delete thisBErr;
    delete nextBErr;

	int stat=0;
	for(int i=0; i<256; i++) if(statisticArr[i])stat++;
	return stat;
}

void sSimpleOctreeNode::serialize(Archive& ar)
{
	ar.serialize(flag_leaf, "flag_leaf", 0);
	ar.serialize(palIdx, "palIdx", 0);
	ar.serializeArray(childIdxArr, "childIdxArr", 0);
}

void RGBOctree::serialize(Archive& ar)
{
	//ar.serialize(sizeNodeArr, "sizeNodeArr", 0);
	ar.serialize(curNodeCnt_, "curNodeCnt_", 0);
	ar.serialize(curNodeLvl_, "curNodeLvl_", 0);
	ar.serialize(soNodeArr, "soNodeArr", 0);
}
