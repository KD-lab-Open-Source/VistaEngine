#ifndef __FLASH_SOURCE_H_INCLUDED__
#define __FLASH_SOURCE_H_INCLUDED__

#include "SourceBase.h"

class Archive;

class SourceFlash : public SourceBase{
public:

	enum EvolutionType{
		LINEAR,
		EXPONENTIAL
	};

	struct EvolutionPrm{
		bool increase_;
		float time_;
		EvolutionType type_;
		bool vipuclaya_;
		float naklon_;
		
		EvolutionPrm(){
			increase_ = true;
			time_ = 1.f;
			type_ = EXPONENTIAL;
			vipuclaya_ = true;
			naklon_ = 2.f;
			expb2_ = scale_ = 1.f;
		}
		
		void serialize(Archive &ar);
		
		void init() const;
		float operator()(float phase);
		
	private:
		// не сериализуются, используются для временный значений
		mutable float expb2_;
		mutable float scale_;

		float _exp(float _x) const{
			float x = _x + naklon_;
			return (exp(x*x) - expb2_) * scale_;
		}
	};

	SourceFlash();
	SourceFlash(const SourceFlash& original);
	SourceBase* clone () const {
		return new SourceFlash(*this);
	}
	~SourceFlash();

	SourceType type() const { return SOURCE_FLASH; }

	void quant();

	void serialize(Archive &ar);

	void showEditor() const;

protected:
	void start();
	void stop();

private:
	bool autoKill_;
	
	Color4c color_;
	float intensive_;

	bool decByDistance_;
	int maxDistance_;

	EvolutionPrm increase_;
	EvolutionPrm decrease_;

	InterpolationLogicTimer phase_;
	LogicTimer inreaseTime_;
};

#endif // #ifndef __FLASH_SOURCE_H_INCLUDED__

