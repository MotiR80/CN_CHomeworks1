#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>

#define True 1
#define PATH "../downloads/"
#define BUFFSIZE 10000

using namespace std;

struct CMD {
    string order;
    string arg;

    void get(string s){;
        for (int i = 0; i < s.length(); ++i) {
            if (s[i] == ' '){
                order = s.substr(0, i);
                arg = s.substr(i+1, s.length() - i);
                return;
            }
        }
        order = s;
    }

    void reset(){
        arg = "";
        order = "";
    };
};

string fileName(string);
void download(struct sockaddr_in&, string);
void upload(struct sockaddr_in&, string);


int main() {

    // creates a downloads directory if it was not created before
    if (mkdir(PATH, S_IRWXG | S_IRWXU | S_IRWXO) == -1)
        cout << "downloads directory already existing.\n";
    else {
        cout << "downloads directory created successfully.\n";
    }

    // read json file
    ifstream configFile("../config.json");
    Json::Value config;
    Json::Reader reader;

    // using reader for parsing json file and store data in config
    reader.parse(configFile, config);


    // get server ip and cmd_port and data_port from json file.
    const char* ip = config["server"]["ip"].asCString();
    int cmd_port = config["server"]["cmd_port"].asInt();
    int data_port = config["server"]["data_port"].asInt();

    int valread;

    // socket descriptor
    int cmd_socket;
    cmd_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in cmd_addr, data_addr;

    // command buffer
    char buffer[128];

    // input command store in cmd
    string command;
    CMD cmd;

    // command channel socket type
    cmd_addr.sin_family = AF_INET;
    cmd_addr.sin_addr.s_addr = inet_addr((const char*)ip);
    cmd_addr.sin_port = htons(cmd_port);
    //----------------------------------------------------------------------------------------------

    // data channel socket type
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = inet_addr((const char*)ip);
    data_addr.sin_port = htons(data_port);

    // make socket address reusable
    int opt = True;

    // connect to the server
    if(connect(cmd_socket, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr)) < 0){
        perror("Connection ");
        exit(EXIT_FAILURE);
    }

    do {

        // get input and store in cmd
        getline(cin, command);
        cmd.get(command);

        // copy string to buffer
        strcpy(buffer, command.c_str());

        // send to server
        send(cmd_socket, buffer, strlen(buffer), 0);

        // download
        if (cmd.order == "retr")
            download(data_addr, cmd.arg);
        // upload
        else if (cmd.order == "upload")
            upload(data_addr, cmd.arg);

        valread = read(cmd_socket, buffer, sizeof(buffer));
        buffer[valread] = '\0';

        cout << buffer;

    }
    while (cmd.order != "quit");

    close(cmd_socket);

}

string fileName(string str){
    const size_t last_slash_idx = str.find_last_of("\\/");
    if (std::string::npos != last_slash_idx)
    {
        str.erase(0, last_slash_idx + 1);
    }
    return str;
}
void download(struct sockaddr_in& data_addr, string path){
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sd, (struct sockaddr*)&data_addr, sizeof(data_addr)) <0){
        perror("data connection");
        return;
    }

    // extract file name from path
    string fName = fileName(path);
    char fPath[30];
    strcpy(fPath, (PATH+fName).c_str());


    // store the file in downloads directory
    char data[BUFFSIZE];
    FILE *fp;

    int nb;
    // read the file
    if((nb = read(sd, data, BUFFSIZE)) <= 0){
        perror("read file ");
        close(sd);
        return;
    }

    // store the file in downloads directory
    fp = fopen(fPath, "w");

    while (nb > 0){
        fwrite(data, 1, nb, fp);
        nb = read(sd, data, BUFFSIZE);
    }

    fclose(fp);
    close(sd);
}
void upload(struct sockaddr_in& data_addr, string path){
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    int dataLen = sizeof(data_addr);
    if (connect(sd, (struct sockaddr*)&data_addr, (socklen_t) dataLen) <0){
        perror("data connection");
        return;
    }

    char buffer[100];

    // get file size
    struct stat st;
    strcpy(buffer, path.c_str());

    char faddr[30];
    strcpy(faddr , buffer);

    if (stat(faddr, &st) == -1) {
        close(sd);
        return ;
    }
    int size = st.st_size;

    // send file size to server
    sprintf(buffer,"%ld", size);

    if (send(sd, buffer, 10, 0)<0){
        perror("send ");
        close(sd);
        return;
    }

    // read file and send
    char data[size];

    FILE *fp;
    fp = fopen(faddr, "r");
    int nb;

    while (!feof(fp)){
        nb = fread(data, 1, BUFFSIZE, fp);
        write(sd, data, nb);
    }
    fclose(fp);
    close(sd);
}
