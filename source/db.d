public struct Habit {
  long id;
  string description;
  long frequency;
  long elapsed;
  bool has_performed;
  double how_often;
  double last_performed;
  bool dirty;
};

class Database {
  d2sqlite3.Database db;
  struct {
	Statement pending;
  } q;
  
  this() {
	db = open();
	q.pending = db.prepare("""
SELECT
id,
description,
frequency,
now()-performed AS elapsed,
performed IS NOT NULL as has_performed,
performed
FROM habits WHERE enabled = TRUE
ORDER BY elapsed / frequency
DESC NULLS LAST
""")
	  }  
  public InputRange!Habit pending() {
	// why am I doing this, GTK is hard in D!
	return Map!(row => Habit(row.peek(0),
							 row.peek(1),
							 row.peek(2),
							 row.peek(3),
							 row.peek(4),
							 row.peek(5),
							 row.peek(6)))(q.pending.execute());
  }
  public Habit find
		
  


  
