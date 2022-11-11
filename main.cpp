#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "include/thread_pool.h"
#include <unistd.h>
#include "include/http_client.h"

extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);

int main()
{
    const int MAX_CLIENT = 8;

    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    // 端口复用
    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 绑定ip等
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(5005);
    int ret = bind(listen_fd, (const sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
    {
        std::cerr << " error \n";
    }

    // 监听
    ret = listen(listen_fd, 5);
    if(ret == -1)
    {
        std::cerr << " error \n";
    }


    sockaddr_in addr2;
    socklen_t size = sizeof(addr2);
    int fd = accept(listen_fd, (sockaddr*)&addr2, &size);

    // 用户数组
    client cts[MAX_CLIENT];

    // 线程池
    thread_pool<client>* pool;
    pool = new thread_pool<client>();

    int epoll_fd = epoll_create(MAX_CLIENT);
    epoll_event events[MAX_CLIENT];

    addfd(epoll_fd, fd, false);
    client::epoll_fd = epoll_fd;

    while (true)
    {

        int number = epoll_wait(epoll_fd, events, MAX_CLIENT, -1);

        if ((number < 0) && (errno != EINTR))
        {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++)
        {

            int sockfd = events[i].data.fd;

            if (sockfd == listen_fd)
            {

                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int fd = accept(listen_fd, (struct sockaddr *)&client_address, &client_addrlength);

                if (fd < 0)
                {
                    printf("errno is: %d\n", errno);
                    continue;
                }

                if (client::link_nums >= MAX_CLIENT)
                {
                    close(fd);
                    continue;
                }
                cts[fd].init(fd, client_address);
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {

                cts[sockfd].close_link();
            }
            else if (events[i].events & EPOLLIN)
            {

                if (cts[sockfd].to_read())
                {
                    pool->add_work(cts + sockfd);
                }
                else
                {
                    cts[sockfd].close_link();
                }
            }
            else if (events[i].events & EPOLLOUT)
            {

                if (!cts[sockfd].to_write())
                {
                    cts[sockfd].close_link();
                }
            }
        }
    }


    close(epoll_fd);
    close(listen_fd);

    return 0;
}