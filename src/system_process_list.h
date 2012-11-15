#ifndef SYSTEM_PROCESS_LIST_H
#define SYSTEM_PROCESS_LIST_H

#include "common.h"

#include <string>
#include <vector>

struct system_process_desc_t
{
	uint32_t    pid;
	std::string name;
	std::string path;

	system_process_desc_t(uint32_t pid, const std::string &name, const std::string &path)
		: pid(pid), name(name), path(path)
	{}
};

class system_process_list_t
{
	std::vector<system_process_desc_t> _system_processes;

	void refresh();

public:
	uint32_t get_pid_for_name(const std::string &name);
};

#endif
