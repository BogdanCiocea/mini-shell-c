#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_STRING_LENGTH 2000

/**
 * Red color for the printing
*/
void red()
{
  printf("\033[1;31m");
}

/**
 * Yellow color for the printing
*/
void yellow()
{
  printf("\033[1;33m");
}

/**
 * White color for the printing
*/
void white()
{
	printf("\033[1;37m");
}

/**
 * Green color for the printing
*/
void green()
{
	printf("\033[1;32m");
}

/**
 * Blue color for the printing
*/
void blue()
{
	printf("\033[1;34m");
}

/**
 * Reset the color for the printing
*/
void reset()
{
  printf("\033[0m");
}

/**
 * This function delays the flow of the program by a number of seconds you want
 * @param number_of_seconds  The number of seconds you want to delay the program
*/
void delay(int number_of_seconds)
{
	// Converting time into milli_seconds
	int milli_seconds = 1000 * number_of_seconds;
 
	// Storing start time
	clock_t start_time = clock();
 
	// looping till required time is not achieved
	while (clock() < start_time + milli_seconds)
		;
}

/**
 * Prints the words you want to print
 * @param nr_of_words The number of words you want to put
 * @param **words The words you want to print
 * @param filename Where the words need to be printed (stdin or a random file)
*/
void echo(int nr_of_words, char** words, FILE* filename)
{
	if (nr_of_words == 0)
		return;

	for (int i = 0; i < nr_of_words; i++) {
		if (words[i])
			fprintf(filename, "%s ", words[i]);
	}
	fprintf(filename, "\n");
}

/**
 * Applies the echo function in different scenarios (don't ask)
*/
void apply_echo(char* command)
{
	char** words = malloc(MAX_STRING_LENGTH * sizeof(char*));
	if (!words) {
		printf("malloc error 1!\n");
		free(*words);
		return;
	}

	for (int i = 0; i < MAX_STRING_LENGTH; i++) {
		words[i] = malloc(MAX_STRING_LENGTH * sizeof(char));
		if (!words[i]) {
			printf("malloc error 2!\n");
			free(words[i]);
			return;
		}
	}

	int num_of_words = 0;
	fgets(command, MAX_STRING_LENGTH, stdin);

	char* token = strtok(command, " ");
	while (token != NULL && num_of_words < MAX_STRING_LENGTH) {
		strcpy(words[num_of_words], token);
		words[num_of_words][strcspn(words[num_of_words], "\n")] = '\0';
		token = strtok(NULL, " ");
		num_of_words++;
	}

	int ok = 0;

	for (int i = 0; i < num_of_words; i++) {
		if (!strcmp(words[i], ">")) {
			if (i + 1 >= num_of_words) {
				printf("Invalid command! Missing filename after '>' symbol.\n");
				ok = 1;
				break;
			}

			num_of_words = i;

			FILE* filename = fopen(words[i + 1], "w");
			if (!filename) {
				printf("fopen error!\n");
				fclose(filename);
				break;
			}

			echo(num_of_words, words, filename);
			fclose(filename);
			ok = 1;
			break;
		}
	}

	if (!ok)
		echo(num_of_words, words, stdout);

	for (int i = 0; i < MAX_STRING_LENGTH; i++)
		free(words[i]);
	free(words);
}

/**
 * Shows content from a file
 * @param filename The file, duh...
 * @param redirect_filename The file where we want to redirect the content to
*/
void cat(const char* filename, const char* redirect_filename)
{
	FILE* file = fopen(filename, "r");
	if (!file) {
		printf("cat: %s: No such file or directory\n", filename);
		return;
	}

	FILE* redirect_file = NULL;
	if (redirect_filename) {
		redirect_file = fopen(redirect_filename, "w");
		if (!redirect_file) {
			printf("Error: Unable to open file %s for writing\n",
					redirect_filename);
			fclose(file);
			return;
		}
	}

	char* line = malloc(MAX_STRING_LENGTH);
	if (!line) {
		printf("malloc error\n");
		fclose(file);
		if (redirect_file)
			fclose(redirect_file);
		return;
	}

	while (fgets(line, MAX_STRING_LENGTH, file)) {
		if (redirect_file)
			fprintf(redirect_file, "%s", line);
		else
			printf("%s", line);
	}

	fclose(file);
	if (redirect_file) {
		fclose(redirect_file);
	}

	free(line);
}

/**
 * This makes the "cat" function work also with redirection
 * (source: trust me bro)
*/
void apply_cat()
{
	char* filename = malloc(MAX_STRING_LENGTH);
	if (!filename) {
		printf("malloc error\n");
		return;
	}

	if (scanf("%s", filename) != 1) {
		printf("Error: Invalid command! Missing filename.\n");
		free(filename);
		return;
	}

	char nextChar = getchar();
	if (nextChar == '>') {
		char redirect_filename[MAX_STRING_LENGTH];
		if (scanf("%s", redirect_filename) != 1) {
			printf("Error: Invalid command! Missing redirect filename.\n");
			free(filename);
			return;
		}
		cat(filename, redirect_filename);
	} else {
		cat(filename, NULL);
	}

	free(filename);
}

/**
 * Compare function for bubble sort
 */
int compare(const void *a, const void *b) {
	return strcmp((*(struct dirent **)a)->d_name, (*(struct dirent **)b)->d_name);
}

/**
 * This function shows the files in the directory
 */
void ls(const char *dir, int op_a)
{
	struct dirent *d;
	DIR *dh = opendir(dir);
	if (!dh) {
		if (errno == ENOENT) {
			perror("Error: Directory doesn't exist");
		} else {
			perror("Error: Unable to read directory");
		}
		exit(EXIT_FAILURE);
	}

	int count = 0;

	while ((d = readdir(dh)) != NULL) {
		if (!op_a && d->d_name[0] == '.')
			continue;

		char path[MAX_STRING_LENGTH];
		snprintf(path, MAX_STRING_LENGTH, "%s/%s", dir, d->d_name);

		struct stat file_stat;
		if (stat(path, &file_stat) < 0) {
			perror("stat error");
			continue;
		}

		if (S_ISDIR(file_stat.st_mode)) {
			blue();
			printf("%s  ", d->d_name);
		} else if (file_stat.st_mode & S_IXUSR) {
			green();
			printf("%s  ", d->d_name);
		} else if (strstr(d->d_name, ".tar") || strstr(d->d_name, ".zip")) {
			red();
			printf("%s  ", d->d_name);
		} else {
			printf("%s  ", d->d_name);
		}

		reset();

		count++;
		if (count >= 5) {
			count = 0;
			printf("\n");
		}
	}

	if (count > 0) {
		printf("\n");
	}

	free(dh);
}

/**
 * This function applies the "ls"
*/
void apply_ls(void)
{
	//char* options = malloc(MAX_STRING_LENGTH * sizeof(char));

	int op_a = 0;

	ls(".", op_a);

	//free(options);
}

/**
 * Deletes a file or multiple files at the same time
*/
void rm()
{
	char* filename = malloc(MAX_STRING_LENGTH);
	if (!filename) {
		printf("malloc error\n");
		free(filename);
		return;
	}

	fgets(filename, MAX_STRING_LENGTH, stdin);

	size_t len = strlen(filename);
	if (len > 0 && filename[len - 1] == '\n') {
		filename[len - 1] = '\0';
	}

	char *tok = strtok(filename, " ");
	while (tok) {
		int ret = remove(tok);
		if (ret != 0) {
			printf("Error: unable to delete the file '%s'\n", tok);
			return;
		}
		tok = strtok(NULL, " ");
	}

	free(filename);
}

/**
 * Creates a file or multiple files at the same time
*/
void touch(char *filename)
{
	char* tok = strtok(filename, " ");
	while (tok) {
		int fd = open(tok, O_WRONLY | O_APPEND | O_CREAT, 0644);
		if (fd < 0) {
			printf("fd error\n");
			return;
		}
		tok = strtok(NULL, " ");
	}
}

/**
 * Writes something in a single file
*/
void nano()
{
	char* filename = malloc(MAX_STRING_LENGTH);
	if (!filename) {
		printf("malloc error\n");
		free(filename);
		return;
	}

	scanf("%s", filename);

	FILE *file = fopen(filename, "w");
	if (!file) {
		printf("Error: File cannot be opened.\n");
		return;
	}

	system("clear");
	white();
	printf("GNU nano 6.2\t\t\t\t%s\t\t\t\t\n", filename);
	printf("Press Ctrl + X to exit.\n\n");
	reset();
	getchar();

	int c, last_char = -1;
	while ((c = getchar()) != EOF) {
		if (c == 24) {
			if (last_char != '\n') {
				putc('\n', file);
			}
			break;
		} else {
			putc(c, file);
			last_char = c;
		}
	}

	fclose(file);

	free(filename);
}

/**
 * Changes the directory 
*/
void cd()
{
	char* pathname = malloc(MAX_STRING_LENGTH);
	if (!pathname) {
		printf("malloc error\n");
		free(pathname);
		return;
	}

	scanf("%s", pathname);
	if (chdir(pathname))
		printf("bash: cd: %s: No such file or directory\n", pathname);

	free(pathname);
}

/**
 * Moves or renames a file 
*/
void mv()
{
	char* source = malloc(FILENAME_MAX);
	if (!source) {
		printf("malloc error\n");
		free(source);
		return;
	}

	scanf("%s", source);

	char* destination = malloc(FILENAME_MAX);
	if (!destination) {
		printf("malloc error\n");
		free(destination);
		return;
	}

	scanf("%s", destination);

	if (destination[strlen(destination) - 1] == '/') {
		size_t new_dest_len = strlen(destination) + strlen(source) + 1;
		char* new_destination = (char*)malloc(new_dest_len);
		if (new_destination == NULL) {
			printf("Error: Memory allocation failed.\n");
			return;
		}
		sprintf(new_destination, "%s%s", destination, source);
		if (rename(source, new_destination) == -1) {
			printf("Error: Cannot move the file %s.\n", source);
		}
		free(new_destination);
	} else {
		if (rename(source, destination) == -1) {
			printf("Error: Cannot move the file %s.\n", source);
		}
	}

	free(source);
	free(destination);
}

/**
 * Get the pathname you are currently in
*/
void pwd()
{
	char* cwd = malloc(MAX_STRING_LENGTH);
	if (!cwd) {
		printf("malloc error\n");
		free(cwd);
		return;
	}

	getcwd(cwd, MAX_STRING_LENGTH);
	printf("%s\n", cwd);

	free(cwd);
}

/**
 * Automatically shows the path you are currently in to have the
 * "linux terminal" feel
*/
void print_path()
{
	char* pathname = malloc(MAX_STRING_LENGTH);
	if (getcwd(pathname, MAX_STRING_LENGTH) != NULL) {
			char* path_parts[50];
			int i = 0;

			char* tok = strtok(pathname, "/");
			while (tok) {
				path_parts[i++] = tok;
				tok = strtok(NULL, "/");
			}

			if (i > 2) {
				char* finalpath = malloc(MAX_STRING_LENGTH);
				sprintf(finalpath, "/%s/%s", path_parts[i - 2],
						path_parts[i - 1]);
				blue();
				printf("%s", finalpath);
				reset();
				printf("$ ");

				free(finalpath);
			} else {
				blue();
				printf("%s", pathname);
				reset();
				printf("$ ");
			}
	}

	free(pathname);
}

/**
 * Make
*/
void make()
{
	system("make");
}

/**
 * Make clean
*/
void make_clean()
{
	system("make clean");
}

/**
 * Executes a file from the directory you are currently in
*/
void execute_command(const char* command)
{
	char* command_buffer = malloc(MAX_STRING_LENGTH);
	strcpy(command_buffer, command);

	char* command_args[MAX_STRING_LENGTH];
	int i = 0;

	char* token = strtok(command_buffer, " ");
	while (token != NULL && i < MAX_STRING_LENGTH - 1) {
		command_args[i++] = token;
		token = strtok(NULL, " ");
	}
	command_args[i] = NULL;

	execvp(command_args[0], command_args);
	perror("execvp error");

	for (int j = 0; j < i; j++) {
		free(command_args[j]);
	}

	exit(EXIT_FAILURE);

	free(command_buffer);
}

/**
 * Changes the permission of a file
*/
void change_permissions()
{
	mode_t mode;
	scanf("%o", &mode);

	char* filename = malloc(MAX_STRING_LENGTH);
	if (!filename) {
		printf("malloc error\n");
		free(filename);
		return;
	}

	scanf("%s", filename);
	
	if (chmod(filename, mode)) {
		perror("chmod error");
	}

	free(filename);
}

/**
 * Opens the VS Code to code on a file
*/
void code(char *command)
{
	char* filename = malloc(MAX_STRING_LENGTH);
	if (!filename) {
		printf("malloc error\n");
		free(filename);
		return;
	}

	scanf("%s", filename);

	getchar();

	sprintf(command, "code %s", filename);
	system(command);

	free(filename);
}

void show_date_time()
{
	time_t currentTime;
	struct tm* timeInfo;
	char buffer[80];

	time(&currentTime);
	timeInfo = localtime(&currentTime);

	strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", timeInfo);
	printf("%s\n", buffer);
}

char username[MAX_STRING_LENGTH];

void calc(char *command) {
	printf("C-style arbitrary precision calculator (version 2.12.7.2)\n"
	   "Calc is open software. For license details type: help copyright\n"
	   "[Type \"exit\" to exit, or \"help\" for help.]\n"
	   "This function can only calculate 2 variables\n\n");

	while (1) {
		printf("; ");
		fgets(command, MAX_STRING_LENGTH, stdin);
		
		bool minus_sign = false;
		bool plus_sign = false;
		bool multiplication_sign = false;
		bool division_sign = false;

		size_t len = strlen(command);
		if (len > 0 && command[len - 1] == '\n') {
			command[len - 1] = '\0';
		}

		unsigned long long a = 0;
		unsigned long long b = 0;

		char *tok = strtok(command, " \t");
		bool exit_command = false;
		bool valid = false;
		unsigned long long sol = 0;

		int numbers_count = 0;

		while (tok) {
			if (!strcmp(tok, "exit")) {
				exit_command = true;
				break;
			} else if (!strcmp(tok, "+")) {
				plus_sign = true;
			} else if (!strcmp(tok, "-")) {
				minus_sign = true;
			} else if (!strcmp(tok, "/")) {
				division_sign = true;
			} else if (!strcmp(tok, "*")) {
				multiplication_sign = true;
			}

			if (!plus_sign && !minus_sign && !multiplication_sign && !division_sign && atoi(tok))
				a = atoi(tok);
			if (!(!plus_sign && !minus_sign && !multiplication_sign && !division_sign)&& atoi(tok))
				b = atoi(tok);

			numbers_count++;
			
			tok = strtok(NULL, " ");
		}

		if (exit_command)
			break;

		if (plus_sign) {
			plus_sign = false;
			valid = true;
			sol = a + b;
		} else if (minus_sign) {
			minus_sign = false;
			valid = true;
			sol = a - b;
		} else if (multiplication_sign) {
			sol = a * b;
			multiplication_sign = false;
			valid = true;
		} else if (division_sign) {
			if (b == 0) {
				printf("Error: Cannot divide by 0.\n");
				continue;
			} else
				sol = a / b;

			division_sign = false;
			valid = true;
		} else if (!valid){
			printf("Error: Invalid something.\n");
			continue;
		} else if (numbers_count >= 4){
			printf("Error: Insert only two numbers.\n");
			numbers_count = 0;
			continue;
		}

		if (valid)
			printf("\t%lld\n\n", sol);
	}
}

/**
 * Why not?
*/
void funny_login()
{
	system("clear");
	printf("\nLogin required.");

	printf("\n\nUsername: ");

	scanf("%s", username);
password_incorrect:
	char* password = getpass("Password: ");
	delay(600);

	// wow
	if (strcmp(password, "password")) {
		printf("Error: username or password incorrect! Try again.\n");
		goto password_incorrect;
	}

	printf("\nLogin successfully.\n");
	delay(600);

	printf("Booting up the system.\n");
	delay(600);

	printf("Scanning for viruses.\n");
	delay(600);

	printf("Checking integrity of program.\n");
	delay(600);

	// printf("Scrolling throught the browser history.\n");
	// delay(1800);

	printf("Calling Google for help.\n");
	delay(1300);

	printf("All good.\n\n");
	delay(600);

	for (int i = 0; i <= 100; i+=13) {
		printf("%d%%\n", i);
		delay(200);
	}

	printf("100%%\n");
	delay(600);
	printf("Welcome!\n");
	delay(600);

	system("clear");
	yellow();
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~Custom linux-like shell~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
	reset();

	free(password);
}

int main(void)
{
	char* command = malloc(MAX_STRING_LENGTH * sizeof(char));

	/* Funny login */
	funny_login();

	while (1) {
username_changed:
		green();
		printf("%s@shell-os", username);
		reset();
		printf(":");
		blue();
		printf("~");
		reset();
		print_path();

sudo:
		scanf("%s", command);
		
		if (!strcmp(command, "echo")) {
			apply_echo(command);
		} else if (!strcmp(command, "exit")) {
			break;
		} else if (!strcmp(command, "cat")) {
			apply_cat(command);
		} else if (!strcmp(command, "ls")) {
			apply_ls();
		} else if (!strcmp(command, "rm")) {
			rm();
		} else if (!strcmp(command, "touch")) {
			char* filename = malloc(MAX_STRING_LENGTH);
			fgets(filename, MAX_STRING_LENGTH, stdin);
			size_t len = strlen(filename);

			if (len > 0 && filename[len - 1] == '\n') {
				filename[len - 1] = '\0';
			}

			touch(filename);

			free(filename);
		} else if (!strcmp(command, "mkdir")) {
			char* dirname = malloc(MAX_STRING_LENGTH);
			scanf("%s", dirname);

			int check = mkdir(dirname, 0777);

			free(dirname);

			if (check) {
				printf("Unable to create directory\n");
				continue;
			}
		} else if (!strcmp(command, "mv")) {
			mv();
		} else if (!strcmp(command, "cd")) {
			cd();
		} else if (!strcmp(command, "nano")) {
			nano();
		} else if (!strcmp(command, "pwd")) {
			pwd();
		} else if (!strcmp(command, "make")) {
			make();
		} else if (!strcmp(command, "clean")) {
			make_clean();
		} else if (!strcmp(command, "code")) {
			code(command);
		} else if (!strcmp(command, "chmod")) {
			change_permissions();
		} else if (!strcmp(command, "date")) {
			show_date_time();
		} else if (!strcmp(command, "shutdown")) {
			system("shutdown -h now");
		} else if (!strcmp(command, "whoami")) {
			printf("%s\n", username);
		} else if (!strcmp(command, "su")) {
			scanf("%s", username);
			goto username_changed;
		} else if (!strcmp(command, "calc")) {
			calc(command);
		} else if (!strcmp(command, "clear")) {
		   	system("clear");
		   	yellow();
			printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~Custom linux-like shell~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
			reset();
		} else if (!strcmp(command, "sudo")) {
			incorrect:
			char *password = getpass("[sudo] password for bogdan: ");
			if (!strcmp(password, "password")) {
				delay(1300);
				goto sudo;
			}
			else {
				delay(1300);
				printf("Sorry, try again.\n");
				goto incorrect;
			}
		} else {
			if (access(command, X_OK) == 0) {
				pid_t pid = fork();
				if (pid < 0) {
					perror("fork error");
					exit(EXIT_FAILURE);
				} else if (pid == 0) {
					execute_command(command);
				} else {
					wait(NULL);
				}
			} else {
				printf("Invalid command! I don't know what \"%s\" means. pls stop :(\n", command);
			}
		}
	}

	system("clear");

	free(command);
	return 0;
}
