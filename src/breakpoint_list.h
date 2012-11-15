#ifndef BREAKPOINT_LIST_H
#define BREAKPOINT_LIST_H

#include "common.h"

#include <vector>

struct breakpoint_t
{
    HANDLE         hProcess;
    DWORD          addr;
    uint8_t        orig_byte;
    db_callback_t  callback;
    const void    *extra;
};

class breakpoint_list_t
{
    typedef std::vector<breakpoint_t> breakpoints_t;

    breakpoints_t breakpoints;

public:
    breakpoint_t *add_breakpoint(HANDLE         hProcess,
                                 DWORD          addr,
                                 db_callback_t  callback,
                                 const void    *extra);

    breakpoint_t *get_breakpoint_for_addr(HANDLE hProcess, DWORD addr);

    void remove_breakpoint_for_addr(HANDLE hProcess, DWORD addr);
    void remove_breakpoints_for_process(HANDLE hProcess);
};

#endif
