create table if not exists habits (
    id INTEGER primary key,
    importance REAL NOT NULL, -- 0 to 1
    description text NOT NULL,
    enabled boolean NOT NULL DEFAULT 1,
    frequency INTEGER NOT NULL, -- in milliseconds
		last_performed INTEGER -- in milliseconds since the epoch
);

create unique index if not exists unique_description on habits(description);

create table if not exists history (
    id serial primary key,
    habit integer references habits(id),
    performed INTEGER NOT NULL); -- when it was performed, each time new

create unique index IF NOT EXISTS by_performed on history(performed);
