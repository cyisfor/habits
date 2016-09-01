$(FOOP)SELECT id,description,frequency,
			 last_performed IS NOT NULL AS has_performed,
			 millinow()-last_performed AS elapsed 
		FROM habits WHERE enabled AND ( ( NOT has_performed ) OR  (frequency / 1.5 < elapsed) )
			 ORDER BY elapsed IS NOT NULL, 
			 			 		frequency IS NOT NULL, 
								elapsed / frequency DESC
