
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>


#include "shell.h"


#define ARRAY_SIZE(a)	(sizeof(a)/sizeof(a[0]))

//Take controll over the keyboard and prevent echo whitespace (for example tab)
static struct termios old, new;

/* Initialize new terminal i/o settings */
void initTermios(int echo) 
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  new = old; /* make new settings same as old settings */
  new.c_lflag &= ~ICANON; /* disable buffered i/o */
  new.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char get_ch(void) 
{
  return getch_(0);
}


static struct {

	char buf[SHELL_CMD_BUFFER_LEN];

	unsigned int index;

	unsigned int match;

} shell;


/* built-in shell commands */
static void shell_help(int argc, char **argv);


static struct shell_command commands[] = {

	{ "help",	shell_help }	

};


static void prompt(void)
{
	printf(SHELL_PROMPT_STRING);

}


static char is_whitespace(char c)

{
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');

}


static void do_command(int argc, char **argv)

{
	unsigned int i;

	unsigned int nl = strlen(argv[0]);

	unsigned int cl;


	for (i = 0; i < ARRAY_SIZE(commands); i++) {

		cl = strlen(commands[i].name);


		if (cl == nl && commands[i].function != NULL &&

				!strncmp(argv[0], commands[i].name, nl)) {

			commands[i].function(argc, argv);

			printf("\n");

		}
	}
}


static void parse_command(void)

{
	unsigned char i;

	char *argv[16];

	int argc = 0;

	char *in_arg = NULL;


	for (i = 0; i < shell.index; i++) {

		if (is_whitespace(shell.buf[i]) && argc == 0)

			continue;


		if (is_whitespace(shell.buf[i])) {

			if (in_arg) {

				shell.buf[i] = '\0';

				in_arg = NULL;

			}
		} else if (!in_arg) {

			in_arg = &shell.buf[i];

			argv[argc] = in_arg;

			argc++;

		}
	}
	shell.buf[i] = '\0';


	if (argc > 0)

		do_command(argc, argv);

}


static void handle_tab(void)

{
	int i, j;

	unsigned int match = 0;

	struct shell_command *cmd;

	unsigned int sl = 0;


	shell.match = 0;


	for (i = 0; i < ARRAY_SIZE(commands); i++) {

		for (j = strlen(commands[i].name); j > 0; j--) {

			if (!strncmp(commands[i].name, shell.buf, shell.index)) {

				match++;

				shell.match |= (1<<i);

				break;
			}

		}
	}


	if (match == 1) {

		for (i = 0; i < ARRAY_SIZE(commands); i++) {

			if (shell.match & (1<<i)) {

				cmd = &commands[i];

				memcpy(shell.buf + shell.index, cmd->name + shell.index,

						strlen(cmd->name) - shell.index);

				shell.index += strlen(cmd->name) - shell.index;

			}
		}

	} else if (match > 1) {

		for (i = 0; i < ARRAY_SIZE(commands); i++) {

			if (shell.match & (1<<i)) {

				printf("\n");

				printf("%s",commands[i].name);


				if (sl == 0 || strlen(commands[i].name) < sl) {

					sl = strlen(commands[i].name);

				}
			}

		}

		for (i = 0; i < ARRAY_SIZE(commands); i++) {

			if (shell.match & (1<<i) && strlen(commands[i].name) == sl) {

				memcpy(shell.buf + shell.index, commands[i].name + shell.index,

						sl - shell.index);

				shell.index += sl - shell.index;

				break;
			}

		}
	}


	printf("\n");

	prompt();
	printf("%s",shell.buf);

}


static void handle_input(char c)

{
	switch (c) {

		case '\r':

		case '\n':

			printf("\n");

			if (shell.index > 0) {

				parse_command();
				shell.index = 0;

				memset(shell.buf, 0, sizeof(shell.buf));

			}
			prompt();

			break;

		case '\b':

			if (shell.index) {

				shell.buf[shell.index-1] = '\0';

				shell.index--;

				printf("%c",c);

				printf(" ");
				printf("%c",c);

			}
			break;


		case '\t':

			handle_tab();
			break;


		default:

			if (shell.index < SHELL_CMD_BUFFER_LEN - 1) {

				printf("%c",c);

				shell.buf[shell.index] = c;

				shell.index++;

			}
	}

}

static void shell_task(void *params)

{
	printf(SHELL_WELCOME_STRING);

	prompt();

	

	while (1) {

		int c;


		//c = getchar();
		c=get_ch();

		if (c != EOF) {

			handle_input((char)c);

		}
	}

}

void main(void)

{
	memset(&shell, 0, sizeof(shell));


	shell_task(NULL);

}


static void shell_help(int argc, char **argv)

{
	unsigned int i;


	for (i = 0; i < ARRAY_SIZE(commands); i++) {

		printf("%s",commands[i].name);

		printf("\n");

	}
}


