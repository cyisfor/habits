create table if not exists habits (
    id serial primary key,
    importance integer,
    description text unique,
    enabled boolean DEFAULT TRUE,
    howOften interval,
    performed INTEGER -- in units of clock_getres(CLOCK_REALTIME) 
);

create table if not exists history (
    id serial primary key,
    habit integer references habits(id),
    performed timestamptz DEFAULT now());

create index byperformed on history(performed);

create or replace function findHabit(_habit_description text) RETURNS integer as $$
DECLARE
_habit integer;
BEGIN
    LOOP
        SELECT id INTO _habit FROM habits WHERE description = _habit_description;
        IF found THEN
            RETURN _habit;
        END IF;
        BEGIN
            INSERT INTO habits (description) VALUES (_description) RETURNING id INTO _habit;
            RETURN _habit;
        EXCEPTION WHEN unique_violation THEN
            -- try selecting again
        END;
    END LOOP;
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION perform(_habit integer) RETURNS void AS $$
BEGIN
    UPDATE habits SET performed = now() WHERE id = _habit;
    INSERT INTO history (habit) VALUES (_habit);
END;
$$ LANGUAGE 'plpgsql';
