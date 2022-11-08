#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "http_client.h"
#include <unistd.h>

int main()
{
    int ret = 0;
    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(5005);

    ret = bind(listen_fd, (const sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
    {
        std::cerr << " error \n";
    }

    ret = listen(listen_fd, 5);
    if(ret == -1)
    {
        std::cerr << " error \n";
    }

    sockaddr_in addr2;
    socklen_t size = sizeof(addr2);
    int fd = accept(listen_fd, (sockaddr*)&addr2, &size);

    client ct;

    ct.init(fd, addr2);

    ct.run();

    close(fd);
    close(listen_fd);

    return 0;
}