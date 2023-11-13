#include "builtin.h"

#define LIMIT 4
#define MULTIPLIER 2

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	// Your code here
	if (strcmp(cmd, "exit") == 0) {
		return 1;
	}
	return 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	// ojo con este if
	if (cmd[0] != 'c' || cmd[1] != 'd') {
		return 0;
	}

	int err = 0;
	char *dir = split_line(cmd, SPACE);

	if (strlen(dir) == 0) {
		chdir(getenv("HOME"));
		strcpy(prompt, getenv("HOME"));
		return 1;
	}

	if (dir[0] == '$') {
		char *env = getenv(dir + 1);
		if (env) {
			err = chdir(env);
			if (err != -1) {
				strcpy(prompt, env);
				return 1;
			}
		}
	} else {
		err = chdir(dir);
		if (err != -1) {
			strcpy(prompt, dir);
			return 1;
		}
	}

	//	parsear linea: -> split_line

	//	si directorio:
	//		si chdir(directorio) ok:
	//			cambiar prompt a directorio
	//			return
	//
	//	sino:
	//		si chdir(getenv(HOME)) ok:
	//			cambiar prompt a HOME
	//			return

	return 0;
}

// va agrandando el buffer para que entre el directorio
char *
do_getcwd(size_t buflen, int counter)
{
	char directory[buflen];
	char *buf = getcwd(directory, buflen);
	if ((buf == NULL) && (counter < LIMIT)) {
		counter += 1;
		return do_getcwd(buflen * MULTIPLIER, counter);
	} else {
		return buf;
	}
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	// solución verificando que el tamaño del directorio entre en el buf.

	// if (strcmp(cmd, "pwd") != 0) {
	// 	return 0;
	// }
	// int counter = 0;
	// size_t buflen = BUFLEN;
	// char* buf = do_getcwd(buflen, counter);
	// if (buf == NULL) {
	// 	perror("Error in pwd\n");
	// 	_exit(-1);
	// }
	// printf(buf);
	// return 1;

	// solución simple sin chequear el tamaño del directorio

	if (strcmp(cmd, "pwd") != 0) {
		return 0;
	}
	char directory[BUFLEN];
	char *buf = getcwd(directory, BUFLEN);
	if (buf == NULL) {
		perror("Error in pwd\n");
		_exit(-1);
	}
	printf(buf);
	printf("\n");
	return 1;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}
