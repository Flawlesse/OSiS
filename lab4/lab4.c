#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

int BEFORE_EXIT = 0;

const char* convert_to_morze(char c){
	switch (c)
	{
	case 'A':
		return ".-";
	case 'B':
		return "-...";
	case 'C':
		return "-.-.";
	case 'D':
		return "-..";
	case 'E':
		return ".";
	case 'F':
		return "..-.";
	case 'G':
		return "--.";
	case 'H':
		return "....";
	case 'I':
		return "..";
	case 'J':
		return ".---"; 
	case 'K':
		return "-.-";
	case 'L':
		return ".-..";
	case 'M':
		return "--";
	case 'N':
		return "-.";
	case 'O':
		return "---";
	case 'P':
		return ".--.";
	case 'Q':
		return "--.-";
	case 'R':
		return ".-.";
	case 'S':
		return "...";
	case 'T':
		return "-";
	case 'U':
		return "..-";
	case 'V':
		return "...-";
	case 'W':
		return ".--";
	case 'X':
		return "-..-";
	case 'Y':
		return "-.--";
	case 'Z':
		return "--..";
	case ' ':
		return "/";
	case '\n':
		return "\n";
	default:
		return "";
	}
}

void ctrl_c_handler(int num) {
	BEFORE_EXIT = 1;
	write(STDOUT_FILENO, "Gracefully exiting...", 21);
}

int main(int argc, int ** argv)
{
	char ch;
	FILE* fp;
	signal(SIGINT, ctrl_c_handler);

	if((fp = fopen((char*)argv[1], "a+t")) == NULL){
		printf("Error with file %s!", argv[1]);
		exit(1);
	}

	while((ch = getc(stdin)) != EOF && !BEFORE_EXIT){
		const char* converted = convert_to_morze(toupper(ch));
		fwrite(converted, sizeof(char), strlen(converted), fp);
		if (!strcmp("", converted))
			continue;
		fwrite(" ", sizeof(char), 1, fp);
	}

	fclose(fp);
	return 0;
}