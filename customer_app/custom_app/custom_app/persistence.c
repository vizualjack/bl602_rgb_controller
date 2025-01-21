#include <easyflash.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


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

void clean_saved_value(const char* key) {
    ef_del_env(key);
}

char keys[512] = {0};
int current_pos = 0;
char key[50] = {0};
bool get_env_keys(env_node_obj_t env, void *arg1, void *arg2) {
    memset(key, 0, 50);
    memcpy(key, env->name, env->name_len);
    key[env->name_len] = '\0';
    if(strstr(keys, key) != NULL) return false;
    printf("next size%d\n", current_pos + env->name_len + 1);
    if(current_pos + env->name_len + 1 >= 512) return false;
    puts("Copying name to list...\n");
    memcpy(keys+current_pos, env->name, env->name_len);
    puts("Set current pos to next one...\n");
    current_pos += env->name_len;
    keys[current_pos] = '\n';
    current_pos += 1;
    return false;
}

char* clean_all_saved_values() {
    puts("Before cleaning:\n");
    ef_print_env();
    puts("Getting names...");
    memset(keys, 0, 512);
    ef_custom_iterating(get_env_keys);
    // printf("Keys: %s\n", keys);
    current_pos = 0;
    char* start = keys;
    char* end = strstr(keys, "\n");
    while(start && end) {
        *end = '\0';
        printf("Key: %s\n", start);
        clean_saved_value(start);
        *end = '\n';
        start = end + 1;
        end = NULL;
        if(start >= keys + 512) break;
        end = strstr(start, "\n");
    }
    puts("After cleaning:\n");
    ef_print_env();
    printf("Keys: %s\n", keys);
    return (char*)keys;
}