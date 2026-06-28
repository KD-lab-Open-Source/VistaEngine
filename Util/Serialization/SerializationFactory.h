#ifndef __CLASS_CREATOR_FACTORY_H_INCLUDED__
#define __CLASS_CREATOR_FACTORY_H_INCLUDED__

#include "Handle.h"
#include "Factory.h"
#include "ComboStrings.h"

///////////////////////////////////////////////////////////////////////////////////
//					SerializationFactory 
//		�� ������������ ����, � ����� FactorySelector<BaseType>
///////////////////////////////////////////////////////////////////////////////////
template<class BaseType, class FactoryArg>
class SerializationFactory : public Factory<StaticString, BaseType, FactoryArg>
{
    using Base = Factory<StaticString, BaseType, FactoryArg>;
public:
    struct ClassCreatorBase : Base::CreatorBase {
        virtual const char* name() {
            return name_;
        }
        virtual const char* nameAlt() {
            return nameAlt_;
        }
		int sizeOf() { return sizeOf_; }

    protected:
        const char* name_;
        const char* nameAlt_;
		int sizeOf_;
    };

    template<class Derived>
    struct ClassCreator : ClassCreatorBase {
		ClassCreator(const char* id, const char* nameAlt) {
              this->name_ = id;
              this->nameAlt_ = nameAlt;
			  this->sizeOf_ = sizeof(Derived);
              instance().add(id, nameAlt, *this);
        }
		BaseType* create() const
		{
			return instance().template createArg<Derived>();
		}
	};

    static SerializationFactory& instance() {
        return Singleton<SerializationFactory>::instance();
    }

    void add(const char* name, const char* nameAlt, ClassCreatorBase& creator_op) {
        if(this->creators_.find(name) == this->creators_.end()){
            if(!comboList_.empty())
                comboList_ += "|";
            comboList_ += name;
			comboStrings_.push_back(name);
			creatorsByIndex_.push_back(&creator_op);
            Base::add(name, creator_op);
			if(nameAlt){
				if(!comboListAlt_.empty())
					comboListAlt_ += "|";
				comboListAlt_ += nameAlt;
				comboStringsAlt_.push_back(nameAlt);
				nameToNameAltMap_ [name] = nameAlt;
			}
        }
		else{
            XBuffer msg;
            msg
              < "������� ��������� ����������� ������\n"
              < name < " (" < nameAlt < ")\n"
			  < "� �������\nSerializationFactory<"
              < typeid(BaseType).name()
              < ">";

            xxassert(0, msg);
        }
    }

    ClassCreatorBase& find(const char* name) {
        return static_cast <ClassCreatorBase&>(*this->creators_ [name]);
    }

    ClassCreatorBase& find(const BaseType* ptr) {
        const char* name = normalizeTypeName(typeid(*ptr).name());
        return static_cast <ClassCreatorBase&>(*this->creators_ [name]);
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
            xassert(0 && "No translation for such class name!");
            return 0;
        }
    }
    int typeIndexByName(const char* name, bool silent = false) {
		for(int i = 0; i < int(comboStrings_.size()); ++i)
			if(comboStrings_[i] == name)
				return i;
		if(!silent)
			xassert(0 && "No type registered with such name!");
		return -1;
    }
    const char* comboList() {
        return comboList_.c_str();
    }
    const char* comboListAlt() {
        return comboListAlt_.c_str();
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

    BaseType* createByIndex(int index){
		if(index >= 0 && index < int(creatorsByIndex_.size()))
			return creatorsByIndex_[index]->create();
		else{
			xassert(index >= 0 && index < int(creatorsByIndex_.size()));
			return 0;
		}
    } 

    int size() const { return int(this->creators_.size()); }

private:
    typedef StaticMap<StaticString, StaticString> Map;
    Map nameToNameAltMap_;

	typedef std::vector<typename Base::CreatorBase*> CreatorsByIndex;
	CreatorsByIndex creatorsByIndex_;

    std::string comboList_;
    std::string comboListAlt_;
	ComboStrings comboStrings_;
	ComboStrings comboStringsAlt_;

    SerializationFactory(){}

    friend Singleton<SerializationFactory>;
};

// ����� ����� ��������� <>, ::
#define REGISTER_CLASS(baseClass, derivedClass, classNameAlt) \
  static FactorySelector<baseClass>::Factory::ClassCreator<derivedClass > INTERNAL_UNIQUE_NAME(normalizeTypeName(typeid(derivedClass).name()), classNameAlt);

#define REGISTER_CLASS_CONVERSION(baseClass, derivedClass, classNameAlt, oldName) \
  static FactorySelector<baseClass>::Factory::ClassCreator<derivedClass > INTERNAL_UNIQUE_NAME(normalizeTypeName(oldName), classNameAlt); \
  static FactorySelector<baseClass>::Factory::ClassCreator<derivedClass > INTERNAL_UNIQUE_NAME(normalizeTypeName(typeid(derivedClass).name()), classNameAlt);


#endif
