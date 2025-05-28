#ifndef _TCHAR_H_
#define _TCHAR_H_

// Handle _T macro
#ifndef _T
 #if defined (UNICODE)
  #define _T(x) L ## x
 #else
  #define _T(x) x
 #endif
#endif

// Handle FBA_DEBUG_HELP_STRING
#if defined (FBA_DEBUG)
 #define FBA_DEBUG_HELP_STRING(s) s
#else
 #define FBA_DEBUG_HELP_STRING(s)
#endif

// Handle TCHAR type - avoid redefinitions when TCHAR_DEFINED is set
// Check for both existence and value (makefile sets -DTCHAR_DEFINED=1)
#if !defined(TCHAR_DEFINED) || TCHAR_DEFINED != 1
 #if defined (UNICODE)
  #if defined (_MSC_VER)
   #define TCHAR wchar_t
  #else
   typedef wchar_t TCHAR;
  #endif
  #define _UNICODE
 #else
  #if defined (_MSC_VER)
   #define TCHAR char
  #else
   typedef char TCHAR;
  #endif
 #endif
 #define TCHAR_DEFINED 1
#endif // TCHAR_DEFINED check

#endif // _TCHAR_H_ 