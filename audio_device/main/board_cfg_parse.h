typedef struct _board_cfg_attr {
    const char             *attr;
    const char             *value;
    struct _board_cfg_attr *next;
} board_cfg_attr_t;

typedef struct {
    const char       *type;
    board_cfg_attr_t *attr;
} board_cfg_line_t;

const char *get_section_data(const char *data, int size, const char *section_name);

int str_len(const char *s);

bool str_same(const char *a, const char *b);

board_cfg_line_t *parse_section(const char *s, int size, int *consume);

void print_cfg_line(board_cfg_line_t *cfg_line);

void free_cfg_line(board_cfg_line_t *cfg_line);
