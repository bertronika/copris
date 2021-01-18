#define PRSET_LEN 12
#define INSTRUC_LEN 5

#define C_RESET 1
#define C_BELL  2
#define C_DBLW  3
#define C_ULON  4
#define C_ULOFF 5
#define C_BON   6
#define C_BOFF  7
#define C_ION   8
#define C_IOFF  9

extern char printerset[][10][INSTRUC_LEN + 1];
