#ifndef ls_osdep_h_
#define ls_osdep_h_

// We can't define /_GNU_SOURCE/ here, nor can we include "probes.generated.h", as
// it may be located outside of the source tree: /libls/ is built with /libls/'s
// binary directory included, unlike everything that includes its headers.
// So no in-header functions.

// The behaviour is same as calling /pipe(pipefd)/, except that both file descriptors are made
// close-on-exec. If the latter fails, the pipe is destroyed, /-1/ is returned and /errno/ is set.
int
ls_cloexec_pipe(int pipefd[2]);

// The behaviour is same as calling /socket(domain, type, protocol)/, except that the file
// descriptor is make close-on-exec. If the latter fails, the pipe is destroyed, /-1/ is reurned and
// /errno/ is set.
int
ls_cloexec_socket(int domain, int type, int protocol);

#endif
