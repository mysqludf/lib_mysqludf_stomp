MYSQL_DEV_DIR=/usr/include/mysql
APR_DEV_DIR=/usr/include/apr-1.0/
MYSQL_LIB_DIR=/usr/lib/mysql/plugin
APR_LIB_DIR=/usr/lib/i386-linux-gnu

install:
	gcc -Wall -O2 -I$(APR_DEV_DIR) -I$(MYSQL_DEV_DIR) lib_mysqludf_stomp.c  /$(APR_LIB_DIR)/libapr-1.so -shared -o $(MYSQL_LIB_DIR)/lib_mysqludf_stomp.so -fPIC

