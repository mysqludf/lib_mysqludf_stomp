#!/bin/bash

echo "Compiling the MySQL UDF"
make

if test $? -ne 0; then
	echo "ERROR: You need libmysqlclient and Apache Portable Runtime development software installed "
	echo "to be able to compile this UDF, on Debian/Ubuntu just run:"
	echo "apt-get install libapr1 libapr1-dev libmysqlclient-dev"
	exit 1
else
	echo "MySQL UDF compiled successfully"
fi

echo -e "\nPlease provide your MySQL root password to install the UDF..."

mysql -u root -p mysql < lib_mysqludf_stomp.sql

if test $? -ne 0; then
	echo "ERROR: unable to install the UDF"
	exit 1
else
	echo "MySQL UDF installed successfully"
fi
