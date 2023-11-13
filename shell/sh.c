#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

char prompt[PRMTLEN] = { 0 };

stack_t ss;

// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

void
sigchld_handler()
{
	int status;
	pid_t child_pid;

	while ((child_pid = waitpid(0, &status, WNOHANG)) > 0) {
		printf("===> terminado: PID=%d\n", child_pid);
	}
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	// registrar signal
	struct sigaction sa = { .sa_handler = sigchld_handler,
		                .sa_flags = SA_RESTART };

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

int
main(void)
{
	init_shell();

	run_shell();

	free(ss.ss_sp);

	return 0;
}
