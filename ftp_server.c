#include <iostream>
#include <string.h>
using namespace std;

class Server{
public:
    int username();
    int password();
    int upload();
    int download();
    string massage();
    int quit();
    int help();

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
        int m;
        if (argv[0] == "user"){
            m = obj.username(argv[1]);
            cout << obj.massage(m);
        }
        else if (argv[0] == "pass"){
            m = obj.password();
            cout << obj.massage(m);
        }
        else if (argv[0] == "retr"){
            obj.download();
            cout << obj.massage(m);
        }
        else if (argv[0] == "Upload"){
            obj.upload();
            cout << obj.massage(m);
        }
        else if (argv[0] == "help"){
            obj.help();
            cout << obj.massage(m);
        }
        else if (argv[0] == "quit"){
            obj.quit();
            cout << obj.massage(m);
        }
        else {
            obj.massage();
        }
    }
}
