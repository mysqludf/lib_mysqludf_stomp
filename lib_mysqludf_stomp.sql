/*
	lib_mysqludf_stomp - a library to send STOMP messages
	Copyright 2005 LogicBlaze Inc.
	Copyright (C) 2011 Dmitry Demianov aka barlone

	this library use part of libstomp code 

	web of STOMP project: http://stomp.codehaus.org/
	email: barlone@yandex.ru

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
*/

DROP FUNCTION IF EXISTS stompsend;
DROP FUNCTION IF EXISTS stompsend1;
DROP FUNCTION IF EXISTS stompsend2;

CREATE FUNCTION stompsend RETURNS STRING SONAME "lib_mysqludf_stomp.so";
CREATE FUNCTION stompsend1 RETURNS STRING SONAME "lib_mysqludf_stomp.so";
CREATE FUNCTION stompsend2 RETURNS STRING SONAME "lib_mysqludf_stomp.so";
