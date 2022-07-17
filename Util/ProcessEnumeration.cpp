#include "stdafx.h"

//Корпорация Microsoft добавила набор функций под названием ToolHelp API в Windows 3.1, чтобы позволить сторонним разработчикам 
//получить доступ к системной информации, которая ранее была доступна только программистам Microsoft. При создании Windows 95,
//эти функции перекочевали в новую систему под названием ToolHelp32 API. Операционная система Windows NT c cамого создания
//содержала средства для получения подобной информации под названием "данные производительности". Интерфейс для доступа к данным
//производительности был крайне запутанным и неудобным (справедливости ради надо отметить, что начиная с Windows NT 4.0,
//Microsoft предоставляет библиотеку Performance Data Helper, значительно облегчающую получение данных производительности;
//мы воспользуемся этой библиотекой при реализации соответствующего метода перечисления процессов).
//Говорят, команда Windows NT долгое время сопротивлялась включению ToolHelp32 API в систему, тем не менее,
//начиная с Windows 2000, ToolHelp32 API присутствует и в этой операционной системе.

//Используя ToolHelp32 API, мы сначала создаем моментальный снимок (snapshot) списка процессов с помощью функции
//CreateToolhelp32Snapshot, а затем проходим по списку используя функции Process32First и Process32Next.
//Структура PROCESSENTRY32, заполняемая этими функциями, содержит всю необходимую информацию.
//Ниже приведен код функции EnumProcesses_ToolHelp, реализующей перечисление процессов с помощью ToolHelp32.

//Перед вызовом функций Process32First и Process32Next мы должны инициализировать поле dwSize структуры
//PROCESSENTRY32 таким образом, чтобы оно содержало размер структуры.
//Функции в свою очередь заносят в это поле количество байтов, записанных в структуру.
//Мы сравниваем это значение со смещением поля szExeFile в структуре, чтобы определить, было ли заполнено имя процесса. 

#include <tlhelp32.h>

//typedef struct tagPROCESSENTRY32
//{
//	DWORD   dwSize;
//	DWORD   cntUsage;
//	DWORD   th32ProcessID;          // this process
//	ULONG_PTR th32DefaultHeapID;
//	DWORD   th32ModuleID;           // associated exe
//	DWORD   cntThreads;
//	DWORD   th32ParentProcessID;    // this process's parent process
//	LONG    pcPriClassBase;         // Base priority of process's threads
//	DWORD   dwFlags;
//	CHAR    szExeFile[MAX_PATH];    // Path
//} PROCESSENTRY32;

typedef bool (*PFNENUMPROC)(DWORD id, const char* name, DWORD lParam);

bool EnumProcesses_ToolHelp(PFNENUMPROC pfnEnumProc, DWORD lParam)
{
	xassert(pfnEnumProc);

	HINSTANCE hKernel;
	HANDLE (WINAPI * _CreateToolhelp32Snapshot)(DWORD, DWORD);
	BOOL (WINAPI * _Process32First)(HANDLE, PROCESSENTRY32 *);
	BOOL (WINAPI * _Process32Next)(HANDLE, PROCESSENTRY32 *);

	// получаем описатель KERNEL32.DLL
	hKernel = GetModuleHandle("kernel32.dll");
	xassert(hKernel);

	// находим необходимые функции в KERNEL32.DLL
	*(FARPROC *)&_CreateToolhelp32Snapshot =
		GetProcAddress(hKernel, "CreateToolhelp32Snapshot");

	*(FARPROC *)&_Process32First =
		GetProcAddress(hKernel, "Process32First");
	*(FARPROC *)&_Process32Next =
		GetProcAddress(hKernel, "Process32Next");

	if (_CreateToolhelp32Snapshot == NULL ||
		_Process32First == NULL ||
		_Process32Next == NULL)
		return false;

	HANDLE hSnapshot;
	PROCESSENTRY32 Entry;

	// создаем моментальный снимок
	hSnapshot = _CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return false;

	// получаем информацию о первом процессе
	Entry.dwSize = sizeof(Entry);
	if (!_Process32First(hSnapshot, &Entry))
		return false;

	// перечисляем остальные процессы
	do
	{
		LPTSTR pszProcessName = NULL;

		if (Entry.dwSize > offsetof(PROCESSENTRY32, szExeFile))
		{
			pszProcessName = strrchr(Entry.szExeFile, '\\');
			if (pszProcessName == NULL)
				pszProcessName = Entry.szExeFile;
		}

		if (!pfnEnumProc(Entry.th32ProcessID, pszProcessName, lParam))
			break;

		Entry.dwSize = sizeof(Entry);
	}
	while (_Process32Next(hSnapshot, &Entry));

	CloseHandle(hSnapshot);
	return true;
}
