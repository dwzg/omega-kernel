/**
 * @file cheat.c
 * @brief `.cht` parser. See cheat.h for the format.
 *
 * All three readers (menu list, game name, option value) share the same line
 * scanner so that they agree on what a section and a key are.
 */
#include "core/cheat.h"

#include <stdio.h>
#include <string.h>

#include "core/text.h"

/** Line buffer sizes used by the original kernel for each reader. */
#define LIST_LINE_LEN 300
#define NAME_LINE_LEN CHT_NAME_LEN

/** Section in the cheat file that holds metadata rather than cheats. */
static const char GAME_INFO_SECTION[] = "GameInfo";

/** State carried from one line to the next. */
typedef struct {
    char section[CHT_NAME_LEN]; /**< Last section name seen. */
    bool in_section;            /**< Keys are accepted. */
    bool in_comment;            /**< Inside a block comment. */
} scan_state_t;

/** What the scanner found on one line. */
typedef struct {
    bool section_closed;    /**< A ']' was found (line is a heading). */
    unsigned section_len;   /**< Name characters read on this line. */
    char key[CHT_NAME_LEN]; /**< Key name (spaces removed). */
    unsigned key_len;
    bool has_equals;    /**< An '=' followed the key. */
    unsigned value_len; /**< Value characters stored. */
} line_t;

/** How the scanner should treat value characters after '='. */
typedef struct {
    char *buf;    /**< Where to store them, or NULL to ignore them. */
    size_t limit; /**< Stop after this many characters. */
} value_sink_t;

typedef enum { LINE_SKIPPED, LINE_SCANNED } line_kind_t;

/**
 * Apply the comment/blank-line rules to @p buf (already trimmed).
 * @return LINE_SKIPPED if the line carries no section or key.
 */
static line_kind_t pre_scan(scan_state_t *st, const char *buf)
{
    if (buf[0] != '#' && (buf[0] != '/' || buf[1] != '/')) {
        if (strstr(buf, "/*") != NULL) {
            st->in_comment = true;
            return LINE_SKIPPED;
        }
        if (strstr(buf, "*/") != NULL) {
            st->in_comment = false;
            return LINE_SKIPPED;
        }
    }
    if (st->in_comment) {
        return LINE_SKIPPED;
    }
    if (strlen(buf) <= 1 || buf[0] == '#' || buf[0] == '=' || buf[0] == '/') {
        st->in_section = false;
        return LINE_SKIPPED;
    }
    return LINE_SCANNED;
}

/** Scan one line for "[section]" or "key=value". */
static void scan_line(scan_state_t *st, const char *buf, line_t *out, const value_sink_t *sink)
{
    bool reading_section = false;
    size_t len = strlen(buf);

    memset(out, 0, sizeof(*out));
    for (size_t i = 0; i < len; i++) {
        char c = buf[i];
        if (c == ' ') {
            continue;
        }
        if (c == '[') {
            reading_section = true;
            st->in_section = false;
            memset(st->section, 0, sizeof(st->section));
            continue;
        }
        if (reading_section && c != ']') {
            if (out->section_len < CHT_NAME_LEN - 1) {
                st->section[out->section_len++] = c;
            }
            continue;
        }
        if (c == ']') {
            st->in_section = true;
            out->section_closed = true;
            break;
        }
        if (!st->in_section) {
            continue;
        }
        if (!out->has_equals && c != '=') {
            if (out->key_len >= CHT_NAME_LEN - 1) {
                break;
            }
            out->key[out->key_len++] = c;
            continue;
        }
        if (c == '=') {
            out->has_equals = true;
            continue;
        }
        if (sink->buf) {
            if (out->value_len >= sink->limit || c == '#') {
                break;
            }
            sink->buf[out->value_len++] = c;
        }
    }
}

/**
 * Copy the text of @p line between @p open (exclusive, or the line start if
 * 0) and @p close (exclusive) into @p out, without surrounding spaces.
 */
static void raw_text(const char *line, char open, char close, char *out, size_t size)
{
    const char *start = line;
    if (open) {
        const char *p = strchr(line, open);
        start = p ? p + 1 : line;
    }
    const char *end = strchr(start, close);
    if (!end) {
        end = start + strlen(start);
    }
    while (start < end && *start == ' ') {
        start++;
    }
    while (end > start && end[-1] == ' ') {
        end--;
    }
    size_t len = (size_t)(end - start);
    if (len >= size) {
        len = size - 1;
    }
    memcpy(out, start, len);
    out[len] = '\0';
}

static void add_entry(cht_entry_t *entries, size_t *count, size_t max, const char *name,
                      unsigned name_len, bool is_section, const char *label)
{
    if (*count >= max) {
        return;
    }
    cht_entry_t *e = &entries[(*count)++];
    memset(e, 0, sizeof(*e));
    memcpy(e->name, name, name_len < CHT_NAME_LEN ? name_len : CHT_NAME_LEN - 1);
    text_copy(e->label, sizeof(e->label), label[0] ? label : e->name);
    e->is_section = is_section;
}

size_t cht_list_entries(const cht_reader_t *reader, cht_entry_t *entries, size_t max_entries)
{
    char buf[LIST_LINE_LEN];
    scan_state_t st = {0};
    value_sink_t no_value = {NULL, 0};
    line_t line;
    size_t count = 0;

    reader->rewind(reader->ctx);
    while (count < max_entries && reader->read_line(reader->ctx, buf, sizeof(buf))) {
        text_trim_right(buf);
        if (buf[0] == '-' && buf[1] == '-') {
            break;
        }
        if (pre_scan(&st, buf) == LINE_SKIPPED) {
            continue;
        }
        scan_line(&st, buf, &line, &no_value);
        /* A line can (oddly) yield both: "key=x]" closes a nameless heading. */
        char label[CHT_NAME_LEN];
        if (line.section_closed) {
            /* Only the characters read on this line form the heading. */
            raw_text(buf, '[', ']', label, sizeof(label));
            add_entry(entries, &count, max_entries, st.section, line.section_len, true, label);
        }
        if (line.has_equals) {
            const char *close = strchr(buf, ']');
            raw_text(close ? close + 1 : buf, 0, '=', label, sizeof(label));
            add_entry(entries, &count, max_entries, line.key, line.key_len, false, label);
        }
    }

    /* Hide [GameInfo] and everything after it. */
    for (size_t i = 0; i < count; i++) {
        if (strcmp(entries[i].name, GAME_INFO_SECTION) == 0) {
            return i;
        }
    }
    return count;
}

void cht_read_game_name(const cht_reader_t *reader, char *out, size_t out_size)
{
    char buf[NAME_LINE_LEN];
    char value[CHT_NAME_LEN];
    scan_state_t st = {0};
    value_sink_t sink = {value, CHT_NAME_LEN - 1};
    line_t line;

    text_copy(out, out_size, "");
    reader->rewind(reader->ctx);
    while (reader->read_line(reader->ctx, buf, sizeof(buf))) {
        text_trim_right(buf);
        if (pre_scan(&st, buf) == LINE_SKIPPED) {
            continue;
        }
        memset(value, 0, sizeof(value));
        scan_line(&st, buf, &line, &sink);
        if (strcmp(st.section, GAME_INFO_SECTION) == 0 && strcmp(line.key, "Name") == 0) {
            /* Show the name with its spaces; it ends like a value at '#'. */
            raw_text(buf, '=', '#', value, sizeof(value));
            text_copy(out, out_size, value);
            return;
        }
    }
}

size_t cht_read_value(const cht_reader_t *reader, const char *section, const char *key,
                      cht_workspace_t *ws)
{
    scan_state_t st = {0};
    value_sink_t sink = {ws->value, CHT_VALUE_LEN};
    line_t line;

    reader->rewind(reader->ctx);
    while (reader->read_line(reader->ctx, ws->line, sizeof(ws->line))) {
        text_trim_right(ws->line);
        if (pre_scan(&st, ws->line) == LINE_SKIPPED) {
            continue;
        }
        scan_line(&st, ws->line, &line, &sink);
        if (strcmp(st.section, section) != 0 || strcmp(line.key, key) != 0) {
            continue;
        }

        /* Found: append continuation lines until the next key or blank line. */
        size_t len = line.value_len;
        while (reader->read_line(reader->ctx, ws->line, sizeof(ws->line))) {
            text_trim_right(ws->line);
            if (pre_scan(&st, ws->line) == LINE_SKIPPED || strchr(ws->line, '=') != NULL) {
                break;
            }
            for (const char *p = ws->line; *p && len < CHT_VALUE_LEN; p++) {
                ws->value[len++] = *p;
            }
        }
        return len;
    }
    return 0;
}

static void add_code(cheat_code_t *codes, size_t max, size_t *count, uint32_t address,
                     uint32_t value)
{
    if (*count < max) {
        codes[*count].address = address;
        codes[*count].value = value;
        (*count)++;
    }
}

void cht_decode_value(const char *value, size_t len, cheat_code_t *codes, size_t max, size_t *count)
{
    char address_buf[9] = {0};
    char byte_buf[3] = {0};
    unsigned address_len = 0;
    unsigned byte_len = 0;
    bool reading_bytes = false;
    bool first_comma = true;
    uint32_t offset = 0;

    for (size_t i = 0; i < len; i++) {
        char c = value[i];
        if (c == ' ') {
            continue;
        }
        if (c == ',') {
            if (first_comma) {
                first_comma = false;
            } else {
                add_code(codes, max, count, text_parse_hex(address_buf) + offset,
                         text_parse_hex(byte_buf));
                offset++;
            }
            byte_len = 0;
            memset(byte_buf, 0, sizeof(byte_buf));
            reading_bytes = true;
            continue;
        }
        if (c == ';') {
            add_code(codes, max, count, text_parse_hex(address_buf) + offset,
                     text_parse_hex(byte_buf));
            reading_bytes = false;
            first_comma = true;
            offset = 0;
            address_len = 0;
            memset(address_buf, 0, sizeof(address_buf));
            continue;
        }
        if (reading_bytes) {
            if (byte_len < sizeof(byte_buf) - 1) {
                byte_buf[byte_len++] = c;
            }
        } else if (address_len < sizeof(address_buf) - 1) {
            address_buf[address_len++] = c;
        }
    }
    /* A trailing ';' leaves nothing pending; the original kernel emitted a
     * bogus code for address 0 in that case. */
    if (address_len > 0) {
        add_code(codes, max, count, text_parse_hex(address_buf) + offset, text_parse_hex(byte_buf));
    }
}

void cht_toggle_option(cht_entry_t *entries, size_t count, size_t index)
{
    if (index >= count || entries[index].is_section) {
        return;
    }
    for (size_t i = index; i > 0 && !entries[i - 1].is_section; i--) {
        entries[i - 1].selected = 0;
    }
    for (size_t i = index + 1; i < count && !entries[i].is_section; i++) {
        entries[i].selected = 0;
    }
    entries[index].selected = !entries[index].selected;
}

size_t cht_selected_count(const cht_entry_t *entries, size_t count)
{
    size_t n = 0;
    for (size_t i = 0; i < count; i++) {
        n += (!entries[i].is_section && entries[i].selected);
    }
    return n;
}

size_t cht_collect_codes(const cht_reader_t *reader, const cht_entry_t *entries, size_t count,
                         cht_workspace_t *ws, cheat_code_t *codes, size_t max_codes)
{
    size_t n = 0;
    for (size_t i = 1; i < count; i++) {
        if (entries[i].is_section || !entries[i].selected) {
            continue;
        }
        size_t section = i - 1;
        while (section > 0 && !entries[section].is_section) {
            section--;
        }
        size_t len = cht_read_value(reader, entries[section].name, entries[i].name, ws);
        cht_decode_value(ws->value, len, codes, max_codes, &n);
    }
    return n;
}

/** Value of a hex digit character, or 255 for anything else. */
static unsigned hex_digit_value(char c)
{
    if (c >= '0' && c <= '9') {
        return (unsigned)(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return (unsigned)(c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f') {
        return (unsigned)(c - 'a' + 10);
    }
    return 255;
}

void cht_library_paths(uint32_t library_id, char *folder, size_t folder_size, char *file,
                       size_t file_size)
{
    /* The id holds four ASCII digits, first digit in the least significant byte. */
    char digits[16];
    unsigned number = 0;

    snprintf(digits, sizeof(digits), "%u%u%u%u", hex_digit_value((char)(library_id & 0xFF)),
             hex_digit_value((char)((library_id >> 8) & 0xFF)),
             hex_digit_value((char)((library_id >> 16) & 0xFF)),
             hex_digit_value((char)((library_id >> 24) & 0xFF)));
    for (const char *p = digits; *p; p++) {
        number = number * 10 + (unsigned)(*p - '0');
    }
    unsigned bucket = (number / 200) * 200;
    if (bucket > 2800) {
        bucket = 2800;
    }
    snprintf(folder, folder_size, "/CHEAT/Eng/%04u", bucket);
    snprintf(file, file_size, "%s.cht", digits);
}
