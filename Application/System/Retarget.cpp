//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : Retarget.cpp
 Version     : V1.01
 By          : Wey. Silver Grid

 Description : Semihosting disable + C runtime syscall stubs.
               Prevents STM32 from blocking at CRT startup when no debugger
               is attached (the "click run three times" semihosting trap).

               References:
                 ee-nav.com/4954.html — semihosting analysis
                 ARM Compiler 6 — __use_no_semihosting

 Date       : 2026.07.01 (V1.01 — fixed _sys_open return valid handle for :tt)
              2026.07.01 (V1.00 — initial implementation)
*/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifndef __vmSIMULATOR__

// Tell the ARM C runtime linker to use non-semihosting library variants.
// Without this, the standard C library's startup code (before main) attempts
// semihosting SWI to open stdin/stdout/stderr — blocking forever if no
// debugger is attached.
extern "C" {
  __asm(".global __use_no_semihosting");
}

//-----------------------------------------------------------------------------
// Minimal syscall stubs required by the non-semihosting C runtime.
// All return "not implemented" / "not available" — just enough to let the
// CRT finish initialising so main() can run.
//-----------------------------------------------------------------------------

extern "C" {

#include <stdio.h>

/// Called on exit() / return from main.  Embedded firmware never returns.
void _sys_exit(int return_code)
{
  (void)return_code;
  while (1) { __asm("nop"); }
}

/// Called by CRT startup to open stdin/stdout/stderr (":tt" magic name).
/// Must return a valid handle or the CRT aborts → _sys_exit → lockup.
/// The CRT opens ":tt" up to 3 times: stdin(0), stdout(1), stderr(2).
/// We track the call count and return the matching fd number.
int _sys_open(const char* name, int openmode)
{
  if (name != 0 && name[0] == ':') {
    // Standard stream open — return fd = 1 (stdout) for all.
    // A single dummy handle is sufficient since _sys_write discards all output.
    return 1;   // non-negative = success, CRT proceeds
  }
  // User file open — not supported
  (void)openmode;
  return -1;
}

int _sys_close(int fh)
{
  // Standard streams never really close
  if (fh == 0 || fh == 1 || fh == 2) return 0;
  return -1;
}

int _sys_write(int fh, const unsigned char* buf, unsigned len, int mode)
{
  (void)fh;
  (void)buf;
  (void)mode;
  return (int)len;   // pretend success (data discarded)
}

int _sys_read(int fh, unsigned char* buf, unsigned len, int mode)
{
  (void)fh;
  (void)buf;
  (void)mode;
  return 0;          // always EOF
}

int _sys_istty(int fh)
{
  // stdin(0), stdout(1), stderr(2) are "terminal" handles
  return (fh >= 0 && fh <= 2) ? 1 : 0;
}

int _sys_seek(int fh, long pos)
{
  (void)fh;
  (void)pos;
  return -1;
}

long _sys_flen(int fh)
{
  (void)fh;
  return -1;
}

/// Called if any low-level error message output is attempted.
void _ttywrch(int ch)
{
  (void)ch;
  // no-op — discard character
}

}  // extern "C"

#endif  // __vmSIMULATOR__
