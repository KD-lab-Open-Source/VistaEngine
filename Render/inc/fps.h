#ifndef __FPS_H_INCLUDED__
#define __FPS_H_INCLUDED__

class FPS
{
	list<double> tiks;
	int max_miliseconds;
public:
	FPS(int _max_miliseconds=2000)
	{
		max_miliseconds=_max_miliseconds;
	}

	void quant()
	{
		double t=xclock();

		tiks.push_back(t);
		int sz=tiks.size();
		if(sz<5)
			return;

		while(!tiks.empty())
		{
			if(sz<6)
				return;
			double num=tiks.back()-tiks.front();
			if(num>max_miliseconds)
			{
				tiks.pop_front();
				sz--;
			}else
				break;
		}
	}

	float GetFPS()
	{
		int sz=tiks.size();
		if(sz<5)
			return 0;
		double num=tiks.back()-tiks.front();
		return ((sz-1)*1000.0f)/(num + FLT_EPS);
	}

	void clear()
	{
		tiks.clear();
	}

	bool is_precisely()
	{
		if(tiks.empty())
			return false;
		return (tiks.back()-tiks.front())>=max_miliseconds*0.8;
	}

	void GetFPSminmax(float& fpsmin,float& fpsmax)
	{
		int sz=tiks.size();
		if(sz<5)
		{
			fpsmin=fpsmax=0;
			return;
		}

		double prevt=tiks.front();
		bool first=true;
		list<double>::iterator it;
		it=tiks.begin();
		++it;
		while(it!=tiks.end())
		{
			double t=*it;
			double deltat=max(t-prevt,0.1);
			float fps=1000/(deltat + FLT_EPS);

			if(first)
			{
				first=false;
				fpsmin=fpsmax=fps;
			}else
			{
				fpsmin=min(fpsmin,fps);
				fpsmax=max(fpsmax,fps);
			}

			prevt=t;
			++it;
		}
	}
};

class FPSInterval
{
	struct TWO
	{
		double begin_time;
		double end_time;
	};
	list<TWO> tiks;
	int max_miliseconds;
public:
	FPSInterval(int _max_miliseconds=2000)
	{
		max_miliseconds=_max_miliseconds;
	}

	void quant_begin(double time)
	{
		TWO t;
		t.begin_time=time;
		t.end_time=-1;

		tiks.push_back(t);
		int sz=tiks.size();
		if(sz<5)
			return;

		while(!tiks.empty())
		{
			if(sz<6)
				return;
			double num=tiks.back().begin_time-tiks.front().begin_time;
			if(num>max_miliseconds)
			{
				tiks.pop_front();
				sz--;
			}else
				break;
		}
	}

	void quant_end(double time)
	{
		if(!tiks.empty())
		{
			tiks.back().end_time=time;
		}
	}

	float GetFPS()
	{
		int sz=tiks.size();
		if(sz<5)
			return 0;
		double num=tiks.back().begin_time-tiks.front().begin_time;
		return ((sz-1)*1000.0f)/(num + FLT_EPS);
	}

	void clear()
	{
		tiks.clear();
	}

	bool is_precisely()
	{
		if(tiks.empty())
			return false;
		return (tiks.back().begin_time-tiks.front().begin_time)>=max_miliseconds*0.8;
	}

	void GetFPSminmax(float& fpsmin,float& fpsmax)
	{
		int sz=tiks.size();
		if(sz<5)
		{
			fpsmin=fpsmax=0;
			return;
		}

		double prevt=tiks.front().begin_time;
		bool first=true;
		list<TWO>::iterator it;
		it=tiks.begin();
		++it;
		while(it!=tiks.end())
		{
			double t=it->begin_time;
			double deltat=max(t-prevt,0.1);
			float fps=1000/(deltat + FLT_EPS);

			if(first)
			{
				first=false;
				fpsmin=fpsmax=fps;
			}else
			{
				fpsmin=min(fpsmin,fps);
				fpsmax=max(fpsmax,fps);
			}

			prevt=t;
			++it;
		}
	}

	void GetInterval(float& imid,float& imin,float& imax)
	{
		imid=imin=imax=0;
		int sz=tiks.size();
		if(sz<5)
		{
			return;
		}

		double prevt=tiks.front().begin_time;
		double prevdt=tiks.front().end_time-tiks.front().begin_time;
		bool first=true;
		list<TWO>::iterator it;
		it=tiks.begin();
		++it;
		while(it!=tiks.end())
		{
			double t=it->begin_time;
			double dt=it->end_time-it->begin_time;
			double deltat=max(t-prevt,0.1);
			float fps=prevdt/(deltat + FLT_EPS);
			imid+=fps;

			if(first)
			{
				first=false;
				imin=imax=fps;
			}else
			{
				imin=min(imin,fps);
				imax=max(imax,fps);
			}

			prevt=t;
			prevdt=dt;
			++it;
		}

		imid/=(sz-1);
	}
};

#endif
