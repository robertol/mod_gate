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
