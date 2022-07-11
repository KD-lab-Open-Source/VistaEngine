#ifndef __GENERIC_FILE_SELECTOR_H_INCLUDED__
#define __GENERIC_FILE_SELECTOR_H_INCLUDED__

#include <string>
#include "Serialization.h"

class GenericFileSelector {
public:
	struct Options {
		Options (const char* filter, const char* initialDir, const char* title = "", bool onlyInitialDir = false)
			: filter_ (filter)
			, initialDir_ (initialDir)
			, title_ (title)
			, onlyInitialDir_ (onlyInitialDir)
		{}
		void serialize (Archive& ar) {
			ar.serialize(title_, "title_", 0);
			ar.serialize(filter_, "filter_", 0);
			ar.serialize(initialDir_, "initialDir_", 0);
			ar.serialize(onlyInitialDir_, "onlyInitialDir_", 0);
		}
	private:
		friend GenericFileSelector;
		std::string title_;
		std::string filter_;
		std::string initialDir_;
        bool onlyInitialDir_;
	};
	static Options DEFAULT_OPTIONS;
	explicit GenericFileSelector (std::string& fileName, const Options& options = DEFAULT_OPTIONS)
		: fileNamePtr_ (&fileName)
	    , options_ (options)
    {}
	
	GenericFileSelector ()
		: fileNamePtr_ (0)
		, options_ (DEFAULT_OPTIONS)
	{
	}

    const char* filter () const { return options_.filter_.c_str (); }
    const char* initialDir () const { return options_.initialDir_.c_str (); }
	const char* title () const { return options_.title_.c_str (); }
	bool onlyInitialDir () const { return options_.onlyInitialDir_; }

	void setFileName (const char* fileName) { fileName_ = fileName; }
    operator const char* () const { return fileName_.c_str (); }

    GenericFileSelector& operator= (const char* fileName) {
        fileName_ = fileName;
		return *this;
    }
    GenericFileSelector& operator= (const std::string& fileName) {
        fileName_ = fileName;
		return *this;
    }

	bool serialize (Archive& ar, const char* name, const char* nameAlt) {
        if (ar.isEdit ()) {
            bool nodeExists = ar.openStruct (name, nameAlt, typeid(GenericFileSelector).name());
			
			if (fileNamePtr_)
	            ar.serialize(*fileNamePtr_, "fileName", "ָלÿ פאיכא");
			else
	            ar.serialize(fileName_, "fileName", "ָלÿ פאיכא");
            ar.serialize(options_, "options_", 0);

            ar.closeStruct (name);
			return nodeExists;
        } else {
			xassert (fileNamePtr_);
            return ar.serialize (*fileNamePtr_, name, nameAlt);
        }
	}
private:
    std::string fileName_;
	std::string* fileNamePtr_;
	const Options& options_;
};

__declspec(selectany) GenericFileSelector::Options GenericFileSelector::DEFAULT_OPTIONS ("ֲסו פאיכû||*.*", ".");

#endif
