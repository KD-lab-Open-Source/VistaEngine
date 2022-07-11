#ifndef __UI_REFERENCES_H__
#define __UI_REFERENCES_H__

class UI_ControlContainer;
class UI_ControlBase;
class UI_Screen;
class Archive;

typedef bool (*UI_ControlFilterFunc)(const UI_ControlBase* p);

/// Ссылка на кнопку интерфейса.
class UI_ControlReferenceBase
{
public:
	virtual ~UI_ControlReferenceBase(){ }

	bool isEmpty() const { return reference_.empty(); }

	bool init(const UI_ControlBase* control);
	void clear();

	virtual UI_ControlBase* control() const;

	const char* referenceString() const { return reference_.c_str(); }
	const string& reference() const { return reference_; }

	bool serialize(Archive& ar, const char* name, const char* nameAlt);

	virtual const char* comboList() const = 0;

protected:
	UI_ControlReferenceBase(const UI_ControlBase* control = 0){ if(control) init(control); }
	UI_ControlReferenceBase(const std::string& reference) : reference_(reference) { }

private:

	/// имена экрана и всех кнопок-родителей через точку (пример - "Ingame interface.Group0.Button1")
	std::string reference_;
};

typedef StaticMap<string, int> UI_ControlMapBackReference;
extern UI_ControlMapBackReference ui_ControlMapBackReference;

typedef StaticMap<int, string> UI_ControlMapReference;
extern UI_ControlMapReference ui_ControlMapReference;

class UI_ControlReference : public UI_ControlReferenceBase
{
public:
	UI_ControlReference(const UI_ControlBase* control = 0) : UI_ControlReferenceBase(control) { }
	UI_ControlReference(const std::string& reference) : UI_ControlReferenceBase(reference) { }

	static UI_ControlBase* getControlByID(int id);
	static int getIdByControl(const UI_ControlBase* control);

	const char* comboList() const; 
};

class UI_ControlReferenceRefinedBase : public UI_ControlReferenceBase
{
public:
	UI_ControlReferenceRefinedBase(const UI_ControlBase* control = 0) : UI_ControlReferenceBase(control) { }
	UI_ControlReferenceRefinedBase(const std::string& reference) : UI_ControlReferenceBase(reference) { }

	virtual UI_ControlFilterFunc filter() const = 0;
	const char* comboList() const; 
};

template<class T>
class UI_ControlReferenceRefined : public UI_ControlReferenceRefinedBase
{
public:
	UI_ControlReferenceRefined(const T* control = 0) : UI_ControlReferenceRefinedBase(control) { }

	T* control() const { return dynamic_cast<T*>(UI_ControlReferenceRefinedBase::control()); }

	UI_ControlFilterFunc filter() const { return controlFilter; }
	static bool controlFilter(const UI_ControlBase* p){ return (dynamic_cast<const T*>(p) != 0); }
};

/// Ссылка на экран интерфейса.
class UI_ScreenReference
{
public:
	UI_ScreenReference(const UI_Screen* screen = 0);
	UI_ScreenReference(const std::string& screen_name);
	~UI_ScreenReference(){ }

	bool isEmpty() const { return screenName_.empty(); }

	bool init(const UI_Screen* screen);
	void clear();
	const char* referenceString() const { return screenName_.c_str(); }

	UI_Screen* screen() const;

	bool serialize(Archive& ar, const char* name, const char* nameAlt);

private:

	/// имя экрана
	std::string screenName_;
};

#endif /* __UI_REFERENCES_H__ */
