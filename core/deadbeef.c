#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include <rte_eal.h>

#include <rte_debug.h>
#include <rte_errno.h>
#include <rte_lcore.h>
#include <rte_memzone.h>
#include <rte_atomic.h>
#include <rte_pause.h>

#include "beef_cmdline.h"

static rte_usage_hook_t usage_hook;

struct thread_s {
    unsigned thread_id;		/* zero is master */
    unsigned lcore_id;
    unsigned nb_threads;
    volatile enum rte_lcore_state_t state;
} __rte_cache_aligned;


struct thread_table_s {
    struct thread_s threads[RTE_MAX_LCORE];
};


#define THREAD_TABLE	"ThreadTable"
#define INVALID_ID	((unsigned) -1)


static struct thread_table_s * Thread_Table_p;

static struct thread_table_s *
create_thread_table(unsigned nb_threads)
{
    if (!Thread_Table_p) {
        const struct rte_memzone *mz;

        mz = rte_memzone_reserve(THREAD_TABLE,
                                 sizeof(*Thread_Table_p),
                                 rte_socket_id(),
                                 RTE_MEMZONE_SIZE_HINT_ONLY |
                                 RTE_MEMZONE_2MB |
                                 RTE_MEMZONE_1GB);

        if (mz) {
            struct thread_table_s *tbl = mz->addr;

            for (unsigned i = 0; i < RTE_DIM(tbl->threads); i++) {
                tbl->threads[i].thread_id = INVALID_ID;
                tbl->threads[i].lcore_id = INVALID_ID;
                tbl->threads[i].nb_threads = nb_threads;
            }
            Thread_Table_p = tbl;
        }
    }
    return Thread_Table_p;
}

static void
setup_thread_table(struct thread_table_s *tbl,
                   unsigned thread_id,
                   unsigned lcore_id)
{
    tbl->threads[thread_id].thread_id = thread_id;
    tbl->threads[thread_id].lcore_id = lcore_id;
    tbl->threads[thread_id].state = WAIT;
}

static inline void
set_state(struct thread_s *th,
          enum rte_lcore_state_t state)
{
    th->state = state;
    rte_wmb();
    fprintf(stderr, "thread:%u state:%d\n", th->thread_id, state);
}

static inline void
state_slaves(struct thread_table_s *tbl,
             enum rte_lcore_state_t state)
{
    unsigned nb_threads = tbl->threads[0].nb_threads;
    for (unsigned i = 1; i < nb_threads; i++)
        set_state(&tbl->threads[i], state);
}

static inline void
stop_slaves(struct thread_table_s *tbl)
{
    state_slaves(tbl, WAIT);
}

static inline void
start_slaves(struct thread_table_s *tbl)
{
    state_slaves(tbl, RUNNING);
}

static inline void
finish_slaves(struct thread_table_s *tbl)
{
    state_slaves(tbl, FINISHED);
}

static inline enum rte_lcore_state_t
read_state(const struct thread_s *th)
{
    enum rte_lcore_state_t state = th->state;
    rte_rmb();
    return state;
}

static inline int
is_eq_state(const struct thread_s *th,
            enum rte_lcore_state_t state)
{
    return read_state(th) == state;
}

static inline int
is_runnging(const struct thread_s *th)
{
    return is_eq_state(th, RUNNING);
}

static inline int
is_waiting(const struct thread_s *th)
{
    return is_eq_state(th, WAIT);
}

static inline int
is_finished(const struct thread_s *th)
{
    return is_eq_state(th, FINISHED);
}


static int
thread_entry(void *arg)
{
    struct thread_table_s *tbl = arg;
    struct thread_s *th = NULL;
    int ret = -1;

    for (unsigned i = 0; i < RTE_DIM(tbl->threads); i++) {
        if (tbl->threads[i].lcore_id == rte_lcore_id())
            th = &tbl->threads[i];
    }

    if (th) {
        ret = 0;
        fprintf(stderr, "hello slave. thread:%u lcore:%u\n",
                th->thread_id, th->lcore_id);

        set_state(th, RUNNING);

        enum rte_lcore_state_t state;

        while ((state = read_state(th)) != FINISHED) {
            rte_pause();
        }

        fprintf(stderr, "bye slave. thread:%u lcore:%u\n",
                th->thread_id, th->lcore_id);
    }
    return ret;
}

static void
master_loop(struct thread_table_s *tbl,
            const char *in_path,
            const char *out_path)
{
    struct thread_s *th = &tbl->threads[0];

    fprintf(stderr, "hello master. lcore:%u\n", th->lcore_id);

    unsigned nb_threads = th->nb_threads - 1;
    for (unsigned i = 1; i < nb_threads; i++)
        while (!is_runnging(&tbl->threads[i]))
            rte_pause();

    fprintf(stderr, "all slaves are up\n");

    beef_cmdline_loop(in_path, out_path);

    finish_slaves(tbl);

    rte_eal_mp_wait_lcore();
    fprintf(stderr, "bye master. lcore:%u\n", th->lcore_id);
}

static void
core_usage(const char *prog)
{
    fprintf(stderr,
            "core module usage:\n"
            "%s [--in PATH] [--out PATH] [--help]\n"
            "  --pipe\tstdin,stdout path\n"
            "  --help\tthis message\n",
            prog);
}

struct core_config_s {
    const char *in_path;
    const char *out_path;
};

static int
core_parse_args(struct core_config_s *config,
                int argc,
                char **argv)
{
    char *prog = argv[0];
    static const struct option long_option[] = {
        { "in", required_argument, NULL, 'i', },
        { "out", required_argument, NULL, 'o', },
        { "help", no_argument, NULL, 'h', },
        { NULL, no_argument, NULL, 0, },
    };
    int opt;
    int ret;
    const int old_optind = optind;
    const int old_optopt = optopt;
    char * const old_optarg = optarg;

    optind = 1;
    while ((opt = getopt_long(argc, argv, "hi:o:",
                              long_option, NULL)) != EOF) {
        switch (opt) {
        case 'i':
            config->in_path = optarg;
            break;

        case 'o':
            config->out_path = optarg;
            break;

        case 'h':
        default:
            core_usage(prog);
            ret = -1;
            goto end;
        }
    }

    if (optind >= 0)
        argv[optind - 1] = prog;
    ret = optind - 1;

 end:
    optind = old_optind;
    optopt = old_optopt;
    optarg = old_optarg;
    return ret;
}

int
main(int argc,
     char **argv)
{
    struct core_config_s config;

    memset(&config, 0, sizeof(config));

    usage_hook = rte_set_application_usage_hook(core_usage);
    int n = rte_eal_init(argc, argv);
    if (n < 0)
        rte_panic("cannot initialize EAL: %s\n", rte_strerror(rte_errno));

    argc -= n;
    argv += n;

    if (rte_eal_process_type() != RTE_PROC_PRIMARY) {
        if (!rte_eal_primary_proc_alive(NULL))
            rte_exit(EXIT_FAILURE, "No primary DPDK process is running.\n");
    }

    n = core_parse_args(&config, argc, argv);
    if (n < 0)
        rte_exit(EXIT_FAILURE, "invalid core args\n");

    struct thread_table_s *tbl;
    tbl = create_thread_table(rte_lcore_count());
    if (!tbl) {
        rte_exit(EXIT_FAILURE, "cannot create thread table\n");
    }

    unsigned thread_id = 1;
    unsigned lcore_id;
    RTE_LCORE_FOREACH(lcore_id) {
        if (lcore_id == rte_get_master_lcore())
            setup_thread_table(tbl, 0, lcore_id);
        else
            setup_thread_table(tbl, thread_id++, lcore_id);
    }

    if (rte_eal_mp_remote_launch(thread_entry, tbl, SKIP_MASTER))
        rte_exit(EXIT_FAILURE, "cannot launch thread\n");

    //    daemon(1, 1);
    master_loop(tbl, config.in_path, config.out_path);

    rte_exit(EXIT_SUCCESS, "bye\n");
    return 0;
}
