#ifndef ELEMENT_TYPE
#error "define ELEMENT_TYPE and/or BY_VALUE then #include this"
#endif

// va_list can't be rewinded (because C is retarded)
typedef struct {
	ELEMENT_TYPE* items;
	size_t length;
} ELEMENT_TYPE ## _array;

void va_list_to_ ## ELEMENT_TYPE ## arrayv
(ELEMENT_TYPE ## _array* result, va_list args) {
	size_t space = 0;
	for(;;) {
		if(result->length == space) {
			space += 0x100;
			result->items = realloc(result->items,sizeof(*result->items)*0x100);
		}
#ifdef BY_VALUE
		ELEMENT_TYPE* value = va_arg(args, ELEMENT_TYPE*);
		if(value == NULL) break;
		memcpy(&result->items[result->length++],value,sizeof(ELEMENT_TYPE));
#else
		ELEMENT_TYPE value = va_arg(args,ELEMENT_TYPE);
		if(value == NULL) break;
		result->items[result->length++] = value;
#endif
	}
	result->items = realloc(result->items,
													sizeof(*result->items)*result->length);
}

// this function will ONLY work if your function has exactly 1
// non-variadic parameter.
void va_list_to_array(target_array* result, ...) {
	va_list args;
	va_start(args,result);
	va_list_to_arrayv(result,args);
	va_end(args);
}
