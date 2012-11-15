#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "common.h"

#include "breakpoint_list.h"

#include <vector>

class debugger_t
{
    DWORD         _pid;
    bool          _isAttached;
    bool          _isPaused;
    volatile bool _doQuit;

    DWORD         _currentProcessId;
    DWORD         _currentThreadId;
    DWORD         _singleSteppingThreadId;
    breakpoint_t *_restoringBreakpoint;

    HANDLE        _hMainProcess;

    breakpoint_list_t _breakpoints;

    struct process_info_t
    {
        DWORD  processId;
        HANDLE hProcess;
        HANDLE hThread;
    };
    std::vector<process_info_t> _process_info_list;

    HANDLE get_process_handle_for_id(DWORD processId)
    {
        for (const process_info_t &pi : _process_info_list)
            if (pi.processId == processId)
                return pi.hProcess;
        return 0;
    }

    struct thread_info_t {
        DWORD  threadId;
        HANDLE hThread;
    };
    std::vector<thread_info_t> _thread_info_list;

    HANDLE get_thread_handle_for_id(DWORD threadId)
    {
        for (const thread_info_t &ti : _thread_info_list)
            if (ti.threadId == threadId)
                return ti.hThread;
        return 0;
    }

    void install_int3_breakpoint(breakpoint_t *bp);
    void uninstall_int3_breakpoint(breakpoint_t *bp);

public:
    debugger_t();
    ~debugger_t();

    void attach_to_pid(uint32_t pid);
    void detach();
    void kill_on_exit(bool do_kill);

    void set_int3_breakpoint(DWORD addr,
                             db_callback_t callback,
                             const void *extra);

    void message_loop();
};

#endif
