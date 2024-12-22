#pragma once

/* Generated with cbindgen:0.27.0 */

/* WARNING: this file is autogenerated by cbindgen. Don't modify this manually. */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


#ifdef __cplusplus
namespace els {
#endif  // __cplusplus

struct String;

typedef char BufferString[80];

enum Material_Tag
#ifdef __cplusplus
  : int32_t
#endif // __cplusplus
 {
  /**
   * Марка
   */
  Steel = 0,
  /**
   * Процентное содержание олова
   */
  Brass = 1,
};
#ifndef __cplusplus
typedef int32_t Material_Tag;
#endif // __cplusplus

struct Material {
  Material_Tag tag;
  union {
    struct {
      int32_t steel;
    };
    struct {
      float brass;
    };
  };
};

struct FactoryPart {
  BufferString name;
  int32_t count;
  int32_t department_no;
  struct Material material;
  float weight;
  float volume;
  struct String *_id;
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Returns the `count` on success, zero on failure
 */
int32_t add_parts(Elasticsearch *handle, struct FactoryPart *parts, int32_t count);

void close_client(Elasticsearch *handle);

Elasticsearch *init_client(void);

/**
 * Retrieves all parts from ElasticSearch.
 *
 * If no parts are retrieved or an error occured, returns null pointer.
 * On success returns a valid pointer allocated by malloc
 */
void retrieve_all(Elasticsearch *handle, struct FactoryPart **parts, int32_t *size);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#ifdef __cplusplus
}  // namespace els
#endif  // __cplusplus
