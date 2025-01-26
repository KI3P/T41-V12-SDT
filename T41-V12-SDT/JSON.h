#ifndef JSON_H
#define JSON_H

#include "SDT.h"
#include <ArduinoJson.h>

extern const char *filename;
extern void loadConfiguration(const char *filename, config_t &config);
extern void saveConfiguration(const char *filename, const config_t &config, bool toFile);

#endif // JSON_H
