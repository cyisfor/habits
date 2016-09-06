#include <stdlib.h> // size_t
#include <stdarg.h>

// va_list can't be rewinded (because C is retarded)
typedef struct {
	$(ELEMENT_TYPE)* items;
	size_t length;
	size_t space;
} $(ELEMENT_TYPE)_array;

static void $(ELEMENT_TYPE)_array_push($(ELEMENT_TYPE)_array* result);
// ...then assign the result->items[result->length] item

static void $(ELEMENT_TYPE)_array_done_pushing($(ELEMENT_TYPE)_array* result); 

static void va_list_to$(ELEMENT_TYPE)_arrayv
($(ELEMENT_TYPE)_array* result, va_list args);

// this function will ONLY work if your function has exactly 1
// non-variadic parameter.
static void va_list_to_$(ELEMENT_TYPE)s* result, ...);
