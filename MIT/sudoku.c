#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
unsigned char board[9][9];
unsigned int marks[9][9];
char combuff[2048];
int processCommand();
void validate_board(){
	for(int i = 0; i < 81; i++)if(board[i%9][i/9] < '1' || board[i%9][i/9] > '9') board[i%9][i/9] = ' ';
	int sqrsums[3][3]; for(int i = 0; i < 9;i++)sqrsums[i%3][i/3] = 0;
	int rowsums[9]; for(int i = 0; i < 9;i++)rowsums[i] = 0;
	for(int x = 0; x < 9; x++){
		int colsum = 0;
		for(int y = 0; y < 9; y++)if(board[x][y] != ' ')
		{
			 colsum += board[x][y] - '1' + 1;
			 rowsums[y] += board[x][y] - '1' + 1;
			 sqrsums[x/3][y/3] += board[x][y] - '1' + 1;
		}
		if(colsum > 45) {puts("\nInvalid board state");return;}
	}
	for(int i = 0; i < 9; i++)
	if(rowsums[i] > 45) {puts("\nInvalid board state");return;} else
	if(sqrsums[i%3][i/3] > 45) {puts("\nInvalid board state");return;}
}
void blankboard(){
	for(int i = 0; i < 81; i++){board[i%9][i/9]=' ';}
}
void blankmarks(){
	for(int i = 0; i < 81; i++){marks[i%9][i/9]=0;}
}
void printBoard(){
	validate_board();
	for(int i = 0; i < 81; i++)
	{
		if(i%9==0) putchar('\n');
		if(i%27==0) puts("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
		if(i%3==0) putchar('|');
		putchar(' ');
		putchar(board[i%9][i/9]);
		putchar(' ');
		if(i%9==8) putchar('|');
	}
	puts("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
}
void printMarks(){
	validate_board();
	for(int outery = 0; outery < 27; outery++){
		unsigned int itemy = outery/3;				//y coordinate in marks[][]
		unsigned int innery = outery%3;				//determines how much to add to offset
		
		putchar('\n');
		if(itemy%3 == 0 && innery==0)puts("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
		for(int outerx = 0; outerx < 27; outerx++){
			unsigned int itemx = outerx/3;				//x coordinate in marks[][]
			unsigned int innerx = outerx%3;				//determines offset
			if(itemx%3==0 && innerx==0){putchar('|');putchar(' ');}
			if(innerx==0){putchar('(');}
			unsigned int shift = innerx + (innery * 3);
			if(marks[itemx][itemy] & 1<<shift)putchar('1'+shift);else putchar(' ');
			if(innerx==2){putchar(')');putchar(' ');}
		}
	}
	puts("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
}
void assign_mark(unsigned int x, unsigned int y, unsigned int value){
	if(x<1 || y<1) return;
	x--;y--;
	x%=9;y%=9;
	if(value == 0){marks[x][y] = 0; return;}
	marks[x][y] |= 1<<(value-1); //
}
void help(){
	puts("\nWelcome to Gek and Intexisty's Sudoku.\nq = quit\nh = help\ns = set, \"s x,y,value\" you can set x,y,0 to unset");
	puts("\nm = mark, \"m x,y,value\" you can mark 0 to reset all marks on a spot");
	puts("\ns or m with no/invalid arguments will merely display the marks or board respectively.");
	puts("\np = save \"p filename.txt\"");
	puts("\nl = load \"l filename.txt\"");
	puts("\nNote once again that there is ONE SPACE before the filename.");
	puts("\nYou can also load saved files by making their filename the first argument of this program.");
	puts("\nt = quickmarks, quickly marks all spaces with what they cannot be based on board state.");
	puts("\na = autosolve, automatically iterates [t] and marks in spaces accordingly. Tells you if the board is invalid.");
	puts("\ni = import, \"i filename.txt\"");
	puts("\nc = clear marks");
	puts("\nFormat of imported games does not matter much, but blank spaces must be zeroes.");
	puts("\nYou can launch sudoku with an imported board using sudoku -i filename.txt");
	putchar('\n');putchar('\n');putchar('\n');
}
void save(){
	validate_board();
	if(strlen(combuff) < 3)return;
	char* fn = combuff + 2;
	FILE* f = NULL; f = fopen(fn, "w");
	if(f){
		for(int i = 0; i < 81; i++){
			//Export s lines
			if(board[i%9][i/9]<='9' && board[i%9][i/9]>='1')
				fprintf(f, "s %d,%d,%d\n",i%9+1,i/9+1,board[i%9][i/9]-'1'+1);
			for(int j = 0; j < 9; j++)
				if(marks[i%9][i/9] & 1<<j)
					fprintf(f, "m %d,%d,%d\n",i%9+1,i/9+1,j+1);
		}
		fclose(f);
	}
}
void load(){
	char* fn = combuff + 2;
	FILE* f = NULL;f = fopen(fn, "r");
	ssize_t line_size = 0; 
	size_t line_buffer_size = 0;
	char* line = NULL;
	blankboard();
	blankmarks();
	if(f){
		line_size = getline(&line, &line_buffer_size, f);
		while(line_size >= 0){
			if(line_size < 2048 && line_size > 1)
				{strcpy(combuff,line);processCommand();}
			line_size = getline(&line, &line_buffer_size, f);
		}
		fclose(f);
	}
	validate_board();
}
void import(){
	char* fn = combuff + 2;
	FILE* f = NULL;f = fopen(fn, "r");
	size_t buffer_size = 0;
	char* content = NULL;
	blankboard();
	blankmarks();
	if(f){
		fseek(f,0L,SEEK_END); buffer_size = ftell(f); fseek(f,0L,SEEK_SET); content = (char*)malloc(buffer_size + 1);fread(content, 1, buffer_size, f);
		for(size_t i = 0, index = 0; i < 81 && index<strlen(content);index++){
			if(content[index] <= '9' && content[index] >= '0'){
				board[i%9][i/9] = content[index];
				if(content[index] == '0')content[index] = ' ';
				i++;
			}
		}
		if(content)free(content);
		fclose(f);
	}
	validate_board();
}
void quickmarks(){
	blankmarks();
	validate_board();
	//TODO go through every square, mark all the ones in the same row, same column, and same box with the same value it has.
	for(int x = 0;x < 9; x++)
	for(int y = 0;y < 9; y++)
	{
		if(board[x][y] < '1' || board[x][y] > '9') continue;
		unsigned int shift = board[x][y] - '1';
		if(shift > 8) continue;
		for(int subx = 0;subx < 9; subx++){
			marks[subx][y] |= 1<<shift;
			marks[x][subx] |= 1<<shift;
		}
		int majorx = x/3;
		int majory = y/3;
		for(int subx = 0;subx<3;subx++)
		for(int suby = 0;suby<3;suby++)
		{
			marks[majorx*3+subx][majory*3+suby] |= 1<<shift;
		}
		
	}
}
void set(){
	validate_board();
	unsigned int x, y, val;
	char* commaloc = combuff+1;
	char* commaloc2 = NULL;
	while(*commaloc != '\0' && *commaloc != ',')commaloc++;
	commaloc2 = commaloc+1;
	if(*commaloc != '\0')while(*commaloc2 != '\0' && *commaloc2 != ',')commaloc2++; else commaloc = NULL;
	if(commaloc && *commaloc2 != '\0')
	{
		x = atoi(combuff+1);
		y = atoi(commaloc+1);
		val = abs(atoi(commaloc2+1));
		x--;y--;x%=9;y%=9;
		if(val == 0) board[x][y] = ' '; else {
			val--;val%=9;
			board[x][y] = '1' + val;
		}
	}
	printBoard();
}
void mark(){
	unsigned int x, y, val;
	char* commaloc = combuff+1;
	char* commaloc2 = NULL;
	while(*commaloc != '\0' && *commaloc != ',')commaloc++;
	commaloc2 = commaloc+1;
	if(*commaloc != '\0')while(*commaloc2 != '\0' && *commaloc2 != ',')commaloc2++; else commaloc = NULL;
	if(commaloc && *commaloc2 != '\0')
	{
		x = atoi(combuff+1);
		y = atoi(commaloc+1);
		val = atoi(commaloc2+1);
		x--;y--;val--;x%=9;y%=9;val%=9;
		marks[x][y] |= 1<<val;
	}
	printMarks();
}
int autosolve(){
	validate_board();
	for(int q = 0; q == 0;){
		blankmarks();
		quickmarks();
		int hasChangedSomething = 0;
		for(int i = 0; i < 81; i++)
		{
			//
			if(marks[i%9][i/9] == 511 && board[i%9][i/9] == ' '){
				puts("\nBoard not solveable!");
				q = 1;
				break;
			}
			//Check to see if any particular bit is "alone empty" i.e. it is the only possibility
			for(int shift = 0; shift < 9; shift++){
				if(
					board[i%9][i/9] == ' ' && 
					(marks[i%9][i/9] | (1<<shift)) == 511
				) //Is only possibility
				{
					board[i%9][i/9] = '1' + shift; hasChangedSomething = 1; i = 81; shift = 9; break;
				}
			}
		}
		if(!hasChangedSomething) q=1;
	}
}
int processCommand(){
	validate_board();
	for(int i = 0; i < 2048; i++)if(combuff[i]=='\n')combuff[i]='\0';
	switch(combuff[0]){
		case 'h':case 'H':default:help();break;
		case 'q':case 'Q':if(combuff[1]!='\0')combuff[0]=' ';break; //Prevent accidental quitting.
		case 'm':case 'M':mark();break;
		case 's':case 'S':set();break;
		case 't':case 'T':quickmarks();break;
		case 'l':case 'L':load();break;
		case 'p':case 'P':save();break;
		case 'a':case 'A':autosolve();break;
		case 'i':case 'I':import();break;
		case 'c':case 'C':blankmarks();break;
	}
}
int main(int argc, char** argv){
	combuff[0]='\0';combuff[2047]='\0';
	blankboard();blankmarks();
	printBoard();
	printMarks();
	help();
	if(argc == 2 && strlen(argv[1]) < 2040){
		strcpy(combuff, "l ");
		strcat(combuff, argv[1]);
		load();
	}
	if(argc > 2){
		if(!strcmp(argv[1],"-i") && strlen(argv[2]) < 2040){
			strcpy(combuff,"i ");
			strcat(combuff,argv[2]);
			import();
		}
	}
	while(combuff[0]!='q' && combuff[0]!='Q'){
		fgets(combuff,2047,stdin);combuff[2047]='\0';
		processCommand();
	}
	return 0;
}