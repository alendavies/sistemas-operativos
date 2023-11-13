#include "exec.h"
#define READ 0
#define WRITE 1
#define TRUE 1
#define FALSE 0

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		char *key, *value = NULL;
		int idx;

		if ((idx = block_contains(eargv[i], '=')) > 0) {
			key = (char *) malloc(idx + 1);
			value = (char *) malloc(strlen(eargv[i]) - idx);

			get_environ_key(eargv[i], key);
			get_environ_value(eargv[i], value, idx);
			setenv(key, value, 0);

			// printf_debug("%s=%s\n", key, value);

			free(key);
			free(value);
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags, int o_creat)
{
	// Your code here
	int fd;
	if (o_creat) {
		return open(file, flags, S_IRUSR | S_IWUSR);
	}
	return open(file, flags);
}

// dup2 wrapper
void
wrapper_dup2(int fd, int flow)
{
	if (fd > 0) {
		if (dup2(fd, flow) < 0) {
			perror("dup2");
			printf_debug(errno);
			printf_debug("Exiting now...");
			_exit(-1);
		};
		return;
	}
	perror("fd < 0 in dup2");
	printf_debug(errno);
	printf_debug("Exiting now...");
	_exit(-1);
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC: {
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);

		if (execvp(e->argv[0], e->argv) < 0) {
			perror("execvp");
			printf_debug(errno);
			printf_debug("Exiting now...");
			_exit(-1);
		};
		_exit(-1);
		break;
	}
	case BACK: {
		// runs a command in background
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		perror("Error in background process\n");
		_exit(-1);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero

		r = (struct execcmd *) cmd;
		int flags;
		int fd;

		if (strlen(r->out_file) > 0) {
			flags = (O_CREAT | O_TRUNC | O_RDWR | O_CLOEXEC);
			fd = open_redir_fd(r->out_file, flags, TRUE);
			wrapper_dup2(fd, STDOUT_FILENO);
		}

		if (strlen(r->in_file) > 0) {
			flags = (O_RDWR | O_CLOEXEC);
			fd = open_redir_fd(r->in_file, flags, FALSE);
			wrapper_dup2(fd, STDIN_FILENO);
		}

		if (strlen(r->err_file) > 0) {
			if (strcmp(r->err_file, "&1") == 0) {
				dup2(STDOUT_FILENO, STDERR_FILENO);
			} else {
				flags = (O_CREAT | O_TRUNC | O_RDWR | O_CLOEXEC);
				fd = open_redir_fd(r->err_file, flags, TRUE);
				wrapper_dup2(fd, STDERR_FILENO);
			}
		}
		cmd->type = EXEC;
		exec_cmd(cmd);
		break;
	}

	case PIPE: {
		// pipes two commands
		p = (struct pipecmd *) cmd;

		int pipe_fds[2];

		if (pipe(pipe_fds) < 0) {
			perror("pipe");
			printf_debug(errno);
			printf_debug("Exiting now...");
			_exit(-1);
		}

		setpgid(0, 0);

		pid_t left_child = fork();
		if (left_child < 0) {
			perror("left_fork_pipe");
			printf_debug(errno);
			printf_debug("Exiting now...");
			exit(-1);
		}
		if (left_child == 0) {
			close(pipe_fds[READ]);
			// con dup2 pongo el output al fd de entrada del pipe
			wrapper_dup2(pipe_fds[WRITE], STDOUT_FILENO);
			close(pipe_fds[WRITE]);
			exec_cmd(p->leftcmd);
		} else {
			// soy el padre y creo al de la derecha
			pid_t right_child = fork();
			if (right_child < 0) {
				perror("right_fork_pipe");
				printf_debug(errno);
				printf_debug("Exiting now...");
				exit(-1);
			}
			if (right_child == 0) {
				close(pipe_fds[WRITE]);
				// con dup2 pongo el input al fd de salida del pipe
				wrapper_dup2(pipe_fds[READ], STDIN_FILENO);
				close(pipe_fds[READ]);
				exec_cmd(p->rightcmd);
			} else {
				// soy el padre y los espero a los dos hijos.
				close(pipe_fds[READ]);
				close(pipe_fds[WRITE]);
				waitpid((p->leftcmd)->pid, NULL, 0);
				waitpid((p->rightcmd)->pid, NULL, 0);
				// free the memory allocated
				// for the pipe tree structure
				free_command(parsed_pipe);
				exit(0);
			}
		}
		break;
	}
	}
}
