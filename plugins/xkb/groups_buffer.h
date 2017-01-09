#ifndef groups_buffer_h_
#define groups_buffer_h_

#include <stddef.h>
#include "libls/vector.h"
#include "libls/string.h"
#include "libls/compdep.h"

typedef struct {
    LSString _buf;
    LS_VECTOR_OF(size_t) _offsets;
} GroupsBuffer;

#define GROUPS_BUFFER_INITIALIZER ((GroupsBuffer) {LS_VECTOR_NEW(), LS_VECTOR_NEW()})

void
groups_buffer_assign(GroupsBuffer *gb, const char *layout);

LS_INHEADER
size_t
groups_buffer_size(const GroupsBuffer *gb)
{
    return gb->_offsets.size;
}

LS_INHEADER
const char *
groups_buffer_at(const GroupsBuffer *gb, size_t index)
{
    return gb->_buf.data + gb->_offsets.data[index];
}

LS_INHEADER
void
groups_buffer_destroy(const GroupsBuffer *gb)
{
    LS_VECTOR_FREE(gb->_buf);
    LS_VECTOR_FREE(gb->_offsets);
}

#endif
