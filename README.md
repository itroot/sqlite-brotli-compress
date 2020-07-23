# Why?

`sqlite-brotli-compress` extends sqlite3 with 2 functions:

```
brotli_compress(value, compression_level)
brotli_decompress(value)
```

It is convinient to use compression to store some raw data.

# How to use

```buildoutcfg
make build

sqlite3 :memory: \
'.load ./brotlicompress' \
'CREATE TABLE compressed_data (id INTEGER PRIMARY KEY, payload BLOB NOT NULL);' \
'CREATE VIEW data AS SELECT id, brotli_decompress(payload) AS payload FROM compressed_data;' \
'INSERT INTO compressed_data (payload) VALUES (brotli_compress("some_string_that_is_easy_to_compressssssssssss", 8));' \
'SELECT *, length(payload) FROM data;' \
'SELECT id, length(payload) FROM compressed_data;'

```

