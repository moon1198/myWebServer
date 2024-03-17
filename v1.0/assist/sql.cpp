#include <mysql/mysql.h>
#include <assert.h>

int main() {
	
	MYSQL *mysql = NULL;
	mysql = mysql_init(mysql);

	char host[] = "127.0.0.1";
	char user[] = "debian-sys-maint";
	char passward[] = "sOQNiW4vRl75vRU8";
	char db[] = "webdb";
	int port = 0;
	mysql_real_connect(mysql, host, user, passward, db, port, NULL, 0);
	assert(mysql != NULL);
	mysql_query(mysql, "select username, passwd from user" );
	MYSQL_RES *result = mysql_store_result(mysql);
	int rows = mysql_num_rows(result);
	int cols = mysql_num_fields(result);

	MYSQL_ROW row = mysql_fetch_row(result);
	MYSQL_FIELD *field = mysql_fetch_field(result);

	mysql_close(mysql);
			

	return 0;
}
