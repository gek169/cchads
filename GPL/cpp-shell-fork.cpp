//working on moving cursor using arrows without an external API

#define KEY_UP 72

#define KEY_DOWN 80

#define KEY_LEFT 75

#define KEY_RIGHT 77
#include <cstdio>
#include <errno.h>
#include <cstdlib>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <climits>
#define CTRL_KEY(k) ((k) & 0x1f)

static inline std::vector<std::string> SplitString(const std::string& s, char delim);


std::vector<std::string> lines;
bool shouldquit = false;
struct termios orig_termios;
void sigdie(int s){
	exit(1);
}
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
        //if(c==27) exit(0);
        //if(c == CTRL_KEY('q')) exit(0);
        //if(c == CTRL_KEY('H'))
        //  if(inputline.size() > 0)
        //      inputline.resize(inputline.size()-1,' ');
        //if(c== '\t')
        //    if(top_suggestion != "") inputline = top_suggestion;
        //if(c=='\n')
        //  command_to_run = inputline;
    }
    else switch (c) {
        case 27:
          exit(0);
        break;
        case CTRL_KEY('H'):
            //if(inputline.size() > 2)
            //inputline.resize(inputline.size()-1,' ');
        break;
        //~ case '\t':
            //~ if(top_suggestion != "") inputline = top_suggestion;
        //~ break;
        default:
            //if(isprint(c) && c != '\n' && c != '\t')
            //inputline.push_back(c);
        break;
    }
}
void refreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
}
void lcommand(std::string com){
	size_t number_start = 0; size_t number_end = 0; bool printall = true;
	auto tokens = SplitString(com, ' ');
	if(tokens.size() > 1){
		printall = false;
		
		number_start = strtoull(tokens[1].c_str(),NULL,10);
		if(number_start == 0 || number_start == ULLONG_MAX) {
				printf("\nInvalid line number.\n");return;
			}number_start--;
		number_end = number_start;
		if(tokens.size() > 2){
			number_end = strtoull(tokens[2].c_str(),NULL,10);
			if(number_end == 0 || number_end == ULLONG_MAX) {
					number_end = number_start;
			}number_end--;
		}
	}
	if(printall){
		for(size_t i = 0; i < lines.size(); i++){
			std::cout << ":" << i+1 << ":" << lines[i];
		}
		return;
	}
	for(size_t i = number_start; i <= number_end && i < lines.size(); i++){
		std::cout << ":" << i+1 << ":" << lines[i];
	}
	
}
void wcommand(std::string com){
	size_t number = 0;
	if(com.length() < 3) return; //there was nothing to append
	number = strtoull(com.c_str()+1,NULL,10);
	if(number == 0 || number == ULLONG_MAX) {
		printf("\nInvalid line number.\n");return;
	}number--;
	while(lines.size() <= number)
		lines.push_back("\n");
	size_t i = 2;
	for(;i < com.size() && com[i] != ' ';i++);
	lines[number] = com.substr(i,com.size());
}
void icommand(std::string com){
	size_t number = 0;
	if(com.length() < 4) return; //there was nothing to append
	number = strtoull(com.c_str()+1,NULL,10);
	if(number == 0 || number == ULLONG_MAX) {
		printf("\nInvalid line number.\n");return;
	}number--;
	while(lines.size() <= number)
		lines.push_back("\n");
	size_t i = 2;
		for(;i < com.size() && com[i] != ' ';i++);
	lines.insert( 
		lines.begin()+number, com.substr(i,com.size())
	);
}
void acommand(std::string com){
	size_t number = 0;
	if(com.length() < 4) return; //there was nothing to append
	number = strtoull(com.c_str()+1,NULL,10);
	if(number == 0 || number == ULLONG_MAX) {
		printf("\nInvalid line number.\n");return;
	}number--;
	while(lines.size() <= number)
		lines.push_back("\n");
	size_t i = 2;
		for(;i < com.size() && com[i] != ' ';i++);
	lines[number].erase(lines[number].back()); //remove the backslash n
	lines[number] += com.substr(i,com.size());
}

void scommand(std::string com){
	std::ofstream fileout;
	for(size_t i = 0; i < com.length(); i++){
		if(com[i] == '\n')com[i] = '\0';
	}
	if(com.length() < 3){printf("\nERROR! invalid file save operation.Check usage.\n");return;}
	const char* fn = com.c_str() + 2;//skip e and the space.
	fileout.open(fn);
	if(!fileout.good()){printf("\nCan't open file %s, aborting...\n",fn);}
	for(std::string& line:lines){
		fileout << line;
	}
	fileout.close();
}

void rcommand(std::string com){
	std::ofstream fileout;
	const char* cflags = "";
	if(com.length() > 4){
		cflags = com.c_str() + 2; //skip the r and the space
	}
	scommand("s /tmp/cppshell.cpp\n");
	std::string compilecommand = "g++ /tmp/cppshell.cpp -o /tmp/shellprog.out";
	compilecommand += std::string(" ") + std::string(cflags);
	system(compilecommand.c_str());
	system("/tmp/shellprog.out");
}
void ecommand(std::string com){
	std::ifstream filein;
	for(size_t i = 0; i < com.length(); i++){
		if(com[i] == '\n')com[i] = '\0';
	}
	if(com.length() < 3){printf("\nERROR! invalid file load operation. aborting...\n");return;}
	const char* fn = com.c_str() + 2;//skip e and the space.
	filein.open(fn);
	if(!filein.good()){printf("\nCan't load file %s, aborting...\n",fn);return;}
	lines.clear();
	std::string cline;
	for(;filein.good();){
		std::getline(filein,cline);cline.push_back('\n');
		lines.push_back(cline);
	}
}
void zcommand(std::string com){
	if(com.length()<4)return;
	scommand("s /tmp/cppshell.cpp\n");
	for(size_t i = 0; i < com.length(); i++){
		if(com[i] == '\n')com[i] = ' ';
	}
	system(com.c_str()+2);
	ecommand("e /tmp/cppshell.cpp\n");
}
void dcommand(std::string com){
	size_t number;
	
	if(com.length() < 2) return;
	number = strtoull(com.c_str()+1,NULL,10);
	if(number == 0 || number == ULLONG_MAX) {
		printf("\nInvalid line number.\n");return;
	}number--;
	if(lines.size() < number) {printf("\nInvalid line number to delete.\n");return;}
	lines.erase(lines.begin() + number);
}
void hcommand(){
	printf("USAGE:\n");
    printf("l lists your entire program.\n");
    printf("l linenum (e.g. l 5) displays a line.\n");
    printf("l linenum linenum2 (e.g. l 5 8) displays a range of lines.\n");
    printf("w<linenum> text (e.g. w5 printf(\"Hello World!\");\nwill write and erase the line at linenum. )\n");
    printf("i<linenum> text (e.g. i5 printf(\"Hello World!\");\nwill write and insert AFTER linenum\n");
    printf("a<linenum> text (e.g. w5 printf(\"Hello World!\");\nwill write and append at linenum.\n");
    printf("d<linenum> will delete a line. (e.g. d47 deletes line 47.)\n");
    printf("s filename.txt will save the file. Note that ALL characters immediately after \"s \" will be treated as part of the filename or path.\n");
    printf("e filename.txt will load a file. Note that ALL characters immediately after \"e \" will be treated as part of the filename or path.\n");
	printf("if line linenum does not exist and you try to change it, then blank lines will be appended to your program to reach it, and it will be blank.\n");
    printf("r <flags> will compile and run your program using g++. By default, the args \"/tmp/cppshell.cpp -o /tmp/shellprog.out\" are used. Your compiler flags are appended to this.\n");
    printf("q will quit the program. But if you accidentally type q, you can type \\qq at the end to cancel.\n");
	printf("z will execute commands using system(). the program you have loaded will be loaded int /tmp/cppshell.cpp before it is executed.\n");
    printf("if your tty does not support backspace and you mistype, you can type \\qq at any time and it will cancel the command.\n");
}
int main(int argc, char** argv){ //Args specify the width and height in characters of the command window.
	signal(SIGINT, sigdie);
	signal(SIGKILL, sigdie);
	signal(SIGABRT, sigdie);
	signal(SIGSEGV, sigdie);
	signal(SIGTERM, sigdie);
    refreshScreen();
    printf("\nCPPSHELL! Enter C++ Code.\n");
    hcommand();
    std::string cline;
	//std::cout << color_text(7,35);
	size_t empty_lines_in_a_row = 0;
	
	do{
		std::cout << color_text(0,0);
		std::cout << "\nCPP$ " << std::flush;
		std::getline(std::cin, cline);
		if(std::cin.eof() || std::cin.bad())shouldquit = true;
		if(cline == ""){empty_lines_in_a_row++;} else {empty_lines_in_a_row = 0;}
		//printf("\n[DEBUG]cline is %s of length %zu\n",cline.c_str(),cline.length());
		for(int i = 0; i < 10000; i++){int x = 5; x *= 3;}//Waste a lot of time.
		if(cline.find("\\qq") !=std::string::npos){printf("\nCommand cancelled...\\qq was present...\n");continue;}
		cline.push_back('\n');
		if(empty_lines_in_a_row > 100){
			shouldquit = true;break;
		}
		if(cline.size() > 0)
		switch(cline[0]){
			case 'l':lcommand(cline);break;
			case 'w':wcommand(cline);break;
			case 'r':rcommand(cline);break;
			case 'i':icommand(cline);break;
			case 'a':acommand(cline);break;
			case 's':scommand(cline);break; //save
			case 'e':ecommand(cline);break; //load
			case 'q':shouldquit=true;break;
			case 'd':dcommand(cline);break;
			case 'z':zcommand(cline);break; //open text editor.
			case 'h':hcommand();break;
		}
		//std::cout << color_text(7,35);
	}while(!shouldquit);
	std::cout << std::flush;
    //disableRawMode();
	return 0;
}





static inline std::vector<std::string> SplitString(const std::string& s, char delim) {
	std::vector<std::string> elems;

	const char* cstr = s.c_str();
	unsigned int strLength = s.length();
	unsigned int start = 0;
	unsigned int end = 0;

	while (end <= strLength) {
		while (end <= strLength) {
			if (cstr[end] == delim)
				break;
			end++;
		}

		elems.push_back(s.substr(start, end - start));
		start = end + 1;
		end = start;
	}

	return elems;
}