#include <windows.h>
#include <iostream>
#include <string>
#include <tlhelp32.h>
#include <vector>
#include <tuple>

using namespace std;

string ProcessIdToName(DWORD processId)
{
	string ret;
	HANDLE handle = OpenProcess(
		PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		FALSE,
		processId /* This is the PID, you can find one from windows task manager */
	);
	if (handle)
	{
		DWORD buffSize = 1024;
		CHAR buffer[1024];
		if (QueryFullProcessImageNameA(handle, 0, buffer, &buffSize))
		{
			ret = buffer;
		}
		else
		{
			printf("Error GetModuleBaseNameA : %lu", GetLastError());
		}
		CloseHandle(handle);
	}
	else
	{
		cout << "Error OpenProcess :" << GetLastError() << endl;
	}
	return ret;
}
wstring getProccesExe(DWORD processId) {
	DWORD pid = 0;

	// Create toolhelp snapshot.
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 process;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);
	size_t limit = 1 << 24; //prevents infinite loop

	// Walkthrough all processes.
	if (Process32First(snapshot, &process))
	{
		do
		{
			if (process.th32ProcessID == processId) {
				CloseHandle(snapshot);
				return process.szExeFile;
			}

		} while (limit-- > 0 && Process32Next(snapshot, &process));
	}
	CloseHandle(snapshot);
	return wstring();

}



BOOL CALLBACK getWindowedProcceses(HWND hwnd, LPARAM lParam) {
	const DWORD TITLE_SIZE = 1024;
	WCHAR windowTitle[TITLE_SIZE];

	GetWindowTextW(hwnd, windowTitle, TITLE_SIZE);

	int length = ::GetWindowTextLength(hwnd);
	wstring title(&windowTitle[0]);
	if (!IsWindowVisible(hwnd) || length == 0 || title == L"Program Manager") {
		return TRUE;
	}

	DWORD dwProcId = 0;
	GetWindowThreadProcessId(hwnd, &dwProcId);
	vector<tuple<wstring, wstring, DWORD, HWND, bool>>& processesInfo =
		*reinterpret_cast<vector<tuple<wstring, wstring, DWORD, HWND, bool>>*>(lParam);
	tuple <wstring, wstring, DWORD, HWND, bool> processInfo;
	wstring processExe = getProccesExe(dwProcId);
	processInfo = make_tuple(title, processExe, dwProcId, hwnd, IsWindowVisible(hwnd));
	processesInfo.push_back(processInfo);

	return TRUE;
}

void suspendOrResume(DWORD processId, bool resume) {
	HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	THREADENTRY32 threadEntry;
	threadEntry.dwSize = sizeof(THREADENTRY32);

	Thread32First(hThreadSnapshot, &threadEntry);

	size_t limit = 1 << 24; //prevents infinite loop
	do {
		if (threadEntry.th32OwnerProcessID == processId) {
			HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE,
				threadEntry.th32ThreadID);
			if (hThread) {
				resume ? ResumeThread(hThread) : SuspendThread(hThread);
				CloseHandle(hThread);
			}
		}
	} while (limit-- > 0 && Thread32Next(hThreadSnapshot, &threadEntry));

	CloseHandle(hThreadSnapshot);
}


class Procceses {
public:
	vector<tuple<wstring, wstring, DWORD, HWND, bool>> processesInfo;
	vector<tuple<wstring, wstring, DWORD, HWND>> proccessesWithWindows;
	Procceses() {
		EnumWindows(getWindowedProcceses, reinterpret_cast<LPARAM>(&processesInfo));
		for (const auto& processInfo : processesInfo) {
			if (get<4>(processInfo)) {
				wstring title = get<0>(processInfo);
				wstring processExe = get<1>(processInfo);
				DWORD dwProcId = get<2>(processInfo);
				HWND hwnd = get<3>(processInfo);
				proccessesWithWindows.push_back(make_tuple(title, processExe, dwProcId, hwnd));
			}

		}
	}
	void FocusWindow(wstring proccesName) {
		for (const auto& processInfo : proccessesWithWindows) {
			if (get<1>(processInfo) == proccesName) {
				SetForegroundWindow(get<3>(processInfo));
				break;
			}
		}
	}
	tuple<wstring, wstring, DWORD> GetActiveWindow()
	{
		wstring title;
		HWND handle = GetForegroundWindow();
		DWORD dwProcId = 0;
		tuple<wstring, wstring, DWORD> processInfo;

		GetWindowThreadProcessId(handle, &dwProcId);
		int len = GetWindowTextLengthW(handle) + 1;
		wchar_t* omgtitle = new wchar_t[len];
		GetWindowTextW(handle, omgtitle, len);
		title += omgtitle;
		string processName = ProcessIdToName(dwProcId);
		wstring wstr(processName.begin(), processName.end());
		wstring processExe = getProccesExe(dwProcId);
		processInfo = make_tuple(title, processExe, dwProcId);
		return processInfo;
	}

};


int main() {
	Procceses myObj;
	for (const auto& processInfo : myObj.proccessesWithWindows) {
		wcout << get<1>(processInfo) << endl;
	}

	return 0;
}