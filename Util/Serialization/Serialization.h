#ifndef __SERIALIZATION_H_INCLUDED__
#define __SERIALIZATION_H_INCLUDED__

#include <vector>
#include <list>
#include <iterator>
#include <typeinfo>
using namespace std;

#include "xUtil.h"

#include "SerializationTypes.h"
#include "ClassCreatorFactory.h"

namespace serialization_helpers{

template<bool C, class T1, class T2>
struct Selector{};

template<class T1, class T2>
struct Selector<false, T1, T2>{
	typedef T2 type;
};

template<class T1, class T2>
struct Selector<true, T1, T2>{
	typedef T1 type;
};

template<class C, class T1, class T2>
struct Select{
	typedef typename Selector<C::value, T1,T2>::type selected_type;
	typedef typename selected_type::type type;
};

template<class T>
struct Identity{
	typedef T type;
};

template<class T>
struct IsClass{
private:
	struct NoType { char dummy; };
	struct YesType { char dummy[100]; };

	template<class U>
	static YesType function_helper(void(U::*)(void));

	template<class U>
	static NoType function_helper(...);
public:
	enum{ value = (sizeof(function_helper<T>(0)) == sizeof(YesType))};
};

};

namespace std {
template<class T, class A> class list;
template<class T, class A> class vector;
template<class T1, class T2> struct pair;
}

/////////////////////////////////////////////////
#ifndef _FINAL_VERSION_
#define xassertStr(exp, str) { string s = #exp; s += "\n"; s += str; xxassert(exp,s.c_str()); }
#else
#define xassertStr(exp, str) 
#endif

/////////////////////////////////////////////////
/// Map: enum <-> name, nameAlt
class EnumDescriptor
{
public:
	EnumDescriptor(const char* typeName) : typeName_(typeName) { ignoreErrors_ = false; }

	const char* name(int key) const;
	const char* nameAlt(int key) const;
	int keyByName(const char* name) const;
	int keyByNameAlt(const char* nameAlt) const;

	bool nameExists(const char* name) const;
	bool nameAltExists(const char* name) const;

	string nameCombination(int bitVector, const char* separator = " | ") const;
	ComboStrings nameCombinationStrings(int bitVector) const;
	string nameAltCombination(int bitVector, const char* separator = "|") const;

	const char* comboList() const { return comboList_.c_str(); }
	const char* comboListAlt() const { return comboListAlt_.c_str(); }

	const ComboStrings& comboStrings() const { return comboStrings_; }
	const ComboStrings& comboStringsAlt() const { return comboStringsAlt_; }

	const char* typeName() const { return typeName_.c_str(); }
	int defaultValue() const;

	void ignoreErrors(bool v){ ignoreErrors_ = v; }

	typedef StaticMap<StaticString, int> NameToKey;

	NameToKey::const_iterator beginNameToKey()const{return nameToKey_.begin();}
	NameToKey::const_iterator endNameToKey()const{return nameToKey_.end();}
protected:
	void add(int key, const char* name, const char* nameAlt);

private:
	class Key 
	{
	public: 
		Key(int value);
		bool operator<(const Key& rhs) const;
		operator int() const { return value_; }
			
	private:
		int value_;
		unsigned int bitsNumber_;
	};

	typedef StaticMap<Key, StaticString> KeyToName;

	KeyToName keyToName_;
	KeyToName keyToNameAlt_;
	NameToKey nameToKey_;
	NameToKey nameAltToKey_;

	string comboList_;
	string comboListAlt_;

	ComboStrings comboStrings_;
	ComboStrings comboStringsAlt_;

	string typeName_;

	bool ignoreErrors_;
};

/////////////////////////////////////////////////
//		Регистрация enums
/////////////////////////////////////////////////
template<class Enum>
const EnumDescriptor& getEnumDescriptor(const Enum& key);

#define BEGIN_ENUM_DESCRIPTOR(enumType, enumName)	\
	struct Enum##enumType : EnumDescriptor { Enum##enumType(); }; \
	Enum##enumType::Enum##enumType() : EnumDescriptor(enumName) {

#define REGISTER_ENUM(enumKey, enumNameAlt)	\
	add(enumKey, #enumKey, enumNameAlt); 

#define ENUM_DESCRIPTOR_IGNORE_ERRORS	ignoreErrors(true);

#define END_ENUM_DESCRIPTOR(enumType)	\
	}  \
	const EnumDescriptor& getEnumDescriptor(const enumType& key){	\
		static Enum##enumType descriptor;	\
		return descriptor;	\
	}

// Для enums, закрытых классами
#define BEGIN_ENUM_DESCRIPTOR_ENCLOSED(nameSpace, enumType, enumName)	\
	struct Enum##nameSpace##enumType : EnumDescriptor { Enum##nameSpace##enumType(); }; \
	Enum##nameSpace##enumType::Enum##nameSpace##enumType() : EnumDescriptor(enumName) {

#define REGISTER_ENUM_ENCLOSED(nameSpace, enumKey, enumNameAlt)	\
	add(nameSpace::enumKey, #enumKey, enumNameAlt); 

#define END_ENUM_DESCRIPTOR_ENCLOSED(nameSpace, enumType)	\
	}  \
	const EnumDescriptor& getEnumDescriptor(const nameSpace::enumType& key){	\
		static Enum##nameSpace##enumType descriptor;	\
		return descriptor;	\
	}


/////////////////////////////////////////////////
//	Вспомогательные функции для отображения
/////////////////////////////////////////////////
template<class Enum>
const char* getEnumName(const Enum& key) {
	return getEnumDescriptor(Enum()).name(key);
}

template<class Enum>
const char* getEnumNameAlt(const Enum& key) {
	return getEnumDescriptor(Enum()).nameAlt(key);
}

template<class Enum>
const EnumDescriptor& getEnumDescriptor(const Enum& key);


/// Вспомогательная функция для работы с комбо-листами
string cutTokenFromComboList(string& comboList);
string getEnumToken(const char*& buffer);
ComboInts makeComboInts(const ComboStrings& values, const ComboStrings& list);
void joinComboList(std::string& outComboList, const ComboStrings& strings, char delimeter = '|');
void splitComboList(ComboStrings& outComboStrings, const char* comboList, char delimeter = '|');
// TODO: переписать
int indexInComboListString(const char* comboList, const char* value);

template<class Pair>
struct PairSerializationTraits
{
	static const char* firstName() { return "&Имя"; }
	static const char* secondName() { return "&Значение"; }
};

////////////////////////////////////////////////////////////////////
//
// Базовый архив.
//
// 1. Основная функция serialize принимает константную
// ссылку на объект (которую меняет), чтобы нормально 
// работала перегрузка во всех случаях.
// 2. nameAlt == 0 - не редактировать данное поле.
// nameAlt начинается с '&' - добавлять значение этого поля к 
// родительскому.
// 3. Запрет на открытие нового блока при сериализации пользовательских
// типов производится определением функции serialize(Archive&, const char*, const char*),
// вместо стандартной serialize(Archive&). По умолчанию UDT всегда сериализуются
// с открытием блока.
// 4. Для сериализации полиморфных указателей использовать serializePolymorphic,
// ShareHandle или PolymorphicWrapper. Они записывают и воссоздают тип (требуется 
// регистрация классов (REGISTER_CLASS, иногда линковщик отсекает, - FORCE_REGISTER_CLASS).
// 5. Для сериализации неполиморфных указателей (крайне редкая задача), использовать
// serializePointer или PointerWrapper.
// 6. Для массивов - serializeArray, wrapper'а пока нет, но при необходимости  возможен.
//
////////////////////////////////////////////////////////////////////

class Archive : public ShareHandleBase {
public:
	enum {
		NULL_POINTER = -1,
        UNREGISTERED_CLASS = -2
	};

	Archive()
	: closure_(0)
	, closureType_(0)
	{}
	virtual ~Archive() {}
	
	virtual bool close() { return true; }
	
	virtual void setVersion(int version) {} 
	int version() const { return 0; }

    virtual void openBlock(const char* name, const char* nameAlt) {}
    virtual void closeBlock() {}

    virtual bool isText() const { return false; }
    virtual bool isOutput() const { return false; }
    virtual bool isEdit() const { return false; }

    bool isInput() const { return !isOutput(); }

	virtual void skipValue(int openCounter = 0) {}


    template<class T>
    bool processNonPrimitiveImpl(T& t, const char* name, const char* nameAlt, bool (T::*)(Archive& ar, const char*, const char*))
    {
        return t.serialize(*this, name, nameAlt);
    }

    template<class T>
    bool processNonPrimitiveImpl(T& t, const char* name, const char* nameAlt, void (T::*)(Archive& ar))
    {
        bool result = openStruct(name, nameAlt, typeid(T).name ());
        t.serialize(*this);
        closeStruct(name);
        return result;
    }

    template<class Enum>
    bool processEnumImpl(Enum& value, const char* name, const char* nameAlt)
    {
        static const EnumDescriptor& descriptor = getEnumDescriptor(Enum(0));
		bool usesNameAlt = this->usesNameAlt();

        if(isInput()){
            const ComboStrings& strings = usesNameAlt ? descriptor.comboStringsAlt() : descriptor.comboStrings();
            int result = processEnum(usesNameAlt ? descriptor.comboListAlt() : descriptor.comboList(), name, nameAlt);
            if (result != -1) {
                if(usesNameAlt){
                    xassert(result >= 0 && result < strings.size() && "Неверный индекс значения Enum-а!");
                    value = (Enum)descriptor.keyByNameAlt(strings[result].c_str());
                }
				else{
                    xassert (result >= 0 && result < strings.size() && "Неверный индекс значения Enum-а!");
                    value = (Enum)descriptor.keyByName(strings[result].c_str());
                }
                return true;
            } else {
                return false;
            }
        }
        else{
            const char* comboValue = usesNameAlt ? descriptor.nameAlt(value) : descriptor.name(value);
            if(comboValue && (*comboValue != '\0' || usesNameAlt)){
                ComboListString combo_list(usesNameAlt ? descriptor.comboListAlt() : descriptor.comboList(), comboValue);
                processEnum(combo_list, name, nameAlt);
            }
			else{
                ErrH.Abort("Unable to write bad/unregistered enum value!");
                return false;
            }
            return true;
        }

    }

    template<class T>
	bool serialize(const T& t, const char* name, const char* nameAlt) {
		using serialization_helpers::Select;
		using serialization_helpers::Identity;
		using serialization_helpers::IsClass;
        return
        Select< IsClass<T>,
            Select< HaveAdvancedSerialization<T>,
                Identity< ProcessAdvancedNonPrimitiveImpl<T> >,
                Identity< ProcessNonPrimitiveImpl<T> >
			>,
            Identity< ProcessEnumImpl<T> >
        >::type::invoke (*this, const_cast<T&>(t), name, nameAlt);
	}

    template<class T1, class T2>
    bool serialize(const std::pair<T1, T2>& t, const char* name, const char* nameAlt) {
        bool result = openStruct(name, nameAlt, typeid(std::pair<T1, T2>).name());
		serialize(const_cast<T1&>(t.first), "first", PairSerializationTraits<std::pair<T1, T2> >::firstName());
        serialize(const_cast<T2&>(t.second), "second", PairSerializationTraits<std::pair<T1, T2> >::secondName());
        closeStruct(name);
		return result;
    }

    template<class Enum>
    bool serialize(const BitVector<Enum>& t, const char* name, const char* nameAlt) {
        const EnumDescriptor& descriptor = getEnumDescriptor(Enum(0));

        const ComboStrings& comboList = descriptor.comboStrings();
        const ComboStrings& comboListAlt = descriptor.comboStringsAlt();
        ComboInts flags = makeComboInts(descriptor.nameCombinationStrings(t), comboList);
		int val = int(t);

		if(processBitVector(val, descriptor, name, nameAlt)){
			if(isInput())
				const_cast<BitVector<Enum>&>(t) = Enum(val);
			return true;
		}
		else{
			return false;
		}
    }

    template<class T, class A>
    bool serialize(const std::vector<T, A>& constCont, const char* name, const char* nameAlt) {

		std::vector<T, A>& cont = const_cast<std::vector<T, A>&>(constCont);
        int cont_size = (int)cont.size();
        
		bool result = openContainer(name, nameAlt, typeid(std::vector<T,A>).name(), typeid(T).name(), cont_size, false);
        makeDefaultArchive<T>();

        setType<T>();

        if(isOutput()) {
            int i = 0;
            std::vector<T, A>::const_iterator it;
            FOR_EACH(cont, it){
                serialize(*it, "@", "@");
            }
        }
        else {
            if(cont_size != -1) {
                std::vector<T, A>::const_iterator it;
                // XXX: HINT!
                if(cont.size() != cont_size) {
                    cont.clear();
                    cont.resize(cont_size);
                }
                int i = 0;
                FOR_EACH(cont, it){
                	// @dilesoft
					// try {
						serialize(*it, "@", "@");
					// } catch (...) {
					// }
                }
            }
        }
        closeContainer(name);
		return result;
    }

    template<class T, class A>
    bool serialize(const std::list<T, A>& constCont, const char* name, const char* nameAlt) {

		std::list<T, A>& cont = const_cast<std::list<T, A>&>(constCont);

        int cont_size = cont.size();
		bool result = openContainer(name, nameAlt, typeid(std::list<T,A>).name(), typeid(T).name(), cont_size, false);

         makeDefaultArchive<T>();
        
		setType<T>();

		if(isOutput()) {
			std::list<T, A>::const_iterator it;
			FOR_EACH(cont, it){
				serialize(*it, "@", "@");
			}
		}
		else {
			if(cont_size != -1) {
				int i = 0;
				// XXX: HINT!
				if(cont.size() != cont_size) {
					if(!cont.empty()) 
						cont.clear();
					cont.resize(cont_size);
				}
				std::list<T, A>::const_iterator it;
				FOR_EACH(cont, it){
					serialize(*it, "@", "@");
				}
			}
		}
        closeContainer(name);
		return result;
    }

    bool serialize(const XBuffer& buffer, const char* name, const char* nameAlt) {
        return processBinary(const_cast<XBuffer&>(buffer), name, nameAlt);
    }

    template<class Enum, class Type, Enum zeroValue, class TypeFactory>
    bool serialize(const EnumToClassSerializer<Enum, Type, zeroValue, TypeFactory>& constValue, const char* name, const char* nameAlt) {
		EnumToClassSerializer<Enum, Type, zeroValue, TypeFactory>& value =
			const_cast<EnumToClassSerializer<Enum, Type, zeroValue, TypeFactory>&>(constValue);
		const EnumDescriptor& descriptor = getEnumDescriptor(value.key());
		const char* baseName = descriptor.name(zeroValue);
		const char* baseNameAlt = descriptor.nameAlt(zeroValue);
		const char* derivedName = descriptor.name(value.key());
		const char* derivedNameAlt = descriptor.nameAlt(value.key());

        typedef ClassCreatorFactory<Type> CF;
        
		if(isInput()){
			int openTypeIndex = openPointer(name, nameAlt, baseName, baseNameAlt, descriptor.comboList(), descriptor.comboListAlt());
			const char* openTypeName = "";
			if(openTypeIndex >= 0) {
				openTypeName = descriptor.comboStrings()[openTypeIndex].c_str();
			}

			value.setKey(openTypeIndex >= 0 ?(Enum)descriptor.keyByName(openTypeName) : zeroValue);

			bool result = true;
			if(openTypeIndex == UNREGISTERED_CLASS) {
				skipValue(1); // { уже открыта
				result = false;
			}

			if(value.type())
				value->serialize(*this);

			if(openTypeIndex >= 0)
				closePointer(name, openTypeName, derivedName);
			else
				closePointer(name, 0, derivedName);
			return result;
        }
        else{
			bool need_default_archive_poly = needDefaultArchive(baseName);
			int openTypeIndex = openPointer(name, nameAlt, baseName, baseNameAlt, derivedName, derivedNameAlt);

			if(need_default_archive_poly){
				const ComboStrings& comboList = descriptor.comboStrings();
				const ComboStrings& comboListAlt = descriptor.comboStringsAlt();
				for(int i = 0; i < comboList.size(); ++i){
					const char* name = comboList[i].c_str();
					const char* nameAlt = comboListAlt[i].c_str();
					EnumToClassSerializer<Enum, Type, zeroValue, TypeFactory> type((Enum)descriptor.keyByName(name));
					if(type.type()){
						if(ShareHandle<Archive> archive = openDefaultArchive(baseName, name, nameAlt)){
							archive->serialize(*type, "name", "nameAlt");
							closeDefaultArchive(archive, baseName, name, nameAlt);
						}
						type.setKey(zeroValue); // delete type;
					}
				}
			}

			if(value.type())
				value->serialize(*this);
			
			closePointer(name, baseName, derivedName);
			return true;
        }
    }

    template<class T> // Для неполиморфных указателей
    bool serializePointer(const T*& t, const char* name, const char* nameAlt) {
        if(isInput()) {
            if(!t)
      			const_cast<T*&>(t) = ObjectCreator<T, T>::create();
      		serialize(*t, name, nameAlt);
			return true;
        }
        else {
      		if(t)
      			serialize(*t, name, nameAlt);
      		else{
      			static T* defaultPointer = ObjectCreator<T, T>::create();
      			serialize(*defaultPointer, name, nameAlt);
      			xassert("Attempt to save non-polymorphic zero pointer");
      		}
			return true;
        }
    }

    template<class T> // Для полиморфных указателей
	bool serializePolymorphic(const T*& t, const char* name, const char* nameAlt) {
        if(isInput()) {
            return loadPointer(const_cast<T*&>(t), name, nameAlt);
        }
        else {
            savePointer(t, name, nameAlt);
			return true;
        }	
    }

    template<class T>
	bool serializeArray(const T& const_array, const char* name, const char* nameAlt) {
        T& array = const_cast<T&> (const_array);

        int array_size = sizeof(array) / (
                                        static_cast<const char *>(static_cast<const void *>(&array[1])) 
                                        - static_cast<const char *>(static_cast<const void *>(&array[0]))
                                    );

		int count = array_size;
		bool result = openContainer(name, nameAlt, typeid(T).name(), typeid(T).name(), count, true);

		if (isInput() && count > array_size) {
			count = array_size;
		}

		for(int i = 0; i < count; ++i) {
			serialize(array[i], "@", "@");
		}

        closeContainer(name);
		return result;
    }

	bool serialize(const bool& value,               const char* name, const char* nameAlt) { return processValue(const_cast<bool&>(value), name, nameAlt); }
	bool serialize(const char& value,               const char* name, const char* nameAlt) { return processValue(const_cast<char&>(value), name, nameAlt); }

	bool serialize(const signed char& value,        const char* name, const char* nameAlt) { return processValue(const_cast<signed char&>(value), name, nameAlt); }
	bool serialize(const signed short& value,       const char* name, const char* nameAlt) { return processValue(const_cast<signed short&>(value), name, nameAlt); }
	bool serialize(const signed int& value,         const char* name, const char* nameAlt) { return processValue(const_cast<signed int&>(value), name, nameAlt); }
	bool serialize(const signed long& value,        const char* name, const char* nameAlt) { return processValue(const_cast<signed long&>(value), name, nameAlt); }

	bool serialize(const unsigned char& value,      const char* name, const char* nameAlt) { return processValue(const_cast<unsigned char&>(value), name, nameAlt); }
	bool serialize(const unsigned short& value,     const char* name, const char* nameAlt) { return processValue(const_cast<unsigned short&>(value), name, nameAlt); }
	bool serialize(const unsigned int& value,       const char* name, const char* nameAlt) { return processValue(const_cast<unsigned int&>(value), name, nameAlt); }
	bool serialize(const unsigned long& value,      const char* name, const char* nameAlt) { return processValue(const_cast<unsigned long&>(value), name, nameAlt); }

	bool serialize(const float& value,              const char* name, const char* nameAlt) { return processValue(const_cast<float&>(value), name, nameAlt); }
	bool serialize(const double& value,             const char* name, const char* nameAlt) { return processValue(const_cast<double&>(value), name, nameAlt); }

	bool serialize(const std::string& value,        const char* name, const char* nameAlt) { return processValue(const_cast<std::string&>(value), name, nameAlt); }
	bool serialize(const PrmString& value,          const char* name, const char* nameAlt) { return processValue(const_cast<PrmString&>(value), name, nameAlt); }
	bool serialize(const ComboListString& value,    const char* name, const char* nameAlt) { return processValue(const_cast<ComboListString&>(value), name, nameAlt); }

	virtual bool openStruct(const char* name, const char* nameAlt, const char* typeName) = 0;
    virtual void closeStruct(const char* name) = 0;

	template<class T>
	void setClosure(T& closure){
		closure_ = reinterpret_cast<void*>(&closure);
#ifdef _DEBUG
		closureType_ = typeid(T).name();
#endif
	}

	template<class T>
	T& closure() const{
		xassert(closure_);
#ifdef _DEBUG
		xassert(strcmp(closureType_, typeid(T).name())== 0);
#endif
		return *reinterpret_cast<T*>(closure_);
	}

	bool hasClosure() const{ return closure_ != 0; }

protected:
    virtual bool baseRegistered(const char* baseName) const {
        return false;
    }
    virtual bool usesNameAlt() const {
        return false;
    }

    virtual bool processValue(bool&,               const char* name, const char* nameAlt) = 0;
    virtual bool processValue(char&,               const char* name, const char* nameAlt) = 0;

    virtual bool processValue(signed char&,        const char* name, const char* nameAlt) = 0;
    virtual bool processValue(signed short&,       const char* name, const char* nameAlt) = 0;
    virtual bool processValue(signed int&,         const char* name, const char* nameAlt) = 0;
    virtual bool processValue(signed long&,        const char* name, const char* nameAlt) = 0;

    virtual bool processValue(unsigned char&,      const char* name, const char* nameAlt) = 0;
    virtual bool processValue(unsigned short&,     const char* name, const char* nameAlt) = 0;
    virtual bool processValue(unsigned int&,       const char* name, const char* nameAlt) = 0;
    virtual bool processValue(unsigned long&,      const char* name, const char* nameAlt) = 0;

    virtual bool processValue(float&,              const char* name, const char* nameAlt) = 0;
    virtual bool processValue(double&,             const char* name, const char* nameAlt) = 0;

    virtual bool processValue(std::string&,        const char* name, const char* nameAlt) = 0;
    virtual bool processValue(PrmString&,          const char* name, const char* nameAlt) = 0;
    virtual bool processValue(ComboListString&,    const char* name, const char* nameAlt) = 0;

	//virtual int			processEnum(const char* value, const ComboStrings& comboList, const char* name, const char* nameAlt, const char* typeName) = 0;
    //virtual ComboInts   processBitVector(const ComboInts& flags, const ComboStrings& comboList, const ComboStrings& comboListAlt, const char* name, const char* nameAlt) = 0;
	
	virtual bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) = 0;
    virtual bool processBitVector(int& flags, const EnumDescriptor& descriptor,const char* name, const char* nameAlt) = 0;

	virtual bool processBinary(XBuffer& buffer, const char* name, const char* nameAlt) {return false;}

    virtual bool openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& _size, bool readOnly) = 0;
    virtual void closeContainer(const char* name) = 0;

    virtual bool needDefaultArchive(const char* baseName) const { return false; }

    virtual int openPointer(const char* name, const char* nameAlt,
                             const char* baseName, const char* baseNameAlt,
                             const char* derivedName, const char* derivedNameAlt) = 0;
    virtual void closePointer(const char* name, const char* typeName, const char* derivedName) = 0;
	const char* typeComboList(const char* typeName) {
		return 0;
	}
    virtual Archive* openDefaultArchive(const char* typeName, const char* derivedTypeName, const char* derivedTypeNameAlt){ return 0; }
    virtual void closeDefaultArchive(ShareHandle<Archive>, const char* typeName, const char* derivedTypeName, const char* derivedTypeNameAlt) {
        xassert(0);
    }
    virtual void setNodeType(const char*) {}

	template<class T>
    void savePointer(const T* ptr, const char* name, const char* nameAlt) {

          typedef ClassCreatorFactory<T> CF;

          const char* baseName = typeid(T).name();
          const char* baseNameAlt = baseName;

          const char* derivedName = 0;
          const char* derivedNameAlt = 0;
          if(ptr) {
        		derivedName = typeid(*ptr).name();
			   	derivedNameAlt = CF::instance().nameAlt(derivedName, true);
		  }

          bool need_default_archive_poly = needDefaultArchive(typeid(T).name());
          const char* openTypeName = "";
		  openPointer(name, nameAlt, baseName, baseNameAlt, derivedName, derivedNameAlt);

          if(need_default_archive_poly)
              makeDefaultArchivePoly<T>();

          if(ptr == 0) {
          }
          else {
              const_cast<T*&>(ptr)->serialize(*this);
          }
          
		  closePointer(name, baseName, derivedName);
    }

	template<class T>
    bool loadPointer(T*& ptr, const char* name, const char* nameAlt) {
		ClassCreatorFactory<T>& factory = ClassCreatorFactory<T>::instance();

	    const char* baseName = typeid(T).name();
	    const char* baseNameAlt = baseName;

        const char* derivedName = 0;
        const char* derivedNameAlt = 0;
        if(ptr) {
            derivedName = typeid(*ptr).name();
            derivedNameAlt = factory.nameAlt(derivedName, true);
        }
		
		const char* comboList = factory.comboList();
		const char* comboListAlt = factory.comboListAlt();
		
		const char* openTypeName = "";
        int openTypeIndex = openPointer(name, nameAlt, baseName, baseNameAlt, comboList, comboListAlt);
		if(openTypeIndex >= 0) {
			openTypeName = factory.comboStrings()[openTypeIndex].c_str();
		}
		
		if(ptr && (!derivedName || strcmp(openTypeName, derivedName) != 0)){
			delete ptr;
			ptr = 0;
		}
		
		bool result = true;
        if(openTypeIndex == UNREGISTERED_CLASS) {
            skipValue(1); // { уже открыта
			result = false;
        } else if(!ptr && openTypeIndex != NULL_POINTER) {
            ptr = factory.createByIndex(openTypeIndex);
			if(!ptr)
				skipValue(1); // { уже открыта
        }

		if(ptr)
	        ptr->serialize(*this);

		closePointer(name, ptr ? openTypeName : 0, derivedName);
		return result;
	}

private:
	template<class T> 
	struct DefaultValue
	{
		static T get() { 
			return T();
		}
	};
	template<class T> 
	struct DefaultValue<T*>
	{
		static T* get() { 
			return 0;
		}
	};
	template<class Enum> 
	struct DefaultValueEnum
	{
		static Enum get() { 
			return getEnumDescriptor(Enum(0)).defaultValue();
		}
	};

    template<class T>
    void makeDefaultArchive(){
		const char* typeName = typeid(T).name();
		if(ShareHandle<Archive> defaultArchive = openDefaultArchive(typeName, 0, 0)){
			T t = DefaultValue<T>::get();
			defaultArchive->serialize(t, "default", "[+]");
			closeDefaultArchive(defaultArchive, typeName, 0, 0);
		}
    };

    template<class Base>
    void makeDefaultArchivePoly() {
        int count = ClassCreatorFactory<Base>::instance().size();

        typedef ClassCreatorFactory<Base> CF;

        for(int i = 0; i < count; ++i) {
      		Base* t = CF::instance().createByIndex(i);
			const char* baseTypeName = typeid(Base).name();
			const char* name = typeid(*t).name();
			const char* nameAlt = CF::instance().nameAlt(name, true);

			if(ShareHandle<Archive> archive = openDefaultArchive(baseTypeName, name, nameAlt)){
      			archive->serialize(*t, "name", "nameAlt");
      			closeDefaultArchive(archive, typeid(Base).name(), name, nameAlt);
			}
      		delete t;
        }
    }

    template<class T>
    void setType(const char* prefix = "") {
        static int id = ++typeIDs_;
        XBuffer buf;
        buf < prefix <= id;
        setNodeType(buf);
    }

    template<class Enum>
    struct ProcessEnumImpl {
        static bool invoke(Archive& ar, const Enum& value, const char* name, const char* nameAlt){
            const EnumDescriptor& descriptor = getEnumDescriptor (Enum (0));
            if(ar.isInput()){
				const ComboStrings& comboList = ar.usesNameAlt() ? descriptor.comboStringsAlt() : descriptor.comboStrings();
				int val = int(Enum(value));
				if(ar.processEnum(val, descriptor, name, nameAlt)){
					const_cast<Enum&>(value) = Enum(val);
					return true;
				}
				return false;
				/*
				return result;
				if (result != -1) {
					if (ar.usesNameAlt ()) {
						xassert(result >= 0 && result < comboList.size() && "Неверный индекс значения Enum-а!");
						const_cast<Enum&>(value) = (Enum)descriptor.keyByNameAlt(comboList[result].c_str());
					} else {
						xassert(result >= 0 && result < comboList.size() && "Неверный индекс значения Enum-а!");
						const_cast<Enum&>(value) = (Enum)descriptor.keyByName(comboList[result].c_str());
					}
					return true;
				} else {
					return false;
				}
				*/
            }
            else{
				int val = int(Enum(value));
				if(ar.processEnum(val, descriptor, name, nameAlt))
					return true;
				else{
					ErrH.Abort ("Unable to write bad/unregistered enum value!");
					return false;
				}
            }
        }
    };

    template<class T>
    struct ProcessAdvancedNonPrimitiveImpl{
        static bool invoke (Archive& ar, T& t, const char* name, const char* nameAlt){
            return t.serialize(ar, name, nameAlt);
        }
    };

    template<class T>
    struct ProcessNonPrimitiveImpl{
        static bool invoke(Archive& ar, T& t, const char* name, const char* nameAlt){
            bool result = ar.openStruct(name, nameAlt, typeid(T).name ());
            t.serialize(ar);
            ar.closeStruct(name);
            return result;
        }
    };

	template<class T>
	struct HaveAdvancedSerialization {
	private:
		struct NoType { char dummy; };
		struct YesType { char dummy[100]; };

		static NoType function_helper(void (T::*arg)(Archive&));
		static YesType function_helper(bool (T::*arg)(Archive&, const char*, const char*));
	public:
		enum { value = (sizeof(function_helper(&T::serialize)) == sizeof(YesType)) };
	};

	void* closure_;
	const char* closureType_;

    static int typeIDs_;
};

__declspec( selectany ) int Archive::typeIDs_ = 0;


#define REGISTER_CLASS(baseClass, derivedClass, classNameAlt) \
  static ClassCreatorFactory<baseClass>::ClassCreator<derivedClass> registered##baseClass##derivedClass(typeid(derivedClass).name(), classNameAlt); \
  int registeredDummy##baseClass##derivedClass = 0;

#define REGISTER_CLASS_CONVERSION(baseClass, derivedClass, classNameAlt, oldName) \
  static ClassCreatorFactory<baseClass>::ClassCreator<derivedClass> registered##baseClass##derivedClass##oldOne(oldName, classNameAlt); \
  int registeredDummy##baseClass##derivedClass##OldOne = 0; \
  static ClassCreatorFactory<baseClass>::ClassCreator<derivedClass> registered##baseClass##derivedClass(typeid(derivedClass).name(), classNameAlt); \
  int registeredDummy##baseClass##derivedClass = 0;

#define FORCE_REGISTER_CLASS(baseClass, derivedClass, classNameAlt) \
  extern int registeredDummy##baseClass##derivedClass; \
  int* registeredDummyPtr##baseClass##derivedClass = &registeredDummy##baseClass##derivedClass;


/// Обертка для сериализации неполиморфных указателей
template<class T>
class PointerWrapper
{
public:
	PointerWrapper(T* t = 0) : t_(t) {}
	operator T* () const { return t_; }
	T* operator->() const { return t_; }
	T& operator*() const { return *t_; }
	T* get() const { return t_; }
	bool serialize(Archive& ar, const char* name, const char* nameAlt) { return ar.serializePointer(t_, name, nameAlt); }

private:
	T* t_;
};

/// Обертка для сериализации полиморфных указателей
template<class T>
class PolymorphicWrapper
{
public:
	PolymorphicWrapper(T* t = 0) : t_(t) {}
	operator T* () const { return t_; }
	T* operator->() const { return t_; }
	T& operator*() const { return *t_; }
	T* get() const { return t_; }
	bool serialize(Archive& ar, const char* name, const char* nameAlt) {
		return ar.serializePolymorphic(t_, name, nameAlt);
	}

private:
	T* t_;
};


bool saveFileSmart(const char* fname, const char* buffer, int size);

#endif //__SERIALIZATION_H_INCLUDED__
