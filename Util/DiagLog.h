#ifndef __DIAG_LOG_H__
#define __DIAG_LOG_H__

#include <format>
#include <string_view>
#include <utility>

// Field diagnostics: one line per event, appended to "diag.log" in the working
// directory (beside _console.log) and flushed immediately, with a copy on stderr
// for anyone running from a terminal.
//
// Neither of the two existing sinks works for this:
//   - kdError/Console queues the message and only writes it on the next
//     graphQuant, so anything logged just before a crash never reaches the file;
//   - stderr alone is swallowed once the game is running (STLport's setvbuf).
// These call sites exist precisely to survive the failure that follows them, so
// they need a sink that has already hit the disk by the time we return.
namespace diag {

void write(std::string_view line);

template<class... Args>
void log(std::format_string<Args...> fmt, Args&&... args)
{
	write(std::format(fmt, std::forward<Args>(args)...));
}

}

#endif //__DIAG_LOG_H__
