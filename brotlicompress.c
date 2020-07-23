#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <stdio.h>
#include <brotli/encode.h>
#include <brotli/decode.h>

static void brotliCompressSqliteFunction(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
)
{
  size_t blob_size = sqlite3_value_bytes(argv[0]);
  const uint8_t *blob_ptr = sqlite3_value_blob(argv[0]);
  int brotli_quality = sqlite3_value_int(argv[1]);
  if (!(brotli_quality >= BROTLI_MIN_QUALITY && brotli_quality <= BROTLI_MAX_QUALITY)) {
    sqlite3_result_error(context, "brotli: wrong quality value", -1);
    return;
  }
  size_t output_blob_size = BrotliEncoderMaxCompressedSize(blob_size);
  uint8_t *output_blob_ptr = sqlite3_malloc(output_blob_size);
  if (output_blob_ptr == NULL) {
    sqlite3_result_error(context, "brotli: error allocating memory", -1);
  }

  BROTLI_BOOL rc = BrotliEncoderCompress(
    brotli_quality,
    BROTLI_DEFAULT_WINDOW,
    BROTLI_MODE_TEXT,
    blob_size,
    blob_ptr,
    &output_blob_size,
    output_blob_ptr
  );

  if (rc == BROTLI_TRUE) {
    sqlite3_result_blob(context, output_blob_ptr, output_blob_size, sqlite3_free);
  } else {
    sqlite3_free(output_blob_ptr);
    sqlite3_result_error(context, "brotli: error compressing data", -1);
  }
}

static int brotliDecoderDecompressWithRealloc(
  size_t encoded_size,
  const uint8_t *encoded_buffer,
  size_t *decoded_size,
  uint8_t **decoded_buffer
) {
  BrotliDecoderState* bds;
  size_t total_out = 0;
  size_t buffer_size = 0;
  uint8_t *buffer = NULL;
  uint8_t *next_out = NULL;
  size_t avail_out = 0;
  bds = BrotliDecoderCreateInstance(NULL, NULL, NULL);
  if (bds == NULL) {
    return BROTLI_DECODER_RESULT_ERROR;
  }

  BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
  while (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
    size_t new_buffer_size = buffer_size * 2 + 1024;
    uint8_t *new_buffer = sqlite3_realloc(buffer, new_buffer_size);
    if (new_buffer == NULL) {
      BrotliDecoderDestroyInstance(bds);
      sqlite3_free(buffer);
      return BROTLI_DECODER_RESULT_ERROR;
    }
    ssize_t shift = next_out - buffer;
    buffer = new_buffer;
    next_out = new_buffer + shift;
    avail_out += new_buffer_size - buffer_size;
    buffer_size = new_buffer_size;
    result = BrotliDecoderDecompressStream(
      bds,
      &encoded_size,
      &encoded_buffer,
      &avail_out,
      &next_out,
      &total_out);
  }

  BrotliDecoderDestroyInstance(bds);
  if (result != BROTLI_DECODER_RESULT_SUCCESS) {
     sqlite3_free(buffer);
     return BROTLI_DECODER_RESULT_ERROR;
  }
  *decoded_size = total_out;
  *decoded_buffer = buffer;
  return BROTLI_DECODER_RESULT_SUCCESS;
}

static void brotliDecompressSqliteFunction(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const size_t blob_size = sqlite3_value_bytes(argv[0]);
  const uint8_t *blob_ptr = sqlite3_value_blob(argv[0]);
  size_t output_blob_size;
  uint8_t *output_blob_ptr;
  BrotliDecoderResult rc = brotliDecoderDecompressWithRealloc(
    blob_size,
    blob_ptr,
    &output_blob_size,
    &output_blob_ptr
  );

  if ( rc==BROTLI_DECODER_RESULT_SUCCESS ){
    sqlite3_result_blob(context, output_blob_ptr, output_blob_size, sqlite3_free);
  } else {
    sqlite3_result_error(context, "brotli: error decompressing data", -1);
  }
}


#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_brotlicompress_init(
  sqlite3 *db,
  char **_,
  const sqlite3_api_routines *pApi
){
  SQLITE_EXTENSION_INIT2(pApi);
  int rc = sqlite3_create_function(
    db,
    "brotli_compress",
    2,
    SQLITE_UTF8,
    0,
    brotliCompressSqliteFunction,
    0,
    0
  );
  if ( rc!=SQLITE_OK ) {
    return rc;
  }

  rc = sqlite3_create_function(
    db,
    "brotli_decompress",
    1,
    SQLITE_UTF8 | SQLITE_DETERMINISTIC,
    0,
    brotliDecompressSqliteFunction,
    0,
    0
  );
  if ( rc!=SQLITE_OK ) {
    return rc;
  }
 
  return SQLITE_OK;
}

