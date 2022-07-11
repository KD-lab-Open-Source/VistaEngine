#ifndef __QUANTIZER_H__
#define __QUANTIZER_H__

#include "XMath/Colors.h"

struct sOctreeNode{
	bool flag_leaf;
	unsigned char nLevel;
	sOctreeNode * childPntArr[8];	// pointers in octree
	union {	// pack noncrossing date
		UINT nPixels;
		UINT iPalIdx;		// only used in leaf, after FillPalette()
	};
	union {
		struct {
			sOctreeNode * pNextNode;	// pointers in priority list,
			sOctreeNode * pPrevNode;	// only used in non-leaf nodes
		};
		struct {
			unsigned int totalR;		// color totals, only used in leaf nodes
			unsigned int totalG;
			unsigned int totalB;
		};
	};

	sOctreeNode(unsigned char level, sOctreeNode* pHead, unsigned int& nLeafs) 
		: nPixels(0), totalR(0), totalG(0), totalB(0), nLevel(level), flag_leaf(level == 0), pNextNode(0), pPrevNode(0)	{

		for (char i = 0; i < 8; i++) childPntArr[i] = 0;
		if (flag_leaf) ++nLeafs;	// If we are a leaf, increment counter
		else if (pHead) {
			// Otherwise, put us at front of list of reducible non-leaf nodes...
			InsertAfter(pHead);
			// ...and determine our priority when it comes to reducing.
			UpdatePosition();
		}
	}

	~sOctreeNode(){
		for (int i = 0; i < 8; i++) if(childPntArr[i]) delete childPntArr[i];
	}

	void UpdatePosition(){
		// Determine our priority, in other words our position in the list.
		xassert(pNextNode);
		xassert(pPrevNode);

		sOctreeNode * pnext = pNextNode;
		// Increment as long as pNextNode should be reduced before we are.
		while(pnext && !ReduceBefore(*pnext)) pnext = pnext->pNextNode;
		xassert(pnext);
		if (pnext != pNextNode){
			Remove();				// Remove us from old position in list...
			InsertBefore(pnext);	// ...and insert us at new position.
		}
	}
	bool ReduceBefore(const sOctreeNode& other) const {
		// true if other should be reduced before we are
		if (nLevel == other.nLevel) return nPixels < other.nPixels;
		return nLevel < other.nLevel;
	}
	void Remove() {	
		xassert(pNextNode);
		xassert(pPrevNode);
		pPrevNode->pNextNode = pNextNode;
		pNextNode->pPrevNode = pPrevNode;
	}
	void InsertBefore(sOctreeNode * pnext) {
		xassert(pnext);
		pPrevNode = pnext->pPrevNode;
		pNextNode = pnext;

		pPrevNode->pNextNode = this;
		pNextNode->pPrevNode = this;
	}
	void InsertAfter(sOctreeNode * pprev) {
		InsertBefore(pprev->pNextNode); 
	}

	unsigned int ChildIndex(unsigned char r, unsigned char g, unsigned char b) const {
		// The octree trick: calculate child index based on the m_Level'th bits
		// of the color bytes.
		unsigned char rBit=(r>>nLevel)&1;
		unsigned char gBit=(g>>nLevel)&1;
		unsigned char bBit=(b>>nLevel)&1;
		return ( (rBit<<2) | (gBit<<1) | (bBit) );
	}

	void SetColor(unsigned char r, unsigned char g, unsigned char b, sOctreeNode * pHead, unsigned int& nLeafs) {
		nPixels++;
		if(flag_leaf){
			// If we are a leaf node, accumulate color bytes in totals.
			totalR += r;
			totalG += g;
			totalB += b;
		}
		else{
			if(pNextNode) UpdatePosition();	// Perhaps we should move further down the list,
											// because we represent more pixels.
			// We are not a leaf node, so delegate to one of our children.
			unsigned int index = ChildIndex(r, g, b);

			// If we don't have a matching child, create it.
			if (childPntArr[index] == 0)
				childPntArr[index] = new sOctreeNode(nLevel - 1, pHead, nLeafs);

			childPntArr[index]->SetColor(r, g, b, pHead, nLeafs);	// recursion
		}
	}

	unsigned int GetPaletteIndex(unsigned char r, unsigned char g, unsigned char b) const {
		// If we are a leaf node, the palette index is simply stored.
		if (flag_leaf) return iPalIdx;
		else {
			// Otherwise, the matching child node must know more about it.
			unsigned int index = ChildIndex(r, g, b);
			xassert(childPntArr[index] != 0);
			return childPntArr[index]->GetPaletteIndex(r, g, b);	// recursion
		}
	}

	void GetQuantized(unsigned int& nLeafs){
		// GetQuantized a non-leaf node to a leaf
		xassert(!flag_leaf);

		unsigned int r = 0;
		unsigned int g = 0;
		unsigned int b = 0;
		for (unsigned int i = 0; i < 8; i++) {
			sOctreeNode * pChild = childPntArr[i];
			if (pChild == 0) continue;

			xassert(pChild->flag_leaf);

			// Accumulate color totals of our children...
			r += pChild->totalR;
			g += pChild->totalG;
			b += pChild->totalB;

			// ...and delete them.
			delete pChild;
			childPntArr[i] = 0;
			--nLeafs;
		}
		totalR = r;
		totalG = g;
		totalB = b;

		// Now we have become a leaf node ourselves.
		flag_leaf = true;
		++nLeafs;
	}

	void FillPalette(Color4c* pPal, UINT& index) {
		if(flag_leaf) {
			// If we are a leaf, calculate the color by dividing the color totals
			// through the number of pixels we represent...
			Color4c col((BYTE)(totalR / nPixels),
				(BYTE)(totalG / nPixels),
				(BYTE)(totalB / nPixels));

			// ...and put it in the palette.
			pPal[index] = col;
			iPalIdx = index++;
		}
		else {
			// Otherwise, ask our child nodes.
			for (UINT i = 0; i < 8; i++)
				if (childPntArr[i] != 0)
					childPntArr[i]->FillPalette(pPal, index); // recursion
		}
	}
};

struct ColorQuantizer {
	friend class RGBOctree;
	Color4c* pPalette;
	int paletteSize;
	ColorQuantizer(){
		pPalette=0;
		paletteSize=0;
		pHead=0;
		pRoot=0;
	}
	void release(){
		if(pPalette) {
			delete [] pPalette; pPalette=0;
		}
		paletteSize=0;
		if(pHead){
			delete pHead; pHead=0;
		}
		if(pRoot){
			delete pRoot; pRoot=0;
		}
	}
	~ColorQuantizer(){
		release();
	}
	void setPaletteSize(unsigned int size){
		if(pPalette) {
			delete [] pPalette; pPalette=0;
		}
		pPalette=new Color4c[size];
		paletteSize=size;
	}
	void setPalette(Color4c* _pPalette, unsigned int _size){
		setPaletteSize(_size);
		xassert(_size==paletteSize);
		for(int i=0; i<_size; i++){
			pPalette[i]=_pPalette[i];
		}
	}
	inline unsigned char findNearestColor(Color4c color){
        register int r1, g1, b1, r2, g2, b2;
        register long dist, newdist;
		r1 = color.r;
        g1 = color.g;
        b1 = color.b;
        dist = 2000000000;
		int ind=0;
        for(int i = 0; i < paletteSize; i++) {
            r2 = pPalette[i].r;
            g2 = pPalette[i].g;
            b2 = pPalette[i].b;
            //newdist = 30*(r1-r2)*(r1-r2) + 59*(g1-g2)*(g1-g2) + 11*(b1-b2)*(b1-b2);
            newdist = (r1-r2)*(r1-r2) + (g1-g2)*(g1-g2) + (b1-b2)*(b1-b2);
            if(newdist < dist) {
                ind = i;
                dist = newdist;
            }
        }
		return ind;
	}
	int quantizeOctree(unsigned long* pSource, unsigned char* pDest, Vect2s bitmapSize, int nMaxColors);
	int ditherFloydSteinberg(unsigned long* pSource, unsigned char* pDest, Vect2s bitmapSize);

	//Ручная индексация
	bool prepare4PutColor(int _nMaxColors);
	void putColor(int rgb);
	bool postProcess();
	unsigned char GetIdxColor(unsigned int rgb);

private:
	//Служебные данные использующиеся для поэтапной индексации(prepare4PutColor, putColor, postProcess, GetIdxColor)
	sOctreeNode* pHead;
	sOctreeNode* pRoot;
	UINT nLeafs; // counter of leaf nodes in octree
	int nMaxColors;
};

struct sSimpleOctreeNode {
	bool flag_leaf;
	unsigned char palIdx;		// only used in leaf, after FillPalette()
	int childIdxArr[8];	// pointers in octree
	sSimpleOctreeNode(){
		for(int i=0; i<8; i++)
			childIdxArr[i]=0;
	}
	void serialize(Archive& ar);
};

class RGBOctree {
public:
	RGBOctree(){
		curNodeCnt_=0;
		curNodeLvl_=0;
	}
	void import(ColorQuantizer& incc){
		xassert(incc.pRoot);
		curNodeCnt_=0;
		int sizeNodeArr=getNodeCnt(incc.pRoot);
		//soNodeArr = new sSimpleOctreeNode[sizeNodeArr];
		soNodeArr.resize(sizeNodeArr);
		putNode2Tree(incc.pRoot);
		xassert(curNodeCnt_==sizeNodeArr);
	}
	unsigned char getIdxColor(unsigned long rgb){
		if(soNodeArr.empty())
			return 0;
		unsigned char r = unsigned char((rgb&0xff0000)>>16);
		unsigned char g = unsigned char((rgb&0xff00)>>8);
		unsigned char b = unsigned char(rgb&0xff);
		curNodeLvl_=7;
		return getIdxColor(0, r, g, b);
	}
	void serialize(Archive& ar);
private:
	//sSimpleOctreeNode * soNodeArr;
	//int sizeNodeArr;
	vector<sSimpleOctreeNode> soNodeArr;
	int curNodeCnt_;
	int curNodeLvl_;
	int getNodeCnt(sOctreeNode* pNode){
		//const int childNode=sizeof(pNode->childPntArr)/sizeof(pNode->childPntArr[0]);
		int cnt=1;
		for(int i=0; i<8; i++){
			if(pNode->childPntArr[i]!=0)
				cnt+=getNodeCnt(pNode->childPntArr[i]);
		}
		return cnt;
	}
	int putNode2Tree(sOctreeNode* pNode){
		int curNodeIdx=curNodeCnt_++;
		soNodeArr[curNodeIdx].flag_leaf=pNode->flag_leaf;
		soNodeArr[curNodeIdx].palIdx=pNode->iPalIdx;
		for(int i=0; i<8; i++){
			if(pNode->childPntArr[i]!=0)
				soNodeArr[curNodeIdx].childIdxArr[i]=putNode2Tree(pNode->childPntArr[i]);
			else
				soNodeArr[curNodeIdx].childIdxArr[i]=0;
		}
		return curNodeIdx;
	}
	unsigned char getIdxColor(int idxNode, unsigned char r, unsigned char g, unsigned char b){
		if (soNodeArr[idxNode].flag_leaf)
			return soNodeArr[idxNode].palIdx;
		unsigned char rBit=(r>>curNodeLvl_)&1;
		unsigned char gBit=(g>>curNodeLvl_)&1;
		unsigned char bBit=(b>>curNodeLvl_)&1;
		curNodeLvl_--;
		xassert(curNodeLvl_>=0);
		int childIdx=soNodeArr[idxNode].childIdxArr[ (rBit<<2) | (gBit<<1) | (bBit) ];
		xassert(childIdx>0); //нет цвета в дереве!
		return getIdxColor(childIdx, r, g, b);
	}
};
#endif //__QUANTIZER_H__
