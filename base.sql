create table if not exists habits (
    id INTEGER primary key,
    importance integer NOT NULL,
    description text NOT NULL,
    enabled boolean NOT NULL DEFAULT 1,
    howOften INTEGER NOT NULL, -- in milliseconds
		last_performed INTEGER -- in milliseconds since the epoch
);

create unique index unique_description on habits(description);

create table if not exists history (
    id serial primary key,
    habit integer references habits(id),
    performed INTEGER NOT NULL); -- when it was performed, each time new

create unique index IF NOT EXISTS by_performed on history(performed);

CREATE TRIGGER IF NOT EXISTS add_history AFTER UPDATE ON habits
  BEGIN
		INSERT INTO history (habit,performed) VALUES (NEW.id, NEW.last_performed)
	END;

CREATE TRIGGER IF NOT EXISTS insert_add_history AFTER INSERT ON habits
  BEGIN
		INSERT INTO history (habit,performed) VALUES (NEW.id, NEW.last_performed)
	END;
