#include "threadpool.h"
#include "pvz_server.h"

const char *DEFAULT_IP = "192.168.48.130";  // arch ip地址
const int DEFAULT_PORT = 2200;

int main(int argc, char *argv[]) {
    // ThreadPool<double> tp(0, 2);
    const char* ip = DEFAULT_IP;
    int port = DEFAULT_PORT;
    int thread_num = 8;
    if(argc >= 2) {
        thread_num = atoi(argv[1]);
    }
    std::cout<<"Thread num: "<<thread_num<<std::endl;
    if(argc <= 3) {
        std::cout<<"Too few arguments, use the default ip and port\n";
    } else {
        ip = argv[2];
        port = atoi(argv[3]);
        std::cout<<"ip: "<<ip<<" port: "<<port<<std::endl;
    }

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address)); // 将sin_zero部分清0
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);
    server_address.sin_port = htons(port);

    int ret = bind(listenfd, (struct sockaddr*)&server_address, sizeof(server_address));
    assert(ret != -1);

    ret = listen(listenfd, 1024);
    assert(ret != -1);

    // 创建线程池
    ThreadPool<PVZServer> *thread_pool = nullptr;
    try {
        thread_pool = new ThreadPool<PVZServer>(listenfd, thread_num);
        assert(thread_pool);
        thread_pool->Run();

    } catch(const std::exception& e) {
        std::cout<<e.what()<<std::endl;
    }    
    delete thread_pool;
    close(listenfd);
    return 0;
}