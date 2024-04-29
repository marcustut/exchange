#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>

#include <cyaml/cyaml.h>

#include "log/log.h"
#include "utils.h"

typedef struct {
  const char* unix_socket_path;
  const char* tcp_host;
  const uint16_t tcp_port;
  const char* user;
  const char* password;
} redis_config_t;

typedef struct {
  redis_config_t* redis;
  const char* log_level;
} config_t;

/*****************************************************************************
 * Following are CYAML schemas to tell libcyaml the expected YAML structure. *
 *****************************************************************************/

/*
 * Schema for redis config
 */
static const cyaml_schema_field_t redis_config_fields_schema[] = {
    CYAML_FIELD_STRING_PTR("unix_socket_path",
                           CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
                           redis_config_t,
                           unix_socket_path,
                           0,
                           CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("tcp_host",
                           CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
                           redis_config_t,
                           tcp_host,
                           0,
                           CYAML_UNLIMITED),
    CYAML_FIELD_UINT("tcp_port", CYAML_FLAG_OPTIONAL, redis_config_t, tcp_port),
    CYAML_FIELD_PTR(STRING,
                    "user",
                    CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
                    redis_config_t,
                    user,
                    {.min = 0, .max = CYAML_UNLIMITED, .missing = "default"}),
    CYAML_FIELD_STRING_PTR("password",
                           CYAML_FLAG_POINTER,
                           redis_config_t,
                           password,
                           0,
                           CYAML_UNLIMITED),
    CYAML_FIELD_END,
};

/*
 * Schema for top level config
 */
static const cyaml_schema_field_t config_fields_schema[] = {
    CYAML_FIELD_MAPPING_PTR("redis",
                            CYAML_FLAG_POINTER,
                            config_t,
                            redis,
                            redis_config_fields_schema),
    CYAML_FIELD_STRING_PTR("log_level",
                           CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                           config_t,
                           log_level,
                           4,
                           CYAML_UNLIMITED),
    CYAML_FIELD_END,
};

/* Top-level schema. The top level value for the config is a mapping.
 *
 * Its fields are defined in config_fields_schema.
 */
static const cyaml_schema_value_t config_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, config_t, config_fields_schema),
};

/*
 * Our CYAML config.
 */
static const cyaml_config_t cyaml_config = {
    .log_fn = cyaml_log,            /* Use the custom logging function. */
    .mem_fn = cyaml_mem,            /* Use the default memory allocator. */
    .log_level = CYAML_LOG_WARNING, /* Logging errors and warnings only. */
};

int parse_log_level(const char* log_level_str) {
  if (strcmp(log_level_str, "info") == 0)
    return LOG_INFO;
  else if (strcmp(log_level_str, "warn") == 0)
    return LOG_WARN;
  else if (strcmp(log_level_str, "error") == 0)
    return LOG_ERROR;
  else if (strcmp(log_level_str, "fatal") == 0)
    return LOG_FATAL;
  else if (strcmp(log_level_str, "debug") == 0)
    return LOG_DEBUG;
  else if (strcmp(log_level_str, "trace") == 0)
    return LOG_TRACE;
  else
    return LOG_INFO;
}

config_t* config_from_path(const char* path) {
  config_t* config;

  cyaml_err_t err = cyaml_load_file(path, &cyaml_config, &config_schema,
                                    (void**)&config, NULL);
  CHECK_ERR(err != CYAML_OK, return NULL, "Failed to parse %s: %s", path,
            cyaml_strerror(err));

  CHECK_ERR(!config->redis->unix_socket_path &&
                (!config->redis->tcp_host || !config->redis->tcp_host),
            return NULL,
            "either `unix_socket_path` or `tcp_host` and `tcp_host` must be "
            "defined in %s",
            path);

  CHECK_ERR((config->redis->tcp_host && !config->redis->tcp_port) ||
                (!config->redis->tcp_host && config->redis->tcp_port),
            return NULL, "`tcp_host` and `tcp_port` must be both defined in %s",
            path);

  return config;
}

void config_free(config_t* config) {
  CHECK_ERR(cyaml_free(&cyaml_config, &config_schema, config, 0) != CYAML_OK,
            NULL, "Failed to free config");
}

#endif /* CONFIG_H */
