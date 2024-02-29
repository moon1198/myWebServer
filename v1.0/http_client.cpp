#include "http_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <iostream>

int Http_client::m_epollfd = -1;
int Http_client::m_user_num = 0;

void modfd(int epollfd, int m_fd, int ev, bool one_shot);
void setnonblock(int fd);
void addfd(int epollfd, int fd, bool one_shot);

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
	m_content = NULL;
	m_file_address = NULL;

	memset(m_file, 0, 256);
	//struct stat m_file_stat;

	memset(m_readbuf, 0, READ_BUF_LEN);
	memset(m_writebuf, 0, WRITE_BUF_LEN);
	m_read_idx = 0;
	m_write_idx = 0;
}

void Http_client::new_user(int fd, const struct sockaddr_in *peer_addr) {
	init(fd, peer_addr);
	addfd(m_epollfd, fd, true);
}

void Http_client::run() {
	//strncpy(m_writebuf, m_readbuf, m_read_idx);
	parse_readbuf();
	fill_writebuf();

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
			return false;
		}
		m_read_idx += len;
	}

	m_readbuf[m_read_idx] = '\0';

	return true;
}

bool Http_client::Write() {
	//while(1) {
	//	int len = write(m_fd, m_writebuf + m_write_idx, WRITE_BUF_LEN);
	//	if (len == -1) {
	//		//all have been write
	//		if (errno & (EAGAIN | EWOULDBLOCK)) {
	//			break;
	//		}
	//		//error disappear
	//		return false;
	//		break;
	//	} else {
	//		m_write_idx += len;
	//	}
	//}

	////连续echo
	//modfd(m_epollfd, m_fd, EPOLLIN, true);
	//refresh();

	printf("%s", m_file_address);
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
					handle_request();
					return GET_REQUEST;
				}
				break;
			case CHECK_STATE_CONTENT :

				ret = parse_content(text + m_start_idx);
				if (ret == BAD_REQUEST) {
					return BAD_REQUEST;
				} else if (ret == GET_REQUEST) {
					handle_request();
					return GET_REQUEST;
				}
				break;
			default :
				//error
				break;
		}
	}
	return NO_REQUEST;
}

bool Http_client::fill_writebuf() {

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
