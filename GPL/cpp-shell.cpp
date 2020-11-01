//working on moving cursor using arrows without an external API

#define KEY_UP 72

#define KEY_DOWN 80

#define KEY_LEFT 75

#define KEY_RIGHT 77

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstdlib>
int main() {
    std::string clear = "clear";
    std::string compile = "g++ -o file.out out.cpp";
    const char *clear_command = clear.c_str();
    std::cout << "C++ interactive shell" << std::endl;
    system(clear_command);
    if (__cplusplus == 201703L)
        std::cout << "Version: C++17" << std::endl;
    else if (__cplusplus == 201402L)
        std::cout << "Version: C++14" << std::endl;
    else if (__cplusplus == 201103L)
        std::cout << "Version: C++11" << std::endl;
    else if (__cplusplus == 199711L)
        std::cout << "Version: C++98" << std::endl;
    else
        std::cout << "Version: pre-standard C++" << std::endl;
    std::vector<std::string> arr;
    int size = arr.size();
    std::fstream file;
    while(true){
        std::string line;
        std::cout<<">>>";
        std::getline(std::cin, line);
        arr.push_back(line);
        //system("echo '\\033[NC'");

        if (line == "End;"){
            std::ofstream outfile ("out.cpp");
            size -= 1;
            for(int i=0; i < size; i++)
                outfile << arr.at(i) << ' ' << std::endl;
            outfile.close();
            std::string str = "g++ ";
            str = str + "-o file.out " + "out.cpp";

            const char *command = str.c_str();

            std::cout << "\nCompiling file using " << command << std::endl;
            system(command);

            std::cout << "\nRunning file...\n";
            system("./file.out");
            system("rm out.cpp && rm file.out");
            break;

        }
    }
    return 0;
}
