#include "groups_buffer.h"

#include <string.h>

void
groups_buffer_assign(GroupsBuffer *gb, const char *layout)
{
    const size_t nlayout = strlen(layout);
    ls_string_assign_b(&gb->_buf, layout, nlayout + 1);

    LS_VECTOR_CLEAR(gb->_offsets);
    LS_VECTOR_PUSH(gb->_offsets, 0);
    int balance = 0;
    for (size_t i = 0; i < nlayout; ++i) {
        switch (layout[i]) {
        case '(':
            ++balance;
            break;
        case ')':
            --balance;
            break;
        case ',':
            if (balance == 0) {
                LS_VECTOR_PUSH(gb->_offsets, i + 1);
                gb->_buf.data[i] = '\0';
            }
            break;
        }
    }
}
