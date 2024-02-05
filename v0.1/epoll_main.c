#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main() {
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sfd > 0);

	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = htons(9006);
	inet_aton("127.0.0.1", &my_addr.sin_addr);

	
	bind(sfd, (const struct sockaddr *) &my_addr, sizeof(my_addr));

	return 0;
}
