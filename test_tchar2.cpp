#define TCHAR_DEFINED 1
#if !defined(TCHAR_DEFINED) || TCHAR_DEFINED != 1
#error "TCHAR_DEFINED not properly set"
#else
typedef char TCHAR;
#endif
int main() { TCHAR test; return 0; } 