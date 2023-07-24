#include "xglobal.h"
#include "Statistics.h"
#include <list>
#include <set>
#include <string>
#include <math.h>
#include "Lmcons.h"

class TimerDataList: public std::list<TimerData*> {};

const char profileStatFileName[]="profstat";

static double total_ticks;
static double total_frames;
static double total_time;
static double time_factor;
static double start_ticks;

inline int ticks2time(__int64 t) { return t ? round((t - start_ticks)*time_factor) : 0; }

TimerData::TimerData(char* title_, int group_, bool dont_attach) 
{ 
	timer_data = true;
	title = title_; 
	group = group_; 
	clear(); 
	if(!dont_attach)
		Profiler::instance().attach(this);
}

void TimerData::clear()
{
	t0 = 0;
	dt_sum = 0;
	n = 0;
	dt_min = 0x7fffffff;
	dt_max = 0;
	t_min = t_max = 0;
	started_ = false;
	accumulated_alloc = AllocationStatistics();
}

TimerData& TimerData::operator += (const TimerData& t)
{
	if(!t.timer_data)
		return *this;
	dt_sum += t.dt_sum;
	n += t.n;
	if(dt_max < t.dt_max){
		dt_max = t.dt_max;
		t_max = t.t_max;
		}
	if(dt_min > t.dt_min){
		dt_min = t.dt_min;
		t_min = t.t_min;
		}
	accumulated_alloc += t.accumulated_alloc;
	return *this;
}

void TimerData::print(XBuffer& buf) 
{ 
	char str[2048];
	sprintf(str, "| %-15.15s | %7.3f | %7.3f | %8.2f | %7.3f | %8i | %7.3f | %8i | %9i | %6i | %5i |\r\n", 
		title, (double)dt_sum*100./total_ticks, n ? (double)dt_sum*time_factor/n : 0, n*1000./total_time, 
		(double)dt_max*time_factor, ticks2time(t_max), t_min ? (double)dt_min*time_factor : 0, ticks2time(t_min),
		accumulated_alloc.size, accumulated_alloc.blocks, accumulated_alloc.operations);
	buf < str;
}

void StatisticalData::print(XBuffer& buf)
{
	char str[2048];
	sprintf(str, "| %-15.15s | %7.3f | %7.3f | %8i | %7.3f | %8i | %7.3f | %8i |\r\n", 
		title, avr(), avr() ? sigma()*100/avr() : 0, n, x_max, ticks2time(t_max), x_min, ticks2time(t_min));
	buf < str;
}

static void print_separator(XBuffer& buf) 
{
	buf < "------------------------------------------------------------------------------------------------------------------------\r\n";
}

static void print_header(XBuffer& buf) 
{
	buf < "| Timing:         |  rate % | dt_avr  |   n_avr  | dt_max  |  (t_max) | dt_min  |  (t_min) |  Total memory allocations  |\r\n";
	print_separator(buf);
	buf < "| Statistics:     |  x_avr  |  err %  |  counter |  x_max  |  (t_max) |  x_min  |  (t_min) |    size   | blocks | calls |\r\n";
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//				Profiler
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
Profiler& Profiler::instance()
{
	static Profiler profiler;
	return profiler;
}

Profiler::Profiler()
: total_data("Total", 0, 1),
timers(*new TimerDataList)
{
	start_ticks = getRDTSC();
	started = 0;
	startLogicQuant_ = 0;
	endLogicQuant_ = 0;
	autoExit_ = false;
}

Profiler::~Profiler()
{
	delete &timers;
}

void Profiler::attach(TimerData* td)
{
	timers.push_back(td);
}

void Profiler::start_stop()
{
	if(!started){
		TimerDataList::iterator i;
		FOR_EACH(timers, i)
			(*i) -> clear();
		total_data.clear();
		
		started = 1;
		milliseconds = clock();
		ticks = getRDTSC();
		frames = 0;
		memory = total_memory_used();
	}
	else{
		milliseconds = clock() - milliseconds;
		ticks = getRDTSC() - ticks;
		
		static XBuffer buf(10000, 1);
		buf.init();
		print(buf);
		if(!endLogicQuant_){
			MessageWindow msg;
			msg.showMessage(buf, 0);
		}
		else {
			XStream ff(0);
			if(ff.open(profileFile_, XS_OUT | XS_APPEND | XS_NOSHARING)){
				const int BUF_CN_SIZE=MAX_COMPUTERNAME_LENGTH + 1;
				DWORD cns = BUF_CN_SIZE;
				char cname[BUF_CN_SIZE];
				GetComputerName(cname, &cns);

				SYSTEMTIME localTime;
				::GetLocalTime(&localTime);

				//char tbuf[BUF_CN_SIZE+256];
				//sprintf(tbuf, "%s_%s_%4u%02u%02u_%02u%02u%02u.txt",profileStatFileName, cname,
				//localTime.wYear, localTime.wMonth, localTime.wDay,
				//localTime.wHour, localTime.wMinute, localTime.wSecond);

				ff < "\r\n";
				ff < cname < "\t" <= localTime.wYear < "." <= localTime.wMonth < "." <= localTime.wDay < " " <= localTime.wHour < ":" <=  localTime.wMinute < " " <= localTime.wSecond < "\r\n";
				ff < title_.c_str() < "\r\n";

				ff.write(buf.buffer(), buf.tell());
			}
		}
		started = 0;
	}
}

void Profiler::quant(unsigned long curLogicQuant)
{
	frames++;

	total_data.stop();
	total_data.start();

	if(endLogicQuant_){
		if(!started){
			if(curLogicQuant >= startLogicQuant_)
                start_stop();
		}
		else if(curLogicQuant >= endLogicQuant_){
			start_stop();
			if(autoExit_)
				ErrH.Exit();
			startLogicQuant_ = endLogicQuant_ = 0;
			started = 0;
		}
	}
}

void Profiler::setAutoMode(int startLogicQuant, int endLogicQuant, const char* title, const char* profileFile, bool autoExit) 
{
	startLogicQuant_ = startLogicQuant; 
	endLogicQuant_ = endLogicQuant;
	autoExit_ = autoExit;
	title_.init();
	title_ < title;
	profileFile_.init();
	profileFile_ < profileFile;
}

#include <set>
struct lessTimerData
{																	       
	bool operator()(const TimerData* c1, const TimerData* c2) const
	{
		return c1 -> group != c2 -> group ? c1 -> group < c2 -> group : c1 -> dt_sum > c2 -> dt_sum;
	}
};
			      
typedef std::multiset<TimerData*, lessTimerData> SortTable;

void Profiler::print(XBuffer& buf)
{
	total_ticks = ticks;
	total_frames = frames;
	total_time = milliseconds;
	time_factor = (double)milliseconds/ticks;

	char total_name[2048];

	buf < "Frames: " <= frames < "\r\n";
	buf < "Time interval: " <= milliseconds < " mS\r\n";
	buf < "Ticks: " < _i64toa(ticks, total_name, 10) < "\r\n";
	buf < "CPU: " <= (double)ticks/(milliseconds*1000.) < " MHz\r\n";
	sprintf(total_name, "%7.3f", frames*1000./milliseconds);
	buf < "FPS: " < total_name < "\r\n";
	buf < "Memory start: " <= memory < "\r\n";
	buf < "Memory end:   " <= total_memory_used() < "\r\n";

	print_separator(buf);
	print_header(buf);	
	print_separator(buf);	

	int group = -1000;
	TimerData total(total_name, 0, 1);

	SortTable sorted_counters;
	TimerDataList::iterator i;
	FOR_EACH(timers, i){
		TimerDataList::iterator j = i; ++j;
		for(; j != timers.end(); ++j)
			if((*i) -> group == (*j) -> group && !strcmp((*i) -> title, (*j) -> title)){
				**j += **i;
				goto add_and_skip;
				}
		sorted_counters.insert(*i);
add_and_skip:;
		}

	SortTable::iterator si;
	FOR_EACH(sorted_counters, si){
		TimerData& td = **si;
		if(group != td.group){
			if(group != -1000){
				sprintf(total_name, "___ Group %1i ___", group);
				total.print(buf);
				}
			total.clear();
			group = td.group;
			}

		td.print(buf);
		total += td;
		}

	sprintf(total_name, "___ Group %1i ___", group);
	total.print(buf);

	print_separator(buf);
	total_data.print(buf);
	print_separator(buf);	
}

double StatisticalData::sigma() const 
{ 
	double d2 = (x2_sum - x_sum*x_sum/n); 
	return n > 1 && d2 > 0 ? sqrt(d2/((double)n*(n - 1))) : 0; 
}

//////////////////////////////////////////////////////////////////////////////
HFONT MessageWindow::mono_font_ = 0;
HWND MessageWindow::stat_wnd_ = 0;
HWND MessageWindow::editbox_wnd_ = 0;
HWND MessageWindow::close_button_wnd_ = 0;

MessageWindow::MessageWindow()
{
	if(!mono_font_) {
		mono_font_ = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
								OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, FF_DONTCARE, "Courier New");
	}
}
MessageWindow::~MessageWindow()
{
	if(mono_font_) {
		DeleteObject(mono_font_);
		mono_font_ = 0;
	}
}

bool MessageWindow::modal_loop_done_ = false;
//INT  nResult;

int MessageWindow::runModalWindow(HWND hwndDialog, HWND hwndParent)
{
	if(hwndParent != NULL)
	EnableWindow(hwndParent,FALSE);

	MSG msg;

	//nResult = 0;

	for(modal_loop_done_ = false; !modal_loop_done_; WaitMessage())	{
		while(PeekMessage(&msg,0,0,0,PM_REMOVE)) {
			if(msg.message == WM_QUIT) {
				modal_loop_done_ = true;
				PostMessage(NULL,WM_QUIT,0,0);
				break;
			}

			if(!IsDialogMessage(hwndDialog,&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	if(hwndParent) {
		EnableWindow(hwndParent,TRUE);
	}

	DestroyWindow(hwndDialog);
	if(hwndParent) {
		SetForegroundWindow(hwndParent);
	}
	return 0;
}

void MessageWindow::showMessage(const char* text, HWND parent)
{
	//static const int delta = strlen("\r\r\n") - strlen("\n");
	//std::string message = text;
	//std::string::size_type pos = 0;
	//int len = strlen(message.c_str());
	//while(pos != std::string::npos && pos < len) {
	//	pos = message.find("\n", pos);
	//	if(pos != std::string::npos) {
    //        message.replace(pos, 1, "\r\r\n");
	//		pos += strlen("\r\r\n");
	//	}
	//}
	createWindow();
	SendMessage(editbox_wnd_, WM_SETTEXT, 0, LPARAM(text));
	ShowWindow(stat_wnd_, SW_SHOW);
	runModalWindow(stat_wnd_, parent);
}

bool MessageWindow::createWindow()
{
	HINSTANCE hInstance = 0;
    WNDCLASSEX wcx; 
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW |CS_VREDRAW;
    wcx.lpfnWndProc = statWndProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = hInstance/*hInstance*/;
    wcx.hIcon = LoadIcon(NULL,IDI_APPLICATION);
    wcx.hCursor = LoadCursor(NULL,IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)GetSysColorBrush(COLOR_3DFACE);
    wcx.lpszMenuName =  0;
    wcx.lpszClassName = windowClassname();
    wcx.hIconSm = NULL;
    RegisterClassEx(&wcx); 

    stat_wnd_ = CreateWindowEx(WS_EX_TOPMOST,
        windowClassname(),
        "Message",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL);

	if (!::IsWindow(stat_wnd_)) 
        return false;
	
	return true;
}

const int ID_CLOSE_BUTTON = 100;

LRESULT CALLBACK MessageWindow::statWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE:
		{
			CREATESTRUCT* create_struct = reinterpret_cast<CREATESTRUCT*>(lParam);
			RECT rcl; 
			rcl.left = 0;
			rcl.top = 0;
			rcl.right = 100;
			rcl.bottom = 100;

			editbox_wnd_ = CreateWindow("EDIT", "",
				ES_READONLY | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL |
				WS_BORDER | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
				0, 0, rcl.right - rcl.left, rcl.bottom - rcl.top, hWnd, NULL, HINSTANCE(GetWindowLong(hWnd, GWL_HINSTANCE)), NULL);

			close_button_wnd_ = CreateWindow("BUTTON", "Close",
				BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE,
				0, 0, rcl.right - rcl.left, rcl.bottom - rcl.top, hWnd, 0, HINSTANCE(GetWindowLong(hWnd, GWL_HINSTANCE)), 0);

			SendMessage(editbox_wnd_,
						UINT(WM_SETFONT),
						WPARAM(mono_font_),
						LPARAM(0));

			//SendMessage(editbox_wnd_, UINT(EM_FMTLINES), WPARAM(FALSE), 0); 

			SendMessage(close_button_wnd_,
						UINT(WM_SETFONT),
						WPARAM(GetStockObject(DEFAULT_GUI_FONT)),
						LPARAM(0));
			//lResult = SendMessage((HWND) hWndControl,      // handle to destination control     (UINT) WM_SETFONT,      // message ID     (WPARAM) wParam,      // = (WPARAM) () wParam;    (LPARAM) lParam      // = (LPARAM) () lParam; 
		}
		break;
	case WM_COMMAND:
		if(HWND(lParam) == close_button_wnd_) {
			modal_loop_done_ = true;
			DestroyWindow(hWnd);
		}
		break;
	case WM_SIZE:
		{
			RECT rc;
			GetClientRect(stat_wnd_,&rc);
			MoveWindow(editbox_wnd_,rc.left,rc.top,rc.right,rc.bottom - 40, TRUE);

			RECT button_rect;
			button_rect.top = rc.bottom - 36;
			button_rect.bottom = button_rect.top + 32;
			button_rect.left = (rc.right - rc.left) / 2 - 40;
			button_rect.right = button_rect.left + 80;
			MoveWindow(close_button_wnd_,button_rect.left,button_rect.top,
					   button_rect.right - button_rect.left,
					   button_rect.bottom - button_rect.top, TRUE);
			break;
		}/*
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd,&ps);
			//InvalidateRect(hWndList_,NULL,FALSE);
			EndPaint(hWnd,&ps);
			break;
		}
		*/
	case WM_DESTROY:
		modal_loop_done_ = true;
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		modal_loop_done_ = true;
		stat_wnd_ = 0;
		//console().Show(false);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


