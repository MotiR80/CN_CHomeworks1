#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

using namespace std;

struct Response{
    char *res;
    string status_address, status_code, status_text;
    string type;
    int length;
    char *data;
    char str[1000];
    size_t resLen;
    bool redirect = false;

    void set_res(){
        if (!redirect) {
            sprintf(str,
                    "HTTP/1.1 %s %s\r\nContent-Length: %d\r\nConnection: close\r\nContent-Type: %s; charset=UTF-8\r\n\r\n",
                    status_code.c_str(), status_text.c_str(), length, type.c_str());
            resLen = strlen(str) + length;
        }
        else{
            strcpy(str, "HTTP/1.1 308 Permanent Redirect\r\nLocation: http://127.0.0.1:34002/index.html\r\n\r\n");
            resLen = strlen(str);
        }
        res = (char*)malloc(resLen);
        memcpy(res, str, strlen(str));
        if (!redirect)
            memcpy(res + strlen(str), data, length);
    }

    void reset(){
        type = "";
        length = 0;
        status_address = "";
        status_text = "";
        status_code = "";
        redirect = false;
    }
};


void content_type(Response&);
void request_parse(string, Response&);
void read_data(Response&);
void send_response(int, Response&);
size_t file_size(const char* file_path);

int main() {
    int listener, new_socket, addrLen, opt = true;
    char buffer[1024];

    int port = 34002;
    char ip[] = "127.0.0.1";
    struct sockaddr_in address;

    //make socket
    if((listener = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket ");
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    addrLen = sizeof(&address);

    //make socket address reusable
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    //bind socket
    if (bind(listener, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("bind ");
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, sizeof(address));

    //now listen
    if (listen(listener, 5) <0){
        perror("listen ");
        exit(EXIT_FAILURE);
    }

    Response res;

    cout << "Listen on ip " << ip<< " and port " << port <<endl<<endl;

    while (true){

        if ((new_socket = accept(listener, (struct sockaddr *) &address, (socklen_t *) &addrLen)) < 0) {
            perror("accept: ");
            exit(EXIT_FAILURE);
        }
        cout << "new socket "<< new_socket<< ", ip is "<<inet_ntoa(address.sin_addr) <<" and port "<< ntohs(address.sin_port)<< endl;

        string request;

        if ((read(new_socket, buffer, sizeof(buffer))) <= 0){
            perror("read ");
            shutdown(new_socket, SHUT_RDWR);
            close(new_socket);
            continue;
        }
        request += buffer;

        request_parse(request, res);
        content_type(res);
        read_data(res);
        send_response(new_socket, res);
        shutdown(new_socket, SHUT_RDWR);
        close(new_socket);
        res.reset();
    }

}

void request_parse(string request, Response& resp){
    int st, fi;

    st = request.find_first_of('/');
    fi = request.find_first_of(' ', st);

    string filePath = request.substr(st+1, fi - st - 1);

    if (filePath == "") {
        resp.status_address = "../index.html";
        resp.redirect = true;
    }
    else
        resp.status_address = "../" + filePath;
}
void content_type(Response& res){
    int dot;
    string format;

    dot = res.status_address.find_last_of('.')+1;

    if (dot != 0)
        format = res.status_address.substr(dot, res.status_address.length() - dot);
    else
        format = "html";

    if (format == "html")
        res.type = "text/html";
    else if (format == "jpeg")
        res.type = "image/jpeg";
    else if (format == "gif")
        res.type = "image/gif";
    else if (format == "mp3")
        res.type = "audio/mp3";
    else if (format == "pdf")
        res.type = "application/pdf";
    else if(format == "mp4")
        res.type = "video/mp4";
}
void read_data(Response& res){

    res.status_code = "200";
    res.status_text = "OK";

    size_t size = file_size(res.status_address.c_str());

    FILE *fp;
    if(!(fp = fopen(res.status_address.c_str(), "rb"))) {
        fp = fopen("../404.html", "rb");
        res.status_code = "404";
        res.status_text = "Not Found";
    }

    char *buffer = (char*)malloc(size);

    res.length = size;

    fread(buffer, 1, size, fp);
    fclose(fp);

    res.data = (char*)malloc(size);
    memcpy(res.data, buffer, size);

    res.set_res();

}
void send_response(int sd, Response& res){

    if(send(sd, res.res, res.resLen, 0) <= 0){
        perror("send ");
    }
}

size_t file_size(const char* file_path) {
    FILE* fin = fopen(file_path, "rb");
    if (fin == NULL) {
      fin = fopen("../404.html", "rb");
    }

    fseek(fin, 0L, SEEK_END);
    size_t size = ftell(fin);
    fclose(fin);
    return size;
}
