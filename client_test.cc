/*不使用浏览器，编写自己的HTTP客户端来测试你的服务器。您的客户端将使用一个TCP连接用于连接到服务器
，向服务器发送HTTP请求，并将服务器响应显示出来。您可以假定发送的HTTP请求将使用GET方法。 
客户端应使用命令行参数指定服务器IP地址或主机名，服务器正在监听的端口，以及被请求对象在服务器上的路径。
以下是运行客户端的输入命令格式。
client.py server_host server_port filename */

// g++ -o client client.cpp
//./client 192.168.48.130 2200 /home/llh/Computer_Network/socket/HelloWorld.html

//分别开两个终端，一个运行web_sever 一个运行client   得到的html文件在 /home/llh/Computer_Network/socket/newhtml.html里

#include <iostream>
#include <unistd.h>  //无 cuistd 头文件
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h> //for struct addrinfo
#include <errno.h> //for stderror
#include <string>
#include <sstream>
#include <fstream>
#include <thread>

#define BUFSIZE 10000

int main(int argc, char **argv) {
    int clientfd;
    struct sockaddr_in sad;  //server sockaddr
    struct sockaddr_in cad;  //client sockaddr
    char buf[BUFSIZ];
    memset(buf, 0, sizeof(buf));

    std::string send_message;
    send_message += "GET ";
    send_message += argv[3]; // /home/llh/Computer_Network/socket/HelloWorld.html
    send_message += " HTTP/1.1\nHost: ";
    send_message += argv[1];
    send_message += ":";
    send_message += argv[2];
    send_message += " Connection: keep-alive\n\n";


    memset(&sad,0,sizeof(sad));
    sad.sin_family  = AF_INET;
	if(inet_pton(AF_INET, argv[1], &(sad.sin_addr)) <= 0 ) {  //将点分十进制串转换成二进制网络字节顺序ip地址
        std::cerr<<"inet_pton error: "<<strerror(errno);
        exit(1); //异常退出
    } 
	sad.sin_port = htons(atoi(argv[2]));              //端口是16位无符号整数，所以用htons来转换  用atoi转成整数

    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr<<"socket error: "<<strerror(errno);
        exit(1); //异常退出
    }
    if(connect(clientfd, (struct sockaddr *)&sad, sizeof(sockaddr_in)) == -1) {
        std::cerr<<"connect error: "<<strerror(errno);
        close(clientfd);
        exit(1); 
    }
    if(send(clientfd, send_message.c_str(), send_message.size(), 0) <= 0) {
        std::cerr<<"send error: "<<strerror(errno);
        close(clientfd);
        exit(1); 
    }
    if(recv(clientfd, buf, BUFSIZE, 0) < 0) {  // <0是出错 ==0是另一端连接关闭
        std::cerr<<"recv error: "<<strerror(errno);
        close(clientfd);
        exit(1);
    }
    std::cerr<<"here\n";
    std::cerr<<buf<<std::endl;

    close(clientfd);
    return 0;
}