#ifndef ls_algo_h_
#define ls_algo_h_

// Minimum of /A_/ and /B_/.
#define LS_MIN(A_, B_) ((A_) < (B_) ? (A_) : (B_))

// Maximum of /A_/ and /B_/.
#define LS_MAX(A_, B_) ((A_) > (B_) ? (A_) : (B_))

// Number of elements in array /Arr_/.
#define LS_ARRAY_SIZE(Arr_) (sizeof(Arr_) / sizeof((Arr_)[0]))

#endif
