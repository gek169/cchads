#include <cstdio>
#include <errno.h>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <cstring>
#define CTRL_KEY(k) ((k) & 0x1f)
struct termios orig_termios;
void die(const char* s){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);exit(1);
}
void disableRawMode() {
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios)==-1)
  die("tcsetattr");
}
void enableRawMode() {
  if(tcgetattr(STDIN_FILENO, &orig_termios)==-1)die("tcgetattr");
  atexit(disableRawMode);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_lflag &= ~(OPOST);
  raw.c_lflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)==-1)die("tcsetattr");
}
uint WIDTH, HEIGHT, numsuggestions;
bool nowidth = false;
std::string screenbuffer;
std::string inputline;
std::string top_suggestion;
std::string command_to_run;

std::string color_text(unsigned char mode, unsigned char color){
    return std::string("\033[") + std::to_string(mode) + 
            std::string(";") + std::to_string(color) + std::string("m");
}
char eReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  return c;
}
void eProcessKeypress() {
    char c = eReadKey();
    if(iscntrl(c))
    {
        if(c==27) exit(0);
        if(c == CTRL_KEY('q')) exit(0);
        if(c == CTRL_KEY('H'))
            if(inputline.size() > 0)
                inputline.resize(inputline.size()-1,' ');
        if(c== '\t')
            if(top_suggestion != "") inputline = top_suggestion;
        if(c=='\n')
            command_to_run = inputline;
    }
    else switch (c) {
        case 27:
          exit(0);
        break;
        case CTRL_KEY('H'):
            if(inputline.size() > 2)
            inputline.resize(inputline.size()-1,' ');
        break;
        //~ case '\t':
            //~ if(top_suggestion != "") inputline = top_suggestion;
        //~ break;
        default:
            if(isprint(c) && c != '\n' && c != '\t')
            inputline.push_back(c);
        break;
    }
}
void refreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
}

int main(int argc, char** argv){ //Args specify the width and height in characters of the command window.
	if(argc < 3){
		puts("\nUsage: duhmenu width height\nexample:\nduhmenu 80 40");
        return 0;
	}
	enableRawMode();
	WIDTH = atoi(argv[1]);
	HEIGHT = atoi(argv[2]);
    if(argc > 3 && !strcmp(argv[3],"--NOWIDTH")) nowidth=true;
	inputline = "";screenbuffer = "";numsuggestions = HEIGHT - 1;top_suggestion = "";
	inputline.resize(WIDTH+1,' ');
	inputline[WIDTH] = '\n';
	for(uint i = 0; i < HEIGHT; i++)
	screenbuffer += inputline;
    inputline = "";
	//Our screen buffer and input line are READY!!!!
	system("clear");
	char c;
    refreshScreen();
	while(1){
		//system("clear");
        eProcessKeypress();
        refreshScreen();
        std::cout << color_text(7,35);
		std::cout << inputline << "\r\n";
        std::cout << color_text(0,0);
        std::string s = "bash -c \"compgen -c "; s+= inputline; s+= "\"";
        FILE* fp;
        fp = popen(s.c_str(), "r");
        if(fp == NULL){
            for(int i = 0; i < numsuggestions; i++)
                std::cout << "\r\ncompgen not working...";
        }
        int i = 0;
        char* line = NULL; size_t len = 0; ssize_t read; top_suggestion = "";
        while ((read = getline(&line, &len, fp)) != -1 && i < numsuggestions) {
            for(int j = 0; j < strlen(line); j++)
                if(line[j] == '\n') line[j] = ' ';
            if(strlen(line) > WIDTH && !nowidth) line[WIDTH] = '\0';
            if(i == 0) top_suggestion = line;
            printf("\n%s", line);
            i++;
        }
        std::cout << std::flush;
        if(fp)pclose(fp);if(line)free(line);
        if(command_to_run != "") break;
	}
    if(command_to_run != "") {
        command_to_run = std::string("exec ") + command_to_run + std::string("& exit");
        disableRawMode();
        system(command_to_run.c_str());
    } else {
        disableRawMode();
        std::cout << std::endl;
    }
	return 0;
}
