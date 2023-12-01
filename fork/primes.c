#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

void
filtrar(int left_fds)
{
	int primo;
	int res_read = read(left_fds, &primo, sizeof primo);

	if (res_read == 0) {
		close(left_fds);
		return;
	}

	printf("primo %d\n", primo);

	int right_fds[2];

	int res_pipe = pipe(right_fds);

	if (res_pipe < 0) {
		printf("Error de pipe\n");
		exit(-1);
	}

	pid_t child_pid = fork();

	if (child_pid < 0) {
		close(left_fds);
		close(right_fds[WRITE]);
		close(right_fds[READ]);
		printf("Error de fork\n");
		exit(-1);
	}

	if (child_pid == 0) {
		close(right_fds[WRITE]);
		close(left_fds);
		filtrar(right_fds[READ]);
	} else {
		close(right_fds[READ]);
		int num;
		res_read = read(left_fds, &num, sizeof num);

		while (res_read > 0) {
			if (res_read < 0) {
				close(right_fds[READ]);
				close(left_fds);
				printf("Error de lectura\n");
				exit(-1);
			}
			if (num % primo != 0) {
				int res_write =
				        write(right_fds[WRITE], &num, sizeof num);
				if (res_write < 0) {
					close(right_fds[WRITE]);
					close(left_fds);
					printf("Error de escritura\n");
					exit(-1);
				}
			}
			res_read = read(left_fds, &num, sizeof num);
		}
		close(right_fds[WRITE]);
		close(left_fds);
		wait(NULL);
	}
}

int
main(int argc, char *argv[])
{
	int numeros = atoi(argv[1]);
	int parent_fds[2];

	int res_pipe = pipe(parent_fds);

	if (res_pipe < 0) {
		printf("Error de pipe\n");
		exit(-1);
	}

	pid_t child_id = fork();

	if (child_id < 0) {
		close(parent_fds[READ]);
		close(parent_fds[WRITE]);
		printf("Error de fork\n");
		exit(-1);
	}

	if (child_id == 0) {
		close(parent_fds[WRITE]);
		filtrar(parent_fds[READ]);
		close(parent_fds[READ]);

	} else {
		close(parent_fds[READ]);

		for (int n = 2; n <= numeros; n++) {
			int res_write = write(parent_fds[WRITE], &n, sizeof n);
			if (res_write < 0) {
				close(parent_fds[WRITE]);
				printf("Error de escritura\n");
				exit(-1);
			}
		}

		close(parent_fds[WRITE]);
		wait(NULL);
	}

	return 0;
}
