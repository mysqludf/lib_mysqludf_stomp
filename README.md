## lib_mysqludf_stomp

A MySQL UDF library to send STOMP messages to a message broker.
Supports authentication using the stompsend2 function (see details below).

## Instructions for Ubuntu

### Install Dependencies
In order do compile the plugin, you need to install Apache Portable Runtime (APR) and MySQL client development packages.

`sudo apt-get install libapr1 libapr1-dev libmysqlclient-dev`

### Compile & Install
To install the UDF, just run the provided install script which will compile and install the library.
The MySQL root password will be requested for installing.

`sudo ./install.sh`

#### If you get errors of missing libraries, edit the Makefile and make sure the paths are correct for your system.

### Using the functions

The plugin will provide you with 3 new functions you can use in your queries.
All of them take the same first 3 parameters (Hostname, Topic, Message) and the others are headers.
For authentication you must use `stompsend2`.
All parameters are strings :

- stompsend(Hostname, Topic, Message);
- stompsend1(Hostname, Topic, Message, HeaderName, HeaderValue);
- stompsend2(Hostname, Topic, Message, Header1Name, Header1Value, Header2Name, Header2Value);

### Example
To send the message "Hello broker" to the "Welcome" topic on server "127.0.0.1", just use :

`SELECT stompsend("127.0.0.1","Welcome", "Hello broker");`

If everything went well, you should get and "OK" response, else you will get a NULL:

```
+--------------------------------------------------+
| stompsend("127.0.0.1","Welcome", "Hello broker") |
+--------------------------------------------------+
| OK                                               |
+--------------------------------------------------+
1 row in set (0.00 sec)
```

If you need to authenticate, you can use the `stompsend2` function with the "login" and "passcode" as headers.
This function was modified in this fork to allow authentication by sending the headers on the CONNECT frame.

`SELECT stompsend2("127.0.0.1","Welcome", "Hello broker", "login","guest","passcode","mypass");`

#### Beware, if the credentials are invalid you will still receive an "OK" from the UDF, so you should NOT rely on that to verify the message was sent.


## Credits

Copyright 2005 LogicBlaze Inc.

Copyright (C) 2011 Dmitry Demianov aka barlone

this library use part of libstomp code

web of STOMP project: http://stomp.codehaus.org

email: barlone@yandex.ru

Authentication support on stompsend2 added by hugorosario

## Licence

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
