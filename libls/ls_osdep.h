/*
 * Copyright (C) 2015-2025  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ls_osdep_h_
#define ls_osdep_h_

// We can't define /_GNU_SOURCE/ here, nor can we include "probes.generated.h", as
// it may be located outside of the source tree: /libls/ is built with /libls/'s
// binary directory included, unlike everything that includes its headers.
// So no in-header functions.

// The behavior is same as calling /pipe(pipefd)/, except that both file descriptors are made
// close-on-exec. If the latter fails, the pipe is destroyed, /-1/ is returned and /errno/ is set.
int ls_cloexec_pipe(int pipefd[2]);

// The behavior is same as calling /socket(domain, type, protocol)/, except that the file
// descriptor is make close-on-exec. If the latter fails, the pipe is destroyed, /-1/ is reurned and
// /errno/ is set.
int ls_cloexec_socket(int domain, int type, int protocol);

#endif
