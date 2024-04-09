
#include "../all.h"

bool is_alpha_char(char c) {
    if (c >= 65 && c <= 90) {
        return true;
    }

    if (c >= 97 && c <= 122) {
        return true;
    }

    return false;
}

bool is_valid_varname_char(char c) {
    // Uppercase
    if (c >= 65 && c <= 90) {
        return true;
    }

    // Lowercase
    if (c >= 97 && c <= 122) {
        return true;
    }

    // Numbers
    if (c >= 48 && c <= 57) {
        return true;
    }

    // Underscore
    if (c == 95) {
        return true;
    }

    return false;
}
bool is_valid_varname_first_char(char c) {

    // Lowercase
    if (c >= 97 && c <= 122) {
        return true;
    }

    // Numbers
    if (c >= 48 && c <= 57) {
        return true;
    }

    // Underscore
    if (c == 95) {
        return true;
    }
    return true;
}

bool is_number(char c) {
    if (c >= 48 && c <= 57) {
        return true;
    }
    return false;
}

bool is_hex_char(char c) {
    if (c >= 48 && c <= 57) {
        return true;
    }
    // A-F
    if (c >= 65 && c <= 70) {
        return true;
    }
    // a-f
    if (c >= 97 && c <= 102) {
        return true;
    }
    return false;
}
bool is_octal_char(char c) {
    if (c >= 48 && c <= 55) {
        return true;
    }
    return false;
}

bool is_whitespace(char c) { return c <= 32; }
bool is_newline(char c) { return c == 10; }
bool is_valid_varname(char *name) { return is_valid_varname_first_char(name[0]); }
bool is_valid_varname_all(char *name) {
    //
    int len = strlen(name);
    if (len == 0)
        return false;
    if (!is_valid_varname_first_char(name[0]))
        return false;
    int i = 0;
    while (i < len) {
        if (!is_valid_varname_char(name[i])) {
            return false;
        }
        i++;
    }
    return true;
}

bool is_valid_number(char *str) {
    int len = strlen(str);
    if (len == 0) {
        return false;
    }
    int i = 0;
    while (i < len) {
        if (!is_number(str[i])) {
            return false;
        }
        i++;
    }
    return true;
}

bool is_valid_hex_number(char *str) {
    int len = strlen(str);
    if (len == 0) {
        return false;
    }
    int i = 0;
    while (i < len) {
        char ch = str[i];
        if (!is_hex_char(ch)) {
            return false;
        }
        i++;
    }
    return true;
}
bool is_valid_octal_number(char *str) {
    int len = strlen(str);
    if (len == 0) {
        return false;
    }
    int i = 0;
    while (i < len) {
        char ch = str[i];
        if (!is_octal_char(ch)) {
            return false;
        }
        i++;
    }
    return true;
}

bool is_valid_macro_number(char *str) {
    int len = strlen(str);
    if (len == 0) {
        return false;
    }
    //
    char last = str[len - 1];
    if (last == 'L') {
        len--;
    }
    //
    last = str[len - 1];
    if (last == 'U') {
        len--;
    }
    //
    int i = 0;
    while (i < len) {
        if (!is_number(str[i])) {
            return false;
        }
        i++;
    }
    return true;
}

bool starts_with(const char *a, const char *b) {
    if (strncmp(a, b, strlen(b)) == 0)
        return 1;
    return 0;
}

bool ends_with(const char *str, const char *suffix) {
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);
    return (str_len >= suffix_len) && (str_is(str + (str_len - suffix_len), suffix));
}

char backslash_char(char ch) {
    if (ch == 'n') {
        ch = '\n';
    } else if (ch == 'r') {
        ch = '\r';
    } else if (ch == 't') {
        ch = '\t';
    } else if (ch == 'f') {
        ch = '\f';
    } else if (ch == 'b') {
        ch = '\b';
    } else if (ch == 'v') {
        ch = '\v';
    } else if (ch == 'f') {
        ch = '\f';
    } else if (ch == 'a') {
        ch = '\a';
    }
    return ch;
}

bool str_is(const char* tkn, const char* comp) {
    while (*tkn == *comp++)
        if (*tkn++ == '\0')
        	return true;
    return false;
}

bool str_in(char* tkn, char* comp) {
    char buf[256];
    strcpy(buf, ",");
    strcpy(buf + 1, tkn);
    strcat(buf, ",");
    return strstr(comp, buf);
}

char *string_replace_backslash_chars(Allocator *alc, char *body) {
    int len = strlen(body);
    if (len == 0)
        return "";
    char *result = al(alc, len + 1);
    int i = 0;
    int ri = 0;
    while (true) {
        const char ch = body[i++];
        if (ch == 0)
            break;
        if (ch == '\\') {
            char ch = body[i++];
            ch = backslash_char(ch);
            result[ri++] = ch;
            continue;
        }
        result[ri++] = ch;
    }
    result[ri] = 0;
    return result;
}

char* op_to_str(int op) {
    if (op == op_add)
        return "+";
    if (op == op_sub)
        return "-";
    if (op == op_mul)
        return "*";
    if (op == op_div)
        return "/";
    if (op == op_mod)
        return "%";
    if (op == op_eq)
        return "==";
    if (op == op_ne)
        return "!=";
    if (op == op_lt)
        return "<";
    if (op == op_gt)
        return ">";
    if (op == op_lte)
        return "<=";
    if (op == op_gte)
        return ">=";
    if (op == op_bit_and)
        return "&";
    if (op == op_bit_or)
        return "|";
    if (op == op_bit_xor)
        return "^";
    if (op == op_shl)
        return "<<";
    if (op == op_shr)
        return ">>";
    return "?";
}

void char_to_hex(const unsigned char ch, char* buf) {
    const char* h = "0123456789ABCDEF";
    const unsigned char first = ch / 16;
    const unsigned char second = ch % 16;
    buf[0] = h[first];
    buf[1] = h[second];
    buf[2] = 0;
}

char* itos(v_i64 val, char* buf, const int base){
    if(val == 0) {
        strcpy(buf, "0");
        return buf;
    }
    int i = 0;
    bool negative = false;
    char rev[64];
    if(val < 0) {
        val *= -1;
        negative = true;
    }
    for (; val; i++, val /= base)
        rev[i] = "0123456789ABCDEF"[val % base];
    if(negative) {
        rev[i++] = '-';
    }
    buf[i] = 0;
    int up = 0;
    while(i-- > 0)
        buf[i] = rev[up++];
    return buf;
}

v_i64 hex2int(char *hex) {
    v_i64 val = 0;
    while (*hex) {
        // get current character then increment
        uint8_t byte = *hex++;
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9')
            byte = byte - '0';
        else if (byte >= 'a' && byte <= 'f')
            byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <= 'F')
            byte = byte - 'A' + 10;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}
v_i64 oct2int(char *oct) {
    v_i64 val = 0;
    while (*oct) {
        // get current character then increment
        uint8_t byte = *oct++;
        // transform octal character to the 3bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '7')
            byte = byte - '0';
        // shift 3 to make space for new digit, and add the 3 bits of the new digit
        val = (val << 3) | (byte & 0x7);
    }
    return val;
}