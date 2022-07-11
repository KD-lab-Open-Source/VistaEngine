#ifndef __CLASS_CREATOR_FACTORY_H_INCLUDED__
#define __CLASS_CREATOR_FACTORY_H_INCLUDED__

#include "Handle.h"
#include "Factory.h"
#include <typeinfo>

typedef std::vector<std::string> ComboStrings;
typedef std::vector<int> ComboInts;

inline std::string getComboToken (const char*& buffer)
{
	while(*buffer == ' ' || *buffer == '|')
		++buffer;
	const char* marker = buffer;
	while(*buffer != '|' && *buffer != 0)
		++buffer;
	
	// так по идее быстрее, но нет времени проверять
	//const char* end = buffer;
	//while(market != end && --end == ' ');
	//return string(marker, buffer - end);

	string str(marker, buffer - marker);
	while(!str.empty() && str[str.size() - 1] == ' ')
		str.erase(str.end() - 1);
	return str;
}

class StaticString{
public:
	StaticString(const char* str = "") : str_(str) {}
	operator const char*()const{ return str_; }
	const char* c_str()const{ return str_; }
	bool operator==(const char* rhs) const{
		return strcmp(str_, rhs) == 0;
	}
	bool operator!=(const char* rhs) const{
		return strcmp(str_, rhs) != 0;
	}
	bool operator<(const char* rhs) const{
		return strcmp(str_, rhs) < 0;
	}
	bool operator>(const char* rhs) const{
		return strcmp(str_, rhs) > 0;
	}
private:
	const char* str_;
};

template<class Base>
class ClassCreatorFactory : public Factory<StaticString, Base > {
public:

    struct ClassCreatorBase : CreatorBase {
        void serialize (Archive&) {}
        virtual const char* name () {
            return name_;
        }
        virtual const char* nameAlt () {
            return nameAlt_;
        }
        virtual const type_info& type_info () = 0;
    protected:
        const char* name_;
        const char* nameAlt_;
    };

    template<class Derived>
      struct ClassCreator : ClassCreatorBase {
          ClassCreator (const char* id, const char* nameAlt) {
              name_ = id;
              nameAlt_ = nameAlt;
              instance ().add (id, nameAlt, *this);
          }
          virtual Base* create () const {
              return ObjectCreator<Base, Derived>::create ();
              // return new Derived;
          }
          virtual const ::type_info& type_info () {
              return typeid(Derived);
          }
      };

    static ClassCreatorFactory& instance () {
        return Singleton<ClassCreatorFactory>::instance ();
    }

    void add(const char* name, const char* nameAlt, ClassCreatorBase& creator_op) {
        if(creators_.find(name) == creators_.end()){
            if(!comboList_.empty())
                comboList_ += "|";
            if(!comboListAlt_.empty())
                comboListAlt_ += "|";
            comboList_ += name;
            comboListAlt_ += nameAlt;
			comboStrings_.push_back(name);
			comboStringsAlt_.push_back(nameAlt);
            nameToNameAltMap_ [name] = nameAlt;
			creatorsByIndex_.push_back(&creator_op);
            __super::add(name, creator_op);
        } else {
            XBuffer msg;
            msg
              < "Попытка повторной регистрации класса\n"
              < name < " (" < nameAlt < ")\n"
			  < "в фабрике\nClassCreatorFactory<"
              < typeid(Base).name()
              < ">";

            xxassert(0, msg);
        }
    }

    ClassCreatorBase& find (const char* name) {
        return static_cast <ClassCreatorBase&>(*creators_ [name]);
    }

    ClassCreatorBase& find (const Base* ptr) {
        const char* name = typeid (*ptr).name ();
        return static_cast <ClassCreatorBase&>(*creators_ [name]);
    }

    const char* nameByNameAlt(const char* nameAlt) const{
		int sz = comboStringsAlt_.size();
		for(int i = 0; i < sz; ++i){
			if(comboStringsAlt_[i] == nameAlt)
				return comboStrings_[i].c_str();
		}
		return 0;
    }

    const char* nameAlt(const char* name, bool silent = false) {
        Map::iterator it = nameToNameAltMap_.find(name);
		if(it != nameToNameAltMap_.end())
			return it->second.c_str();
        if(silent)
            return name;
        else{
            xassert (0 && "No translation for such class name!");
            return 0;
        }
    }
    int typeIndexByName(const char* name, bool silent = false) {
		for(int i = 0; i < comboStrings_.size(); ++i)
			if(comboStrings_[i] == name)
				return i;
		if(!silent)
			xassert (0 && "No type registered with such name!");
		return -1;
    }
    const char* comboList () {
        return comboList_.c_str ();
    }
    const char* comboListAlt () {
        return comboListAlt_.c_str ();
    }
	const ComboStrings& comboStrings() const {
		return comboStrings_;
	}
	const ComboStrings& comboStringsAlt() const {
		return comboStringsAlt_;
	}

    ClassCreatorBase& findByIndex(int index) {
        xassert(index >= 0 && index < comboStrings_.size());
		const char* name = comboStrings_[index].c_str();
        return find(name);
    } 

    Base* createByIndex(int index/*, bool quiet = false*/){
		if(index >= 0 && index < creatorsByIndex_.size())
			return creatorsByIndex_[index]->create();
		else{
			xassert(index >= 0 && index < creatorsByIndex_.size());
			return 0;
		}
    } 

    int size() const{
        return creators_.size ();
    }
private:
    typedef StaticMap<StaticString, StaticString> Map;
    Map nameToNameAltMap_;

	typedef std::vector<CreatorBase*> CreatorsByIndex;
	CreatorsByIndex creatorsByIndex_;

    std::string comboList_;
    std::string comboListAlt_;
	ComboStrings comboStrings_;
	ComboStrings comboStringsAlt_;

    ClassCreatorFactory(){
    }

    friend Singleton<ClassCreatorFactory>;
};

#endif
