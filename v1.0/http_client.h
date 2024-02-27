#ifndef __HTTP_CLIENT_H_
#define __HTTP_CLIENT_H_

#include <cstddef>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>

#define	READ_BUF_LEN 1024
#define	WRITE_BUF_LEN 4096

class Http_client
{
public:
	Http_client() : m_peer_addr(NULL) {};
	~Http_client(){};
	void init(int fd, const struct sockaddr_in *peer_addr);
	void new_user(int fd, const struct sockaddr_in *peer_addr);
	static int m_epollfd;
	static int m_user_num;
	int m_fd;

	bool Read();
	bool Write();
	void run ();


private:
	int m_read_idx;
	int m_write_idx;

	const struct sockaddr_in *m_peer_addr;
	char m_readbuf[READ_BUF_LEN];
	char m_writebuf[WRITE_BUF_LEN];

};



#endif
