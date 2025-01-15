#include <easyflash.h>
#include <stdlib.h>


char* get_saved_value(const char* key) {
    struct env_node_obj info_obj;
    if (!ef_get_env_obj(key, &info_obj)) return NULL;
    if(info_obj.value_len <= 0) return NULL;
    char* pass = malloc(info_obj.value_len);
    ef_get_env_blob(key, pass, info_obj.value_len, NULL);
    return pass;
}

int get_value_length(const char* value) {
    int length = 0;
    while(value[length] != '\0') {
        length++;
    }
    return length + 1;
}

// The end of value MUST be a null termination sign '\0'
void set_saved_value(const char* key, const char* value) {
    ef_set_env_blob(key, value, get_value_length(value));
}