#ifndef ls_vector_h_
#define ls_vector_h_

#include <stddef.h>

#include "alloc_utils.h"

#define LS_VECTOR_OF(Type_) \
    struct { \
        Type_ *data; \
        size_t size, capacity; \
    }

#define LS_VECTOR_NEW() \
    {NULL, 0, 0}

#define LS_VECTOR_NEW_RESERVE(Type_, Capacity_) \
    {LS_XNEW(Type_, Capacity_), 0, Capacity_}

#define LS_VECTOR_INIT(Vec_) \
    do { \
        (Vec_).data = NULL; \
        (Vec_).size = 0; \
        (Vec_).capacity = 0; \
    } while (0)

#define LS_VECTOR_INIT_RESERVE(Vec_, Capacity_) \
    do { \
        (Vec_).data = ls_xmalloc(Capacity_, sizeof(*(Vec_).data)); \
        (Vec_).size = 0; \
        (Vec_).capacity = (Capacity_); \
    } while (0)

#define LS_VECTOR_CLEAR(Vec_) \
    do { \
        (Vec_).size = 0; \
    } while (0)

#define LS_VECTOR_RESERVE(Vec_, ReqCapacity_) \
    do { \
        if ((Vec_).capacity < (ReqCapacity_)) { \
            (Vec_).capacity = (ReqCapacity_); \
            (Vec_).data = ls_xrealloc((Vec_).data, (Vec_).capacity, sizeof(*(Vec_).data)); \
        } \
    } while (0)

#define LS_VECTOR_ENSURE(Vec_, ReqCapacity_) \
    do { \
        while ((Vec_).capacity < (ReqCapacity_)) { \
            (Vec_).data = ls_x2realloc((Vec_).data, &(Vec_).capacity, sizeof(*(Vec_).data)); \
        } \
    } while (0)

#define LS_VECTOR_SHRINK(Vec_) \
    do { \
        (Vec_).capacity = (Vec_).size; \
        (Vec_).data = ls_xrealloc((Vec_).data, (Vec_).capacity, sizeof(*(Vec_).data)); \
    } while (0)

#define LS_VECTOR_PUSH(Vec_, Elem_) \
    do { \
        if ((Vec_).capacity == (Vec_).size) { \
            (Vec_).data = ls_x2realloc((Vec_).data, &(Vec_).capacity, sizeof(*(Vec_).data)); \
        } \
        (Vec_).data[(Vec_).size++] = (Elem_); \
    } while (0)

#define LS_VECTOR_REMOVE(Vec_, StartPos_, NRemove_) \
    do { \
        if ((Vec_).size != (StartPos_) + (NRemove_)) { \
            memmove((Vec_).data + (StartPos_), \
                    (Vec_).data + (StartPos_) + (NRemove_), \
                    ((Vec_).size - (StartPos_) - (NRemove_)) * sizeof(*(Vec_).data)); \
        } \
        (Vec_).size -= (NRemove_); \
    } while (0)

#define LS_VECTOR_POP(Vec_) (Vec_).data[--(Vec_).size]

#define LS_VECTOR_FREE(Vec_) free((Vec_).data)

#endif
