#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

char error_message[30] = "An error has occurred\n";

void exitc(int i) {

	if (i == 1) {
		exit(0);
	}
	else {
		write(STDERR_FILENO, error_message, strlen(error_message)); 
	}

}

void cd(int i, char **arglist) {

	if (i == 2) {
		if (chdir(arglist[1]) != 0) {
			write(STDERR_FILENO, error_message, strlen(error_message));
		}
	}	
	else {
		write(STDERR_FILENO, error_message, strlen(error_message));
	}

}

int wsc(char *buffer) {
	while (*buffer != '\0') {
		if (!isspace((unsigned char)*buffer)) {
			return 1;
		}
		buffer++;
	}
	return 0;
}

void intmode(int mode, char *filename) {

	char *output;
	char *buffer;
	char *buffcopy = NULL;

	size_t bufsize = 0;
	size_t linesize;

	char *arglist[100];
	char *redlist[100];
	char **pathlist;

	int pathsize = 1;
	int loopval = 0;
	int dollarindex = 0;

	FILE *file;

	// Initial allocation of pathlist
	pathlist = malloc(sizeof(char*));
	pathlist[0] = malloc(sizeof(char) * (strlen("/bin") + 1));
	strcpy(pathlist[0], "/bin\0");

	if (mode == 1) {
		file = fopen(filename, "r");
		if (file == NULL) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}
	}

	// Execution loop
	while(1) {

		output = NULL;
		buffer = NULL;

		if (mode == 1) {
			if (getline(&buffer, &bufsize, file) == -1) {
				exit(0);
			}
		}

		// Arglist counter
		int i = 0;
		int j = 0;
		char *arg;

		if (mode == 0) {
			printf("wish> ");
			getline(&buffer, &bufsize, stdin);
		}

		// Empty command simply restarts the loop
		if (strlen(buffer) == 1) {
			free(buffer);
			continue;
		}

		// Trim the newline off the input
		buffer[strcspn(buffer, "\n")] = 0;



		// Redirection check
		buffcopy = buffer;

		int whitespacecheck = wsc(buffer);
		if (whitespacecheck == 0) {
			free(buffer);
			continue;
		}

		while (((arg = strsep(&buffer, ">")) != NULL)) {
			redlist[j] = arg;
			j++;
		}

		if (j > 2) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			free(buffer);
			free(arg);
			continue;
		}
		else if (j == 1) {
			buffer = malloc(sizeof(char) * strlen(buffcopy));
			strcpy(buffer, buffcopy);
			free(buffcopy);
		}
		else {
			if (strlen(redlist[0]) == 0 || strlen(redlist[1]) == 0) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}

			buffer = malloc(sizeof(char) * strlen(redlist[0]));
			strcpy(buffer, redlist[0]);
			output = malloc(sizeof(char) * strlen(redlist[1]));
			strcpy(output, redlist[1]);

			char *outputlist[100];
			int z = 0;
			while (((arg = strsep(&output, " \t")) != NULL)) {
				if (strcmp(arg, "\0") != 0) {
					outputlist[z] = arg;
					z++;
                                }
			}


			if (z > 1) {
				free(output);
				free(arg);
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}
			else if (z == 1) {
				output = outputlist[0];
			}
			else if (output == NULL || z == 0) {
				free(buffer);
				free(arg);
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}

			free(buffcopy);
		}



		// Tokenize the input stream using space as a delimiter
		while (((arg = strsep(&buffer, " \t")) != NULL)) {
			if (strcmp(arg, "\0") != 0) {
				arglist[i] = arg;
				i++;
			}
		}
		arglist[i] = NULL;



		// Free buffer and argument once arglist is formed
		if (buffer != NULL ) {
			free(buffer);
		}
		if (arg != NULL) {
			free(arg);
		}
		
		// Branching tree for built-in commands
		// loop functionality
		if (strcmp(arglist[0], "loop") == 0) {
			loopval = atoi(arglist[1]);
			if (loopval < 1 || i < 2) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}
			for (int a = 0; a < i; a++) {
				if (strcmp(arglist[a], "$loop") == 0) {
					dollarindex = a;
				}
			}
		}

		// exit
		if (strcmp(arglist[0], "exit") == 0) {
			exitc(i);
		}
		// cd
		else if (strcmp(arglist[0], "cd") == 0) {
			cd(i, arglist);
		}
		// path
		else if (strcmp(arglist[0], "path") == 0) {

			if (pathsize > 0) {
				for (int a = 0; a < pathsize; a++) {
					free(pathlist[a]);
				}
				free(pathlist);
			}
			if (i > 1) {
				pathlist = malloc(sizeof(char*) * (i - 1));
				for (int a = 0; a < (i - 1); a++) {
					pathlist[a] = malloc(sizeof(char) * (1 + strlen(arglist[a + 1])));
					strcpy(pathlist[a], arglist[a + 1]);
					pathsize = (i - 1);
				}
			}
			else if (i == 1) {
				pathsize = 0;
			}
		}
		// Extra command to print out paths (not for official use for program)
		else if (strcmp(arglist[0], "pathlist") == 0) {
			for (int a = 0; a < pathsize; a++) {
				printf("%s\n", pathlist[a]);
			}
		}



		// All other commands
		else {

			int accessval = 0;

			for (int a = 0; a < pathsize; a++) {

				char *exepath;
				exepath = malloc(sizeof(char) * (strlen(pathlist[a]) + strlen(arglist[0]) + 1));
				strcat(exepath, pathlist[a]);
				strcat(exepath, "/");

				if (loopval != 0) {
					strcat(exepath, arglist[2]);
				}
				else{
					strcat(exepath, arglist[0]);
				}

				if (access(exepath, X_OK) == 0) {
					
					accessval = 1;
					int child;
					char **altargs;

					// Loop command is being used without $loop
					if (loopval != 0 && dollarindex == 0) {
						altargs = malloc(sizeof(char*) * (i - 2));
						for (int a = 0; a < (i - 2); a++) {
							altargs[a] = malloc(sizeof(char) * strlen(arglist[a + 2]));
							strcpy(altargs[a], arglist[a + 2]);
						}
						altargs[i - 2] = NULL;
						for (int a = 0; a < loopval; a++) {

							child = fork();
							if (child == 0) {
								if (output != NULL) {
									close(STDOUT_FILENO);
									open(output, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
								}
								child = execv(exepath, altargs);
								write(STDERR_FILENO, error_message, strlen(error_message));
								if (mode == 1) {
									exit(0);
								}
							}
							else if (child > 0) {
								wait(NULL);
							}
							else {
								write(STDERR_FILENO, error_message, strlen(error_message)); 
							}
						}
					}

					// Loop command including $loop
					else if (dollarindex != 0) {
						altargs = malloc(sizeof(char*) * (i - 2));
						for (int a = 0; a < (i - 2); a++) {
							altargs[a] = malloc(sizeof(char) * strlen(arglist[a + 2]));
							strcpy(altargs[a], arglist[a + 2]);
						}
						altargs[i - 2] = NULL;
						for (int a = 0; a < loopval; a++) {

							// Atoi implementation to replace $loop with correct integer value
							char dollarval[10];
							sprintf(dollarval, "%d", (a + 1));
							strcpy(altargs[dollarindex - 2], dollarval);
							altargs[i - 2] = NULL;

							child = fork();
							if (child == 0) {
								if (output != NULL) {
                                                                        close(STDOUT_FILENO);
                                                                        open(output, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
                                                                }
								child = execv(exepath, altargs);
								write(STDERR_FILENO, error_message, strlen(error_message));
								if (mode == 1) {
									exit(0);
								}

							}
							else if (child > 0) {
								wait(NULL);
							}
							else {
								write(STDERR_FILENO, error_message, strlen(error_message));
							}
						}
					}

					// Execution of general purpose command
					else {
						child = fork();
						if (child == 0) {
							if (output != NULL) {
                                                               	close(STDOUT_FILENO);
                                                        	open(output, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
                                                        }
							child = execv(exepath, arglist);
							write(STDERR_FILENO, error_message, strlen(error_message));
							if (mode == 1) {
								exit(0);
							}
						}
						else if (child > 0){
							 wait(NULL);	
						}
						else {
							write(STDERR_FILENO, error_message, strlen(error_message));
						}
					}

					// Reset values so future commands don't loop on accident
					loopval = 0;
					dollarindex = 0;
				}
			}
			if (accessval == 0) {
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
		}
	}
}

int main(int argc, char *argv[]) {

	// Batch Mode
	if (argc == 2) {
		intmode(1, argv[1]);
	}

	// Greater than 2 arguments, invoking more than one file
	else if (argc > 2) {
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);	
	}

	// Interactive Mode
	else {
		intmode(0, NULL);
	}
	
	return 0;

}
