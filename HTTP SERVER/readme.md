### HTTP_SERVER

## Code
This struct is use to make response.

```cpp
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
```
`void set_res()` this function sets response.

###

```cpp
void content_type(Response&);
void request_parse(string, Response&);
void read_data(Response&);
void send_response(int, Response&);
size_t file_size(const char* file_path);
```
The function `void request_parse(string, Response&);` parses the request and find requst path.

The function `void content_type(Response&);` extract content type from the path.

The function `void read_data(Response&);` read the requested file and store the data in a char array.

The function `size_t file_size(const char* file_path);` gets the file size.

The function `void send_response(int, Response&);` send the response to the web browser.

###

```cpp
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

```
We make a socket then bind and listen.

###

```cpp
Response res;
```
Make a response object.

###

```cpp
while (true){

        if ((new_socket = accept(listener, (struct sockaddr *) &address, (socklen_t *) &addrLen)) < 0) {
            perror("accept: ");
            exit(EXIT_FAILURE);
        }
```
Accept the new call.

###

```cpp
string request;

        if ((read(new_socket, buffer, sizeof(buffer))) <= 0){
            perror("read ");
            shutdown(new_socket, SHUT_RDWR);
            close(new_socket);
            continue;
        }
        request += buffer;
```

Start to read the request and store it in `request`.

###

```cpp
request_parse(request, res);
        content_type(res);
        read_data(res);
        send_response(new_socket, res);
        shutdown(new_socket, SHUT_RDWR);
        close(new_socket);
        res.reset();
    }

}
```

Then parse the request then extract content type and read the requested file then send it and close the socket.

## Example
First we run server.

<img src="https://github.com/MotiR80/CN_CHomeworks1/blob/main/HTTP%20SERVER/Examples/run-server.png" width =450/>

Then open browser and enter ip:port --> `127.0.0.1:34002/`.

<img src="https://github.com/MotiR80/CN_CHomeworks1/blob/main/HTTP%20SERVER/Examples/open-browser-goto-website.png" width =450/>

We redirect to address `127.0.0.1:34002/index.html`.

<img src="https://github.com/MotiR80/CN_CHomeworks1/blob/main/HTTP%20SERVER/Examples/redirect.png" width =450/>

Now click on image.

<img src="https://github.com/MotiR80/CN_CHomeworks1/blob/main/HTTP%20SERVER/Examples/open-img.png" width =450/>

Now click on gif.

<img src="https://github.com/MotiR80/CN_CHomeworks1/blob/main/HTTP%20SERVER/Examples/open-gif.png" width =450/>

Now click on pdf.

<img src="https://github.com/MotiR80/CN_CHomeworks1/blob/main/HTTP%20SERVER/Examples/open-pdf.png" width =450/>

Now click on music.

<img src="https://github.com/MotiR80/CN_CHomeworks1/blob/main/HTTP%20SERVER/Examples/open-music.png" width =450/>

Now we enter invalid address. The 404.html file show.

<img src="https://github.com/MotiR80/CN_CHomeworks1/blob/main/HTTP%20SERVER/Examples/invalid-addr.png" width =450/>
