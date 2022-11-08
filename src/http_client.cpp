#include "../include/http_client.h"

#include <iostream>
#include <algorithm>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

std::string path = "/home/wlle/";

buffer::buffer(int len)
    :last_index(0), now_index(0), buffer_len(len), buffers(new char[len])
{}

/*
    std::string file_path;      // 文件路径
    std::string file_name;      // 文件名
    std::string str;            // 文件内容
    std::string method;         // 请求方法
    std::string m_version;      // HTTP协议版本号，我们仅支持HTTP1.1
    std::string m_host;         // 主机名
    int m_content_length; // HTTP请求的消息总长度
    bool m_linger;        // HTTP请求是否要求保持连接
*/

client::client() {}

client::~client() {}

void client::init(int fd2, const sockaddr_in& addr2)
{

    fd = fd2;
    addr = addr2;

    xin_xi = new massage();
    xin_xi->file_path = path;
    xin_xi->fd = fd;

    reader.init(xin_xi);
    writer.init(xin_xi);
}

void client::run()
{
    if(!reader.read())
    {
        std::cerr << " 读取信息时出错 \n";
        return;
    }

    if(!reader.process())
    {
        std::cerr << " 处理请求时出错 \n";
        return;
    }

    std::cout << " 文件路径 : " << xin_xi->file_path
              << "\n 请求文件名 : " << xin_xi->file_name
              << "\n 通讯主机IP及端口 : " << xin_xi->m_host
              << "\n HTTP协议版本号 : " << xin_xi->m_version
              << "\n 请求长度 : " << xin_xi->m_content_length
              << std::endl;
    
    if(!writer.process())
    {
        std::cerr << " 写失败 \n";
        return;
    }

}


/*-----------------------------------*/
/*  read    */


client::m_read::m_read()
    : mas(nullptr), read_buffer(buffer(1024))
{}

client::m_read::~m_read() {}

void client::m_read::init(massage* mass)
{
    mas = mass;
    check_state = CHECK_STATE_REQUESTLINE;
}

// 缓冲区读取函数
bool client::m_read::read()
{
    if(read_buffer.last_index > read_buffer.buffer_len)
        return false;
    
    int len = recv(mas->fd, read_buffer.buffers + read_buffer.last_index, read_buffer.buffer_len - read_buffer.last_index, 0);
    if(len < 0)
    {
        std::cerr << " read error \n";
        return false;
    }
    else if(len == 0)
    {
        std::cout << " link break \n";
        return false;
    }

    read_buffer.last_index += len;
    mas->m_content_length = len;
    return true;
}

// 处理函数
bool client::m_read::process()
{
    bool ret = true;
    std::string text;

    while(text != "error" && read_buffer.now_index <= read_buffer.last_index)
    {
        text = get_line();
        std::cout << "text : " << text << std::endl;

        switch(check_state)
        {
            case CHECK_STATE_REQUESTLINE:
            {
                ret = process_hang(text);
                if(!ret)
                {
                    std::cerr << " 处理请求行有误 \n";
                    return false;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                ret = process_tou(text);
                if(!ret && check_state != CHECK_STATE_CONTENT)
                {
                    std::cerr << " 处理请求头有误 \n";
                    return false;
                }
                break;
            }
            case CHECK_STATE_CONTENT:
            {
                ret = process_ti(text);
                if(!ret)
                {
                    std::cerr << " 处理请求体有误 \n";
                    return false;
                }
                break;
            }
            default:
            {
                return false;
            }
        };
    }
    return true;
}

std::string client::m_read::get_line()
{
    if(read_buffer.now_index == read_buffer.last_index)
        return "error";
    char* a = std::find(read_buffer.buffers + read_buffer.now_index, read_buffer.buffers + read_buffer.last_index, '\n');
    int index2 = read_buffer.now_index;
    int index1 = a - read_buffer.buffers;

    std::string s(index1 - index2 - 1, ' ');
    for(int i = 0; i != s.size(); ++i)
    {
        s[i] = read_buffer.buffers[index2 + i];
    }

    read_buffer.now_index = index1 + 1;
    
    return s;
}

bool func(const char& pa)
{
    return (pa == ' ') || (pa == '\t');
}

void func2(int lhs, int rhs, const buffer& read)
{
    for(int i = lhs; i != rhs; ++i)
    {
        std::cout << read.buffers[i];
    }
    std::cout << '\n';
}

bool client::m_read::process_hang(std::string text)
{
    int lhs = -1, rhs = -1;

    /*   GET   */
    rhs = std::find_if(text.begin() + rhs + 1, text.end(), func) - text.begin();
    lhs = std::find_if_not(text.begin() + lhs + 1, text.end(), func) - text.begin();

    if(text.substr(lhs, rhs - lhs) == "GET")
        mas->method = "GET";
    else
        return false;

    /*    请求的文件名    */
    lhs = rhs;
    rhs = std::find_if(text.begin() + rhs + 1, text.end(), func) - text.begin();
    lhs = std::find_if_not(text.begin() + lhs + 1, text.end(), func) - text.begin();

    if(text[lhs] == '/')
        mas->file_name = text.substr(lhs + 1, rhs - lhs - 1);
    else
        return false;

    /*   HTTP协议版本号    */
    lhs = rhs;
    rhs = std::find_if(text.begin() + rhs + 1, text.end(), func) - text.begin();
    lhs = std::find_if_not(text.begin() + lhs + 1, text.end(), func) - text.begin();

    if(text.substr(lhs, 5) == "HTTP/")
        mas->m_version = text.substr(lhs + 5);
    else
        return false;

    check_state = CHECK_STATE_HEADER;

    return true;
}

bool client::m_read::process_tou(std::string text)
{
    if(text.size() == 0)
    {
        check_state = CHECK_STATE_CONTENT;
        return false;
    }

    int kong_ge = 0;
    kong_ge = std::find_if(text.begin() + kong_ge, text.end(), func) - text.begin();

    if(text.substr(0, kong_ge) == "Host:")
    {
        mas->m_host = text.substr(kong_ge + 1);
    }
    else if(text.substr(0, kong_ge) == "Connection:")
    {
        if(text.substr(kong_ge + 1) == "keep-alive")
            mas->m_linger = true;
        else if(text.substr(kong_ge + 1) == "close")
            mas->m_linger = false;
        else
            return false;
    }
    
    return true;
}

bool client::m_read::process_ti(std::string)
{
    return true;
}

/*----------------------------------------*/
/*  write   */


client::m_write::m_write()
    : mas(nullptr), write_buffer(buffer(1024))
{}

client::m_write::~m_write() {}

void client::m_write::init(massage* mass)
{
    mas = mass;
    file_status = " 一切正常 ";
}

bool client::m_write::open_file(const char* file)
{
    if(access(file, F_OK) != 0)
    {
        file_status = " 没有对应文件 ";
        return false;
    }

    struct stat file_state;

    if(stat(file, &file_state) != 0)
    {
        file_status = " 获取文件信息失败 ";
        return false;
    }

    if(!(file_state.st_mode & S_IROTH))
    {
        file_status = " 文件不可读 ";
        return false;
    }

    if(S_ISDIR(file_state.st_mode))
    {
        file_status = " 此文件为目录 ";
        return false;
    }

    int f = open(file, O_RDONLY);

    void* pstr = mmap(nullptr, file_state.st_size, PROT_READ, MAP_PRIVATE, f, 0);
    if(pstr == MAP_FAILED)
    {
        file_status = " 内存映射失败 ";
        return false;
    }

    file_size = file_state.st_size;
    file_text = (char*)pstr;

    close(f);

    return true;
}

bool client::m_write::close_file()
{
    if(file_text)
    {
        munmap(file_text, file_size);
        file_text = nullptr;
        return true;
    }
    return false;
}

bool client::m_write::process()
{
    std::string file = mas->file_path + mas->file_name;


    if(!open_file(file.data()))
    {
        std::cerr << file_status;
        file_text = const_cast<char*>(file_status.data());
    }

    if(!start_write())
    {
        return false;
    }

    close_file();

    return true;
}

bool client::m_write::start_write()
{
    if(!add_status_line())
        return false;

    if(!add_headers())
        return false;

    write_to_buffer(file_text);
    
    return true;
}

bool client::m_write::write_to_buffer(const std::string& hang)
{
    if(write_buffer.last_index > write_buffer.buffer_len)
    {
        file_status = " 往缓冲区写数据失败 ";
        return false;
    }

    if(hang.size() >= (write_buffer.buffer_len - write_buffer.last_index - 1))
    {
        file_status = " 往缓冲区写数据失败 ";
        return false;
    }

    if(write(mas->fd, hang.data(), hang.size()) <= 0)
    {
        file_status = " 往缓冲区写数据失败 ";
        return false;
    }
    write_buffer.last_index += hang.size();

    return true;
}

//  添加响应行
bool client::m_write::add_status_line()
{
    std::string text = "HTTP/" + mas->m_version + " " + "200 OK\r\n";
    return write_to_buffer(text);
}

//  添加响应体
bool client::m_write::add_content(const char *content)
{
    return true;
}

//  添加响应体类型
bool client::m_write::add_content_type()
{
    std::string text = "Content-Type: text/html\r\n";
    return write_to_buffer(text);
}

//  添加响应体长度
bool client::m_write::add_content_length()
{
    std::string text = "Content-Length: " + std::to_string(file_size) + "\r\n";
    return write_to_buffer(text);
}

//  添加响应头
bool client::m_write::add_headers()
{
    bool k = true;
    k = add_content_length();
    if(!k)
        return false;
    k = add_content_type();
     if(!k)
        return false;
    k = add_linger();
     if(!k)
        return false;
    k = add_blank_line();
     if(!k)
        return false;
    
    return k;
}

// 添加是否保持连接
bool client::m_write::add_linger()
{
    std::string text = "Connection: ";
    if(mas->m_linger)
        text += "keep-alive\r\n";
    else
        text += "close\r\n";
    
    return write_to_buffer(text);
}

// 添加隔断行
bool client::m_write::add_blank_line()
{
    return write_to_buffer("\r\n");
}
