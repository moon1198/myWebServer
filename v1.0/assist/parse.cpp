#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>

enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, CONNECT, OPTIONS, TRACE, PATCH};

enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT};

enum HTTP_CODE
{
	NO_REQUEST = 0,
	GET_REQUEST,
	BAD_REQUEST,
	NO_RESOURCE,
	FORBIDDEN_REQUEST,
	FILE_REQUEST,
	INTERNAL_ERROR,
	CLOSED_CONNECTION
};

enum LINE_STATUS {
	LINE_OK = 0,	//完整行
	LINE_BAD,		//错误行
	LINE_OPEN		//不完整行
};

int m_check_idx = 0;	//location of preprocess_line
int m_start_idx = 0;	//location of parsing
int m_read_idx = 0;
enum METHOD m_method;

int m_content_len = 0;
char m_root_path[256];
char m_file[256];

struct stat m_file_stat;
char* m_url;
char* m_host;
char* m_connect;
char* m_version;
char* m_content;
char* m_file_address;


enum CHECK_STATE check_state = CHECK_STATE_REQUESTLINE;


HTTP_CODE handle_request();
HTTP_CODE parse_readbuf(char *);
bool fill_writebuf();
HTTP_CODE parse_requestline(char* line);
LINE_STATUS preprocess_line(char *text);
HTTP_CODE parse_headers(char* line);
HTTP_CODE parse_content(char* text);
HTTP_CODE handle_request();


int main() {
	//FILE *fp = fopen("request.txt", "r");
	struct stat attr;
	int fd = open("request.txt", O_RDONLY);
	assert(fd > 0);
	fstat(fd, &attr);
	char *text = (char *) mmap(NULL, attr.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	printf("%s", text);

	m_read_idx = attr.st_size;


	assert(getcwd(m_root_path, 256) != NULL);
	//Read()
	parse_readbuf(text);
	printf("%s", m_file_address);

	fill_writebuf();

	//mod()
	//push();

	munmap(text, attr.st_size);
	return 0;

}

HTTP_CODE parse_readbuf(char *text) {
	check_state = CHECK_STATE_REQUESTLINE;
	HTTP_CODE ret = NO_REQUEST;
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

bool fill_writebuf() {

	return true;
}

LINE_STATUS preprocess_line(char *text) {
	m_start_idx = m_check_idx;
	for (; m_check_idx < m_read_idx; ++ m_check_idx) {
		if (text[m_check_idx] == '\r') {
			if (text[m_check_idx + 1] == '\n') {
				text[m_check_idx ++] = '\0';
				text[m_check_idx ++] = '\0';
				return LINE_OK;
			} else {
				return LINE_BAD;
			}
		}
	}
	return LINE_OPEN;
}

enum HTTP_CODE parse_requestline(char* line) {
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

enum HTTP_CODE parse_headers(char* line) {
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


enum HTTP_CODE parse_content(char* text) {
	//判断报文内容是否全部收到
	if (m_read_idx >= (m_check_idx + m_content_len)) {
		text[m_content_len] = '\0';
		m_content = text;
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

enum HTTP_CODE handle_request() {
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
