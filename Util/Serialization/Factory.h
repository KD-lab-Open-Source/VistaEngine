#ifndef __FACTORY_H__
#define __FACTORY_H__

#include "StaticMap.h"
#include "SafeCast.h"
#include <typeinfo>

// Частичной специализацией подставляют параметры коструктора
template<class Base, class Derived>
class ObjectCreator 
{
public:
	static Base* create() { 
		return new Derived;
	}
};

struct Arguments0{
	template<class Factory, class Key, class Product>
	struct FactoryBase{
		Product* create(Key key, bool silent = false) const{
			return reinterpret_cast<const Factory* const>(this)->createWithArguments(key, Arguments0(), silent);
		}
	};
	template<class Derived>
	static Derived* passToNew(Arguments0 args){
		return new Derived();
	}
};

template<class Arg1>
struct Arguments1{
	template<class Factory, class Key, class Product>
	struct FactoryBase{
		Product* create(typename Key key, Arg1 arg1, bool silent = false) const{
			return reinterpret_cast<const Factory* const>(this)->create(key, Arguments1(arg1), silent);
		}
	};

	template<class Derived>
	static Derived* passToNew(Arguments1 args){
		return new Derived(args.arg1);
	}

	Arguments1(Arg1 a1)
	: arg1(a1)
	{}
	Arg1 arg1;
};

template<class Arg1, class Arg2>
struct Arguments2{
	template<class Factory>
	struct FactoryBase{
		void create(typename Factory::Key key, Arg1 arg1, Arg2 arg2, bool silent = false) const{
			safe_cast<const Factory* const>(this)->create(key, Arguments2(arg1, arg2), silent);
		}
	};
	template<class Derived>
	static Derived* passToNew(Arguments2 args){
		return new Derived(args.arg1, args.arg2);
	}
	Arguments2(Arg1 a1, Arg2 a2)
	: arg1(a1), arg2(a2)
	{}
	Arg1 arg1;
	Arg2 arg2;
};

/// Фабрика объектов.
/**
Параметры:
@param Product - базовый класс для создаваемых объектов
@param ProductClassID - тип идентификатора класса, используется как ключ в std::map
Использовать частичную специализацию класса #ObjectCreator для передачи аргументов в конструктор
*/
template<class ProductClassID, class Product, class Arguments = Arguments0>
class Factory : public Arguments::FactoryBase<Factory, ProductClassID, Product>
{
public:
	struct CreatorBase
	{
        virtual Product* create(/*Arguments args = Arguments()*/) const { return 0; }
        virtual const char* typeName() const { return ""; }
	};

	template<class Derived>
	struct Creator : CreatorBase
	{
		Creator() {
		}
		Creator(const ProductClassID& id) {
			instance().add(id, *this);
		}
        Product* create() const
        {
			return ObjectCreator<Product, Derived>::create();
        }
		/*
        Product* create(Arguments args) const
        {
			return Arguments::passToNew<Derived>(args);
        }
		*/
        const char* typeName() const 
		{ 
			return typeid(Derived).name(); 
		}
	};

	void add(const ProductClassID& class_id, CreatorBase& creator_op)
	{
		if(creators_.find(class_id) != creators_.end()) {
			XBuffer msg;
			msg < "Попытка повторной регистрации класса в "
				< typeid(this).name();
			xxassert(0, msg);
		} else {
			creators_.insert(Creators::value_type(class_id, &creator_op));
		}
	}

	template<class Derived>
	void add(const ProductClassID& class_id)
	{
		static Creator<Derived> creator(class_id);
		add(class_id, creator);
	}

	Product* create(const ProductClassID& class_id, bool silent = false) const 
	{
		Creators::const_iterator it = creators_.find(class_id);
		if(it != creators_.end())
			return it->second->create();

		xassert(silent && "Неопознанный идентификатор класса");
		return 0;
	}

	/*
	Product* createWithArguments(const ProductClassID& class_id, Arguments args, bool silent = false) const {
		Creators::const_iterator it = creators_.find(class_id);
		if(it != creators_.end())
			return it->second->create(args);

		xassert(silent && "Неопознанный идентификатор класса");
		return 0;
	}
	*/

	const char* typeName(const ProductClassID& class_id, bool silent = false) const
	{
		Creators::const_iterator it = creators_.find(class_id);
		if(it != creators_.end())
			return it->second->typeName();

		xassert(silent && "Неопознанный идентификатор класса");
		return "";
	}

	const CreatorBase* find(const ProductClassID& class_id) const
	{
		Creators::const_iterator it = creators_.find(class_id);
		if(it != creators_.end())
			return it->second;

		return 0;
	}


	static Factory& instance() {
		return Singleton<Factory>::instance();
	}

protected:
	typedef StaticMap<ProductClassID, CreatorBase*> Creators;
	Creators creators_;
private:
};

// Имена не должны содержать <>, используйте typedef
#define REGISTER_CLASS_IN_FACTORY(Factory, classID, derivedClass) \
	Factory::Creator<derivedClass> factory##Factory##derivedClass(classID); \
	int factoryDummy##Factory##derivedClass = 0;


#define FORCE_REGISTER_CLASS_IN_FACTORY(Factory, classID, derivedClass) \
	extern int factoryDummy##Factory##derivedClass; \
	int* factoryDummyPtr##Factory##derivedClass = &factoryDummy##Factory##derivedClass;

#endif /* __FACTORY_H__ */
