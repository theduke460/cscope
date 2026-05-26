/*
 * gscope.c - gscope mode support for cscope
 *
 * When invoked as 'gscope', cscope operates in generic text search mode:
 *
 *   - Single search field replaces all C-specific fields
 *   - Case-insensitive matching always enabled
 *   - Boolean search expression support:
 *
 *       &&      AND operator — all terms must appear on the line
 *       ||      OR operator  — either term must appear on the line
 *       (( ))   grouping     — explicit grouping; content evaluated
 *                              recursively so && and || both work inside
 *
 * Expression syntax:
 *
 *   Simple AND:         dave&&barry
 *   Simple OR:          dave||david
 *   Grouped OR:         ((dave||david))&&barry
 *   Grouped AND:        ((dave&&barry))
 *   Mixed within group: ((str1&&str2||str3))  -- (str1 AND str2) OR str3
 *   Multi-group:        ((dave||david))&&((barry||barry jr))
 *
 * Evaluation rules:
 *   1. Top-level && splits the expression into AND operands,
 *      respecting (( )) group boundaries
 *   2. Each AND operand is either:
 *      a. A (( )) group  — content evaluated recursively
 *      b. A plain term with || — evaluated as OR
 *      c. A plain substring  — direct match
 *   3. (( )) group content is split on || into alternatives;
 *      each alternative is itself evaluated as a full expression
 *      (enabling && inside groups)
 *   4. All matching is case-insensitive
 *
 * Activated automatically via argv[0] detection in main.c
 */

#include "global.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* Global flag - set in main() when argv[0] basename is "gscope" */
BOOL isgscope = NO;

/* Forward declaration for mutual recursion */
static BOOL eval_and_expr(const char *pattern, const char *line_lower);

/*
 * lowercase_copy - copy src to dst lowercased, up to maxlen chars
 */
static void
lowercase_copy(char *dst, const char *src, int maxlen)
{
    int i;
    for (i = 0; src[i] && i < maxlen; i++)
        dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = '\0';
}

/*
 * eval_or_group - evaluate an OR expression against line_lower
 *
 * group_pat : OR expression e.g. "dave||david" or "str1&&str2||str3"
 * line_lower: already-lowercased line
 *
 * Splits on || and evaluates each alternative as a full expression
 * (via eval_and_expr) enabling && within alternatives.
 *
 * Returns YES if ANY || alternative matches.
 */
static BOOL
eval_or_group(const char *group_pat, const char *line_lower)
{
    char    pat[PATLEN + 1];
    char    tok[PATLEN + 1];
    char    *p, *delim;
    int     len;

    lowercase_copy(pat, group_pat, PATLEN);
    p = pat;

    while (p != NULL) {
        delim = strstr(p, "||");
        if (delim != NULL) {
            len = delim - p;
            if (len > PATLEN) len = PATLEN;
            strncpy(tok, p, len);
            tok[len] = '\0';
            p = delim + 2;
        } else {
            strncpy(tok, p, PATLEN);
            tok[PATLEN] = '\0';
            p = NULL;
        }
        if (tok[0] == '\0') continue;

        /* Recursively evaluate each alternative as a full expression */
        if (eval_and_expr(tok, line_lower) == YES)
            return YES;
    }
    return NO;
}

/*
 * next_and_token - extract the next top-level && token from *pp
 *
 * Scans forward respecting (( )) group boundaries so that &&
 * inside a group is NOT treated as a top-level AND separator.
 *
 * On return:
 *   tok  is filled with the token text
 *   *pp  points past the && delimiter, or NULL if no more tokens
 *
 * Returns the length of the token, or 0 if nothing left.
 */
static int
next_and_token(char **pp, char *tok, int maxlen)
{
    char *p = *pp;
    char *start;
    int  depth = 0;
    int  len;

    if (p == NULL || *p == '\0') {
        *pp = NULL;
        tok[0] = '\0';
        return 0;
    }

    start = p;
    while (*p != '\0') {
        if (p[0] == '(' && p[1] == '(') { depth++; p += 2; continue; }
        if (p[0] == ')' && p[1] == ')') { if (depth > 0) depth--; p += 2; continue; }
        if (depth == 0 && p[0] == '&' && p[1] == '&') {
            len = p - start;
            if (len > maxlen) len = maxlen;
            strncpy(tok, start, len);
            tok[len] = '\0';
            *pp = p + 2;
            return len;
        }
        p++;
    }

    len = p - start;
    if (len > maxlen) len = maxlen;
    strncpy(tok, start, len);
    tok[len] = '\0';
    *pp = NULL;
    return len;
}

/*
 * eval_and_expr - evaluate a full boolean expression against line_lower
 *
 * Splits on top-level && only (respecting (( )) group boundaries),
 * then evaluates each operand as:
 *   - a (( )) group  -> eval_or_group (which recurses back here per alternative)
 *   - a term with || -> eval_or_group directly
 *   - a plain term   -> substring match
 *
 * Returns YES if ALL && operands match.
 */
static BOOL
eval_and_expr(const char *pattern, const char *line_lower)
{
    char    pat_lower[PATLEN + 1];
    char    tok[PATLEN + 1];
    char    *p;

    lowercase_copy(pat_lower, pattern, PATLEN);
    p = pat_lower;

    while (p != NULL) {
        if (next_and_token(&p, tok, PATLEN) == 0)
            break;
        if (tok[0] == '\0') continue;

        if (tok[0] == '(' && tok[1] == '(') {
            /* (( )) group — extract content and evaluate as OR expression */
            char *close = strstr(tok, "))");
            char  group[PATLEN + 1];
            int   glen;

            if (close != NULL) {
                glen = close - (tok + 2);
                if (glen > PATLEN) glen = PATLEN;
                strncpy(group, tok + 2, glen);
                group[glen] = '\0';
            } else {
                /* malformed group - treat content after (( as OR expression */
                strncpy(group, tok + 2, PATLEN);
                group[PATLEN] = '\0';
            }
            if (eval_or_group(group, line_lower) == NO)
                return NO;

        } else if (strstr(tok, "||") != NULL) {
            /* top-level OR expression without grouping */
            if (eval_or_group(tok, line_lower) == NO)
                return NO;

        } else {
            /* plain substring match */
            if (strstr(line_lower, tok) == NULL)
                return NO;
        }
    }

    return YES;
}

/*
 * gscope_match - public entry point for boolean expression matching
 *
 * pattern  : raw search expression from user input
 * line     : source line to test
 *
 * Returns YES if expression matches line, NO otherwise.
 */
BOOL
gscope_match(const char *pattern, const char *line)
{
    char line_lower[TEMPSTRING_LEN + 1];
    lowercase_copy(line_lower, line, TEMPSTRING_LEN);
    return eval_and_expr(pattern, line_lower);
}

/*
 * findgscope - search all source files for lines matching pattern
 *
 * Supports full boolean expression syntax via gscope_match().
 * Writes results to refsfound in cscope format:
 *   filename <unknown> lineno matched-line-text
 */
char *
findgscope(char *pattern)
{
    unsigned int    i;
    unsigned long   lineno;
    char            line[TEMPSTRING_LEN + 1];
    FILE            *src;
    char            *file;

    for (i = 0; i < nsrcfiles; ++i) {
        file = filepath(srcfiles[i]);
        progress("Search", (long)searchcount, (long)nsrcfiles);
        searchcount++;

        src = fopen(file, "r");
        if (src == NULL) {
            posterr("Cannot open file %s", file);
            continue;
        }

        lineno = 0;
        while (fgets(line, sizeof(line), src) != NULL) {
            lineno++;
            line[strcspn(line, "\n")] = '\0';

            if (gscope_match(pattern, line)) {
                (void) fprintf(refsfound,
                    "%s <unknown> %lu %s\n",
                    srcfiles[i], lineno, line);
            }
        }
        (void) fclose(src);
    }
    return NULL;
}
