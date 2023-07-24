#include "stdafx.h"

//���������� Microsoft �������� ����� ������� ��� ��������� ToolHelp API � Windows 3.1, ����� ��������� ��������� ������������� 
//�������� ������ � ��������� ����������, ������� ����� ���� �������� ������ ������������� Microsoft. ��� �������� Windows 95,
//��� ������� ������������ � ����� ������� ��� ��������� ToolHelp32 API. ������������ ������� Windows NT c c����� ��������
//��������� �������� ��� ��������� �������� ���������� ��� ��������� "������ ������������������". ��������� ��� ������� � ������
//������������������ ��� ������ ���������� � ��������� (�������������� ���� ���� ��������, ��� ������� � Windows NT 4.0,
//Microsoft ������������� ���������� Performance Data Helper, ����������� ����������� ��������� ������ ������������������;
//�� ������������� ���� ����������� ��� ���������� ���������������� ������ ������������ ���������).
//�������, ������� Windows NT ������ ����� �������������� ��������� ToolHelp32 API � �������, ��� �� �����,
//������� � Windows 2000, ToolHelp32 API ������������ � � ���� ������������ �������.

//��������� ToolHelp32 API, �� ������� ������� ������������ ������ (snapshot) ������ ��������� � ������� �������
//CreateToolhelp32Snapshot, � ����� �������� �� ������ ��������� ������� Process32First � Process32Next.
//��������� PROCESSENTRY32, ����������� ����� ���������, �������� ��� ����������� ����������.
//���� �������� ��� ������� EnumProcesses_ToolHelp, ����������� ������������ ��������� � ������� ToolHelp32.

//����� ������� ������� Process32First � Process32Next �� ������ ���������������� ���� dwSize ���������
//PROCESSENTRY32 ����� �������, ����� ��� ��������� ������ ���������.
//������� � ���� ������� ������� � ��� ���� ���������� ������, ���������� � ���������.
//�� ���������� ��� �������� �� ��������� ���� szExeFile � ���������, ����� ����������, ���� �� ��������� ��� ��������. 

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

	// �������� ��������� KERNEL32.DLL
	hKernel = GetModuleHandle("kernel32.dll");
	xassert(hKernel);

	// ������� ����������� ������� � KERNEL32.DLL
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

	// ������� ������������ ������
	hSnapshot = _CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return false;

	// �������� ���������� � ������ ��������
	Entry.dwSize = sizeof(Entry);
	if (!_Process32First(hSnapshot, &Entry))
		return false;

	// ����������� ��������� ��������
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