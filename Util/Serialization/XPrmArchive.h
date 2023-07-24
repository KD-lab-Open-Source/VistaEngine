#ifndef __XPRM_ARCHIVE_H__
#define __XPRM_ARCHIVE_H__

#include <vector>
#include <stack>
#include "Handle.h"
#include "Serialization.h"

class MultiIArchive;
class MultiOArchive;

class XPrmOArchive : public Archive
{
friend MultiOArchive;
public:
	XPrmOArchive(const char* fname, bool fake = false);
	~XPrmOArchive();

	void open(const char* fname); 
	bool close();  // true if there were changes, so file was updated

	bool isText () const{ return true; }
	bool isOutput() const { return true; }

    void setNodeType (const char*) {}

	// To simulate sub-blocks
	void openBlock(const char* name, const char* nameAlt) {}
	void closeBlock() {}

	bool openStruct(const char* name, const char* nameAlt, const char* typeName);
    void closeStruct (const char* name);

protected:
    bool processBinary (XBuffer& buffer, const char* name, const char* nameAlt);
	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt){
        openNode(name, nameAlt);
        saveString(descriptor.name(value));
        closeNode(name);
        return true;
    }
    bool processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
		saveString(descriptor.nameCombination(flags).c_str());
        closeNode(name);
        return true;
    }

    bool processValue(char& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(signed char& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(signed short& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(signed int& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(signed long& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(unsigned char& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(unsigned short& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(unsigned int& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(unsigned long& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(float& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(double& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ <= value;
        closeNode(name);
        return true;
    }
    bool processValue(PrmString& t, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        saveStringEnclosed(t);
        closeNode(name);
        return true;
    }
    bool processValue(ComboListString& t, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        saveStringEnclosed(t);
        closeNode(name);
        return true;
    }
    bool processValue(string& str, const char* name, const char* nameAlt) { 
        openNode(name, nameAlt);
        saveStringEnclosed(str.c_str()); 
        closeNode(name);
        return true;
    }
    bool processValue(bool& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt);
        buffer_ < (value ? "true" : "false");
        closeNode(name);
        return true;
    }

protected:
	bool openNode(const char* name, const char* nameAlt);
    void closeNode(const char* name);

    bool openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& size, bool readOnly);
    void closeContainer(const char* name);

    int openPointer (const char* name, const char* nameAlt,
                             const char* baseName, const char* baseNameAlt,
                             const char* typeName, const char* typeNameAlt);
    void closePointer (const char* name, const char* baseName, const char* derivedName);

private:
	XBuffer buffer_;
	string offset_;
	string fileName_;
	char disabledChars_[256];

	XStream binaryStream_;;
    
    // introduced by admix:
    std::stack<bool> isContainer_;
    std::stack<std::size_t> containerSize_;
	///////////////////////////////////
	bool inContainer () {
		if(isContainer_.empty ())
			return false;
		else
			return isContainer_.top ();
	}
	void saveString(const char* value) {
		buffer_ < value;
	}
	void saveStringEnclosed(const char* value);

	bool openItem(const char* name, const char* nameAlt);
	void closeItem(const char* name);
	
	void openBracket();
	void closeBracket();
	
	template<class T>
	void saveElement(const T& t) {
		buffer_ < offset_.c_str();
		(*this) & serialize(t, "t", 0);
		buffer_ < ",\r\n";
	}

	void openCollection(int counter){
		openBracket();
		buffer_ < offset_.c_str() <= counter < ";\r\n";
	}
	void closeCollection(bool erasePrevComma) {
		if(erasePrevComma){
			buffer_ -= 3;
			buffer_ < "\r\n";
		}
		closeBracket();
	}
};


class XPrmIArchive : public Archive
{
friend MultiIArchive;
public:
	XPrmIArchive(const char* fname = 0);
	~XPrmIArchive();

	bool open(const char* fname, int blockSize = 0);  // true if file exists
	bool close();
	bool findSection(const char* sectionName);

	void setVersion(int version) { version_ = version; } // Для сложной конверсии: вручную записывать, выставлять и кастить архив к XPrmIArchive
	int version() const { return version_; }

	unsigned int crc();

	// To simulate sub-blocks
	void openBlock(const char* name, const char* nameAlt) {}
	void closeBlock() {}

	XBuffer& buffer() {
		return buffer_;
	}

	bool isText() const { return true; }

    bool openStruct (const char* name, const char* nameAlt, const char* typeName);
    void closeStruct (const char* name);

protected:
    bool processBinary (XBuffer&, const char* name, const char* nameAlt);

	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt);
	bool processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt);

    bool openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& count, bool readOnly);
    void closeContainer(const char* name);

	int openPointer (const char* name, const char* nameAlt,
                             const char* baseName, const char* baseNameAlt,
                             const char* typeName, const char* typeNameAlt);
    void closePointer (const char* name, const char* typeName, const char* derivedName);

    bool openNode(const char* name, const char* nameAlt);
    void closeNode(const char* name);

    bool processValue(char& value, const char* name, const char* nameAlt); 
    bool processValue(signed char& value, const char* name, const char* nameAlt); 
    bool processValue(signed short& value, const char* name, const char* nameAlt); 
    bool processValue(signed int& value, const char* name, const char* nameAlt); 
    bool processValue(signed long& value, const char* name, const char* nameAlt); 

    bool processValue(unsigned char& value, const char* name, const char* nameAlt); 
    bool processValue(unsigned short& value, const char* name, const char* nameAlt); 
    bool processValue(unsigned int& value, const char* name, const char* nameAlt); 
    bool processValue(unsigned long& value, const char* name, const char* nameAlt); 

    bool processValue(float& value, const char* name, const char* nameAlt);
    bool processValue(double& value, const char* name, const char* nameAlt);
    bool processValue(PrmString& value, const char* name, const char* nameAlt);
    bool processValue(ComboListString& value, const char* name, const char* nameAlt);
    bool processValue(std::string& value, const char* name, const char* nameAlt);
    bool processValue(bool& value, const char* name, const char* nameAlt);

private:
	string fileName_;
	XBuffer buffer_;
	char replaced_symbol;
	int putTokenOffset_;
    bool badItem_;
	int version_;
	XStream binaryStream_;

    // introduced by admix:
    void popBlock () {
        xassert (! parserStack_.empty ());
        parserStack_.pop ();
    }
    void pushBlock (bool nodeExists, bool isContainer) {
        pushBlock (nodeExists, isContainer, buffer_.tell());

    }
    void pushBlock (bool nodeExists, bool isContainer, int blockStart) {
        badItem_ = false;
        ParserState state;
        state.nodeExists = nodeExists;
        state.isContainer = isContainer;
        state.blockStart = blockStart;
        parserStack_.push (state);
    }

    bool isContainer () {
        if(parserStack_.empty ())
            return false;
        else
            return parserStack_.top ().isContainer;
    }

    struct ParserState {
        bool nodeExists;
        bool isContainer;
        int  blockStart;
    };

    std::stack<ParserState> parserStack_;

	/////////////////////////////////////
    bool isNodeExists() {
        if(badItem_)
            return false;
        if(parserStack_.empty ()) {
            return true;
        }
        else{
            return parserStack_.top ().nodeExists;
        }
    }
    
	const char* getToken();
	void releaseToken();
	void putToken();
	void skipValue(int openCounter = 0);

	void passString(const char* value);
	bool loadString(string& value); // false if zero string should be loaded
	int line() const;

	bool openItem(const char* name);
	void closeItem(const char* name);

	void openBracket() {
		passString("{");
	}
	void closeBracket() {
		passString("}");
	}

	void closeStructure();
	int openCollection();
	template<class T>
	void loadElement(T& t) {
		(*this) & WRAP_NAME(t, "t", 0);
		string name;
		loadString(name);
		if(name != ",")
			putToken();
	}
};

#endif //__XPRM_ARCHIVE_H__
