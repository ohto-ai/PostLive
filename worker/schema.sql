-- PostLive auth database (apply with:
--   npx wrangler d1 execute postlive --remote --file=schema.sql)

CREATE TABLE IF NOT EXISTS users (
    account             TEXT PRIMARY KEY,
    password_salt       TEXT    NOT NULL, -- hex, 16 random bytes per user
    password_hash       TEXT    NOT NULL, -- PBKDF2-SHA256 hex over the client-sent MD5 string
    password_iterations INTEGER NOT NULL, -- PBKDF2 iterations used for password_hash
    gender              TEXT,
    avatar              TEXT,
    created_at          INTEGER NOT NULL, -- unix seconds
    updated_at          INTEGER NOT NULL
);
