SET client_min_messages TO warning;
SET log_min_messages    TO warning;

-- Create a document text span data type.

CREATE TYPE span;

--
-- essential IO
--
CREATE OR REPLACE FUNCTION span_in(cstring)
	RETURNS span
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION span_out(span)
	RETURNS cstring
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

--
--  The type itself.
--

CREATE TYPE span (
	INPUT = span_in,
	OUTPUT = span_out,
-- values of passedbyvalue and alignment are copied from the named type.
  STORAGE = plain,
	INTERNALLENGTH = 48, -- 40 for string + 4 + 4 for offsets.
  ALIGNMENT = double, -- because 48 is multiple of 8.
-- string category, to automatically try string conversion, etc.
	CATEGORY = 'S',
	PREFERRED = false
);

--
--  Typecasting functions.
--

CREATE OR REPLACE FUNCTION span(text)
	RETURNS span
    AS 'span', 'text_to_span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION text(span)
	RETURNS text
    AS 'span', 'span_to_text'
	LANGUAGE C STRICT IMMUTABLE;

--
--  Explicit type casts.
--

CREATE CAST (span AS text)             WITH FUNCTION text(span);
CREATE CAST (text AS span)             WITH FUNCTION span(text);

--
--	Comparison functions and their corresponding operators.
--

CREATE OR REPLACE FUNCTION span_eq(span, span)
	RETURNS bool
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR = (
	leftarg = span,
	rightarg = span,
	negator = <>,
	procedure = span_eq,
	restrict = eqsel,
	commutator = =,
	join = eqjoinsel,
	hashes, merges
);

CREATE OR REPLACE FUNCTION span_ne(span, span)
	RETURNS bool
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR <> (
	leftarg = span,
	rightarg = span,
	negator = =,
	procedure = span_ne,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OR REPLACE FUNCTION span_le(span, span)
	RETURNS bool
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR <= (
	leftarg = span,
	rightarg = span,
	negator = >,
	procedure = span_le
);

CREATE OR REPLACE FUNCTION span_lt(span, span)
	RETURNS bool
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR < (
	leftarg = span,
	rightarg = span,
	negator = >=,
	procedure = span_lt
);

CREATE OR REPLACE FUNCTION span_ge(span, span)
	RETURNS bool
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR >= (
	leftarg = span,
	rightarg = span,
	negator = <,
	procedure = span_ge
);

CREATE OR REPLACE FUNCTION span_gt(span, span)
	RETURNS bool
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR > (
	leftarg = span,
	rightarg = span,
	negator = <=,
	procedure = span_gt
);

--
-- Support functions for indexing.
--

CREATE OR REPLACE FUNCTION span_cmp(span, span)
	RETURNS int4
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION hash_span(span)
	RETURNS int4
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

--
-- The btree indexing operator class.
--

CREATE OPERATOR CLASS span_ops
DEFAULT FOR TYPE SPAN USING btree AS
    OPERATOR    1   <  (span, span),
    OPERATOR    2   <= (span, span),
    OPERATOR    3   =  (span, span),
    OPERATOR    4   >= (span, span),
    OPERATOR    5   >  (span, span),
    FUNCTION    1   span_cmp(span, span);

--
-- The hash indexing operator class.
--

CREATE OPERATOR CLASS span_ops
DEFAULT FOR TYPE span USING hash AS
    OPERATOR    1   =  (span, span),
    FUNCTION    1   hash_span(span);

--
-- Aggregates.
--

CREATE OR REPLACE FUNCTION span_smaller(span, span)
	RETURNS span
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE AGGREGATE min(span)  (
    SFUNC = span_smaller,
    STYPE = span,
    SORTOP = <
);

CREATE OR REPLACE FUNCTION span_larger(span, span)
	RETURNS span
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE AGGREGATE max(span)  (
    SFUNC = span_larger,
    STYPE = span,
    SORTOP = >
);

--
-- Is function.
--

CREATE OR REPLACE FUNCTION is_span(text)
	RETURNS bool
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

--
-- Accessor functions
--
CREATE OR REPLACE FUNCTION get_span_doc(span)
	RETURNS text
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION get_span_begin(span)
	RETURNS int4
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION get_span_end(span)
	RETURNS int4
	AS 'span'
	LANGUAGE C STRICT IMMUTABLE;
