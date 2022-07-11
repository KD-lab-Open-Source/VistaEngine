#ifndef __LOCSTRING_H__
#define __LOCSTRING_H__

class Archive;

class LocString
{
public:
	LocString();
	
	void serialize(Archive& ar);
	void update();
	
	const char* c_str() const { return text_.c_str(); }
	bool empty() const { return text_.empty(); }

	const char* key() const { return key_.c_str(); }

private:
	string key_;
	string text_;
	int id_;
};

#endif //__LOCSTRING_H__
