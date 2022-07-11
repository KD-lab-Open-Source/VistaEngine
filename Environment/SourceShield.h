#ifndef __SHILD_SOURCE_H__
#define __SHILD_SOURCE_H__

#include "SourceEffect.h"
class Archive;

class SourceShield : public SourceDamage
{
public:
	SourceShield();
	SourceShield(const SourceShield& src);
	~SourceShield();
	SourceType type() const { return SOURCE_SHIELD; }
	SourceBase* clone() const { return new SourceShield(*this); }

	void quant();
	void serialize(Archive& ar);

	bool inZone(const Vect3f& coord) const{
		xxassert(height_ <= radius(), "высота защитного купола не может быть больше радиуса источника");
		return sqr(position().x - coord.x) + sqr(position().y - coord.y) + sqr(position().z - centerDeep_ - coord.z) < sqr(centerDeep_ + height_);
	}
	
	void setPose(const Se3f& pos, bool init);
	void setRadius(float _radius);

	void showDebug() const;

	bool canApply(const UnitBase* target) const;

	float height() const { return height_; }
	float sphereRadius() const { return centerDeep_ + height_; }
	Vect3f sphereCenter() const { return Vect3f(position().x, position().y, position().z - centerDeep_); }

	/** ускоренная проверка на нахождение начальной и конечной точки в связной (относительно чужих полей) области.
	при этом прямой путь может отсутствовать */
	static bool traceShieldsCoherence(const Vect3f& point_start, const Vect3f& point_finish, const Player* owner, Vect3f* intersect = 0);
	//! трассировка прямого пути между точками
	static bool traceShieldsThrough(const Vect3f& point_start, const Vect3f& point_finish, const Player* owner, Vect3f* intersect = 0, Vect3f* sphereCenter = 0);
	//! трассировка прямого пути между точками, при этом считается что точки рядом и сканируются поля только в окрестности конечной точки
	static bool traceShieldsDelta(const Vect3f& start, const Vect3f& finish, const Player* owner, Vect3f& intersection, Vect3f& center);
	//! выводит ориентированный эффект на поверхности купола
	static void shieldExplodeEffect(const Vect3f& center, const Vect3f& pos, const EffectAttribute& effect);

protected:
	void start();
	void stop();
	bool killRequest();
private:
	// высота купола над землей(центром источника), не может быть больше радиуса
	float height_;
	// глубина(смещение) центра защитной сферы от центра источника
	float centerDeep_;

	float phase_;

	void sphereInit();

	// модель купола
	cObject3dx* sphere_;
	string modelName_;
	// Цвет купола
	CKeyColor diffuseColor_;
	InterpolatorPhase interpolatorPhase_;
	InterpolatorColor interpolatorColor_;
	string textureName_;
	float activateDTime_;
	float activatePhase_;
	bool deactivate_;
	float activateSpeed_;
};


#endif //__SHILD_SOURCE_H__
