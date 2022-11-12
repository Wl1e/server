#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "http_client.h"
#include <signal.h>
#include "thread_pool.hpp"

extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);

void addsig(int sig, void(handler)(int))
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL) != -1;
}

int main()
{
    const int MAX_CLIENT = 8;

    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    addsig(SIGPIPE, SIG_IGN);

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
    ret = listen(listen_fd, MAX_CLIENT);
    if(ret == -1)
    {
        std::cerr << " error \n";
    }

    // 用户数组
    client cts[MAX_CLIENT + 3];

    // 线程池
    thread_pool<client>* pool;
    pool = new thread_pool<client>;

    // epoll
    int epoll_fd = epoll_create(MAX_CLIENT);
    epoll_event events[MAX_CLIENT + 3];

    addfd(epoll_fd, listen_fd, false);
    client::epoll_fd = epoll_fd;

    // client ct;
    // sockaddr_in addr2;
    // socklen_t size = sizeof(addr2);
    // int fd = accept(listen_fd, (sockaddr*)&addr2, &size);

    // ct.init(fd, addr);
    // ct.run();


    while (true)
    {

        int number = epoll_wait(epoll_fd, events, MAX_CLIENT, -1);

        if (number < 0)
        {
            std::cerr << "epoll failure\n";
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;

            if (sockfd == listen_fd)
            {

                sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int fd = accept(listen_fd, (sockaddr *)&client_address, &client_addrlength);

                if (fd < 0)
                {
                    std::cerr << " error \n";
                    continue;
                }

                if (client::link_nums >= MAX_CLIENT)
                {
                    close(fd);
                    continue;
                }

                std::cout << " 用户接入 -> "
                          << " fd : " << fd
                          << " IP : " << client_address.sin_addr.s_addr
                          << " 端口 : " << client_address.sin_port
                          << std::endl;

                cts[fd].init(fd, client_address);
                addfd(epoll_fd, fd, true);
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
    delete pool;

    return 0;
}