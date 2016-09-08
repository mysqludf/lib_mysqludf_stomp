# lib_mysqludf_stomp
## a library to send STOMP messages

Copyright 2005 LogicBlaze Inc.

Copyright (C) 2011 Dmitry Demianov aka barlone

this library use part of libstomp code

web of STOMP project: http://stomp.codehaus.org

email: barlone@yandex.ru


## Instructions for Ubuntu

### Install Dependencies
In order do compile the plugin, you need to install Apache Portable Runtime (APR) and MySQL client development packages.

`sudo apt-get install libapr1 libapr1-dev libmysqlclient-dev`

### Compile & Install
To install the UDF, just run the provided install script which will compile and install the library.
The MySQL root password will be requested for installing.

`sudo ./install.sh`

#### If you get errors of missing libraries, edit the Makefile and make sure the paths are correct for your system.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied.
See the License for the specific language governing permissions and
limitations under the License.
