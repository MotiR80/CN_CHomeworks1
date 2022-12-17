#include <iostream>
#include <string.h>
using namespace std;

class Server{
public:
    string username();
    string password();
    string upload();
    string download();
    string erorr();
    string quit();
    string help();

private:
    int isAdmin = 0,
        isUser = 0;
};

string* arg_value(string s){
   int loc;
   static string args[2];
   loc = s.find(' ');
    if (loc != -1){
        args[0] = s.substr(0, loc);
        args[1] = s.substr(loc+1, s.length() - loc - 1);
    } else args[0] = s, args[1] = s;
    return args;
}

int main() {
    Server obj;
    string command;
    string* argv;
    argv = arg_value(command);

    while (argv[0] != "exit"){
        if (argv[0] == "user"){
            obj.username();
        }
        else if (argv[0] == "pass"){
            obj.password();
        }
        else if (argv[0] == "retr"){
            obj.download();
        }
        else if (argv[0] == "Upload"){
            obj.upload();
        }
        else if (argv[0] == "help"){
            obj.help();
        }
        else if (argv[0] == "quit"){
            obj.quit();
        }
        else {
            obj.erorr();
        }
    }
}
