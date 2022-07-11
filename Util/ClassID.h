#ifndef __CLASSID_H__
#define __CLASSID_H__

class Archive;

/// Идентификатор класса.
/**
Состоит из двух частей - идентификатора базового класса (значение enum \c ClassTypeEnum)
и счётчика - идентификатора производного класса.
*/
template<class ClassTypeEnum>
class ClassID
{
public:
	ClassID(ClassTypeEnum type_id) : typeID_(type_id), counter_(0) { }
	ClassID(ClassTypeEnum type_id, unsigned counter) : typeID_(type_id), counter_(counter) { }

	ClassTypeEnum typeID() const { return typeID_; }
	unsigned counter() const { return counter_; }

	bool operator < (const ClassID<ClassTypeEnum>& id) const {
		return (typeID_ < id.typeID_) ? true : ((typeID_ > id.typeID_) ? false : counter_ < id.counter_);
	}

	bool operator == (const ClassID<ClassTypeEnum>& id) const { return (typeID_ == id.typeID_ && counter_ == id.counter_); }

	void serialize(Archive& ar) {
		ar.serialize(typeID_, "typeID", "typeID");
		ar.serialize(counter_, "counter", "counter");
	}

private:
	ClassTypeEnum typeID_;
	int counter_;
};

#endif //__CLASSID_H__
