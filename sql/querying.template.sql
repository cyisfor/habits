SELECT id,description,frequency,
			 last_performed IS NOT NULL AS has_performed,
			 millinow()-last_performed AS elapsed 
			 $(ENABLED)
		FROM habits WHERE $(CRITERIA)
			 ORDER BY elapsed IS NOT NULL, 
			 			 		frequency IS NOT NULL, 
								elapsed / frequency DESC
        LIMIT 30
