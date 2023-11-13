#include "runcmd.h"

#define MAX_BACK 10

int status = 0;
struct cmd *parsed_pipe;

int size = 0;
pid_t back_pids[MAX_BACK] = { 0 };

// runs the command in 'cmd'
int
run_cmd(char *cmd)
{
	pid_t p;
	struct cmd *parsed;

	// if the "enter" key is pressed
	// just print the prompt again
	if (cmd[0] == END_STRING)
		return 0;

	// "history" built-in call
	if (history(cmd))
		return 0;

	// "cd" built-in call
	if (cd(cmd))
		return 0;

	// "exit" built-in call
	if (exit_shell(cmd))
		return EXIT_SHELL;

	// "pwd" built-in call
	if (pwd(cmd))
		return 0;

	// parses the command line
	parsed = parse_line(cmd);

	pid_t first_background_pid = -1;

	// forks and run the command
	if ((p = fork()) == 0) {
		// keep a reference
		// to the parsed pipe cmd
		// so it can be freed later
		if (parsed->type == BACK) {
			if (first_background_pid == -1) {
				// This is the first background process encountered
				first_background_pid = getpid();
			} else {
				// Set PGID to be the same as the first background process
				setpgid(0, first_background_pid);
			}
		} else {
			setpgid(0, getpid());
		}
		if (parsed->type == PIPE) {
			parsed_pipe = parsed;
		}

		exec_cmd(parsed);
	}

	// stores the pid of the process
	parsed->pid = p;

	if (parsed->type != BACK) {
		waitpid(p, &status, 0);
		print_status_info(parsed);
	}

	free_command(parsed);

	return 0;
}
