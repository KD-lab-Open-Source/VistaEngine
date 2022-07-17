#include "StdAfx.h"

#include "xMath.h"
#include "XPrmArchive.h"
#include "Dictionary.h"
#include "crc.h"

///////////////////////////////////////////////////////////////////////////////////////
//			String Util
///////////////////////////////////////////////////////////////////////////////////////
inline void replace(string& s, const char*src, const char* dest)
{
	int pos = 0;
	while(1){
		pos = s.find(src, pos);
		if(pos >= s.size())
			break;
		s.replace(pos, strlen(src), dest);
		pos += strlen(dest);
		}
}

inline string& expand_spec_chars(string& s)
{
	replace(s, "\\", "\\\\");
	replace(s, "\n", "\\n");
	replace(s, "\r", "\\r");
	replace(s, "\a", "\\a");
	replace(s, "\t", "\\t");
	replace(s, "\v", "\\v");
	replace(s, "\"", "\\\"");
	return s;
}

inline string& collapse_spec_chars(string& s)
{
	int pos = 0;
	while(1){
		pos = s.find("\\", pos);
		if(pos >= s.size() - 1)
			break;
		char* dest;
		switch(s[pos + 1]){
			case '\\':
				dest = "\\";
				break;
			case 'n':
				dest = "\n";
				break;
			case 'r':
				dest = "\r";
				break;
			case 'a':
				dest = "\a";
				break;
			case 't':
				dest = "\t";
				break;
			case 'v':
				dest = "\v";
				break;
			case '"':
				dest = "\"";
				break;

			default:
				xassert(0);
				ErrH.Abort("Unknown special character");
			}

		s.replace(pos, 2, dest);
		pos += strlen(dest);
		}

	return s;
}

///////////////////////////////////////////////////////////////////////////////////////
//			ScriptParser
///////////////////////////////////////////////////////////////////////////////////////
XPrmOArchive::XPrmOArchive(const char* fname) :
buffer_(10, 1), 
binaryStream_(0)
{
	memset(disabledChars_, 0, sizeof(disabledChars_));
	disabledChars_['.'] = 1;
	disabledChars_[','] = 1;
	disabledChars_['!'] = 1;
	disabledChars_['#'] = 1;
	disabledChars_['$'] = 1;
	disabledChars_['%'] = 1;
	disabledChars_['^'] = 1;
	disabledChars_['&'] = 1;
	disabledChars_['*'] = 1;
	disabledChars_['('] = 1;
	disabledChars_[')'] = 1;
	disabledChars_['/'] = 1;
	disabledChars_['\\'] = 1;
	disabledChars_['<'] = 1;
	disabledChars_['>'] = 1;
	for(int i = 128; i < 256; i++)
		disabledChars_[i] = 1;

	open(fname);
}

XPrmOArchive::~XPrmOArchive() 
{
	close();
}

void XPrmOArchive::open(const char* fname)
{
	if(fname)
		fileName_ = fname;
	buffer_.alloc(10000);
	buffer_.SetDigits(6);
}

bool XPrmOArchive::close()
{
	xassert(offset_.empty() && "Block isnt closed");
	if(binaryStream_.isOpen()){
		if(binaryStream_.ioError())
			return false;
		binaryStream_.close();
	}
	return saveFileSmart(fileName_.c_str(), buffer_, buffer_.tell());
}

struct BinaryLink{
	long offset;
	long length;
	unsigned long crc;

	BinaryLink()
	: offset(0)
	, length(0)
	, crc(0)
	{
	}
    
	void serialize(Archive& ar){
		ar.serialize(offset, "offset", 0);
		ar.serialize(length, "length", 0);
		ar.serialize(crc, "crc", 0);
	}
};

bool XPrmOArchive::processBinary (XBuffer& buffer, const char* name, const char* nameAlt)
{
	if(!binaryStream_.isOpen()){
		XBuffer file_name;
		file_name < fileName_.c_str() < ".bin";
		binaryStream_.open(file_name, XS_OUT);
	}
	xassert(binaryStream_.isOpen());
	if(!binaryStream_.isOpen())
		return false;

	BinaryLink link;
	link.offset = binaryStream_.tell();
    link.length = buffer.tell();
	link.crc = crc32((unsigned char*)(buffer.buffer()), buffer.tell(), 0);
	
	binaryStream_.write(buffer.buffer(), buffer.tell());

	serialize(link, name, nameAlt);
    return true;
}


//////////////////////////////////////////////////////
void XPrmOArchive::saveStringEnclosed(const char* prmString)
{
	if(prmString){
		const char* begin = prmString;
		const char* end = prmString + strlen(prmString);
		while(begin < end && *begin == ' ')
			++begin;
		while(end > begin && *(end - 1) == ' ')
			--end;
		string s1(begin, end);
		expand_spec_chars(s1);
		buffer_ < "\"" < s1.c_str() < "\"";
	}
	else
		buffer_ < "0";
}

bool XPrmOArchive::openStruct( const char* name, const char* nameAlt, const char* typeName ) 
{
	openNode(name, nameAlt);
	openBracket ();

	isContainer_.push (false);
	return true;
}

void XPrmOArchive::closeStruct( const char* name ) 
{
	isContainer_.pop ();

	closeBracket ();
	if(inContainer ())
		saveString (",\r\n");
	else
		saveString (";\r\n");
}

bool XPrmOArchive::openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& size, bool readOnly ) 
{
	containerSize_.push(size);

	if(!inContainer())
		openItem(name, nameAlt);
	else
		saveString(offset_.c_str ());

	openCollection(size);
	isContainer_.push(true);
	return true;
}

void XPrmOArchive::closeContainer( const char* name ) 
{
	xassert (!isContainer_.empty());
	isContainer_.pop ();
	closeCollection (containerSize_.top () != 0);
	if(inContainer ())
		saveString (",\r\n");
	else
		saveString (";\r\n");
	xassert (!containerSize_.empty());
	containerSize_.pop ();
}

int XPrmOArchive::openPointer( const char* name, const char* nameAlt,      const char* baseName, const char* baseNameAlt,      const char* typeName, const char* typeNameAlt ) 
{
	if(! inContainer ())
		openItem (name, nameAlt);

	if(typeName) {
		if(inContainer ()) {
			saveString (offset_.c_str ());
		}
		else{
			//				saveString (name);
			//				saveString (" = ");
		}
		saveStringEnclosed (typeName);
		saveString (" ");
		openBracket ();
	}
	else{
		saveString (offset_.c_str ());
		if(inContainer ()) {
		}
		saveString ("0");
	}
	isContainer_.push (false);
	return NULL_POINTER;
}

void XPrmOArchive::closePointer( const char* name, const char* baseName, const char* derivedName ) 
{
	isContainer_.pop ();
	if(derivedName) {
		closeBracket ();
	}
	if(inContainer ())
		saveString (",\r\n");
	else
		saveString (";\r\n");
	//closeItem (name);
}

bool XPrmOArchive::openItem( const char* name, const char* nameAlt ) 
{
	if(name){
#ifndef _FINAL_VERSION_
		const char* p = name;
		while(*p){
			if(disabledChars_[*(p++)]){
				XBuffer buf;
				buf < "???????????? ?????? ? ????? ????????: " < name;
				ErrH.Abort(buf);
			}
		}
#endif // _FINAL_VERSION_
		buffer_ < offset_.c_str() < name < " = ";
	}
	return true;
}

void XPrmOArchive::closeItem( const char* name ) 
{
	if(name)
		buffer_ < ";\r\n";
}

void XPrmOArchive::openBracket() 
{
	buffer_ < "{\r\n";
	offset_ += "\t";
}

void XPrmOArchive::closeBracket() 
{
	xassert (! offset_.empty () && "Trying to pop element from empty stack!");
	offset_.erase(offset_.end() - 1);
	buffer_ < offset_.c_str() < "}";
}

bool XPrmOArchive::openNode( const char* name, const char* nameAlt ) 
{
	if(name && strlen(name) && !inContainer()) {
		openItem(name, nameAlt);
	}
	else{
		saveString(offset_.c_str ());
	}
	return true;
}

void XPrmOArchive::closeNode( const char* name ) 
{
	if(inContainer ()) {
		saveString (",\r\n");
	}
	else
		closeItem (name);
}
//////////////////////////////////////////////
namespace {
	static bool isalpha_table[256];
	static bool isdigit_table[256];
	static bool iscsym_table[256];
	static bool isspace_table[256];
	static bool is_table_initialized = false;
};

XPrmIArchive::XPrmIArchive(const char* fname) :
buffer_(10, 1),
binaryStream_(0)
{
	version_ = 0;

	if(!is_table_initialized) {
		for(int c = 0; c < 128; ++c) {
			isalpha_table[c] = isalpha(c);
			isdigit_table[c] = isdigit(c);
			isspace_table[c] = isspace(c);
			iscsym_table[c] = __iscsym(c);
		}
		for(int c = 128; c <= 255; ++c) {
			isalpha_table[c] = false;
			isdigit_table[c] = false;
			isspace_table[c] = false;
			iscsym_table[c] = false;
		}
		is_table_initialized = true;
	}
	
	if(fname && !open(fname))
		ErrH.Abort("File not found: ", XERR_USER, 0, fname);
}

XPrmIArchive::~XPrmIArchive() 
{
	close();
}

bool XPrmIArchive::processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) 
{
    openNode(name, nameAlt);
	bool result = false;
    if(isNodeExists()){
		flags = 0;
        for(;;){
            string name;
            loadString(name);
            if(name == ";"){
                putToken();
                break;
            }
            else if(name == "|") {
                continue;
            }
			flags |= descriptor.keyByName(name.c_str());
        }
        closeNode(name);
        return true;
    }
    else{
        closeNode(name);
        return false;
    }
}

int XPrmIArchive::openCollection()
{
    openBracket();
    int counter;
    buffer_ >= counter;
    passString(";");
    return counter;
}

void XPrmIArchive::closeStructure()
{
    // ?? ?????? closeStructure() ?????? ???? ?????? popBlock()
    // readingStarts_.pop_back();
    for(;;){
        string token;
        loadString(token);
        if(token == "}"){
            break;
        }
        else{
            passString("=");
            skipValue();
            passString(";");
        }
    }
}

bool XPrmIArchive::open(const char* fname, int blockSize)
{
	fileName_ = fname;
	XStream ff(0);
	if(!ff.open(fname, XS_IN))
		return false;
	if(!blockSize)
		blockSize = ff.size();
	else
		blockSize = min(blockSize, ff.size());
	buffer_.alloc(blockSize + 1);
	ff.read(buffer_.buffer(), blockSize);
	buffer_[blockSize] = 0;
	replaced_symbol = 0;
	putTokenOffset_ = 0;
	pushBlock(true, false, 0);
	return true;
}

bool XPrmIArchive::close()
{
	buffer_.alloc(10);
	return true;
}

bool XPrmIArchive::processValue(char& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists()) {
        signed short val;
        buffer_ >= val;
        value = char(val);
    }
    closeNode(name);
    return result;
} 
bool XPrmIArchive::processValue(signed char& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists()) {
        signed short val;
        buffer_ >= val;
        value = signed char(val);
    }
    closeNode(name);
    return result;
} 
bool XPrmIArchive::processValue(signed short& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists())
        buffer_ >= value;
    closeNode(name);
    return result;
} 
bool XPrmIArchive::processValue(signed int& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists())
        buffer_ >= value;
    closeNode(name);
    return result;
} 
bool XPrmIArchive::processValue(signed long& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists())
        buffer_ >= value;
    closeNode(name);
    return result;
} 

bool XPrmIArchive::processValue(unsigned char& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists())
        buffer_ >= value;
    closeNode(name);
    return result;
} 
bool XPrmIArchive::processValue(unsigned short& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists())
        buffer_ >= value;
    closeNode(name);
    return result;
} 
bool XPrmIArchive::processValue(unsigned int& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists())
        buffer_ >= value;
    closeNode(name);
    return result;
} 
bool XPrmIArchive::processValue(unsigned long& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists())
        buffer_ >= value;
    closeNode(name);
    return result;
} 

bool XPrmIArchive::processValue(float& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists())
        buffer_ >= value;
    closeNode(name);
    return result;
}
bool XPrmIArchive::processValue(double& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists())
        buffer_ >= value;
    closeNode(name);
    return result;
}
bool XPrmIArchive::processValue(PrmString& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
	// try {
    if(isNodeExists()) {
        std::string str;
        if(loadString (str))
            value = str;
        else
            value = 0;
    }
	// } catch (...) {
	// }
    closeNode(name);
    return result;
}
bool XPrmIArchive::processValue(ComboListString& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists()){
        loadString(value.value());
    }
    closeNode(name);
    return result;
}
bool XPrmIArchive::processValue(std::string& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists()){
        loadString(value);
    }
    closeNode(name);
    return result;
}
bool XPrmIArchive::processValue(bool& value, const char* name, const char* nameAlt) {
    bool result = openNode(name, nameAlt);
    if(isNodeExists()){
        string str;
        loadString(str);
        if(str == "true")
            value = true;
        else if(str == "false")
            value = false;
        else
            value = static_cast<bool>(atoi (str.c_str ()));
    }
    closeNode(name);
    return result;
}

bool __forceinline isalphaX(unsigned char c) {
	return isalpha_table[c];
}
bool __forceinline isdigitX(unsigned char c) {
	return isdigit_table[c];
}
bool __forceinline iscsymX(unsigned char c) {
	return iscsym_table[c];
}
bool __forceinline isspaceX(unsigned char c) {
	return isspace_table[c];
}

const char* XPrmIArchive::getToken()	
{
	xassert(!replaced_symbol && "Unreleased token");
	
	putTokenOffset_ = buffer_.tell();

	// Search begin of token
	const char* i = &buffer_();
	for(;;){
		if(!*i)
			return 0; // eof

		if(*i == '/'){	     
			if(*(i + 1) == '/'){ // //-comment
				i += 2;
				if((i = strstr(i, "\n")) == 0) 
					return 0;
				i++;
				continue;
				}
			if(*(i + 1) == '*'){ // /* */-comment
				i += 2;
				while(!(*i == '*' && *(i + 1) == '/')){
					if(!*i)
						return 0; // error
					i++;
					}
				i += 2;
				continue;
				}
			}
		if(isspaceX(*i))
			i++;
		else
			break;
		}

	// Search end of token
	const char* marker = i;
	if(isalphaX(*i) || *i == '_'){ // Name
		i++;
		while(iscsymX(*i))
			i++;
		}
	else
		if(isdigitX(*i) || (*i == '.' && (isdigitX(*(i + 1)) || *(i + 1) == 'f'))){ // Numerical Literal
			i++;
			while(iscsymX(*i) || *i == '.' || (*i == '+' || *i == '-') && (*(i - 1) == 'E' || *(i - 1) == 'e'))
				i++;
		}
		else
			if(*i == '"'){ // Character Literal 
				i++;
				// ???????? ????? '"', ???? ????? '"' ????? '\' ? ????? '"' ?? ??????? ';',
				// ?? ??? ?? ??????????? '"', ??? ????????? '"' ????????? ???????????? 
				while ((i = strstr(i, "\"")) != 0){
					if (*(i - 1) == '\\' && *(i + 1) != ';')
						i++;
					else {
						i++;
						break;
					}
				}
				if (i == 0)
					return 0; // error
			}
			else
				if(*i == '-' && *(i + 1) == '>'){ // ->
					i += 2;
					if(*i == '*')			// ->*
						i++;
					}
				else
					if(*i == '<' && *(i + 1) == '<'){ // <<
						i += 2;
						if(*i == '=')			// <<=
							i++;
						}
					else
						if(*i == '>' && *(i + 1) == '>'){ // >>
							i += 2;
							if(*i == '=')			// >>=
								i++;
							}
						else
							if(*i == '.' && *(i + 1) == '.' && *(i + 2) == '.') // ...
								i += 3;
							else
								if(*i == '#' && *(i + 1) == '#' || // ##
									*i == ':' && *(i + 1) == ':' || // ::
									*i == '&' && *(i + 1) == '&' || // &&
									*i == '|' && *(i + 1) == '|' || // ||
									*i == '.' && *(i + 1) == '*' || // .*
									*i == '+' && *(i + 1) == '+' || // ++
									*i == '-' && *(i + 1) == '-' || // --
									(*i == '+' || *i == '-' || *i == '*' || *i == '/' || *i == '%' || 
									*i == '^' || *i == '|' || *i == '&' || 
									*i == '!' || *i == '<' || *i == '>' || *i == '=') && *(i + 1) == '=') // x=
										i += 2;
								else
									i++;

	buffer_.set(i - buffer_.buffer());
	replaced_symbol = buffer_();
	buffer_() = 0;
	return marker;
}

void XPrmIArchive::releaseToken()
{
	//	xassert(replaced_symbol);
	buffer_() = replaced_symbol;
	replaced_symbol = 0;
}

void XPrmIArchive::putToken() 
{
	buffer_.set(putTokenOffset_);
}


void XPrmIArchive::passString(const char* token)
{
	const char* s = getToken();
	xassert(s);
	if(strcmp(s, token) != 0){
		XBuffer msg;
		msg  < "Expected Token: \"" < token
			< "\", Received Token: \"" < s < "\", file: \"" < fileName_.c_str() < "\", line: " <= line();
		xassertStr(0 && "Expected another token", msg);
		releaseToken();
		//ErrH.Abort(msg);
	} else {
		releaseToken();
	}
}

bool XPrmIArchive::loadString(string& str)
{
	const char* s = getToken();
	xassert(s);
	str = s;
	releaseToken();
	if(str[0] == '"'){
		if(str[str.size() - 1] != '"'){
			XBuffer err;
			err < "Quotes aren't closed: " < s < "\n";
			err < "Line: " <= line();
			ErrH.Abort(err);
			} 
		str.erase(0, 1);
		xassert (!str.empty());
		str.pop_back();
		collapse_spec_chars(str);
	}
	else if(str == "0")
		return false;
	return true;
}

void XPrmIArchive::skipValue(int open_counter)
{
	if(open_counter)
		popBlock();

	for(;;){
		const char* str = getToken();
		xassert(str);
		if(str[0] == '{' && str[1] == '\0')
			++open_counter;
		else if(str[0] == '}' && str[1] == '\0'
				&& open_counter)
			--open_counter;
		else if(open_counter == 0) {
			if ((str[0] == '}' || str[0] == ';' || str[0] == ',') && str[1] == '\0') {
				releaseToken();
				putToken();
				break;
			}
		}
		releaseToken();
	}
}

bool XPrmIArchive::findSection(const char* sectionName)
{
	for(int i = 0; i < 2; i++){
		string name;
		loadString(name);
		if(name == sectionName){
			putToken();
			return true;
		}
		else{
			passString("=");
			skipValue();
			passString(";");
			bool endOfFile = !getToken();
			releaseToken();
			if(!endOfFile)
				putToken();
			else{
				buffer_.set(0);
				replaced_symbol = 0;
				putTokenOffset_ = 0;
			}
		}
	}
	return false;
}

int XPrmIArchive::line() const 
{
	return 1 + count((const char*)buffer_, (const char*)buffer_ + buffer_.tell(), '\n');
}

bool XPrmIArchive::openNode (const char* name, const char* nameAlt)
{
    if(isContainer())
        return true;
    else
        return !(badItem_ = !openItem(name));
}

void XPrmIArchive::closeNode (const char* name) {
    //popBlock ();

    if (isNodeExists ()) {
        if (! isContainer ()) {
            closeItem (name);
        }
        else {
            std::string tok = getToken ();
            releaseToken ();
            if (tok != ",")
                putToken ();
        }
    }
	badItem_ = false;
}

bool XPrmIArchive::openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& count, bool readOnly)
{
	if (isNodeExists ()) {
		if (!isContainer ()) {
			if (! openItem (name)) {
				pushBlock (false, true);
				count = -1;
				return false;
			}
		}
		openBracket ();
        pushBlock (true, true);
        std::string str_size;
        loadString (str_size);
        passString (";");

        count = atoi (str_size.c_str ());
		return true;
    }
    else {
        count = -1;
        pushBlock (false, true);
		return false;
    }
}
void XPrmIArchive::closeContainer (const char* name) {
   	bool needCloseStructure = isNodeExists ();
    popBlock ();
    if(!isNodeExists()){
        return;
    }
    //if (isContainer ()) {
	if(needCloseStructure){
		if (name) {
			std::string tok = getToken();
			releaseToken();
	        
			if(tok == ","){
				tok = getToken();
				releaseToken();
			}
			else
				putToken();

			while(tok != "}"){
				putToken();

				skipValue();

				tok = getToken();
				releaseToken();

				if(tok == ","){
					tok = getToken();
					releaseToken();
				}
			}
			putToken();
		}
	//}
    //}
	//if (needCloseStructure) {
		closeStructure();
        if(isContainer()){
            std::string tok = getToken();
            releaseToken();
            
			if(tok != ",")
				putToken();
        } else {
			passString (";");
        }
	}
}

bool XPrmIArchive::openItem(const char* name) 
{
    bool firstConversion = true;
    if(name){
        int pass = 0;
        for(;;){
            const char* str = getToken();
            const char* token = str ? str : "}"; // to simulate end of block when end of file
            if(!str)
                token = "}";
			if(strcmp(token, name) == 0){
	            releaseToken();
                break;
			}
            if(token[0] == '}' && token[1] == '\0'){
				releaseToken();
                if(pass++ == 2){
                    putToken();
                    return false;
                }
                else
                    buffer_.set(parserStack_.top ().blockStart);
            }
            else{
				releaseToken();
				// if(firstConversion){
                //     XBuffer buf;
                //     buf < "?????????? ?????????/??????? ???????? \"" < name < "\" ??? ??????: "
                //       < fileName_.c_str() < ", line " <= line() < "\n?????????? ???????????? ??????.";
                //     kdWarning("&Serialization", buf);
                //     firstConversion = false;
                // }
                passString("=");
                skipValue();
                passString(";");
            }
        }

        passString("=");
    }


    return true;
}

void XPrmIArchive::closeItem(const char* name) 
{
    if(name)
        passString(";");
}

bool XPrmIArchive::openStruct (const char* name, const char* nameAlt, const char* typeName) {
	bool exists = isNodeExists ();
	if (exists) {
		if (! isContainer ()) {
			// ???? ?? ?????? ??????????:
			// ???????? ????? ??????? "name" ? ??????? ?????
			exists = openItem (name);
			if (exists) {
				openBracket();
			}
			else {
			}
		}
		else {
			openBracket ();
		}
	}
    pushBlock(exists, false);
	return exists;
}

void XPrmIArchive::closeStruct (const char* name) {
	bool needCloseStructure = isNodeExists();
    popBlock();
    if (isNodeExists()){
		if (needCloseStructure){
			closeStructure();
		}
		if(needCloseStructure){
			if(!isContainer()){
				passString(";");
			}
			else {
				std::string tok = getToken();
				releaseToken();
				if(tok != ","){
					putToken();
				}
			}
		}
    }
}

int XPrmIArchive::openPointer (const char* name, const char* nameAlt,
                               const char* baseName, const char* baseNameAlt,
                               const char* typeName, const char* typeNameAlt)
{
    if (!isNodeExists ()) 
        return NULL_POINTER;
	bool exists = true;
	
    if (! isContainer ()) {
        exists = openItem (name);
    }
	if (exists) {
		std::string str;
		loadString (str);
		if (strcmp (str.c_str (), "0") == 0) {
			return NULL_POINTER;
		}
		else {
			openBracket ();
			pushBlock (true, false);
			//return str;
			int result = indexInComboListString(typeName, str.c_str());
			if(result == NULL_POINTER){
				XBuffer msg(256, 1);
				msg < "ERROR! no such class registered: ";
				msg < str.c_str();
				xassertStr (0, static_cast<const char*>(msg));
				return UNREGISTERED_CLASS;
			}
			return result;
		}
	}
	else {
		pushBlock (false, false);
		return NULL_POINTER;
	}
}

void XPrmIArchive::closePointer (const char* name, const char* typeName, const char* derivedName)
{
    if (!isNodeExists ())
        return;

	// popBlock ()
    if (typeName) {
	    popBlock ();
        closeStructure ();
    }
    if (isContainer ()) {
        std::string tok = getToken ();
        releaseToken ();
        if (tok != ",")
            putToken ();
    }
    else {
        passString(";");
    }
}

bool XPrmIArchive::processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt)
{
    openNode(name, nameAlt);
	if(isNodeExists()){
		std::string str;
		loadString (str);
	    closeNode(name);
		value = descriptor.keyByName(str.c_str());
		if(!descriptor.nameExists(str.c_str())){
			XBuffer msg;
			msg < str.c_str();
			if(strcmp(str.c_str(), str.c_str()) != 0)
				msg < "(\"" < str.c_str() < "\")";
			msg < "\nfile: \"" < fileName_.c_str() < "\", line: " <= line();
			xassertStr(0 && "Unregistered Enum value:", msg);
			return false;
		}
		return true;
	}
	else {
	    closeNode(name);
		return false;
    }
}


bool XPrmIArchive::processBinary (XBuffer& buffer, const char* name, const char* nameAlt)
{
	const char* str = "";
    bool result = openNode(name, nameAlt);
	if(isNodeExists()){
		openBracket();
        str = getToken();
		if(strcmp(str, "offset") == 0){
			BinaryLink link;
			releaseToken();
			passString("=");
			str = getToken();
			link.offset = atol(str);
			releaseToken();
			passString(";");
			passString("length");
			passString("=");
			str = getToken();
			link.length = atol(str);
			releaseToken();
			passString(";");

			bool gotCRC = false;
            str = getToken();
			if(strcmp(str, "crc") == 0){ // CONVERSION 2006.12.12
				releaseToken();
				gotCRC = true;
				passString("=");
				str = getToken();
				link.crc = atol(str);
				releaseToken();
				passString(";");
			}
			else{
				releaseToken();
				putToken();
			}
            
			if(!binaryStream_.isOpen()){
				XBuffer file_name;
				file_name < fileName_.c_str() < ".bin";
				binaryStream_.open(file_name, XS_IN);
			}
            if(binaryStream_.isOpen()){
                binaryStream_.seek(link.offset, XS_BEG);

                if(buffer.makeFree()){
                    buffer.set(0);
                    buffer.alloc(link.length);
                    long readLength = binaryStream_.read(buffer.buffer(), link.length);
					if(readLength == link.length){
						if(gotCRC){
							unsigned long calculatedCRC = crc32((unsigned char*)(buffer.buffer()), link.length, 0);
							if(calculatedCRC != link.crc){
								xassert(0 && "Binary block's CRC doesn't match!");
								buffer.set(0);
								result = false;
							}
						}
					}
					else{
						xassert(0 && "Unable to read entire block");
						buffer.set(0);
						result = false;
					}
					buffer.set(readLength);
                }
                else{
					if(buffer.size() == link.length){
                        binaryStream_.read(buffer.buffer(), link.length);
						if(gotCRC){
							unsigned long calculatedCRC = crc32((unsigned char*)(buffer.buffer()), link.length, 0);
							if(link.crc != calculatedCRC){
								xassert(0 && "Binary block's CRC doesn't match!");
								result = false;
							}
						}
					}
					else{
		                xassert(0 && "Wrong binary block length");
						result = false;
					}
                }
            }
			else{
				//xassert(0 && "Unable to open .bin file");
				result = false;
			}
		}
		else
			releaseToken();
		closeBracket();
    }
	closeNode(name);
    return result;
}

unsigned int XPrmIArchive::crc() 
{
	return crc32((unsigned char*)buffer_.buffer(), buffer_.size(), startCRC32);
}



//////////////////////////////////////////
void Vect2f::serialize(Archive& ar) 
{
    ar.serialize(x, "x", "&x");
    ar.serialize(y, "y", "&y");
}

void Vect2i::serialize(Archive& ar) 
{
    ar.serialize(x, "x", "&x");
    ar.serialize(y, "y", "&y");
}

void Vect2s::serialize(Archive& ar) 
{
	ar.serialize(x, "x", "&x");
	ar.serialize(y, "y", "&y");
}

void Vect3f::serialize(Archive& ar) 
{
    ar.serialize(x, "x", "&x");
    ar.serialize(y, "y", "&y");
    ar.serialize(z, "z", "&z");
}

void Vect3d::serialize(Archive& ar) 
{
	ar.serialize(x, "x", "&x");
	ar.serialize(y, "y", "&y");
	ar.serialize(z, "z", "&z");
}

void Mat3f::serialize(Archive& ar) 
{
	ar.serialize(xrow(), "xrow", "xrow");
	ar.serialize(yrow(), "yrow", "yrow");
	ar.serialize(zrow(), "zrow", "zrow");
}

void Mat3d::serialize(Archive& ar) 
{
	ar.serialize(xrow(), "xrow", "xrow");
	ar.serialize(yrow(), "yrow", "yrow");
	ar.serialize(zrow(), "zrow", "zrow");
}

void MatXf::serialize(Archive& ar) 
{
	ar.serialize(rot(), "rotation", "??????????");
	ar.serialize(trans(), "position", "???????");
}

void MatXd::serialize(Archive& ar) 
{
	ar.serialize(rot(), "rotation", "??????????");
	ar.serialize(trans(), "position", "???????");
}

void QuatF::serialize(Archive& ar) 
{
    ar.serialize(s_, "s", "&s");
    ar.serialize(x_, "x", "&x");
    ar.serialize(y_, "y", "&y");
    ar.serialize(z_, "z", "&z");
}

void QuatD::serialize(Archive& ar) 
{
	ar.serialize(s_, "s", "&s");
	ar.serialize(x_, "x", "&x");
	ar.serialize(y_, "y", "&y");
	ar.serialize(z_, "z", "&z");
}

void Se3f::serialize(Archive& ar) 
{
	ar.serialize(rot(), "rotation", "??????????");
	ar.serialize(trans(), "position", "???????");
}

void Se3d::serialize(Archive& ar) 
{
	ar.serialize(rot(), "rotation", "??????????");
	ar.serialize(trans(), "position", "???????");
}

void Vect4f::serialize(Archive& ar) 
{
	ar.serialize(x, "x", "&x");
	ar.serialize(y, "y", "&y");
	ar.serialize(z, "z", "&z");
	ar.serialize(z, "w", "&w");
}

void Mat4f::serialize(Archive& ar) 
{
	ar.serialize(xrow(), "xrow", "xrow");
	ar.serialize(yrow(), "yrow", "yrow");
	ar.serialize(zrow(), "zrow", "zrow");
	ar.serialize(wrow(), "wrow", "wrow");
}

bool saveFileSmart(const char* fname, const char* buffer, int size)
{
	XStream testf(0);
	if(testf.open(fname, XS_IN)){
		if(testf.size() == size){
			PtrHandle<char> buf = new char[size];
			testf.read(buf, size);
			if(!memcmp(buffer, buf, size))
				return true;
		}
	}
	testf.close();
	
	XStream ff(0);
	if(ff.open(fname, XS_OUT)) {
		ff.write(buffer, size);
	} 
#ifndef _FINAL_VERSION_
	else{
		XBuffer buf;
		buf < "Unable to write file: \n" < fname;
		xxassert(0, buf);
	}
#endif

	return !ff.ioError();
}
