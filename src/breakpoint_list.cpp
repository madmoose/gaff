#include "breakpoint_list.h"

#include <algorithm>

breakpoint_t *breakpoint_list_t::add_breakpoint(HANDLE         hProcess,
                                                DWORD          addr,
                                                db_callback_t  callback,
                                                const void    *extra)
{
    breakpoint_t bp;
    bp.hProcess = hProcess;
    bp.addr     = addr;
    bp.callback = callback;
    bp.extra    = extra;

    breakpoints.push_back(bp);

    return &breakpoints.back();
}

breakpoint_t *breakpoint_list_t::get_breakpoint_for_addr(HANDLE hProcess, DWORD addr)
{
    for (breakpoint_t &bp : breakpoints)
        if (bp.hProcess == hProcess && bp.addr == addr)
            return &bp;

    return 0;
}

void breakpoint_list_t::remove_breakpoint_for_addr(HANDLE hProcess, DWORD addr)
{
    auto it = std::find_if(breakpoints.begin(), breakpoints.end(),
        [hProcess, addr](const breakpoint_t &bp){ return bp.hProcess == hProcess && bp.addr == addr; });

    breakpoints.erase(it);
}

void breakpoint_list_t::remove_breakpoints_for_process(HANDLE hProcess)
{
    auto end = std::remove_if(breakpoints.begin(), breakpoints.end(),
        [hProcess](const breakpoint_t &bp){ return bp.hProcess == hProcess; });

    breakpoints.erase(end, breakpoints.end());
}
