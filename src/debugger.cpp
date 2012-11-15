#include "debugger.h"

#include <cstdio>

#ifndef DBG_EXCEPTION_HANDLED
#define DBG_EXCEPTION_HANDLED 0x00010001L
#endif

debugger_t::debugger_t()
{
    _pid         = 0;
    _isAttached  = false;
    _isPaused    = false;
    _doQuit      = false;

    _currentProcessId       = 0;
    _currentThreadId        = 0;
    _singleSteppingThreadId = 0;

    _hMainProcess = 0;
}

debugger_t::~debugger_t()
{
}

void debugger_t::attach_to_pid(uint32_t pid)
{
    BOOL r = DebugActiveProcess(pid);
    if (!r)
    {
        printf("Failed to attach to process pid 0x%x\n", pid);
        return;
    }

    _isAttached = true;
    _pid = pid;

    for (;;)
    {
        DEBUG_EVENT e;

        WaitForDebugEvent(&e, INFINITE);
        _currentProcessId = e.dwProcessId;
        _currentThreadId  = e.dwThreadId;
        _isPaused = true;

#if 1
        switch (e.dwDebugEventCode)
        {
            case EXCEPTION_DEBUG_EVENT:      puts("EXCEPTION_DEBUG_EVENT"); break;
            case CREATE_THREAD_DEBUG_EVENT:  puts("CREATE_THREAD_DEBUG_EVENT"); break;
            case CREATE_PROCESS_DEBUG_EVENT: puts("CREATE_PROCESS_DEBUG_EVENT"); break;
            case EXIT_THREAD_DEBUG_EVENT:    puts("EXIT_THREAD_DEBUG_EVENT"); break;
            case EXIT_PROCESS_DEBUG_EVENT:   puts("EXIT_PROCESS_DEBUG_EVENT"); break;
            case LOAD_DLL_DEBUG_EVENT:       puts("LOAD_DLL_DEBUG_EVENT"); break;
            case UNLOAD_DLL_DEBUG_EVENT:     puts("UNLOAD_DLL_DEBUG_EVENT"); break;
            case OUTPUT_DEBUG_STRING_EVENT:  puts("OUTPUT_DEBUG_STRING_EVENT"); break;
            case RIP_EVENT:                  puts("RIP_EVENT"); break;
            default:
                printf("other: %d\n", e.dwDebugEventCode);
        }
#endif

        switch (e.dwDebugEventCode)
        {
            case CREATE_PROCESS_DEBUG_EVENT:
            {
                if (!_hMainProcess)
                    _hMainProcess = e.u.CreateProcessInfo.hProcess;

                process_info_t pi = {
                    e.dwProcessId,
                    e.u.CreateProcessInfo.hProcess,
                    e.u.CreateProcessInfo.hThread
                };

                _process_info_list.push_back(pi);

                thread_info_t ti = {
                    e.dwThreadId,
                    e.u.CreateProcessInfo.hThread
                };

                _thread_info_list.push_back(ti);

                CloseHandle(e.u.CreateProcessInfo.hFile);
                break;
            }
            case CREATE_THREAD_DEBUG_EVENT:
            {
                thread_info_t ti = {
                    e.dwThreadId,
                    e.u.CreateProcessInfo.hThread
                };

                _thread_info_list.push_back(ti);
                break;
            }
            case LOAD_DLL_DEBUG_EVENT:
                CloseHandle(e.u.LoadDll.hFile);
                break;
            default:
                break;
        }

        if (e.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
            break;

        ContinueDebugEvent(_currentProcessId, _currentThreadId, DBG_CONTINUE);
        _isPaused = false;
    }
}

void debugger_t::detach()
{
    if (!_isAttached)
        return;

    DebugActiveProcessStop(_pid);
    _isAttached = false;
}

void debugger_t::kill_on_exit(bool do_kill)
{
    DebugSetProcessKillOnExit(do_kill);
}

void debugger_t::install_int3_breakpoint(breakpoint_t *bp)
{
    DWORD bytes_read, bytes_written;
    BOOL  r;

    r = ReadProcessMemory(bp->hProcess, (void*)bp->addr, &bp->orig_byte, 1, &bytes_read);
    // if (!r) ErrorExit("ReadProcessMemory");

    uint8_t cc = 0xCC;
    r = WriteProcessMemory(bp->hProcess, (void*)bp->addr, &cc, 1, &bytes_written);
    // if (!r) ErrorExit("WriteProcessMemory");

    r = FlushInstructionCache(bp->hProcess, (void*)bp->addr, 1);
    // if (!r) ErrorExit("FlushInstructionCache");
}

void debugger_t::uninstall_int3_breakpoint(breakpoint_t *bp)
{
    DWORD bytes_written;
    BOOL  r;

    r = WriteProcessMemory(bp->hProcess, (void*)bp->addr, &bp->orig_byte, 1, &bytes_written);
    // if (!r) ErrorExit("WriteProcessMemory");

    r = FlushInstructionCache(bp->hProcess, (void*)bp->addr, 1);
    // if (!r) ErrorExit("FlushInstructionCache");
}

void debugger_t::set_int3_breakpoint(DWORD addr,
                                     db_callback_t callback,
                                     const void *extra)
{
    breakpoint_t *bp;

    bp = _breakpoints.add_breakpoint(_hMainProcess, addr, callback, extra);

    install_int3_breakpoint(bp);
}

void debugger_t::message_loop()
{
    if (!_isAttached)
        return;

    DWORD dwContinueStatus = DBG_EXCEPTION_HANDLED;

    for (;;)
    {
        assert(_isPaused);

        if (_isPaused)
        {
            ContinueDebugEvent(_currentProcessId, _currentThreadId, dwContinueStatus);
            _isPaused = false;
        }

        DEBUG_EVENT e;
        WaitForDebugEvent(&e, INFINITE);
        _currentProcessId = e.dwProcessId;
        _currentThreadId  = e.dwThreadId;
        _isPaused = true;

        HANDLE hProcess = get_process_handle_for_id(_currentProcessId);
        HANDLE hThread  = get_thread_handle_for_id(_currentThreadId);

        if (_doQuit)
        {
            break;
        }

        dwContinueStatus = DBG_CONTINUE;

#if 0
        switch (e.dwDebugEventCode)
        {
            case EXCEPTION_DEBUG_EVENT:      puts("EXCEPTION_DEBUG_EVENT"); break;
            case CREATE_THREAD_DEBUG_EVENT:  puts("CREATE_THREAD_DEBUG_EVENT"); break;
            case CREATE_PROCESS_DEBUG_EVENT: puts("CREATE_PROCESS_DEBUG_EVENT"); break;
            case EXIT_THREAD_DEBUG_EVENT:    puts("EXIT_THREAD_DEBUG_EVENT"); break;
            case EXIT_PROCESS_DEBUG_EVENT:   puts("EXIT_PROCESS_DEBUG_EVENT"); break;
            case LOAD_DLL_DEBUG_EVENT:       puts("LOAD_DLL_DEBUG_EVENT"); break;
            case UNLOAD_DLL_DEBUG_EVENT:     puts("UNLOAD_DLL_DEBUG_EVENT"); break;
            case OUTPUT_DEBUG_STRING_EVENT:  puts("OUTPUT_DEBUG_STRING_EVENT"); break;
            case RIP_EVENT:                  puts("RIP_EVENT"); break;
            default:
                printf("other: %d\n", e.dwDebugEventCode);
        }
#endif

        switch (e.dwDebugEventCode)
        {
            case EXCEPTION_DEBUG_EVENT:
            {
                switch(e.u.Exception.ExceptionRecord.ExceptionCode)
                {
                    case EXCEPTION_BREAKPOINT:
                    {
                        // The process for handling int3 breakpoints is:
                        // 1. Backup EIP by one
                        // 2. Remove the int3 (0xCC) breakpoint and reinstall the original opcode
                        // 3. Set the thread in single-step mode
                        // 4. Let the debuggee step once
                        // 5. Reinstall the breakpoint after the original opcode has been run

                        CONTEXT lcContext;
                        lcContext.ContextFlags = CONTEXT_FULL;
                        BOOL r = GetThreadContext(hThread, &lcContext);
                        assert(r);

                        --lcContext.Eip;

                        breakpoint_t *bp = _breakpoints.get_breakpoint_for_addr(_hMainProcess, lcContext.Eip);

                        if (bp && bp->callback)
                        {
                            uninstall_int3_breakpoint(bp);

                            bp->callback(this, hProcess, hThread, bp->addr, bp->extra);

                            // Set single step mode
                            lcContext.EFlags |= 0x0100;
                            r = SetThreadContext(hThread, &lcContext);

                            _singleSteppingThreadId = _currentThreadId;
                            _restoringBreakpoint    = bp;
                        }

                        dwContinueStatus = DBG_EXCEPTION_HANDLED;
                        break;
                    }
                    case STATUS_SINGLE_STEP:
                    {
                        if (_restoringBreakpoint)
                        {
                            assert(_singleSteppingThreadId);
                            breakpoint_t *bp = _restoringBreakpoint;

                            // Disable single step mode
                            CONTEXT lcContext;
                            lcContext.ContextFlags = CONTEXT_FULL;
                            BOOL r = GetThreadContext(hThread, &lcContext);
                            assert(r);

                            lcContext.EFlags &= ~0x0100;

                            r = SetThreadContext(hThread, &lcContext);
                            assert(r);

                            install_int3_breakpoint(bp);
                            _restoringBreakpoint = 0;
                            _singleSteppingThreadId = 0;

                            dwContinueStatus = DBG_EXCEPTION_HANDLED;
                        }
                        break;
                    }
                    default:
                        dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                }
            }
            case CREATE_THREAD_DEBUG_EVENT:
                // Add to thread list
                break;
            case CREATE_PROCESS_DEBUG_EVENT:
                // Add to process list
                CloseHandle(e.u.CreateProcessInfo.hFile);
                break;
            case EXIT_THREAD_DEBUG_EVENT:
                // Remove from thread list
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                // breakpoints.remove_breakpoints_for_process(hMainProcess);
                _hMainProcess = 0;
                _doQuit = true;
                return;
                break;
            case LOAD_DLL_DEBUG_EVENT:
                CloseHandle(e.u.LoadDll.hFile);
                break;
            case UNLOAD_DLL_DEBUG_EVENT:
                break;
            case OUTPUT_DEBUG_STRING_EVENT:
                break;
            case RIP_EVENT:
                break;
            default:
                break;
        }
    }
}
