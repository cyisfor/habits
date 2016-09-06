#ifndef ELEMENT_TYPE
#error "define ELEMENT_TYPE and/or BY_VALUE then #include this"
#endif

// va_list can't be rewinded (because C is retarded)
typedef struct {
	ELEMENT_TYPE* items;
	size_t length;
} ELEMENT_TYPE ## _array;

static void ELEMENT_TYPE ## _array_push(ELEMENT_TYPE ## _array* result) {
	++result->length; // we set this item previously
	if(result->length == result->space) {
		result->space += 0x100;
		result->items = realloc(result->items,
														sizeof(*result->items)*result->space);
	}
}

static void ELEMENT_TYPE ## _array_done_pushing(ELEMENT_TYPE ## _array* result) {
	result->space = result->length;
	result->items = realloc(result->items,
													sizeof(*result->items)*result->space);
}


static void va_list_to_ ## ELEMENT_TYPE ## _arrayv
(ELEMENT_TYPE ## _array* result, va_list args) {
	size_t space = 0;
	for(;;) {
		ELEMENT_TYPE ## _array_push(result);
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
	ELEMENT_TYPE ## _array_done_pushing(result);
}

// this function will ONLY work if your function has exactly 1
// non-variadic parameter.
static void va_list_to_ ## ELEMENT_TYPE ## s(ELEMENT_TYPE ## _array* result, ...) {
	va_list args;
	va_start(args,result);
	va_list_to_ ## ELEMENT_TYPE ## _arrayv(result,args);
	va_end(args);
}
