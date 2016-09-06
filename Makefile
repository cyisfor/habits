CFLAGS+=-g -fdiagnostics-color=always -Igen -Isrc
LINK=gcc $(CFLAGS) -Isrc -o $@ $^ $(LDFLAGS)

all: | bin/make
	@./bin/make

bin/make: obj/make.o obj/string_array.o obj/target_array.o obj/myassert.o obj/path.o obj/apply_template.o | bin obj
	$(LINK)

obj/make.o: src/make.c gen/string_array.h gen/target_array.h

define TEMPLATE =
./apply_template $< >$@.temp
mv $@.temp $@
endef

gen/string_array.c: src/array.template.c apply_template | gen
	ELEMENT_TYPE=string $(TEMPLATE)

gen/string_array.h: src/array.template.h apply_template | gen
	HEADER="typedef const char* string;" ELEMENT_TYPE=string $(TEMPLATE)

gen/target_array.c: src/array.template.c apply_template | gen
	ELEMENT_TYPE=target $(TEMPLATE)

gen/target_array.h: src/array.template.h apply_template | gen
	HEADER="#include \"target.h\"" ELEMENT_TYPE=target $(TEMPLATE)

bin/apply_template: CFLAGS:=$(CFLAGS) -DDO_MAIN
bin/apply_template: src/apply_template.c obj/myassert.o 
	$(LINK)

bin/make_specialescapes: data_to_header_string/make_specialescapes.c
	$(LINK)

bin/data_to_header_string: data_to_header_string/main.c obj/data_to_header_string/convert.o
	$(LINK)

obj/data_to_header_string/convert.o: gen/specialescapes.c data_to_header_string/convert.c | obj/data_to_header_string
	gcc $(CFLAGS) -c -o $@ -I data_to_header_string/ data_to_header_string/convert.c

gen/specialescapes.c: bin/make_specialescapes
	./$< > $@.temp
	mv $@.temp $@

obj/%.o: src/%.c
	gcc $(CFLAGS) -c -o $@ $<

obj/%.o: gen/%.c
	gcc $(CFLAGS) -c -o $@ $<

obj/target_array.o: gen/target_array.c
obj/string_array.o: gen/string_array.c

bin obj gen obj/data_to_header_string:
	mkdir $@

.PHONY: all
