#ifndef _THREAD_H_
#define _THREAD_H_

#include <sys/queue.h>

struct beef_thread_s;

struct beef_usage_s {
    uint64_t nb_ins;
    uint64_t nb_outs;

    uint64_t nb_effects;
    uint64_t effect_tsc;

    uint64_t nb_idles;
    uint64_t idle_tsc;
};

struct beef_port_s;
struct beef_task_s {
    MARKER cacheline0;

    char name[32];
    STAILQ_ENTRY(fpe_core_task_s) node;
    struct beef_thread_s *th;
    void (*entry)(struct fpe_core_task_s *,
                  uint64_t *nb_ins,
                  uint64_t *nb_outs);

    MARKER cacheline1 __rte_cache_min_aligned;
    struct beef_usage_s usage;

    void *private;
    unsigned nb_out_ports;
    unsigned burst_size;

    struct beef_port_s *in_port;
    
    struct beef_port_s *out_ports[MAX_NB_OUT_PORTS];
} __rte_cache_aligned;
STAILQ_HEAD(beef_task_head_s, beef_task_s);

struct beef_db_s {
    void *array[64];
    uint64_t valid_bit;
};

STAILQ_HEAD(beef_thread_head_s, beef_thread_s);
struct beef_thread_s {
    MARKER cacheline0;
    STAILQ_ENTRY(beef_thread_s) node;

    struct beef_thread_head_s thread_head;	/* master only */
    struct beef_task_head_s task_head;

    unsigned thread_id;	/* master:0 slave:1~ */
    unsigned lcore_id;
    struct beef_port_s *null_port;
} __rte_cache_aligned;

#endif /* !_THREAD_H_ */
