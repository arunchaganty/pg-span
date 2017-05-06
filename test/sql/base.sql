\set ECHO none
BEGIN;

\i test/pgtap-core.sql
\i sql/span.sql

-- Before 8.4, there was no unnest(), so create one.
CREATE FUNCTION create_unnest(
) RETURNS SETOF BOOLEAN LANGUAGE PLPGSQL AS $$
BEGIN
    IF pg_version_num() < 80400 THEN
	EXECUTE $F$ CREATE FUNCTION unnest(
	    anyarray
	) RETURNS SETOF anyelement LANGUAGE sql AS $_$
	    SELECT $1[i]
	      FROM generate_series(array_lower($1, 1), array_upper($1, 1)) AS i;
	$_$;$F$;
    END IF;
END;
$$;

SELECT * FROM create_unnest();

SELECT plan(69);
--SELECT * FROM no_plan();

SELECT has_type('span');
SELECT is( NULL::span, NULL, 'spans should be NULLable' );

SELECT lives_ok(
    $$ SELECT '$$ || v || $$'::span $$,
    '"' || v || '" is a valid span'
)  FROM unnest(ARRAY[
    'docid:10-20',
    'abcdefghijklmnopqabcdefghijklmnopqabcde:10-20000',
    'ab4.ed:10-20'
]) AS v;

SELECT throws_ok(
    $$ SELECT '$$ || v || $$'::span $$,
    NULL,
    '"' || v || '" is not a valid span'
)  FROM unnest(ARRAY[
   'abc',
   'abc:1',
   'abc:1:2',
   'abc:2-1',
   'abcdefghijklmnopqabcdefghijklmnopqabcdef:10-20000',
   ':2-1'
]) AS v;

-- Test =, <=, and >=.
SELECT collect_tap(ARRAY[
    ok(span_cmp(lv::span, rv::span) = 0, 'span_cmp(' || lv || ', ' || rv || ') should = 0'),
    ok(lv::span = rv::span,  lv || ' should = ' || rv),
    ok(lv::span <= rv::span,  lv || ' should be <= ' || rv),
    ok(lv::span >= rv::span,  lv || ' should be >= ' || rv)
]) FROM (VALUES
    ('abc:1-2',  'abc:1-2'),
    ('abc:10-20',  'abc:10-20')
 ) AS f(lv, rv);

-- Test span <> span
SELECT collect_tap(ARRAY[
    ok(span_cmp(lv::span, rv::span) <> 0, 'span(' || lv || ', ' || rv || ') should <> 0'),
    ok(lv::span <> rv::span, lv || ' should not equal ' || rv)
]) FROM (VALUES
    ('abc:1-2', 'abc:10-20'),
    ('abc:1-2', 'def:1-2'),
    ('abc:1-10', 'abc:5-10')
  ) AS f(lv, rv);

-- Test >, >=, <, and <=.
SELECT collect_tap(ARRAY[
    ok( span_cmp(lv::span, rv::span) > 0, 'span(' || lv || ', ' || rv || ') should > 0'),
    ok( span_cmp(rv::span, lv::span) < 0, 'span(' || rv || ', ' || lv || ') should < 0'),
    ok(lv::span > rv::span, lv || ' should be > ' || rv),
    ok(lv::span >= rv::span, lv || ' should be >= ' || rv),
    ok(rv::span < lv::span, rv || ' should be < ' || lv),
    ok(rv::span <= lv::span, rv || ' should be <= ' || lv)
]) FROM (VALUES
    ('abc:5-10', 'abc:3-7'),
    ('abc:5-10', 'abc:1-20'),
    ('def:1-10', 'abc:3-7')
  ) AS f(lv, rv);

-- Test sort ordering
CREATE TABLE vs (
    id span
);

INSERT INTO vs VALUES ('abc:3-10'), ('abc:2-20'), ('abc:3-15'), ('def:1-10');

SELECT is(max(id), 'def:1-10', 'max(span) should work')
  FROM vs;

SELECT is(min(id), 'abc:2-20', 'min(span) should work')
  FROM vs;

SELECT results_eq(
    $$ SELECT id FROM vs ORDER BY id USING < $$,
    $$ VALUES ('abc:2-20'::span), ('abc:3-10'::span), ('abc:3-15'::span), ('def:1-10'::span) $$,
    'ORDER BY span USING < should work'
);

SELECT results_eq(
    $$ SELECT id FROM vs ORDER BY id USING > $$,
    $$ VALUES ('def:1-10'::span), ('abc:3-15'::span), ('abc:3-10'::span), ('abc:2-20'::span) $$,
    'ORDER BY span USING > should work'
);

-- Test constructors.
SELECT is( text('abc:1-10'::span), 'abc:1-10', 'construct to text' );
SELECT is( span('abc:1-10'), 'abc:1-10'::span, 'construct from text' );

-- Test casting.
SELECT is( span('abc:1-10'::text), 'abc:1-10', 'cast to text' );
SELECT is( text('abc:1-10')::span, 'abc:1-10'::span, 'cast from text' );

-- Test is_span().
SELECT has_function('is_span');
SELECT has_function('is_span', ARRAY['text']);
SELECT function_returns('is_span', 'boolean');

SELECT is(
    is_span(stimulus),
    expected,
    'is_span(' || stimulus || ') should return ' || expected::text
) FROM (VALUES
    ('abc:1-10',                 true),
    ('abc:1',                    false),
    ('abc:10-1',                 false)
) v(stimulus, expected);

-- Test get_span_major
SELECT has_function('get_span_doc');
SELECT has_function('get_span_doc', 'span');
SELECT function_returns('get_span_doc', 'text');
SELECT is(get_span_doc('abc:1-10'::span), 'abc', 'doc check');

-- Test get_span_begin
SELECT has_function('get_span_begin');
SELECT has_function('get_span_begin', 'span');
SELECT function_returns('get_span_begin', 'integer');
SELECT is(get_span_begin('abc:1-10'::span), 1, 'begin check');

-- Test get_span_end
SELECT has_function('get_span_end');
SELECT has_function('get_span_end', 'span');
SELECT function_returns('get_span_end', 'integer');
SELECT is(get_span_end('abc:1-10'::span), 10, 'end check');

SELECT * FROM finish();
ROLLBACK;
