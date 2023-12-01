#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#ifndef NARGS
#define NARGS 4
#endif

void
ejecutar(char *args[])
{
	pid_t child_id = fork();

	if (child_id < 0) {
		printf("Error de fork\n");
		exit(-1);
	}

	if (child_id == 0) {
		execvp(args[0], args);
	} else {
		wait(NULL);
	}
}

int
main(int argc, char *argv[])
{
	char *linea;
	size_t size;
	int char_leidos;

	char *args[NARGS + 2] = { 0 };
	args[NARGS + 1] = NULL;

	args[0] = argv[1];

	int cant_args = 1;

	char_leidos = getline(&linea, &size, stdin);

	while (char_leidos != -1) {
		if (linea[char_leidos - 1] == '\n') {
			linea[char_leidos - 1] = '\0';
		}

		args[cant_args] = linea;
		cant_args++;

		if (cant_args == NARGS + 1) {
			ejecutar(args);
			cant_args = 1;
		}

		linea = NULL;

		char_leidos = getline(&linea, &size, stdin);
	}

	if (cant_args > 1 && linea != NULL) {
		args[cant_args] = NULL;
		ejecutar(args);
	}

	return 0;
}
