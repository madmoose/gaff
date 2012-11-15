#include "common.h"

#include "debugger.h"
#include "system_process_list.h"

#include <cstdio>

void read_stack(HANDLE hProcess, HANDLE hThread, DWORD *stack, size_t size)
{
	CONTEXT lcContext;
	BOOL    r;
	DWORD   bytesRead;

	lcContext.ContextFlags = CONTEXT_FULL;
	r = GetThreadContext(hThread, &lcContext);
	// if (!r) ErrorExit("GetThreadContext");

	r = ReadProcessMemory(hProcess, (void*)lcContext.Esp, stack, 4*size, &bytesRead);
	// if (!r) ErrorExit("ReadProcessMemory");
}

void dump_stack(debugger_t *dbg, HANDLE hProcess, HANDLE hThread, DWORD addr, const void *v)
{
	// static int first_timestamp = -1;
	// if (first_timestamp == -1)
	// 	first_timestamp = timeGetTime();

	// printf("\nt = %08d\n", timeGetTime() - first_timestamp);

	if (v)
	{
		char *s = strdup((const char*)v);
		char *c = strchr(s, '#');
		if (c) *c = 0;
		printf("[%s] ", s);
		free(s);
	}
	else
		printf("[%08x] ", addr);

	const char *arg_types = 0;
	if (v) arg_types = strchr((const char*)v, '#');

	if (!arg_types)
	{
		DWORD stack[8];
		read_stack(hProcess, hThread, stack, 8);
		for (int i = 0; i != 8; ++i)
		{
			printf("\tst[%02d] = %08x", i, stack[i]);
			//printf("\tst[%02d] = %08x %f", i, stack[i], *(float*)(&stack[i]));
		}
	}
	else
	{
		char t;
		int argc = strlen(arg_types + 1) + 1;
		DWORD stack[argc];

		read_stack(hProcess, hThread, stack, argc);

		for (int i = 1; i != argc; ++i)
		{
			if (i > 1)
				printf(", ");

			switch (arg_types[i])
			{
				case 'd': printf("%4d",  stack[i]); break;
				case 'x': printf("%04x", stack[i]); break;
				case 'p': printf("%p",   stack[i]); break;
				case 'f': printf("%f", *(float*)&stack[i]); break;
				default:  printf("?"); break;
			}
		}
	}
	putchar('\n');
}

int main(int argc, char **argv)
{
	debugger_t dbg;
	system_process_list_t system_process_list;

	if (argc < 2)
		return -1;

	uint32_t pid = system_process_list.get_pid_for_name(argv[1]);

	printf("pid: %d\n", pid);

	dbg.attach_to_pid(pid);
	dbg.kill_on_exit(true);

	// These are some example breakpoints I've used for investigating Westwood's Blade Runner

	// dbg.set_int3_breakpoint(0x004558B0, dump_stack, "draw_actor_anim_frame#pddfffff");
	// dbg.set_int3_breakpoint(0x00456F48, dump_stack, "draw_item_anim_frame#pddddff");

	// dbg.set_int3_breakpoint(0x0042A874, dump_stack, "make_table_1#d");
	// dbg.set_int3_breakpoint(0x0042A898, dump_stack, "make_table_2#d");
	// dbg.set_int3_breakpoint(0x0042A8BC, dump_stack, "make_table_3#d");

	// dbg.set_int3_breakpoint(0x0042A8CC, dump_stack, "make_table_4#d");
	// dbg.set_int3_breakpoint(0x0042A8F0, dump_stack, "make_table_5#d");
	// dbg.set_int3_breakpoint(0x0042A914, dump_stack, "make_table_6#d");

	// dbg.set_int3_breakpoint(0x0042AA04, dump_stack, "draw_slice_fun_1#dpp");
	// dbg.set_int3_breakpoint(0x0042AB48, dump_stack, "draw_slice_fun_2#dpp");
	// dbg.set_int3_breakpoint(0x0042ACD0, dump_stack, "draw_slice_fun_3#dpp");

	dbg.message_loop();

	dbg.detach();

	return 0;
}
