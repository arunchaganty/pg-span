span 0.1.0
=============

[![PGXN version](https://badge.fury.io/pg/span.svg)](https://badge.fury.io/pg/span)
[![Build Status](https://travis-ci.org/theory/pg-span.png)](https://travis-ci.org/arunchaganty/pg-span)

This library contains a single PostgreSQL extension, a data type to represent text spans in documents called
"span". This repository was heavily adapted from [`pg-span`](https://github.com/theory/pg-span/).

Installation
------------

To build span:

    make
    make install
    make installcheck

If you encounter an error such as:

    "Makefile", line 8: Need an operator

You need to use GNU make, which may well be installed on your system as
`gmake`:

    gmake
    gmake install
    gmake installcheck

If you encounter an error such as:

    make: pg_config: Command not found

Be sure that you have `pg_config` installed and in your path. If you used a
package management system such as RPM to install PostgreSQL, be sure that the
`-devel` package is also installed. If necessary tell the build process where
to find it:

    env PG_CONFIG=/path/to/pg_config make && make installcheck && make install

If you encounter an error such as:

    ERROR:  must be owner of database regression

You need to run the test suite using a super user, such as the default
"postgres" super user:

    make installcheck PGUSER=postgres

Once span is installed, you can add it to a database. If you're running
PostgreSQL 9.1.0 or greater, it's a simple as connecting to a database as a
super user and running:

    CREATE EXTENSION span;

If you've upgraded your cluster to PostgreSQL 9.1 and already had span
installed, you can upgrade it to a properly packaged extension with:

    CREATE EXTENSION span FROM unpackaged;

For versions of PostgreSQL less than 9.1.0, you'll need to run the
installation script:

    psql -d mydb -f /path/to/pgsql/share/contrib/span.sql

If you want to install span and all of its supporting objects into a
specific schema, use the `PGOPTIONS` environment variable to specify the
schema, like so:

    PGOPTIONS=--search_path=extensions psql -d mydb -f span.sql

Dependencies
------------
The `span` data type has no dependencies other than PostgreSQL and PL/pgSQL.

Copyright and License
---------------------

Copyright (c) 2010-2016 The pg-span Maintainers: Arun Chaganty.

This module is free software; you can redistribute it and/or modify it under
the [PostgreSQL License](http://www.opensource.org/licenses/postgresql).

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose, without fee, and without a written agreement is
hereby granted, provided that the above copyright notice and this paragraph
and the following two paragraphs appear in all copies.

In no event shall The pg-span Maintainers be liable to any party for direct,
indirect, special, incidental, or consequential damages, including lost
profits, arising out of the use of this software and its documentation, even
if The pg-span Maintainers have been advised of the possibility of such
damage.

The pg-span Maintainers specifically disclaim any warranties, including, but
not limited to, the implied warranties of merchantability and fitness for a
particular purpose. The software provided hereunder is on an "as is" basis,
and The pg-span Maintainers no obligations to provide maintenance, support,
updates, enhancements, or modifications.
