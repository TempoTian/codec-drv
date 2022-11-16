#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"
#include <string.h>
#include "board_cfg_parse.h"

#define IN_STR(a, const_b, len) in_str(a, const_b, sizeof(const_b) - 1, len)

static bool is_word(char s)
{
    return ((s >= 'A' && s <= 'Z') || (s >= 'a' && s <= 'z') || (s >= '0' && s <= '9') || s == '_' || s == '-' ||
            s == '.');
}

bool str_same(const char *a, const char *b)
{
    while (*b && *a == *b) {
        a++;
        b++;
    }
    if (*b == 0) {
        if (is_word(*a) == false) {
            return true;
        }
    }
    return false;
}

static const char *in_str(const char *s, const char *org, int org_len, int len)
{
    while (len > org_len) {
        if (*s == *org && memcmp(s, org, org_len) == 0 && is_word(s[org_len]) == false) {
            return s;
        }
        s++;
        len--;
    }
    return NULL;
}

char *get_file_data(char *file_name, int *size)
{
    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL) {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long s = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *data = NULL;
    if (s > 0) {
        *size = (int) s;
        data = (char *) malloc(*size);
        if (data) {
            fread(data, *size, 1, fp);
            printf("Got file size %d\n", *size);
        }
    }
    fclose(fp);
    return data;
}

const char *get_section_data(const char *data, int size, const char *section_name)
{
    const char *s = data;
    while (1) {
        int left = size - (s - data);
        s = IN_STR(s, "board_name:", left);
        if (s == NULL) {
            break;
        }
        s += sizeof("board_name:") - 1;
        while (!is_word(*s)) {
            s++;
        }
        if (str_same(s, section_name)) {
            while (*(s++) != '\n')
                ;
            return s;
        }
    }
    return NULL;
}

int str_len(const char *s)
{
    int len = 0;
    while (is_word(*s)) {
        len++;
        s++;
    }
    return len;
}

void print_cfg_line(board_cfg_line_t *cfg_line)
{
    printf("%.*s: {", str_len(cfg_line->type), cfg_line->type);
    board_cfg_attr_t *attr = cfg_line->attr;
    while (attr) {
        printf("%.*s: %.*s", str_len(attr->attr), attr->attr, str_len(attr->value), attr->value);
        if (attr->next) {
            printf(", ");
        }
        attr = attr->next;
    }
    printf("}\n");
}

void free_cfg_line(board_cfg_line_t *cfg_line)
{
    if (cfg_line) {
        board_cfg_attr_t *attr = cfg_line->attr;
        while (attr) {
            board_cfg_attr_t *nxt = attr->next;
            free(attr);
            attr = nxt;
        }
        free(cfg_line);
    }
}

board_cfg_line_t *parse_section(const char *s, int size, int *consume)
{
    board_cfg_line_t *cfg_line;
    const char *start = s;
    const char *end = s + size;
    const char *name = s;
    bool is_comment = false;
    while (size) {
        if (*name == '#') {
            is_comment = true;
        }
        if (is_comment) {
            if (*name == '\n') {
                is_comment = false;
            }
        } else {
            if (is_word(*name)) {
                break;
            }
        }
        size--;
        name++;
    }
    if (size == 0 || str_same(name, "board_name")) {
        return NULL;
    }
    cfg_line = calloc(1, sizeof(board_cfg_line_t));
    if (cfg_line == NULL) {
        return NULL;
    }
    board_cfg_attr_t *tail = NULL;
    cfg_line->type = name;
    s = name;
    while (*(s++) != '{');
    int word_len = 0;
    const char *attr = NULL;

    while (s < end) {
        if (*s == '}') {
            *consume = (s + 1 - start);
            break;
        }
        if (is_word(*s)) {
            if (word_len == 0) {
                if (attr == NULL) {
                    attr = s;
                } else {
                    board_cfg_attr_t *cfg_attr = calloc(1, sizeof(board_cfg_attr_t));
                    if (cfg_attr) {
                        cfg_attr->attr = attr;
                        cfg_attr->value = s;
                        if (tail == NULL) {
                            cfg_line->attr = cfg_attr;
                        } else {
                            tail->next = cfg_attr;
                        }
                        tail = cfg_attr;
                    }
                    attr = NULL;
                }
            }
            word_len++;
        } else {
            word_len = 0;
        }
        s++;
    }
    return cfg_line;
}
#if 0
int main() {
    int file_size = 0;
    char* file_data = get_file_data("board_cfg.txt", &file_size);
    if (file_data) {
        char* section_name[] = {"ESP_LYRAT_V4_2", "ESP32_S3_KORVO2_V3", "ESP32_S3_BOX"};
        for (int i = 0 ; i < sizeof(section_name)/sizeof(char*); i++) {
            const char* section_data = get_section_data(file_data, file_size, section_name[i]);
            if (section_data) {
                int left = file_size - (section_data - file_data);
                int consume = 0;
                while (1) {
                    board_cfg_line_t* cfg_line = parse_section(section_data, left, &consume);
                    if (cfg_line == NULL) {
                        break;
                    }
                    left = left - consume;
                    section_data += consume;
                    print_cfg_line(cfg_line);
                    free_cfg_line(cfg_line);
                }
            }
        }
        free(file_data);
    }
    return 0;
}
#endif
