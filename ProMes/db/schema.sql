CREATE TABLE IF NOT EXISTS entries (
  id SERIAL PRIMARY KEY,
  entry_date DATE,
  question_key TEXT,
  answer_bool BOOLEAN,
  answer_int INT
);


CREATE TABLE IF NOT EXISTS questions (
id SERIAL PRIMARY KEY,
question_key TEXT NOT NULL UNIQUE,
question_text TEXT NOT NULL,
slot INT NOT NULL,
days TEXT NOT NULL DEFAULT '1111111',
answer_type TEXT NOT NULL DEFAULT 'bool',
active BOOLEAN NOT NULL DEFAULT true
);
