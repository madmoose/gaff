#include "system_process_list.h"

#include <algorithm>

DWORD WINAPI GetProcessImageFileName(
	HANDLE hProcess,
	LPTSTR lpImageFileName,
	DWORD nSize
);

void system_process_list_t::refresh()
{
	DWORD *processes = 0, cbNeeded, cProcesses;
	unsigned int array_size = 256;

	do {
		free(processes);
		processes = (DWORD *)calloc(array_size, sizeof(DWORD));
		assert(processes);

		BOOL r = EnumProcesses(processes, array_size * sizeof(DWORD), &cbNeeded);
		assert(r);

		cProcesses = cbNeeded / sizeof(DWORD);
		assert(cProcesses <= array_size);
		array_size *= 2;
	} while (cProcesses == array_size);

	std::sort(processes, processes + cProcesses);

	_system_processes.reserve(cProcesses);

	for (DWORD i = 0; i != cProcesses; ++i)
	{
		DWORD processID = processes[i];

		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		                              PROCESS_VM_READ,
		                              FALSE, processID);

		if (!hProcess)
			continue;

		HMODULE hMod;
		DWORD   cbNeeded2;

		char szProcessName[1024] = "";
		char lpImageFileName[1024] = "";

		// Only get the first module
		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded2))
			GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName));

		GetProcessImageFileName(hProcess, lpImageFileName, 1024);

		_system_processes.push_back(system_process_desc_t(processID, szProcessName, lpImageFileName));

		printf("%4d %s\n", processID, szProcessName);

		CloseHandle(hProcess);
	}
}

uint32_t system_process_list_t::get_pid_for_name(const std::string &name)
{
	if (_system_processes.empty())
		refresh();

	for (const auto &desc : _system_processes)
		if (desc.name == name)
			return desc.pid;

	return 0;
}
