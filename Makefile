all: brotlicompress.so check
.PHONY: all

build: brotlicompress.so
    @:
.PHONY: build

brotlicompress.so:
	gcc -Wall -O3 -fPIC brotlicompress.c -o brotlicompress.so -lbrotlienc -lbrotlidec -lbrotlicommon -shared

check:
	valgrind --leak-check=full --error-exitcode=1 \
	sqlite3 :memory: \
	'.load ./brotlicompress' \
	'.mode csv' \
	'.import testdata/hn.csv hn' \
	'.mode line' \
	'SELECT record as error_record_tag FROM hn WHERE CAST(brotli_decompress(brotli_compress(record, 5)) AS TEXT) != record;'
.PHONY: check

clean:
	rm brotlicompress.so
.PHONY: clean
