#include "http_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <iostream>

//定义http响应的一些状态信息
static const char *ok_200_title = "OK";

static const char *error_400_title = "Bad Request";
static const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
static const char *error_403_title = "Forbidden";
static const char *error_403_form = "You do not have permission to get file form this server.\n";
static const char *error_404_title = "Not Found";
static const char *error_404_form = "The requested file was not found on this server.\n";
static const char *error_500_title = "Internal Error";
static const char *error_500_form = "There was an unusual problem serving the request file.\n";


int Http_client::m_epollfd = -1;
int Http_client::m_user_num = 0;

void modfd(int epollfd, int m_fd, int ev, bool one_shot);
void setnonblock(int fd);
void addfd(int epollfd, int fd, bool one_shot);
void delfd(int epollfd, int fd);

void Http_client::init(int fd, const struct sockaddr_in *peer_addr) {
	assert(getcwd(m_root_path, 256) != NULL);
	m_fd = fd;
	m_peer_addr = peer_addr;
	refresh();
}
void Http_client::refresh() {

	m_check_idx = 0;	//location of preprocess_line
	m_start_idx = 0;	//location of parsing
	
	//METHOD m_method ;
	m_url = NULL;
	m_version = NULL;
	m_host = NULL;
	m_content_len = 0;
	m_connect = NULL;
	m_linger = false;
	m_content = NULL;
	m_file_address = NULL;

	memset(m_file, 0, 256);
	//struct stat m_file_stat;

	memset(m_readbuf, 0, READ_BUF_LEN);
	memset(m_writebuf, 0, WRITE_BUF_LEN);
	m_read_idx = 0;
	m_write_idx = 0;
	m_iov_count = 0;
	bytes_to_write = 0;
	bytes_have_write = 0;
}

void Http_client::new_user(int fd, const struct sockaddr_in *peer_addr) {
	init(fd, peer_addr);
	addfd(m_epollfd, fd, true);
}

void Http_client::run() {
	//strncpy(m_writebuf, m_readbuf, m_read_idx);
	Http_client::HTTP_CODE parse_ret = parse_readbuf();
	if (parse_ret == NO_REQUEST) {
		modfd(m_epollfd, m_fd, EPOLLIN, true);
		return ;
	}

	if (!fill_writebuf(parse_ret)) {
		//writebuf full out
		close_conn();
		return ;
	}

	modfd(m_epollfd, m_fd, EPOLLOUT, true);
}


bool Http_client::Read() {
	while(1) {
		int len = read(m_fd, m_readbuf + m_read_idx, READ_BUF_LEN - m_read_idx);
		if (len == -1) {
			//all have been read
			if (errno & (EAGAIN | EWOULDBLOCK)) {
				break;
			}
			//error disappear
			return false;
			break;
			//disconnect
		} else if(len == 0){
			//break;
			return false;
		}
		m_read_idx += len;
	}
	//if (m_read_idx == 0) {
	//	return false;
	//}

	m_readbuf[m_read_idx] = '\0';

	return true;
}

bool Http_client::Write() {

	if (bytes_to_write == 0) {
		return false;
	}

	while (1) {
		int len = writev(m_fd, write_iov, m_iov_count);
		if (len < 0) {
			//输出再次阻塞
			if (errno & EAGAIN || errno & EWOULDBLOCK) {
				modfd(m_epollfd, m_fd, EPOLLOUT, true);
				return true;
			}
			//how to handle after return false
			//close connection or not
			return false;
		}

		if (static_cast<size_t>(len) < write_iov[0].iov_len) {
			write_iov[0].iov_len -= len;
			write_iov[0].iov_base = (char* ) write_iov[0].iov_base + len;
		} else if (static_cast<size_t>(len) >= write_iov[0].iov_len) {
			write_iov[1].iov_len -= len - write_iov[0].iov_len;
			write_iov[1].iov_base = (char* ) write_iov[1].iov_base + (len - write_iov[0].iov_len);
			write_iov[0].iov_len = 0;
		}

		bytes_have_write += len;
		if (bytes_have_write >= bytes_to_write) {
			break;
			//return true;
		}
	}

	unmap();

	if (m_linger) {
		//连续echo
		modfd(m_epollfd, m_fd, EPOLLIN, true);
		refresh();
	} else {
		return false;
	}

	//printf("%s", m_file_address);
	return true;
}

Http_client::HTTP_CODE Http_client::parse_readbuf() {
	char* text = m_readbuf;
	check_state = CHECK_STATE_REQUESTLINE;
	Http_client::HTTP_CODE ret = NO_REQUEST;
	while (preprocess_line(text) == LINE_OK) {
		switch (check_state) {
			case CHECK_STATE_REQUESTLINE :
				ret = parse_requestline(text + m_start_idx);
				if (ret == BAD_REQUEST) {
					return BAD_REQUEST;
				}
				break;
			case CHECK_STATE_HEADER :
				ret = parse_headers(text + m_start_idx);
				if (ret == BAD_REQUEST) {
					return BAD_REQUEST;
				} else if (ret == GET_REQUEST) {
					return handle_request();
					//return GET_REQUEST;
				}
				break;
			case CHECK_STATE_CONTENT :

				ret = parse_content(text + m_start_idx);
				if (ret == BAD_REQUEST) {
					return BAD_REQUEST;
				} else if (ret == GET_REQUEST) {
					return handle_request();
					//return GET_REQUEST;
				}
				break;
			default :
				return INTERNAL_ERROR;
				//error
				break;
		}
	}
	return NO_REQUEST;
}

bool Http_client::fill_writebuf(Http_client::HTTP_CODE request_state) {

	switch (request_state) {
	case BAD_REQUEST:
		//error_400
		add_status_line(400, error_400_title);
		//input content-length
		add_headers(strlen(error_400_form));
		add_content(error_400_form);
		break;
	case NO_RESOURCE:
		//error_404
		add_status_line(404, error_404_title);
		add_headers(strlen(error_404_form));
		add_content(error_404_form);
		break;
	case FORBIDDEN_REQUEST:
		//error_403
		add_status_line(403, error_403_title);
		add_headers(strlen(error_403_form));
		add_content(error_403_form);
		break;
	case INTERNAL_ERROR:
		//error_500
		add_status_line(500, error_500_title);
		add_headers(strlen(error_500_form));
		add_content(error_500_form);
		break;
	case FILE_REQUEST:
		//ok_200
		add_status_line(200, ok_200_title);
		break;
	default:
		//will be not reach here
		break;
	}
	if (m_file_address) {
		//have file should to be sent
		if (m_file_stat.st_size != 0) {
			add_headers(m_file_stat.st_size);
			write_iov[0].iov_base = m_writebuf;
			write_iov[0].iov_len = m_write_idx;
			write_iov[1].iov_base = m_file_address;
			write_iov[1].iov_len = m_file_stat.st_size;
			bytes_to_write = write_iov[0].iov_len + write_iov[1].iov_len;
			m_iov_count = 2;
			return true;
		} else {
			const char* empty_file = "<html><body></body></html>";
			add_headers(strlen(empty_file));
			add_content(empty_file);
		}
	}

	write_iov[0].iov_base = m_writebuf;
	write_iov[0].iov_len = m_write_idx;
	bytes_to_write = write_iov[0].iov_len;
	m_iov_count = 1;

	return true;
}

Http_client::LINE_STATUS Http_client::preprocess_line(char *line) {
	m_start_idx = m_check_idx;
	for (; m_check_idx < m_read_idx; ++ m_check_idx) {
		if (line[m_check_idx] == '\r') {
			if (line[m_check_idx + 1] == '\n') {
				line[m_check_idx ++] = '\0';
				line[m_check_idx ++] = '\0';
				return LINE_OK;
			} else {
				return LINE_BAD;
			}
		}
	}
	return LINE_OPEN;
}

Http_client::HTTP_CODE Http_client::parse_requestline(char* line) {
	char* tmp = NULL;
	char* url;
	//确定“ ” 或者“\t“位置
	tmp = strpbrk(line, " \t");
	if (tmp == NULL) {
		assert(0);
	}

	*tmp++ = '\0';
	if (strcasecmp(line, "GET") == 0) {
		m_method = GET;
	} else if (strcasecmp(line, "POST") == 0) {
		m_method = POST;
	}else {
		return BAD_REQUEST;
	}

	tmp += strspn(tmp, " \t");
	if (strncasecmp(tmp, "http://", 7) == 0) {
		tmp += 7;
	} else if (strncasecmp(tmp, "https://", 8) == 0) {
		tmp += 8;
	}
	url = strpbrk(tmp, "/");
	if (url == NULL) {
		return BAD_REQUEST;
	}
	tmp = strpbrk(url, " \t");
	if (tmp == NULL) {
		return BAD_REQUEST;
	}
	*tmp++ = '\0';

	//strcpy(m_url, url);
	m_url = url;
	tmp += strspn(tmp, " \t");
	if (strcasecmp(tmp, "HTTP/1.1") == 0) {
		//strcpy(m_version, tmp);
		m_version = tmp;
	} else {
		return BAD_REQUEST;
	}


	check_state = CHECK_STATE_HEADER;
	return NO_REQUEST;
}

Http_client::HTTP_CODE Http_client::parse_headers(char* line) {
	//读到空行，判断是否有content
	if (strlen(line) == 0) {
		if (m_content_len == 0) {
			return GET_REQUEST;
		} else {
			check_state = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
	}

	char* tmp = NULL;
	tmp = strpbrk(line, " \t");
	if (tmp == NULL) {
		assert(0);
	}
	*tmp ++ = '\0';
	tmp += strspn(tmp, " \t");
	if (strcasecmp(line, "Host:") == 0) {
		//strcpy(m_host, tmp);
		m_host = tmp;
	} else if (strcasecmp(line, "Connection:") == 0) {
		//strcpy(m_connect, tmp);
		m_connect = tmp;
		if (strcasecmp(m_connect, "keep-alive") == 0) {
			m_linger = true;
		}

	} else if (strcasecmp(line, "Content-Length:") == 0) {
		m_content_len = atoi(tmp);
	} else {
		//跳过不需要的属性
		return NO_REQUEST;
	}

	return NO_REQUEST;
}


Http_client::HTTP_CODE Http_client::parse_content(char* text) {
	//判断报文内容是否全部收到
	if (m_read_idx >= (m_check_idx + m_content_len)) {
		text[m_content_len] = '\0';
		m_content = text;
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

Http_client::HTTP_CODE Http_client::handle_request() {
	strcpy(m_file, m_root_path);
	strcat(m_file, "/root");
	if (strlen(m_url) == 1 && m_url[0] == '/') {
		strcat(m_file, "/judge.html");
		//strcat(m_file, "/index.html");
	} else {
		switch (*(m_url + 1)) {
		case '0':
			strcat(m_file, "/register.html");
			break;
		case '1':
			strcat(m_file, "/log.html");
			break;
		case '5':
			strcat(m_file, "/picture.html");
			break;
		case '6':
			strcat(m_file, "/video.html");
			break;
		case '7':
			strcat(m_file, "/fans.html");
			break;
		default:
			strcat(m_file, m_url + 1);
			break;
		}
	}

	//exist or not
	if (stat(m_file, &m_file_stat) == -1) {
		return NO_RESOURCE;
	}
	//have authority or not
	if (!(m_file_stat.st_mode & S_IROTH)) {
		return FORBIDDEN_REQUEST;
	}
	//is directory whether or not
	if (S_ISDIR(m_file_stat.st_mode)) {
		return BAD_REQUEST;
	}
	int fd = open(m_file, O_RDONLY);
	assert(fd > 0);

	m_file_address = (char *) mmap(NULL, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	return FILE_REQUEST;

}

bool Http_client::add_status_line(int http_code, const char* msg) {
	if (add_response("HTTP/1.1 %d %s\r\n", http_code, msg) == -1) {
		return false;
	}
	return true;
}

bool Http_client::add_headers(int length) {
	return add_content_length(length) && add_content_type() && add_linger() &&  add_blank_line();
}

bool Http_client::add_content_length(int length) {
	if (add_response("Content-Length: %d\r\n", length) == -1) {
		return false;
	}
	return true;
}

bool Http_client::add_content_type() {
	if (add_response("Content-Type: %s\r\n", "text/html") == -1) {
		return false;
	}
	return true;
}

bool Http_client::add_linger() {
	if (add_response("Connection: %s\r\n", m_connect) == -1) {
		return false;
	}
	return true;
}

bool Http_client::add_blank_line() {
	if (add_response("\r\n") == -1) {
		return false;
	}
	return true;
}


bool Http_client::add_content(const char* msg) {
	if (add_response("%s", msg) == -1) {
		return false;
	}
	return true;
}


int Http_client::add_response(const char* format, ...) {
	//the buf is full
	if (m_write_idx >= WRITE_BUF_LEN) {
		return false;
	}
	va_list arg_list;
	va_start(arg_list, format);
	int len = vsnprintf(m_writebuf + m_write_idx, WRITE_BUF_LEN - m_write_idx - 1, format, arg_list);
	if (len >= WRITE_BUF_LEN - m_write_idx - 1) {
		va_end(arg_list);
		return -1;
	}
	m_write_idx += len;
	va_end(arg_list);

	return len;
}

void Http_client::unmap() {
	if (m_file_address) {
		munmap(m_file_address, m_file_stat.st_size);
		m_file_address = 0;
	}
}

void Http_client::close_conn() {
	delfd(m_epollfd, m_fd);
	close(m_fd);
}
