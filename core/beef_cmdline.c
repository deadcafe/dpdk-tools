#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <rte_eal.h>

#include <rte_debug.h>
#include <rte_pause.h>

#include <cmdline.h>
#include <cmdline_parse.h>
#include <cmdline_parse_string.h>
#include <cmdline_socket.h>

#include "beef_cmdline.h"


/*****************************************************************************
 * quit:	exit application
 *****************************************************************************/
struct cmd_quit_result {
    cmdline_fixed_string_t quit;
};

static void
cmd_quit_parsed(__attribute__((unused)) void *parsed_result,
                struct cmdline *cl,
                __attribute__((unused)) void *data)
{
    cmdline_quit(cl);
}

cmdline_parse_token_string_t cmd_quit_tok =
    TOKEN_STRING_INITIALIZER(struct cmd_quit_result, quit, "quit");

static const cmdline_parse_inst_t cmd_quit = {
    .f = cmd_quit_parsed,	/* function to call */
    .data = NULL,		/* 2nd arg of func */
    .help_str = "exit application",
    .tokens = {        		/* token list, NULL terminated */
        (void *) &cmd_quit_tok,
        NULL,
    },
};

/*****************************************************************************
 * cmd table
 *****************************************************************************/
static cmdline_parse_ctx_t main_ctx[] = {
    (cmdline_parse_inst_t *) &cmd_quit,
    NULL,
};

static int
setup_pipe(const char *in_path,
           const char *out_path)
{
    if (in_path) {
        int fd = open(in_path, O_RDONLY | O_NONBLOCK);

        if (fd >= 0)
            dup2(fd, fileno(stdin));
    }

    if (out_path) {
        int fd = open(out_path, O_WRONLY | O_NONBLOCK);

        if (fd >= 0)
            dup2(fd, fileno(stdout));
    }
    return 0;
}

int
beef_cmdline_loop(const char *in_path,
                  const char *out_path)
{
    struct cmdline *cl;
    int ret = -1;

    setup_pipe(in_path, out_path);

    cl = cmdline_stdin_new(main_ctx, "beef> ");
    if (cl) {
        while ((ret = cmdline_poll(cl)) == RDLINE_RUNNING) {
            rte_pause();
            //            fprintf(stderr, ".");
        }

        fprintf(stderr, "ret:%d\n", ret);

        cmdline_stdin_exit(cl);
    }
    return ret;
}
