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
}#include <iostream>
#include <string>
#include <winsock.h>

#define BUFF 1024
#define True 1
#define False 0

using namespace std;


int username(string);
int password(string);
int upload(string);
int download(string);
string massage(int);
int quit();
int help();

struct Client {
    int isAdmin = 0,
        isUser = 0,
        socket = 0;
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

void commands(string* argv){
    int m;
    if (argv[0] == "user"){
        m = username(argv[1]);
        cout << massage(m);
    }
    else if (argv[0] == "pass"){
        m = password(argv[1]);
        cout << massage(m);
    }
    else if (argv[0] == "retr"){
        download(argv[1]);
        cout << massage(m);
    }
    else if (argv[0] == "Upload"){
        upload(argv[1]);
        cout << massage(m);
    }
    else if (argv[0] == "help"){
        help();
        cout << massage(m);
    }
    else if (argv[0] == "quit"){
        quit();
        cout << massage(m);
    }
    else {
        massage(m);
    }
}

int main() {
    char* ip;
    int port;

    int opt = True;
    int listen_sd , addrlen , new_socket,
            max_clients = 10 , activity, i , valread , sd;

    Client client[10];
    int max_sd;

    struct sockaddr_in address;

    // socket type
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr((const char*)ip);
    address.sin_port = htons(port);
    //----------------------------------------------------------------------------------------------

    // data buffer with size 1k
    char buffer[1024];

    // set socket descriptors
    fd_set readsds;

    //create master socket or listen socket descriptor
    if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    //----------------------------------------------------------------------------------------------

    //make master socket address reusable
    if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    //----------------------------------------------------------------------------------------------

    //bind master socket to ip and port from config.json
    if ((bind(listen_sd, (struct sockaddr*)&address, sizeof(address))) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    //-----------------------------------------------------------------------------------------------

    cout << "Listen on port: " << port;
    // listen on port with maximum 5 pending connection
    if ((listen(listen_sd, 5)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    //------------------------------------------------------------------------------------------------

    // accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (true){
        // first clear the socket discriptor set
        FD_ZERO(&readsds);

        // add master socket to socket discriptor set (readsds)
        FD_SET(listen_sd, &readsds);
        max_sd = listen_sd;

        // add client socket to the set
        for (int j = 0; j < max_clients; ++j) {
            // socket discriptor
            sd = client[j].socket;

            //if valid socket decriptor then add to read list
            if (sd > 0)
                FD_SET (sd, &readsds);

            //highest descriptor number needs for select function
            if (sd > max_sd)
                max_sd = sd;
        }

    }
}



    string command;
    string* argv;
    argv = arg_value(command);

    while (argv[0] != "exit"){
