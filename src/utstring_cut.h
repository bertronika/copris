// An addition to utstring.h that terminates the string at given index
#define utstring_cut(s,n)  \
    do {                   \
        (s)->i=(n);        \
        (s)->d[(n)]='\0';  \
    } while (0)
