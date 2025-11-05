#ifndef SLIT_FOREACH_SAFE_H
#define SLIT_FOREACH_SAFE_H

#include <sys/queue.h>

#ifndef SLIST_FOREACH_SAFE
#define	SLIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = SLIST_FIRST((head));				\
	    (var) && ((tvar) = SLIST_NEXT((var), field), 1);		\
	    (var) = (tvar))
#endif

#endif