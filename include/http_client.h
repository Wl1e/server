#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <string>

// 缓冲区
struct buffer
{
    using index = int;

    buffer(int);

    char *buffers;
    index last_index;
    index now_index;
    int buffer_len;
};

struct massage
{
    int fd;
    std::string file_path;      // 文件路径
    std::string file_name;      // 文件名
    std::string method;         // 请求方法
    std::string m_version;      // HTTP协议版本号，我们仅支持HTTP1.1
    std::string m_host;         // 主机名
    int m_content_length; // HTTP请求的消息总长度
    bool m_linger;        // HTTP请求是否要求保持连接
};

class client
{
public:
    client();
    ~client();

    void init(int, const sockaddr_in &);
    void run();

    // 没写
    void close_link();

private:
    int fd;
    sockaddr_in addr;

public:
    bool to_read();

private:
    class m_read
    {

        buffer read_buffer; // 缓冲区
        massage* mas;

    private:
        enum STATUS
        {
            LINE_OK,  // 读取一整行
            LINE_BAD, // 读取出错
            LINE_OPEN // 没读完整
        };

    public:

    private:
        /*
            解析客户端请求时，主状态机的状态
            CHECK_STATE_REQUESTLINE:当前正在分析请求行
            CHECK_STATE_HEADER:当前正在分析头部字段
            CHECK_STATE_CONTENT:当前正在解析请求体
        */
        enum CHECK_STATE
        {
            CHECK_STATE_REQUESTLINE,
            CHECK_STATE_HEADER,
            CHECK_STATE_CONTENT
        };

        CHECK_STATE check_state;

    public:
        m_read();
        ~m_read();
        void init(massage*);

        // 读数据
        bool read();

        // 返回信息

    public:
        // 处理函数
        bool process();

    private:
        bool process_hang(std::string); // 处理请求行
        bool process_tou(std::string);  // 处理请求头
        bool process_ti(std::string);   // 处理请求体

        // 提出一行
        std::string get_line();
        // 处理/r/n
//        STATUS chu_li_line();

    private:
        enum
        {
            READ_ERROR
        };
    };

public:
    bool to_write();

private:
    class m_write
    {
        buffer write_buffer; // 缓冲区
        massage* mas;
        char* file;          // 文件路径 是path+name
        std::string file_status;   // 文件状态
        char* file_text;     // 文件内容
        int file_size;       // 文件大小

    public:
        m_write();
        ~m_write();

        void init(massage*);

        bool process();

        bool write_to_client();

    private:
        bool start_write();

        bool write_to_buffer(const std::string&);
        bool add_response(const char *format, ...);
        bool add_content(const char *content);
        bool add_content_type();
        bool add_status_line();
        bool add_headers();
        bool add_content_length();
        bool add_linger();
        bool add_blank_line();

        bool open_file(const char*);
        bool close_file();

    private:
        
    };

private:
    client::m_read reader;
    client::m_write writer;

    massage* xin_xi;

public:
    static int epoll_fd;
    static int link_nums;
};

#endif