#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include "StaticMap.h"
#include "Serialization.h"
#include "LibraryWrapper.h"

#include "TreeInterface.h"

class Dictionary : public ShareHandleBase
{
public:
	Dictionary();

	bool isOn() const { return isOn_; }
	void setIsOn(bool isOn) { isOn_ = isOn; }

	const char* translate(const char* name) const;
	bool useFallback()const { return useFallback_; }
	string translateComboList(const char* comboList) const;
	ComboStrings translateComboList (const ComboStrings& comboList);
	void serialize(Archive& ar);
private:
    unsigned int codePage_;
	typedef StaticMap<string, string*> Map;
	list<string> strings_;
	bool useFallback_;
	Map map_;
	bool isOn_;
};

#ifndef _FINAL_VERSION_
class TranslationManagerImpl;
class ATTRIB_EDITOR_API TranslationManager{
public:
	TranslationManager();
	~TranslationManager();

    static TranslationManager& instance();
	const char* translate(const char* key) const;

	void setTranslationsDir(const char* dir);

	void setDefaultLanguage(const char* languageName);
    void setLanguage(const char* languageName);
	const char* language() const;
	const char* languageList() const;

private:
    TranslationManagerImpl* impl_;
};
#endif

#ifndef _FINAL_VERSION_
# define TRANSLATE(text) TranslationManager::instance().translate(text)
#else
# define TRANSLATE(text) text
#endif

#endif //__DICTIONARY_H__
