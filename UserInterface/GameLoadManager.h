#ifndef __GAME_LOAD_MANAGER_H__
#define __GAME_LOAD_MANAGER_H__

#include "XTL\Handle.h"
#include "Render\Src\VisError.h"

typedef void (*RedrawFunction)(void);

class LoadManager
{
	struct Part{
		Part(float b, float n) : base(b), next(n) {}
		float base;
		float next;
	};
	typedef vector<Part> Parts;

public:
	LoadManager() {
		fRedraw_ = 0;
		init();
	}

	void init(){
		xassert(started_ == false);
		if(!started_){
			parts_.clear();
			parts_.push_back(Part(0.f, 1.f));
			progressCurrent_ = 0.f;
			callBackCount_ = callBacks_ = 0;
			startTime_ = GetTickCount();
			if(debug_)
				dprintf("Game load inited\n");
		}
	}

	void startLoad(bool debug = false, RedrawFunction redraw = 0){
		xassert(!started_);
		debug_ = debug;
		fRedraw_ = redraw;
		init();
		started_ = true;
	}

	void finishLoad(){
		xassert(started_);
		if(parts_.size() > 1)
			parts_.erase(parts_.begin() + 1, parts_.end());
		setProgress(1.f);
		started_ = false;
	}
	
	void setProgress(float val){
		if(started_){
			float oldProgress = progressCurrent_;
			progressCurrent_ = getRealProgress(val);
			xassert(progressCurrent_ >= oldProgress);
			if(progressCurrent_ - oldProgress > 0.01f){
				if(debug_)
					dprintf("%.2f\t%.1f\n", 0.001f * (GetTickCount() - startTime_), progressCurrent_ * 100);
				showProgress();
			}
		}
	}

	void setProgressAndStartSub(float cur, float next){
		if(started_){
			setProgress(cur);
			parts_.push_back(Part(progress(), getRealProgress(next)));
		}
	}

	void finishSub(){
		if(started_){
			setProgress(1.f);
			xassert(parts_.size() > 1);
			parts_.pop_back();
		}
	}

	void finishAndStartSub(float next){
		if(started_){
			setProgress(1.f);
			xassert(parts_.size() > 1);
			parts_.pop_back();
			parts_.push_back(Part(progressCurrent_, getRealProgress(next)));
		}
	}

	void setCallBackCount(int val){
		if(started_){
			callBacks_ = 0;
			callBackCount_ = val;
		}
	}

	void callBack(){
		if(started_){
			xassert(callBackCount_ > 0 && callBacks_ <= callBackCount_);
			setProgress((float)callBacks_++ / (float)callBackCount_);
		}
	}

	float progress() const { return progressCurrent_; }

private:
	bool debug_;
	bool started_;
	
	float progressCurrent_;

	int callBackCount_;
	int callBacks_;

	Parts parts_;

	DWORD startTime_;

	float getRealProgress(float relative){
		xassert(!parts_.empty());
		Part part = parts_.back();
		return part.base + (part.next - part.base) * clamp(relative, 0.f, 1.f);
	}

	RedrawFunction fRedraw_;

	void showProgress() { 
		if(fRedraw_)
			(*fRedraw_)();
	}
};

typedef Singleton<LoadManager> GameLoadManager;

#endif //__GAME_LOAD_MANAGER_H__
