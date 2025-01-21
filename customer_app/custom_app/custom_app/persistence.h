#ifndef _EASY_FLASH_
#define _EASY_FLASH_

extern char* get_saved_value(const char* key);
extern void set_saved_value(const char* key, const char* value);
extern void clean_saved_value(const char* key);
extern char* clean_all_saved_values();

#endif