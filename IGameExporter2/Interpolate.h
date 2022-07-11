#pragma once

void CalcLagrange(vector<float>& data,float& a0,float& a1,float& a2,float& a3);
void CalcLinear(vector<float>& data,float& a0,float& a1);
template<int size_template>
struct VectTemplate
{
	enum 
	{
		size=size_template,
	};

	float data[size];

	float& operator[](int i){return data[i];}
	const float& operator[](int i)const{return data[i];}


	bool operator==(const VectTemplate& rhs) const{
		if(size != rhs.size)
			return false;
		for(int i = 0; i < size; ++i){
			if(fabsf((*this)[i] != rhs[i]) > FLT_COMPARE_TOLERANCE)
				return false;
		}
		return true;
	}


	float distance(const VectTemplate& p)
	{
		float d=0;
		for(int i=0;i<size;i++)
		{
			float x=data[i]-p[i];
			d+=x*x;
		}
		return sqrtf(d);
	}
};

template<class T>
class InterpolatePosition
{
	vector<T> position;
public:
	struct One
	{
		ITPL itpl;
		int interval_begin;//потом во float будет переводиться.
		int interval_size;
		T a0,a1,a2,a3;//y=a0+a1*t+a2*t*t+a3*t*t*t;
 
		inline float IntervalToT(int interval)
		{
			return (interval-interval_begin)/float(interval_size-1)-0.5f;
		}

		inline T Calc(float t)
		{
			float t2=t*t;
			float t3=t2*t;
			T out;
			for(int i=0;i<T::size;i++)
				out[i]=a0[i]+a1[i]*t+a2[i]*t2+a3[i]*t3;
			return out;
		}

		bool operator!=(const One& rhs) const{
            return !operator==(rhs);
        }
		bool operator==(const One& rhs) const
		{
			if(itpl != rhs.itpl || 
				interval_begin != rhs.interval_begin ||
				interval_size != rhs.interval_size)
				return false;

            switch(itpl){
            case ITPL_SPLINE:
                return
                  a0 == rhs.a0 &&
                  a1 == rhs.a1 &&
                  a2 == rhs.a2 &&
                  a3 == rhs.a3;
                break;
            case ITPL_LINEAR:
                return
                  a0 == rhs.a0 &&
                  a1 == rhs.a1;
                break;
            case ITPL_CONSTANT:
                return a0 == rhs.a0;
                break;
            default:
                return false;
                break;
            }
		}
	};
	size_t size() const{ return out_data.size(); }

	bool operator!=(const InterpolatePosition& rhs) const
	{
		return !operator==(rhs);
	}

	bool operator==(const InterpolatePosition& rhs) const
	{
		if(out_data.size() != rhs.out_data.size())
			return false;
		if(cycled != rhs.cycled)
			return false;
		if(out_data != rhs.out_data)
			return false;

		if(interval_size != rhs.interval_size)
			return false;
		/*
        if(position.size() != rhs.position.size())
			return false;
			for(int i = 0; i < position.size(); ++i){
			if(!(position[i] == rhs.position[i]))
				return false;
		}
		*/
		return true;
	}

	vector<One> out_data;
	int interval_size;

	InterpolatePosition();
	~InterpolatePosition();

	void Interpolate(vector<T>& position,float delta,bool cycled,bool use_linear=false);
	void InterpolateFakeConst(vector<T>& position,float delta,bool cycled);

	bool Save(Saver& saver) const;
	void SaveFixed(Saver& saver) const;

	T GetTestValue(int num)
	{
		for(int i=0;i<out_data.size();i++)
		{
			One& o=out_data[i];
			if(o.interval_begin<=num && o.interval_begin+o.interval_size>num)
			{
				float t=(num-o.interval_begin)/(float)o.interval_size;
				t-=0.5f;
				return o.Calc(t);
			}
		}

		xassert(0);
		return out_data[0].Calc(0);
	}
protected:
	bool cycled;
	void Add(One& one);
	void AddConstant(float delta,int min_interval);
	void AddCubic(float delta);
	void AddCubic2(float delta);
	void AddUnknownConstant(float delta);
	void AddLinear(float delta);
};

template<class T>
InterpolatePosition<T>::InterpolatePosition()
{
	cycled=false;
	interval_size = 0;
}

template<class T>
InterpolatePosition<T>::~InterpolatePosition()
{
}
template<class T>
void InterpolatePosition<T>::Interpolate(vector<T>& position_,float delta,bool cycled_,bool use_linear)
{
	cycled=cycled_;
	position=position_;
	if(position.empty())
	{
		xassert(0 && !position.empty());
		return;
	}

	One all;
	all.itpl=ITPL_UNKNOWN;
	all.interval_begin=0;
	all.interval_size=(int)position.size();
	out_data.push_back(all);
	AddConstant(delta,8);
	if(use_linear)
		AddLinear(delta);
	else
		AddCubic(delta);
	AddConstant(delta,2);
	AddUnknownConstant(delta);

	int begin=0;
	for(int i=0;i<out_data.size();i++)
	{
		One& cur=out_data[i];
		xassert(begin==cur.interval_begin);
		xassert(cur.interval_size>0);
		begin+=cur.interval_size;
	}
	xassert(begin==position.size());
}

template<class T>
void InterpolatePosition<T>::InterpolateFakeConst(vector<T>& position_,float delta,bool cycled_)
{
	cycled=cycled_;
	position=position_;
	if(position.empty())
	{
		xassert(0 && !position.empty());
		return;
	}

	One all;
	all.itpl=ITPL_CONSTANT;
	all.interval_begin=0;
	all.interval_size=position.size();
	
	T beginPosition = position[0];
	for (int i=all.interval_begin+1; i<all.interval_begin+all.interval_size; i++)
	{
		T currentPosition = position[i];
		if (currentPosition.distance(beginPosition) > delta)
		{
			all.itpl = ITPL_UNKNOWN;
			break;
		}
	}
	out_data.push_back(all);
	
	//AddConstant(delta,2);

	int begin=0;
	for(int i=0;i<out_data.size();i++)
	{
		One& cur=out_data[i];
		xassert(begin==cur.interval_begin);
		xassert(cur.interval_size>0);
		begin+=cur.interval_size;
	}
	xassert(begin==position.size());
}

template<class T>
void InterpolatePosition<T>::Add(One& one)
{
	for(int i=0;i<out_data.size();i++)
	{
		One& cur=out_data[i];
		if(cur.interval_begin<=one.interval_begin && 
			cur.interval_begin+cur.interval_size>one.interval_begin)
		{
			xassert(cur.interval_begin+cur.interval_size>=one.interval_begin+one.interval_size);
			xassert(cur.itpl==ITPL_UNKNOWN);

			if(cur.interval_begin==one.interval_begin && cur.interval_size==one.interval_size)
			{//replace
				cur=one;
				return;
			}

			if(cur.interval_begin==one.interval_begin && cur.interval_size>one.interval_size)
			{//left
				One next=cur;
				next.interval_begin=one.interval_begin+one.interval_size;
				next.interval_size=cur.interval_size-one.interval_size;
				cur=one;
				out_data.insert(out_data.begin()+i+1,next);
				return;
			}

			xassert(cur.interval_begin<one.interval_begin);
			xassert(cur.interval_size>one.interval_size);
			if(cur.interval_begin+cur.interval_size==one.interval_begin+one.interval_size)
			{//right
				cur.interval_size=one.interval_begin-cur.interval_begin;
				out_data.insert(out_data.begin()+i+1,one);
				return;
			}
			
			//middle
			xassert(cur.interval_begin+cur.interval_size>one.interval_begin+one.interval_size);

			One next=cur;
			next.interval_begin=one.interval_begin+one.interval_size;
			next.interval_size=cur.interval_begin+cur.interval_size-(one.interval_begin+one.interval_size);
			cur.interval_size=one.interval_begin-cur.interval_begin;
			out_data.insert(out_data.begin()+i+1,one);
			out_data.insert(out_data.begin()+i+2,next);
			return;
		}
	}

	xassert(0 && "InterpolatePosition::Add");
}

template<class T>
void InterpolatePosition<T>::AddConstant(float delta,int min_interval)
{
	int size=(int)position.size();
	for(int iout=0;iout<out_data.size();iout++)
	{
		One one=out_data[iout];
		int size=one.interval_size;
		int iend=one.interval_begin+one.interval_size;

		if(one.itpl==ITPL_UNKNOWN)
		for(int ibegin=one.interval_begin;ibegin<iend;)
		{
			T begin=position[ibegin];
			T cur;
			if(ibegin+min_interval < iend)
			{
				cur=position[ibegin+min_interval-1];
			}else
			{
				cur=position[iend-1];
			}
			if (cur.distance(begin) > delta)
			{
				ibegin++;
				continue;
			}

			int interval_size=1;
			for(int j=ibegin+1;j<iend;j++)
			{
				T cur=position[j];
				float dt=cur.distance(begin);
				if(dt<delta)
				{
					interval_size++;
				}else
				{
					break;
				}
			}

			if(interval_size>=min_interval)
			{
				One one;
				one.itpl=ITPL_CONSTANT;
				one.interval_begin=ibegin;
				one.interval_size=interval_size;
				one.a0=begin;//Потом на среднее по интервалу заменить
				for(int ione=0;ione<T::size;ione++)
				{
					one.a1[ione]=
					one.a2[ione]=
					one.a3[ione]=0;
				}

				Add(one);
				ibegin+=interval_size;
				break;
			}

			ibegin++;
		}
	}
}

template<class T>
void InterpolatePosition<T>::AddCubic(float delta)
{
	const int nsub=T::size;
	int isub;
	for(int iout=0;iout<out_data.size();iout++)
	{
		One one=out_data[iout];
		int size=one.interval_size;
		int iend=one.interval_begin+one.interval_size;
		if(one.itpl==ITPL_UNKNOWN)
		if(one.interval_size<=1)
		{//linear
/*		Уже не нужно, так как в сплайнах на 1 точку больше стало.
			if(one.interval_size==2)
			{
				One out;
				int ibegin=one.interval_begin;
				out.itpl=ITPL_LINEAR;
				out.interval_begin=ibegin;
				out.interval_size=one.interval_size;
				T p0=position[ibegin];
				T p1=position[ibegin+1];
				
				for(isub=0;isub<nsub;isub++)
				{
					out.a2[isub]=out.a3[isub]=0;
					out.a0[isub]=(p1[isub]+p0[isub])*0.5f;
					out.a1[isub]=p1[isub]-p0[isub];
				}

				Add(out);
				ibegin+=out.interval_size;
			}
*/
		}else
		{//cubic
			for(int ibegin=one.interval_begin;ibegin<iend;)
			{
				int add=min(3,size);
				int interval_size=add-1;
				vector<float> xyz[nsub];

				for(int j=0;j<add;j++)
				{
					T begin=position[ibegin+j];
					for(isub=0;isub<nsub;isub++)
					{
						xyz[isub].push_back(begin[isub]);
					}
				}

				One prev_one;
				for(j=ibegin+add;j<=iend;j++)
				{
					One o;
					T cur;
					if(j<position.size())
						cur=position[j];
					else
					{
						if(cycled)
						{
							cur=position.front();
							//Для кватернионов
							T rot_prev=position.back();
							T cur_inv;
							for(isub=0;isub<nsub;isub++)
								cur_inv[isub]=-cur[isub];
							float d=rot_prev.distance(cur);
							float d_inv=rot_prev.distance(cur_inv);
							if(d_inv<d)
								cur=cur_inv;
						}else
							cur=position.back();
					}

					for(isub=0;isub<nsub;isub++)
						xyz[isub].push_back(cur[isub]);


					for(isub=0;isub<nsub;isub++)
						CalcLagrange(xyz[isub],o.a0[isub],o.a1[isub],o.a2[isub],o.a3[isub]);

					float dt=0;
					for(int k=0;k<xyz[0].size();k++)
					{
						float t=(k/(float)(xyz[0].size()-1)-0.5f);
						T pos=o.Calc(t);
						T posx;
						for(isub=0;isub<nsub;isub++)
							posx[isub]=xyz[isub][k];

						dt+=pos.distance(posx);
					}
					dt/=xyz[0].size();
					if(dt<delta)
					{
						prev_one=o;
						interval_size++;
					}else
					{
						break;
					}
				}

				if(interval_size==2)
				{
					One out;
					out.itpl=ITPL_LINEAR;
					out.interval_begin=ibegin;
					out.interval_size=interval_size;
					T p0=position[ibegin];
					T p1=position[ibegin+1];
					
					for(isub=0;isub<nsub;isub++)
					{
						out.a2[isub]=out.a3[isub]=0;
						out.a0[isub]=(p1[isub]+p0[isub])*0.5f;
						out.a1[isub]=p1[isub]-p0[isub];
					}

					prev_one=out;
				}

				if(interval_size>1)
				{
					One one=prev_one;
					bool is_linear=true;
					for(isub=0;isub<nsub;isub++)
					{
						if(fabs(one.a2[isub])>delta || fabs(one.a3[isub])>delta)
							is_linear=false;
					}

					one.itpl=is_linear?ITPL_LINEAR:ITPL_SPLINE;
					one.interval_begin=ibegin;
					one.interval_size=interval_size;
					Add(one);
					ibegin+=interval_size;
					break;
				}

				ibegin++;
			}
		}
	}
}
template<class T>
void InterpolatePosition<T>::AddCubic2(float delta)
{
	const int nsub=T::size;
	int isub;
	bool end = false;
	for(int iout=0;iout<out_data.size();iout++)
	{
		One prev_one;
		One one=out_data[iout];
		int size=one.interval_size;
		int iend=one.interval_begin+one.interval_size;
		int ibegin = one.interval_begin;
		int oldend = iend;
		int interval = 3;
		int rypos = ibegin;
		int rnpos = iend;
		if(one.itpl==ITPL_UNKNOWN)
		if(one.interval_size>1)
		{//cubic
			
			int interval_size = 1;
			while (interval <= size)
			{
				One o;
				T cur;
				vector<float> xyz[nsub];
				for(int j=ibegin;j<ibegin+interval;j++)
				{
					T begin=position[j];
					for(isub=0;isub<nsub;isub++)
					{
						xyz[isub].push_back(begin[isub]);
					}
				}
				if((ibegin+interval) <position.size())
					cur = position[ibegin+interval];
				else
				{
					if(cycled)
					{
						cur=position.front();
						//Для кватернионов
						T rot_prev=position.back();
						T cur_inv;
						for(isub=0;isub<nsub;isub++)
							cur_inv[isub]=-cur[isub];
						float d=rot_prev.distance(cur);
						float d_inv=rot_prev.distance(cur_inv);
						if(d_inv<d)
							cur=cur_inv;
					}else
						cur=position.back();
				
				}
				for(isub=0;isub<nsub;isub++)
				{
					xyz[isub].push_back(cur[isub]);
				}

				for(isub=0;isub<nsub;isub++)
					CalcLagrange(xyz[isub],o.a0[isub],o.a1[isub],o.a2[isub],o.a3[isub]);

				float dt=0;
				for(int k=0;k<xyz[0].size();k++)
				{
					float t=(k/(float)(xyz[0].size()-1)-0.5f);
					T pos=o.Calc(t);
					T posx;
					for(isub=0;isub<nsub;isub++)
						posx[isub]=xyz[isub][k];

					dt+=pos.distance(posx);
				}
				dt/=xyz[0].size();
				iter++;
				if(dt<delta)
				{
					prev_one = o;
					interval_size=interval;
					rypos = ibegin+interval;
				}else
				{
					rnpos = ibegin+(interval<=size?interval:size);
					iend = rnpos - (rnpos-rypos)/2;
					break;
				}
				interval *= 4;
			}

			if(interval_size > 1)
			while (abs(rnpos - rypos) > 1)
			{
				One o;
				T cur;
				vector<float> xyz[nsub];
				
				for(int j=ibegin;j<iend;j++)
				{
					T begin=position[j];
					for(isub=0;isub<nsub;isub++)
					{
						xyz[isub].push_back(begin[isub]);
					}
				}

				if(iend <position.size())
					cur = position[iend];
				else
				{
					if(cycled)
					{
						cur=position.front();
						//Для кватернионов
						T rot_prev=position.back();
						T cur_inv;
						for(isub=0;isub<nsub;isub++)
							cur_inv[isub]=-cur[isub];
						float d=rot_prev.distance(cur);
						float d_inv=rot_prev.distance(cur_inv);
						if(d_inv<d)
							cur=cur_inv;
					}else
						cur=position.back();
				
				}

				for(isub=0;isub<nsub;isub++)
				{
					xyz[isub].push_back(cur[isub]);
				}


				for(isub=0;isub<nsub;isub++)
					CalcLagrange(xyz[isub],o.a0[isub],o.a1[isub],o.a2[isub],o.a3[isub]);

				float dt=0;
				for(int k=0;k<xyz[0].size();k++)
				{
					float t=(k/(float)(xyz[0].size()-1)-0.5f);
					T pos=o.Calc(t);
					T posx;
					for(isub=0;isub<nsub;isub++)
						posx[isub]=xyz[isub][k];

					dt+=pos.distance(posx);
				}
				dt/=xyz[0].size();
				iter++;
				if(dt<delta)
				{
					prev_one = o;
					rypos = iend;
					interval_size=iend-ibegin;
					iend += (rnpos-iend)/2;
				}else
				{
					rnpos = iend;
					iend -= (iend-rypos)/2;
				}
			}
			if (interval_size > 1)
			{
				One one=prev_one;
				bool is_linear=true;
				for(isub=0;isub<nsub;isub++)
				{
					if(fabs(one.a2[isub])>delta || fabs(one.a3[isub])>delta)
						is_linear=false;
				}

				one.itpl=is_linear?ITPL_LINEAR:ITPL_SPLINE;
				one.interval_begin=ibegin;
				one.interval_size=interval_size;
				Add(one);
				ibegin+=interval_size;
			}
			
		}
	}
}

template<class T>
void InterpolatePosition<T>::AddUnknownConstant(float delta)
{
	const int nsub=T::size;
	bool fixed;

	do
	{
		fixed=false;
		for(int iout=0;iout<out_data.size();iout++)
		{
			One one=out_data[iout];
			if(one.itpl==ITPL_UNKNOWN)
			{
				One out;
				out.itpl=ITPL_CONSTANT;
				out.interval_begin=one.interval_begin;
				out.interval_size=1;
				out.a0=position[one.interval_begin];
				for(int ione=0;ione<T::size;ione++)
				{
					out.a1[ione]=
					out.a2[ione]=
					out.a3[ione]=0;
				}
				Add(out);
				fixed=true;
				break;
			}
		}
	}while(fixed);
}

template<class T>
void InterpolatePosition<T>::SaveFixed(Saver& saver) const
{

	for(int i = 0; i < out_data.size(); i++){
		const One& o = out_data[i];
		saver << int(o.itpl);
		saver << float(o.interval_begin) / float(interval_size);
		saver << 1.0f / (float(o.interval_size) / float(interval_size));

	    const int nsub = T::size;
		switch(o.itpl){
		case ITPL_CONSTANT:
			for(int j = 0; j < nsub; ++j)
				saver << o.a0[j];

            for(int j = 0; j < nsub * 3; ++j)
				saver << 0.0f;
			break;
		case ITPL_LINEAR:
			for(int j = 0; j < nsub; ++j)
				saver << o.a0[j] << o.a1[j];

			for(int j = 0; j < nsub * 2; ++j)
				saver << 0.0f;
			break;
		case ITPL_SPLINE:
			for(int j = 0; j < nsub; ++j)
				saver << o.a0[j] << o.a1[j] << o.a2[j] << o.a3[j];
			break;
		}

		/*

		switch(o.itpl){
		case ITPL_CONSTANT:
			for(int j = 0; j < 1; ++j)
				saver << o.a0[j] << o.a1[j] << o.a2[j] << o.a3[j];

            for(int j = 0; j < 3; ++j)
				saver << 0.0f << 0.0f << 0.0f << 0.0f;
			break;
		case ITPL_LINEAR:
			for(int j = 0; j < 2; ++j)
				saver << o.a0[j] << o.a1[j] << o.a2[j] << o.a3[j];

            for(int j = 0; j < 2; ++j)
				saver << 0.0f << 0.0f << 0.0f << 0.0f;
			break;
		case ITPL_SPLINE:
			for(int j = 0; j < 4; ++j)
				saver << o.a0[j] << o.a1[j] << o.a2[j] << o.a3[j];
			break;
		}

		*/




		/*
		for(int j = 0; j < nsub; ++j)
			saver << o.a0[j];
		for(int j = 0; j < nsub; ++j)
			saver << o.a1[j];
		for(int j = 0; j < nsub; ++j)
			saver << o.a2[j];
		for(int j = 0; j < nsub; ++j)
			saver << o.a3[j];
			*/
	}
}

template<class T>
bool InterpolatePosition<T>::Save(Saver& saver) const
{
	bool ok=true;
	const int nsub=T::size;
	saver<<(int)out_data.size();
	for(int i=0;i<out_data.size();i++)
	{
		const One& o=out_data[i];
		saver << (int)o.itpl;
		saver << float(o.interval_begin) / float(interval_size);
		saver << float(o.interval_size) / float(interval_size);
		switch(o.itpl)
		{
		case ITPL_SPLINE:
			{
				for(int i=0;i<nsub;i++)
				{
					saver<<o.a0[i]<<o.a1[i]<<o.a2[i]<<o.a3[i];
				}
			}
			break;
		case ITPL_LINEAR:
			{
				for(int i=0;i<nsub;i++)
				{
					saver<<o.a0[i]<<o.a1[i];
				}
			}
			break;
		case ITPL_CONSTANT:
			{
				for(int i=0;i<nsub;i++)
				{
					saver<<o.a0[i];
				}
			}
			break;
		default:
			ok=false;
			break;
		}
	}

	return ok;
}

template<class T>
void InterpolatePosition<T>::AddLinear(float delta)
{
	const int nsub=T::size;
	int isub;
	for(int iout=0;iout<out_data.size();iout++)
	{
		One one=out_data[iout];
		int size=one.interval_size;
		int iend=one.interval_begin+one.interval_size;
		if(one.itpl==ITPL_UNKNOWN)
		{//cubic
			for(int ibegin=one.interval_begin;ibegin<iend;)
			{
				int add=min(3,size);
				int interval_size=add-1;
				vector<float> xyz[nsub];

				for(int j=0;j<add;j++)
				{
					T begin=position[ibegin+j];
					for(isub=0;isub<nsub;isub++)
					{
						xyz[isub].push_back(begin[isub]);
					}
				}

				One prev_one;
				for(j=ibegin+add;j<=iend;j++)
				{
					One o;
					T cur;
					if(j<position.size())
						cur=position[j];
					else
					{
						if(cycled)
						{
							cur=position.front();
							//Для кватернионов
							T rot_prev=position.back();
							T cur_inv;
							for(isub=0;isub<nsub;isub++)
								cur_inv[isub]=-cur[isub];
							float d=rot_prev.distance(cur);
							float d_inv=rot_prev.distance(cur_inv);
							if(d_inv<d)
								cur=cur_inv;
						}else
							cur=position.back();
					}

					for(isub=0;isub<nsub;isub++)
						xyz[isub].push_back(cur[isub]);


					for(isub=0;isub<nsub;isub++)
					{
						CalcLinear(xyz[isub],o.a0[isub],o.a1[isub]);
						o.a2[isub]=0;
						o.a3[isub]=0;
					}

					float dt=0;
					for(int k=0;k<xyz[0].size();k++)
					{
						float t=(k/(float)(xyz[0].size()-1)-0.5f);
						T pos=o.Calc(t);
						T posx;
						for(isub=0;isub<nsub;isub++)
							posx[isub]=xyz[isub][k];

						dt+=pos.distance(posx);
					}
					dt/=xyz[0].size();

					if(dt<delta)
					{
						prev_one=o;
						interval_size++;
					}else
					{
						break;
					}
				}

				if(interval_size>1)
				{
					One one=prev_one;
					bool is_linear=true;
					for(isub=0;isub<nsub;isub++)
					{
						if(fabs(one.a2[isub])>delta || fabs(one.a3[isub])>delta)
							is_linear=false;
					}
					xassert(is_linear);

					one.itpl=is_linear?ITPL_LINEAR:ITPL_SPLINE;
					one.interval_begin=ibegin;
					one.interval_size=interval_size;
					Add(one);
					ibegin+=interval_size;
					break;
				}

				ibegin++;
			}
		}
	}
}
