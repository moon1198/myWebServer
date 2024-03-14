#ifndef __HTTP_CLIENT_H_
#define __HTTP_CLIENT_H_

#include "log/log.h"
#include <cstddef>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#define	READ_BUF_LEN 1024
#define	WRITE_BUF_LEN 4096


class Http_client
{
public:
	Http_client() : m_peer_addr(NULL) {};
	~Http_client(){};
	void init(int fd, const struct sockaddr_in *peer_addr, int close_log);
	void new_user(int fd, const struct sockaddr_in *peer_addr, int close_log);
	void close_conn();

	enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, CONNECT, OPTIONS, TRACE, PATCH};
	
	enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT};
	
	enum HTTP_CODE
	{
		NO_REQUEST = 0,			//请求不完整，等待新的补充
		GET_REQUEST,			//
		BAD_REQUEST,

		//指示了响应报文类型，
		NO_RESOURCE,
		FORBIDDEN_REQUEST,		//访问资源无读取权限
		FILE_REQUEST,
		INTERNAL_ERROR,
		CLOSED_CONNECTION
	};
	
	enum LINE_STATUS 
	{
		LINE_OK = 0,	//完整行
		LINE_BAD,		//错误行
		LINE_OPEN		//不完整行
	};

	static int m_epollfd;
	static int m_user_num;
	int m_fd;
	int m_close_log;


	const struct sockaddr_in* get_addr();
	bool Read();
	bool Write();
	void run ();


private:
	int m_read_idx;
	int m_write_idx;


	//parse
	int m_check_idx;	//location of preprocess_line
	int m_start_idx;	//location of parsing
						//
	//parse result
	METHOD m_method;
	int m_content_len;
	char* m_url;
	char* m_host;
	char* m_connect;
	bool m_linger;
	char* m_version;
	char* m_content;

	char* m_file_address;
	char m_root_path[256];
	char m_file[256];
	struct stat m_file_stat;


	CHECK_STATE check_state;

	const struct sockaddr_in *m_peer_addr;
	char m_readbuf[READ_BUF_LEN];
	char m_writebuf[WRITE_BUF_LEN];
	int m_iov_count;
	struct iovec write_iov[2];
	int bytes_to_write;
	int bytes_have_write;

private:
	void refresh();

	HTTP_CODE parse_readbuf();
	LINE_STATUS preprocess_line(char *text);
	HTTP_CODE parse_requestline(char* line);
	HTTP_CODE parse_headers(char* line);
	HTTP_CODE parse_content(char* text);

	HTTP_CODE handle_request();

	bool fill_writebuf(HTTP_CODE);
	bool add_status_line(int http_code, const char* msg);
	bool add_headers(int length);
	bool add_content_length(int);
	bool add_content_type();
	bool add_linger();
	bool add_blank_line();
	bool add_content(const char* msg);
	int add_response(const char* format, ...);

	void unmap();

};



#endif
