sed -i.bak "/^#define _stprintf snprintf/i\
#define MAX_STRING_SIZE 1024" src/burner/gami.mm
