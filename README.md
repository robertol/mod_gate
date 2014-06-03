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

## Additional Information

Redistribution and use in source and binary forms, with or without modification,
are permitted under the terms of the Apache License Version 2.0. Copyright
information is located in the COPYING file. The software license is located in
the LICENSE file.
