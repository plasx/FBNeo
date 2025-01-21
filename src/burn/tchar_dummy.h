// tchar_dummy.h
#ifndef FBNEO_TCHAR_DUMMY_H
#define FBNEO_TCHAR_DUMMY_H

#ifndef _WIN32
  // For non-Windows platforms:
  #include <stdio.h>
  #include <stdarg.h>

  // Fake TCHAR is just 'char'
  typedef char TCHAR;

  // _T(x) is a no-op here, so _T("hello") => "hello"
  #define _T(x) x

  // Map _tfopen to fopen
  #define _tfopen(filename, mode) fopen(filename, mode)

  // If the code calls _stprintf(buf, "format", args...),
  // we can simply map it to sprintf, or better, snprintf for safety.
  // But we must handle variable arguments. The easiest is a macro that
  // calls a small inline function.
  static inline int _stprintf(char* buffer, const char* format, ...) {
      va_list args;
      va_start(args, format);
      // e.g. limit to 1024 chars. Adjust as needed:
      int ret = vsnprintf(buffer, 1024, format, args);
      va_end(args);
      return ret;
  }

#endif // !_WIN32

#endif // FBNEO_TCHAR_DUMMY_H