#include "DiagLog.h"

#include <cstdio>
#include <ctime>

namespace {

// CP1251 0x80..0xFF -> Unicode. Everything below 0x80 is ASCII in both encodings.
const unsigned short cp1251ToUnicode[128] = {
	0x0402,0x0403,0x201A,0x0453,0x201E,0x2026,0x2020,0x2021,
	0x20AC,0x2030,0x0409,0x2039,0x040A,0x040C,0x040B,0x040F,
	0x0452,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
	0x0098,0x2122,0x0459,0x203A,0x045A,0x045C,0x045B,0x045F,
	0x00A0,0x040E,0x045E,0x0408,0x00A4,0x0490,0x00A6,0x00A7,
	0x0401,0x00A9,0x0404,0x00AB,0x00AC,0x00AD,0x00AE,0x0407,
	0x00B0,0x00B1,0x0406,0x0456,0x0491,0x00B5,0x00B6,0x00B7,
	0x0451,0x2116,0x0454,0x00BB,0x0458,0x0405,0x0455,0x0457,
	0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
	0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
	0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
	0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
	0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
	0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
	0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
	0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F
};

bool isValidUtf8(std::string_view s)
{
	for(size_t i = 0; i < s.size(); ){
		const unsigned char c = (unsigned char)s[i];
		int extra;
		if(c < 0x80)             { ++i; continue; }
		else if((c & 0xE0) == 0xC0) extra = 1;
		else if((c & 0xF0) == 0xE0) extra = 2;
		else if((c & 0xF8) == 0xF0) extra = 3;
		else return false;

		if(i + extra >= s.size())
			return false;
		for(int k = 1; k <= extra; ++k)
			if(((unsigned char)s[i + k] & 0xC0) != 0x80)
				return false;
		i += extra + 1;
	}
	return true;
}

std::string cp1251ToUtf8(std::string_view s)
{
	std::string out;
	out.reserve(s.size() * 2);
	for(char ch : s){
		const unsigned char c = (unsigned char)ch;
		if(c < 0x80){
			out += (char)c;
			continue;
		}
		const unsigned code = cp1251ToUnicode[c - 0x80];
		if(code < 0x800){
			out += (char)(0xC0 | (code >> 6));
			out += (char)(0x80 | (code & 0x3F));
		}
		else {
			out += (char)(0xE0 | (code >> 12));
			out += (char)(0x80 | ((code >> 6) & 0x3F));
			out += (char)(0x80 | (code & 0x3F));
		}
	}
	return out;
}

}

namespace diag {

void write(std::string_view line)
{
	// Opened once and kept open: every caller here is already reporting something
	// broken, so reopening per line would only add a way to fail. Append rather
	// than truncate — a user who relaunches before sending the file still has the
	// failing session in it, and these events are rare enough not to grow it.
	//
	// Byte-order mark on creation, because the file has to survive being opened by
	// whatever editor a user reaches for on a non-UTF-8 Windows.
	static FILE* file = []{
		FILE* f = fopen("diag.log", "ab");
		if(f){
			if(ftell(f) == 0)
				fwrite("\xEF\xBB\xBF", 1, 3, f);
			const time_t now = time(0);
			char stamp[64] = "";
			if(const tm* t = localtime(&now))
				strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", t);
			fprintf(f, "\n--- session %s ---\n", stamp);
		}
		return f;
	}();

	// Two encodings meet in these lines: source string literals are UTF-8, but names
	// that came out of the game data (parameter and attribute names, file paths from
	// the .spg tree) are CP1251. Written raw they mix, and the file decodes as neither
	// -- which is exactly how a parameter name reaches us as unreadable mojibake. UTF-8
	// is self-checking, so anything that fails that check is game data: convert it.
	const std::string text = isValidUtf8(line) ? std::string(line) : cp1251ToUtf8(line);

	if(file){
		fprintf(file, "%.*s\n", (int)text.size(), text.data());
		fflush(file);   // the next thing to run may be the crash
	}
	fprintf(stderr, "%.*s\n", (int)text.size(), text.data());
}

}
