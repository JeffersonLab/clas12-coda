#include "configParser.h"
#include <stdarg.h>

int cfgParseInt(const char *str_tmp, int *val) {
    const char *p = str_tmp;
printf("%s - start\n", __func__);
    while(*p == ' ' || *p == '\t') p++;
    while(*p != ' ' && *p != '\t' && *p != '\0') p++; // skip keyword
    if (sscanf(p, "%d", val) == 1) {
printf("%s - end 1\n", __func__);
        return 1;
    }
printf("%s - end 0\n", __func__);
    return 0;
}

int cfgParseChannelInt(const char *str_tmp, int *chan, int *val) {
    const char *p = str_tmp;
printf("%s - start\n", __func__);
    while(*p == ' ' || *p == '\t') p++;
    while(*p != ' ' && *p != '\t' && *p != '\0') p++; // skip keyword
    if (sscanf(p, "%d %d", chan, val) == 2) {
printf("%s - end 2\n", __func__);
        return 2;
    }
printf("%s - end 0\n", __func__);
    return 0;
}

int cfgParseAllChannelInt(const char *str_tmp, int *vals, int num_channels) {
    const char *p = str_tmp;
printf("%s - start\n", __func__);
    while(*p == ' ' || *p == '\t') p++;
    while(*p != ' ' && *p != '\t' && *p != '\0') p++; // skip keyword

    for(int i = 0; i < num_channels; i++) {
        int n = 0;
        if (sscanf(p, "%d%n", &vals[i], &n) != 1) {
printf("%s - end i\n", __func__);
            return i;
        }
        p += n;
    }
printf("%s - end 0\n", __func__);
    return num_channels;
}

int cfgParseFloat(const char *str_tmp, float *val) {
    const char *p = str_tmp;
printf("%s - start\n", __func__);
    while(*p == ' ' || *p == '\t') p++;
    while(*p != ' ' && *p != '\t' && *p != '\0') p++; // skip keyword
    if (sscanf(p, "%f", val) == 1) {
printf("%s - end 1\n", __func__);
        return 1;
    }
printf("%s - end 0\n", __func__);
    return 0;
}

int cfgParseChannelFloat(const char *str_tmp, int *chan, float *val) {
    const char *p = str_tmp;
printf("%s - start\n", __func__);
    while(*p == ' ' || *p == '\t') p++;
    while(*p != ' ' && *p != '\t' && *p != '\0') p++; // skip keyword
    if (sscanf(p, "%d %f", chan, val) == 2) {
printf("%s - end 2\n", __func__);
        return 2;
    }
printf("%s - end 0\n", __func__);
    return 0;
}

int cfgParseAllChannelFloat(const char *str_tmp, float *vals, int num_channels) {
    const char *p = str_tmp;
printf("%s - start\n", __func__);
    while(*p == ' ' || *p == '\t') p++;
    while(*p != ' ' && *p != '\t' && *p != '\0') p++; // skip keyword

    for(int i = 0; i < num_channels; i++) {
        int n = 0;
        if (sscanf(p, "%f%n", &vals[i], &n) != 1) {
printf("%s - end i\n", __func__);
            return i;
        }
        p += n;
    }
printf("%s - end 0\n", __func__);
    return num_channels;
}

int cfgAppendString(char *dest_str, int max_len, const char *format, ...) {
    int current_len = strlen(dest_str);
    //printf("%s - start\n", __func__);
    if (current_len >= max_len - 1) return current_len;

    va_list args;
    va_start(args, format);
    int written = vsnprintf(dest_str + current_len, max_len - current_len, format, args);
    va_end(args);

    if (written < 0 || written >= (max_len - current_len)) {
        // Truncation occurred, but we don't care deeply here.
      printf("%s - end 1\n", __func__);
        return max_len - 1;
    }
    //printf("%s - end 0\n", __func__);
    return current_len + written;
}

int cfgUploadAllChannelInt(char *dest_str, int max_len, const char *keyword, int *vals, int num_channels, int multiplier) {
printf("%s - start\n", __func__);
    cfgAppendString(dest_str, max_len, "%s", keyword);
    for(int i = 0; i < num_channels; i++) {
        cfgAppendString(dest_str, max_len, " %d", vals[i] * multiplier);
    }
    cfgAppendString(dest_str, max_len, "\n");
    return strlen(dest_str);
}

int cfgUploadAllChannelIntMask(char *dest_str, int max_len, const char *keyword, int vals, int num_channels) {
printf("%s - start\n", __func__);
    cfgAppendString(dest_str, max_len, "%s", keyword);
    for(int i = 0; i < num_channels; i++) {
      cfgAppendString(dest_str, max_len, " %d", (vals>>i) & 0x1);
    }
    cfgAppendString(dest_str, max_len, "\n");
    return strlen(dest_str);
}

int cfgUploadAllChannelFloat(char *dest_str, int max_len, const char *keyword, float *vals, int num_channels, float multiplier) {
printf("%s - start\n", __func__);
    cfgAppendString(dest_str, max_len, "%s", keyword);
    for(int i = 0; i < num_channels; i++) {
        cfgAppendString(dest_str, max_len, " %7.3f", vals[i] * multiplier);
    }
    cfgAppendString(dest_str, max_len, "\n");
    return strlen(dest_str);
}

