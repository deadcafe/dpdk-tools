

#include "thread.h"


struct beef_thread_s *
beef_thread_create(unsigned thread_id,
                   unsigned lcore_id)
{
    struct beef_thread_s *th;

    th = rte_zmalloc_socket("thread", sizeof(*th), RTE_CACHE_LINE_SIZE,
                            rte_lcore_to_socket_id(lcore_id));
    if (th) {
        th->thread_id = thread_id;
        th->lcore_id = lcore_id;

        STAILQ_INIT(&th->thread_head);
        STAILQ_INIT(&th->task_head);
    }
    return th;
}

struct beef_task_s *
beef_task_create(struct beef_thread_s *th,
                 const char *name,
                 struct beef_port_s *in_port)
{



}
