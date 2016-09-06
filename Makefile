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

apply_template: CFLAGS:=$(CFLAGS) -DDO_MAIN
apply_template: src/apply_template.c obj/myassert.o 
	$(LINK)

obj/%.o: src/%.c
	gcc $(CFLAGS) -c -o $@ $<

obj/%.o: gen/%.c
	gcc $(CFLAGS) -c -o $@ $<

obj/target_array.o: gen/target_array.c
obj/string_array.o: gen/string_array.c

bin obj gen:
	mkdir $@

.PHONY: all
