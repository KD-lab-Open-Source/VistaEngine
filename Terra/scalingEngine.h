#ifndef __SCALINGENGINE_H__
#define __SCALINGENGINE_H__

void scaleRGBAImage(unsigned long* srcBuf, Vect2s srcBufSize, unsigned long* dstBuf, Vect2s dstBufSize);

class BoxFilter
{
public:
	inline float filterWidth(){ return 0.5f; }
    inline float getFilterVal(float t) {
		if((t > -0.5f) && (t <= 0.5f)) return(1.f);
		return(0.f);
	}
};

class BSplineFilter
{
public:
	inline float filterWidth(){ return 2.0f; }
    inline float getFilterVal(float t) {
		if(t < 0) t = -t;
		if(t < 1) {
			float tt = t*t;
			return ((.5f*tt*t) - tt + (2.f/3.f));
		} else if(t < 2.f) {
			t = 2.f - t;
			return ((1.f/6.f) * (t*t*t));
		}
		return(0.f);
	}
};

class TriangleFilter //BilinearFilter
{
public:
	inline float filterWidth(){ return 1.0f; }
    inline float getFilterVal(float t) {
		if(t < 0.f) t = -t;
		if(t < 1.f) return (1.f - t);
		return 0.f;
	}
};

class FilterFilter
{
public:
	inline float filterWidth(){ return 1.0f; }
    inline float getFilterVal(float t) {
		/* f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 */
		if(t < 0.0f) t = -t;
		if(t < 1.0f) return((2.0f * t - 3.0f) * t * t + 1.0f);
		return(0.0f);
	}
};

class BellFilter
{
public:
	inline float filterWidth(){ return 1.5f; }
    inline float getFilterVal(float t) {
		if(t < 0) t = -t;
		if(t < .5f) return(.75f - (t * t));
		if(t < 1.5f) {
			t = (t - 1.5f);
			return(.5f * (t * t));
		}
		return(0.0f);
	}
};

class Lanczos3Filter
{
public:
	inline float sinc(float x) {
		x *= M_PI;
		if(x != 0) return(sin(x) / x);
		return(1.0f);
	}
	inline float filterWidth(){ return 3.f; }
    inline float getFilterVal(float t) {
		if(t < 0) t = -t;
		if(t < 3.0) return(sinc(t) * sinc(t/3.0f));
		return(0.0f);
	}
};

class MitchellFilter
{
public:
	inline float filterWidth(){ return 2.f; }
    inline float getFilterVal(float t) {
		static const float B=(1.0f / 3.0f);
		static const float C=(1.0f / 3.0f);
		float tt;
		tt = t * t;
		if(t < 0) t = -t;
		if(t < 1.0f) {
			t = (((12.0f - 9.0f * B - 6.0f * C) * (t * tt))
			+ ((-18.0f + 12.0f * B + 6.0f * C) * tt)
			+ (6.0f - 2 * B));
			return(t / 6.0f);
		} else if(t < 2.0f) {
			t = (((-1.0f * B - 6.0f * C) * (t * tt))
			+ ((6.0f * B + 30.0f * C) * tt)
			+ ((-12.0f * B - 48.0f * C) * t)
			+ (8.0f * B + 24 * C));
			return(t / 6.0f);
		}
		return(0.0f);
	}
};



class GaussianFilter
{
public:
	inline float filterWidth(){ return 3.f; }
    inline float getFilterVal(float t) {
		if(t < 0) t = -t;
        if(t < 3.f)
	        return exp(-t*t / 2.0f) / sqrt(2*M_PI);
		else 
            return 0.0f;
	}
};

class HammingFilter
{
public:
	inline float filterWidth(){ return 0.5f; }
    inline float getFilterVal(float t) {
		if(t < 0) t = -t;
        if(t < 0.5f) {
			float dWindow = 0.54 + 0.46 * cos (2*M_PI * t); 
			float dSinc = (t == 0) ? 1.0 : sin (M_PI * t) / (M_PI * t); 
			return dWindow * dSinc;
		}
		else
            return 0.0; 
	}
};

class CBlackmanFilter
{
public:
	inline float filterWidth(){ return 0.5f; }
    inline float getFilterVal(float t) {
		if(t < 0) t = -t;
        if(t < 0.5f) {
			const float dWidth=0.5f;
			float dN = 2.0f * dWidth + 1.0f; 
			return 0.42f + 0.5f * cos(2*M_PI * t / ( dN - 1.0f )) + 
					0.08f * cos (4*M_PI * t / ( dN - 1.0f )); 
		}
		else
            return 0.0; 
	}
};
 



//GET_A Надоли &0xFF
#define GET_A(c) (((c)>>24)&0xFF)
#define GET_R(c) (((c)>>16)&0xFF)
#define GET_G(c) (((c)>>8)&0xFF)
#define GET_B(c) ((c)&0xFF)
struct Image{
	int	xsize;
	int	ysize;
	unsigned long *	data;
	bool autoRelease;
	Image(int _sx, int _sy, bool _autoRelease=true){
		xsize=_sx;
		ysize=_sy;
		data=new unsigned long[xsize*ysize];
		autoRelease=_autoRelease;
	}
	Image(unsigned long* _data, Vect2s _size, bool _autoRelease=false){
		xsize=_size.x;
		ysize=_size.y;
		data=_data;
		autoRelease=_autoRelease;
	}
	~Image(){
		if(autoRelease)
			delete data;
	}
};

template <class FilterClass>
class FilterScaling : public FilterClass
{
public:
    FilterScaling(){}
//private:
	class ContribList{
	public:
		struct Contrib {
			short pixel;
			float weight;
		};
		Contrib* pContribList;
		short* pNumContribList;

		short sizePRow;
		short maxAmountContribs;
		ContribList(int _sizePRow, int _maxAmountContribs){
			sizePRow=_sizePRow;
			maxAmountContribs=_maxAmountContribs;
			pContribList=new Contrib[sizePRow*maxAmountContribs];
			pNumContribList=new short[sizePRow];
			for(int i=0; i<sizePRow; i++)pNumContribList[i]=0;
		}
		~ContribList(){
			delete [] pNumContribList;
			delete [] pContribList;
		}
		Contrib& contrib(int i, int c){
			xassert(i<sizePRow && c<maxAmountContribs);
			return pContribList[i*maxAmountContribs+c];
		}
		short& amountContrib(int i){
			xassert(i<sizePRow);
			return pNumContribList[i];
		}
		void pushBack(int i, short pixel, float weight){
			xassert(i<sizePRow);
			xassert(pNumContribList[i] < maxAmountContribs);
			Contrib& c=pContribList[i*maxAmountContribs+pNumContribList[i]];
			c.pixel=pixel;
			c.weight=weight;
			pNumContribList[i]++;
		}
	};

	void zoom(Image *src, Image *dst){
		int i, j, k;

		// create intermediate image to hold horizontal zoom
		Image tmp(dst->xsize, src->ysize);
		//zoom scale factors
		float xscale = (float) dst->xsize / (float) src->xsize; 
		float yscale = (float) dst->ysize / (float) src->ysize;

		// pre-calculate filter contributions for a row
		short numcontribInPixel;
		numcontribInPixel= xscale<1.0 ? (int)((filterWidth()/xscale)*2+1+1) : (int)(filterWidth()*2+1+1);
		ContribList contribListH(dst->xsize, numcontribInPixel);
		if(xscale < 1.0) {
			float width = filterWidth() / xscale;
			float fscale = 1.0 / xscale;
			for(i = 0; i < dst->xsize; ++i) {
				float center = (float) i / xscale; // filter calculation variables
				int left = round(ceil(center - width));
				int right = round(floor(center + width));
				float sumWeight=0;
				for(j = left; j <= right; ++j) {
					float weight = center - (float)j; // filter calculation variables
					weight = getFilterVal(weight / fscale) / fscale;
					int n; // pixel number
					if(j < 0) {
						n = -j;
					} else if(j >= src->xsize) {
						n = (src->xsize - j) + src->xsize - 1;
					} else {
						n = j;
					}
					contribListH.pushBack(i, n, weight);
					sumWeight+=weight;
				}
				sumWeight=1.f/sumWeight;
				for(j = left; j <= right; ++j) {
					contribListH.contrib(i,j-left).weight*=sumWeight;
				}
			}
		} else {
			for(i = 0; i < dst->xsize; ++i) {
				float center = (float) i / xscale; // filter calculation variables
				int left = round(ceil(center - filterWidth()));
				int right = round(floor(center + filterWidth()));
				float sumWeight=0;
				for(j = left; j <= right; ++j) {
					float weight = center - (float) j; // filter calculation variables
					weight = getFilterVal(weight);
					int n; // pixel number
					if(j < 0) {
						n = -j;
					} else if(j >= src->xsize) {
						n = (src->xsize - j) + src->xsize - 1;
					} else {
						n = j;
					}
					contribListH.pushBack(i, n, weight);
					sumWeight+=weight;
				}
				sumWeight=1.f/sumWeight;
				for(j = left; j <= right; ++j) {
					contribListH.contrib(i,j-left).weight*=sumWeight;
				}
			}
		}

		// apply filter to zoom horizontally from src to tmp
		unsigned long* raster; //a row or column of pixels
		for(k = 0; k < tmp.ysize; ++k) {
			//get_row(raster, src, k);
			raster=&(src->data[k*src->xsize]);
			for(i = 0; i < tmp.xsize; ++i) {
				float r=0,g=0,b=0,a=0;
				int maxContrib=contribListH.amountContrib(i);
				for(j = 0; j < maxContrib; ++j) {
					unsigned long c=raster[contribListH.contrib(i,j).pixel];
					float& kf=contribListH.contrib(i,j).weight;
					r+=(float)GET_R(c)*kf;
					g+=(float)GET_G(c)*kf;
					b+=(float)GET_B(c)*kf;
					a+=(float)GET_A(c)*kf;
				}
				short ri,gi,bi,ai;
				ri=round(r); gi=round(g); bi=round(b); ai=round(a);
				ri=clamp(ri,0,255);
				gi=clamp(gi,0,255);
				bi=clamp(bi,0,255);
				ai=clamp(ai,0,255);
				tmp.data[i+ k*tmp.xsize]=(ai<<24) | (ri<<16) | (gi<<8) | bi;
			}
		}

		// pre-calculate filter contributions for a column
		numcontribInPixel= yscale<1.0 ? (int)((filterWidth()/yscale)*2+1+1) : (int)(filterWidth()*2+1+1);
		ContribList contribListV(dst->ysize, numcontribInPixel);
		if(yscale < 1.0) {
			float width = filterWidth() / yscale;
			float fscale = 1.0 / yscale;
			for(i = 0; i < dst->ysize; ++i) {
				float center = (float) i / yscale; // filter calculation variables
				int left = round(ceil(center - width));
				int right = round(floor(center + width));
				float sumWeight=0;
				for(j = left; j <= right; ++j) {
					float weight = center - (float) j; // filter calculation variables
					weight = getFilterVal(weight / fscale) / fscale;
					int n; // pixel number
					if(j < 0) {
						n = -j;
					} else if(j >= tmp.ysize) {
						n = (tmp.ysize - j) + tmp.ysize - 1;
					} else {
						n = j;
					}
					contribListV.pushBack(i, n, weight);
					sumWeight+=weight;
				}
				sumWeight=1.f/sumWeight;
				for(j = left; j <= right; ++j) {
					contribListV.contrib(i,j-left).weight*=sumWeight;
				}
			}
		} else {
			for(i = 0; i < dst->ysize; ++i) {
				float center = (float) i / yscale; // filter calculation variables
				int left = round(ceil(center - filterWidth()));
				int right = round(floor(center + filterWidth()));
				float sumWeight=0;
				for(j = left; j <= right; ++j) {
					float weight = center - (float) j; // filter calculation variables
					weight = getFilterVal(weight);
					int n; // pixel number
					if(j < 0) {
						n = -j;
					} else if(j >= tmp.ysize) {
						n = (tmp.ysize - j) + tmp.ysize - 1;
					} else {
						n = j;
					}
					contribListV.pushBack(i, n, weight);
					sumWeight+=weight;
				}
				sumWeight=1.f/sumWeight;
				for(j = left; j <= right; ++j) {
					contribListV.contrib(i,j-left).weight*=sumWeight;
				}
			}
		}

		// apply filter to zoom vertically from tmp to dst 
		raster = new unsigned long[tmp.ysize];
		for(k = 0; k < dst->xsize; ++k) {
			//get_column(raster, tmp, k);
			for(int m=0; m < tmp.ysize; m++) 
				raster[m]=tmp.data[m*tmp.xsize+k];
			for(i = 0; i < dst->ysize; ++i) {
				float r=0,g=0,b=0,a=0;
				int maxContrib=contribListV.amountContrib(i);
				for(j = 0; j < maxContrib; ++j) {
					unsigned long c=raster[contribListV.contrib(i,j).pixel];
					float& kf=contribListV.contrib(i,j).weight;
					r+=(float)GET_R(c)*kf;
					g+=(float)GET_G(c)*kf;
					b+=(float)GET_B(c)*kf;
					a+=(float)GET_A(c)*kf;
				}
				short ri,gi,bi,ai;
				ri=round(r); gi=round(g); bi=round(b); ai=round(a);
				ri=clamp(ri,0,255);
				gi=clamp(gi,0,255);
				bi=clamp(bi,0,255);
				ai=clamp(ai,0,255);
				dst->data[k+ i*dst->xsize]=(ai<<24) | (ri<<16) | (gi<<8) | bi;
			}
		}
		delete raster;
	}

};


#endif //__SCALINGENGINE_H__
