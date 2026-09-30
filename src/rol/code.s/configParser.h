#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdio.h>
#include <string.h>

/**
 * Extract a single integer from a string after skipping the keyword.
 * e.g., "FADC250_MODE 1"
 * Returns 1 if successful, 0 otherwise.
 */
int cfgParseInt(const char *str_tmp, int *val);

/**
 * Extract a channel and an integer from a string.
 * e.g., "FADC250_CH_MODE 5 1"
 * Returns 2 if successful, <2 otherwise.
 */
int cfgParseChannelInt(const char *str_tmp, int *chan, int *val);

/**
 * Extract a mask of multiple integers from a string.
 * e.g., "FADC250_ALLCH_MODE 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1"
 * Returns the number of integers successfully parsed (should equal num_channels).
 */
int cfgParseAllChannelInt(const char *str_tmp, int *vals, int num_channels);

/**
 * Extract a single float from a string after skipping the keyword.
 * e.g., "FADC250_GAIN 1.0"
 * Returns 1 if successful, 0 otherwise.
 */
int cfgParseFloat(const char *str_tmp, float *val);

/**
 * Extract a channel and an float from a string.
 * e.g., "FADC250_CH_GAIN 5 1.0"
 * Returns 2 if successful, <2 otherwise.
 */
int cfgParseChannelFloat(const char *str_tmp, int *chan, float *val);

/**
 * Extract a mask of multiple floats from a string.
 * e.g., "FADC250_ALLCH_GAIN 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0"
 * Returns the number of floats successfully parsed (should equal num_channels).
 */
int cfgParseAllChannelFloat(const char *str_tmp, float *vals, int num_channels);

/**
 * Append a formatted string to a destination string buffer safely.
 * Returns the new length of the string, or the old length if there was no space.
 */
int cfgAppendString(char *dest_str, int max_len, const char *format, ...);

/**
 * Generic helper for uploading (printing) ALLCH variables.
 * For example, if you pass "FADC250_ALLCH_MODE", it appends "FADC250_ALLCH_MODE 1 1 ... 1\\n"
 * Multiplier allows you to convert from hardware scaled values back to nanoseconds or etc.
 */
int cfgUploadAllChannelInt(char *dest_str, int max_len, const char *keyword, int *vals, int num_channels, int multiplier);
int cfgUploadAllChannelFloat(char *dest_str, int max_len, const char *keyword, float *vals, int num_channels, float multiplier);
int cfgUploadAllChannelIntMask(char *dest_str, int max_len, const char *keyword, int vals, int num_channels);

/**
 * Generic macros for parsing config file lines.
 * These are designed to be used in an if-else chain.
 */

#define MASK_INVERT 0x10000

#define CFG_PARSE_SLOT_MASK(KEYWORD, MATCH_STR, STR_TMP, STRUCT_ARR, FIELD, NUM_CHAN, FLAG) \
    else if (!strcmp(KEYWORD, MATCH_STR)) { \
        int _vals[NUM_CHAN], _mask = 0, _c, _s; \
        if (cfgParseAllChannelInt(STR_TMP, _vals, NUM_CHAN) == NUM_CHAN) { \
            for (_c = 0; _c < NUM_CHAN; _c++) \
{\
              if(FLAG & MASK_INVERT) \
                _mask|= _vals[_c] ? 0 : (1<<_c); \
              else \
                _mask|= _vals[_c] ? (1<<_c) : 0; \
}\
            for (_s = slot1; _s < slot2; _s++) \
{\
                STRUCT_ARR[_s].FIELD = _mask; \
}\
        } \
    }

#define CFG_PARSE_SLOT_INT(KEYWORD, MATCH_STR, STR_TMP, STRUCT_ARR, FIELD) \
    else if (!strcmp(KEYWORD, MATCH_STR)) { \
        int _val, _s; \
        if (cfgParseInt(STR_TMP, &_val)) { \
            for (_s = slot1; _s < slot2; _s++) { \
                STRUCT_ARR[_s].FIELD = _val; \
            } \
        } \
    }

#define CFG_PARSE_SLOTCH_INT(KEYWORD, MATCH_STR, STR_TMP, STRUCT_ARR, FIELD, NUM_CHAN) \
    else if (!strcmp(KEYWORD, MATCH_STR)) { \
        int _ch, _val, _s; \
        if (cfgParseInt(STR_TMP, &_val)) { \
            for (_ch = 0; _ch<NUM_CHAN; _ch++) { \
                for (_s = slot1; _s < slot2; _s++) { \
                    STRUCT_ARR[_s].FIELD[_ch] = _val; \
                } \
            } \
        } \
    }

#define CFG_PARSE_SLOTCH_CH_INT(KEYWORD, MATCH_STR, STR_TMP, STRUCT_ARR, FIELD, NUM_CHAN) \
    else if (!strcmp(KEYWORD, MATCH_STR)) { \
        int _ch, _val, _s; \
        if (cfgParseChannelInt(STR_TMP, &_ch, &_val) == 2) { \
            if (_ch >= 0 && _ch < NUM_CHAN) { \
                for (_s = slot1; _s < slot2; _s++) { \
                    STRUCT_ARR[_s].FIELD[_ch] = _val; \
                } \
            } \
        } \
    }

#define CFG_PARSE_SLOTCH_ALLCH_INT(KEYWORD, MATCH_STR, STR_TMP, STRUCT_ARR, FIELD, NUM_CHAN) \
    else if (!strcmp(KEYWORD, MATCH_STR)) { \
        int _vals[NUM_CHAN], _s, _c; \
        if (cfgParseAllChannelInt(STR_TMP, _vals, NUM_CHAN) == NUM_CHAN) { \
            for (_s = slot1; _s < slot2; _s++) { \
                for (_c = 0; _c < NUM_CHAN; _c++) { \
                    STRUCT_ARR[_s].FIELD[_c] = _vals[_c]; \
                } \
            } \
        } \
    }

#define CFG_PARSE_SLOTCH_FLOAT(KEYWORD, MATCH_STR, STR_TMP, STRUCT_ARR, FIELD, NUM_CHAN) \
    else if (!strcmp(KEYWORD, MATCH_STR)) { \
        int _ch, _s; float _val; \
        if (cfgParseFloat(STR_TMP, &_val)) { \
            for (_ch = 0; _ch<NUM_CHAN; _ch++) { \
                for (_s = slot1; _s < slot2; _s++) { \
                    STRUCT_ARR[_s].FIELD[_ch] = _val; \
                } \
            } \
        } \
    }

#define CFG_PARSE_SLOTCH_CH_FLOAT(KEYWORD, MATCH_STR, STR_TMP, STRUCT_ARR, FIELD, NUM_CHAN) \
    else if (!strcmp(KEYWORD, MATCH_STR)) { \
        int _ch, _s; float _val; \
        if (cfgParseChannelFloat(STR_TMP, &_ch, &_val) == 2) { \
            if (_ch >= 0 && _ch < NUM_CHAN) { \
                for (_s = slot1; _s < slot2; _s++) { \
                    STRUCT_ARR[_s].FIELD[_ch] = _val; \
                } \
            } \
        } \
    }

#define CFG_PARSE_SLOTCH_ALLCH_FLOAT(KEYWORD, MATCH_STR, STR_TMP, STRUCT_ARR, FIELD, NUM_CHAN) \
    else if (!strcmp(KEYWORD, MATCH_STR)) { \
        float _vals[NUM_CHAN]; int _s, _c; \
        if (cfgParseAllChannelFloat(STR_TMP, _vals, NUM_CHAN) == NUM_CHAN) { \
            for (_s = slot1; _s < slot2; _s++) { \
                for (_c = 0; _c < NUM_CHAN; _c++) { \
                    STRUCT_ARR[_s].FIELD[_c] = _vals[_c]; \
                } \
            } \
        } \
    }

#endif /* CONFIG_PARSER_H */
