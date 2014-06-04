# mod_jerk

## About

This is an Apache module designed for dealing with jerks on the Internet. It is
written from scratch specifically for Apache 2.x.

It works using an SQLite database which contains block lists. There are two
lists so far: IP blocks and User Agents. The IP block table contains an IP range
(address beginning to address ending inclusive) and a HTTP status code. If the
connection's IP falls within that range, mod_jerk will return the given HTTP
code, terminating the request. It will optionally log a record as well.

The other table is a list of user agents to block which works the same way.

## Building

This was developed on Ubuntu Linux, FreeBSD, and Mac OSX. It has not been ported
to Windows. To build, you need to the following packages:

  * Apache, APR and APR Util headers
  * CMake

On Ubuntu, you can build as follows:

    bash $ /usr/lib/pbuilder/pbuilder-satisfydepends
    bash $ fakeroot debian/rules clean
    bash $ dpkg-buildpackage
  
On other systems you can build generally as follows:

    bash $ cmake .
    bash $ make
    bash $ make install

## Usage

You can define a filter to be run across the entire web server as follows:

    <IfModule jerk_module>

    JerkDefaultDatabase /etc/apache2/jerk.db

    </IfModule>

Basically, the existence of a default filter database causes the handler to use it.

However, you might want to just run it in specfic locations. The following
defines a named filter and then uses it within a specfic directory:

    <IfModule jerk_module>

    JerkFilterDeclare  GLOBAL
    JerkFilterDatabase GLOBAL /etc/apache2/jerk.db

    <Directory /var/www/mysite/content/filter>
        Options Indexes FollowSymLinks MultiViews Includes
        AllowOverride None
        Order allow,deny
        allow from all

        JerkFilter GLOBAL
    </Directory>

    </IfModule>

Using this method, you can define multiple filter databases and use them in
different places and ways. Any

## Configuration

The following is the schema for a filter database. It includes a single IP
record which permits connections from 127.0.0.1. Usually you would only include
records of things you want to block. But this is there just as an example. This
will actually result in the request being allowed. HTTP code 200 is the only
code a record can have that will allow the request to proceed as normal.

```sql
BEGIN TRANSACTION;

CREATE TABLE ua (
  id integer primary key, 
  name text, 
  http_code int, 
  http_message text);

CREATE TABLE ip (
  id integer primary key,
  start_text text,
  end_text text,
  start int,
  end int,
  http_code int, 
  http_message text );

INSERT INTO "ip" VALUES(1,'127.0.0.1','127.0.0.1', 2130706433, 2130706433, 200, 'Message');

COMMIT;
```

If there were in a file called <tt>/tmp/jerk.sql</tt> and you wanted to create
the database in <tt>/etc/apache2/jerk.db</tt>, you would do the following:

    root $ sqlite3 /etc/apache2/jerk.db < /tmp/jerk.sql

## Additional Information

Redistribution and use in source and binary forms, with or without modification,
are permitted under the terms of the Apache License Version 2.0. Copyright
information is located in the COPYING file. The software license is located in
the LICENSE file.
