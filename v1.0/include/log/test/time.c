#include <time.h>
#include <stdio.h>

int main() {
	time_t tm = time(NULL);
	char* ch = ctime(&tm);
	printf("ctime ch : %s\n", ch);

	struct tm* tm_bt = localtime(&tm);

	return 0;
}
