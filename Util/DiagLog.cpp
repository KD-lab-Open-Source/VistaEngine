#include "DiagLog.h"

#include <cstdio>
#include <ctime>

namespace diag {

void write(std::string_view line)
{
	// Opened once and kept open: every caller here is already reporting something
	// broken, so reopening per line would only add a way to fail. Append rather
	// than truncate — a user who relaunches before sending the file still has the
	// failing session in it, and these events are rare enough not to grow it.
	static FILE* file = []{
		FILE* f = fopen("diag.log", "at");
		if(f){
			const time_t now = time(0);
			char stamp[64] = "";
			if(const tm* t = localtime(&now))
				strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", t);
			fprintf(f, "\n--- session %s ---\n", stamp);
		}
		return f;
	}();

	if(file){
		fprintf(file, "%.*s\n", (int)line.size(), line.data());
		fflush(file);   // the next thing to run may be the crash
	}
	fprintf(stderr, "%.*s\n", (int)line.size(), line.data());
}

}
