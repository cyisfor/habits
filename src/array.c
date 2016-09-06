#ifndef ELEMENT_TYPE
#error "define ELEMENT_TYPE and/or BY_VALUE then #include this"
#endif

#include <stdlib.h> // size_t

#define E(n) ELEMENT_TYPE ## n

// va_list can't be rewinded (because C is retarded)
typedef struct {
	ELEMENT_TYPE* items;
	size_t length;
	size_t space;
} E(_array);

static void E(_array_push)(E(_array)* result) {
	++result->length; // we set this item previously
	if(result->length == result->space) {
		result->space += 0x100;
		result->items = realloc(result->items,
														sizeof(*result->items)*result->space);
	}
}

static void E(_array_done_pushing)(E(_array)* result) {
	result->space = result->length;
	result->items = realloc(result->items,
													sizeof(*result->items)*result->space);
}


#define VLTO(s) va_list_to ## ELEMENT_TYPE ## s

static void VLTO(_arrayv)
(E(_array)* result, va_list args) {
	size_t space = 0;
	for(;;) {
		E(_array_push)(result);
#ifdef BY_VALUE
		ELEMENT_TYPE* value = va_arg(args, ELEMENT_TYPE*);
		if(value == NULL) break;
		memcpy(&result->items[result->length],value,sizeof(ELEMENT_TYPE));
#else
		ELEMENT_TYPE value = va_arg(args,ELEMENT_TYPE);
		if(value == NULL) break;
		result->items[result->length] = value;
#endif
	}
	E(_array_done_pushing)(result);
}

// this function will ONLY work if your function has exactly 1
// non-variadic parameter.
static void VLTO(s)(E(_array)* result, ...) {
	va_list args;
	va_start(args,result);
	VLTO(_arrayv)(result,args);
	va_end(args);
}
