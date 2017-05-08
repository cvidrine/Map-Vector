/* File: maptest.c
* ----------------
* A small program to exercise some basic functionality of the CMap. You should
* supplement with additional tests of your own.  You may change/extend/replace
* this test program in any way you see fit.
* jzelenski
*/

#include "cmap.h"
#include "cvector.h"
#include <assert.h>
#include <ctype.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Function: verify_int
* ---------------------
* Used to compare a given result with what was expected and report on whether
* passed/failed.
*/
static void verify_int(int expected, int found, char *msg)
{
    printf("%s expect: %d found: %d. %s\n", msg, expected, found,
        (expected == found) ? "Seems ok." : "##### PROBLEM HERE #####");
}

static void verify_ptr(void *expected, void *found, char *msg)
{
    printf("%s expect: %p found: %p. %s\n", msg, expected, found,
        (expected == found) ? "Seems ok." : "##### PROBLEM HERE #####");
}

static void verify_int_ptr(int expected, int *found, char *msg)
{
    if (found == NULL)
        printf("%s found: %p %s\n", msg, found, "##### PROBLEM HERE #####");
    else
        verify_int(expected, *found, msg);
}

void simple_cmap()
{
    char *words[] = {"apple", "pear", "banana", "cherry", "kiwi", "melon", "grape", "plum"};
    char *extra = "strawberry";
    int len, nwords = sizeof(words)/sizeof(words[0]);
    CMap *cm = cmap_create(sizeof(int), nwords, NULL);

    printf("\n----------------- Testing simple cmap ------------------ \n");
    printf("Created empty CMap.\n");
    verify_int(0, cmap_count(cm), "cmap_count");
    verify_ptr(NULL, cmap_get(cm, "nonexistent"), "cmap_get(\"nonexistent\")");

    printf("\nAdding %d keys to CMap.\n", nwords);
    for (int i = 0; i < nwords; i++) {
        len = strlen(words[i]);
        cmap_put(cm, words[i], &len); // associate word w/ its strlen
    }
    verify_int(nwords, cmap_count(cm), "cmap_count");
    verify_int_ptr(strlen(words[0]), cmap_get(cm, words[0]), "cmap_get(\"apple\")");

    printf("\nAdd one more key to CMap.\n");
    len = strlen(extra);
    cmap_put(cm, extra, &len);
    verify_int(nwords+1, cmap_count(cm), "cmap_count");
    verify_int_ptr(strlen(extra), cmap_get(cm, extra), "cmap_get(\"strawberry\")");

    printf("\nReplace existing key in CMap.\n");
    len = 2*strlen(extra);
    cmap_put(cm, extra, &len);
    verify_int(nwords+1, cmap_count(cm), "cmap_count");
    verify_int_ptr(len, cmap_get(cm, extra), "cmap_get(\"strawberry\")");

    printf("\nRemove key from CMap.\n");
    cmap_remove(cm, words[0]);
    verify_int(nwords, cmap_count(cm), "cmap_count");
    verify_ptr(NULL, cmap_get(cm, words[0]), "cmap_get(\"apple\")");

    printf("\nUse iterator to count keys.\n");
    int nkeys = 0;
    for (const char *key = cmap_first(cm); key != NULL; key = cmap_next(cm, key))
        nkeys++;
    verify_int(cmap_count(cm), nkeys, "Number of keys");

    cmap_dispose(cm);
}


/* Function: frequency_test
* -------------------------
* Runs a test of the CMap to count letter frequencies from a file.
* Each key is single-char string, value is count (int).
* Reads file char-by-char, updates map, prints total when done.
*/
static void frequency_test()
{
    printf("\n----------------- Testing frequency ------------------ \n");
    CMap *counts = cmap_create(sizeof(int), 26, NULL);

    int val, zero = 0;
    char buf[2];
    buf[1] = '\0';  // null terminator for string of one char

   // initialize map to have entries for all lowercase letters, count = 0
    for (char ch = 'a'; ch <= 'z'; ch++) {
        buf[0] = ch;
        cmap_put(counts, buf, &zero);
    }
    FILE *fp = fopen("/afs/ir/class/cs107/samples/assign1/gettysburg_frags", "r"); 
    assert(fp != NULL);
    while ((val = getc(fp)) != EOF) {
        if (isalpha(val)) { // only count letters
            buf[0] = tolower(val);
            (*(int *)cmap_get(counts, buf))++;
        }
    }
    fclose(fp);

    int total = 0;
    for (const char *key = cmap_first(counts); key != NULL; key = cmap_next(counts, key))
        total += *(int *)cmap_get(counts, key);
    printf("Total of all frequencies = %d\n", total);
    // correct count should agree with shell command
    // tr -c -d "[:alpha:]" < /afs/ir/class/cs107/samples/assign1/gettysburg_frags | wc -c
    cmap_dispose(counts);
}

#define NUM_SYNONYMS 16
#define NUM_HEADWORDS 35000

static void cleanup_cvec(void *p)
{
    cvec_dispose(*(CVector **)p);
}

static void cleanup_str(void *p)
{
    free(*(char **)p);
}

/**
 * Reads a single line from FILE * using fgets into the client's
 * buffer. Removes the newline and returns true if line was non-empty
 * false othewise.
 */
static bool read_line(FILE *fp, char *buf, int sz)
{
    if (!fgets(buf, sz, fp)) return false;
    int last = strlen(buf) - 1;
    if (buf[last] == '\n') buf[last] = '\0'; // strip newline
    return (*buf != '\0');
}

/**
 * Tokenizes thesaurus data file and builds map of word -> synonyms.
 * Each line of data file is expected to be of the form:
 *
 *     cold,arctic,blustery,freezing,frigid,icy,nippy,polar
 *
 * The first word (or phrase) is primary, and rest of line are synonyms of first.
 * The ',' delimits words, and the '\n' marks the end of the entry.
 */
static CMap *read_thesaurus(FILE *fp)
{
    CMap *thesaurus = cmap_create(sizeof(CVector *), NUM_HEADWORDS, cleanup_cvec);
    printf("Loading thesaurus..");
    fflush(stdout);

    char line[10000], buffer[128];
    while (read_line(fp, line, sizeof(line))) { // read one line
        if (line[0] == '#') {               // echo file comment
            printf(" (%s)", line+1);
            continue;
        }
        char *cur = line;
        sscanf(line, "%127[^,]", buffer);   // first word of line is headword
        cur += strlen(buffer);
        CVector *synonyms = cvec_create(sizeof(char *), NUM_SYNONYMS, cleanup_str);
        cmap_put(thesaurus, buffer, &synonyms);
        while (sscanf(cur, ",%127[^,]", buffer) == 1) { // all subsequent words are synonyms
            char *synonym = strdup(buffer);
            cvec_append(synonyms, &synonym);
            cur += strlen(buffer) + 1;
        }
        if (cmap_count(thesaurus) % 1000 == 0) {
            printf(".");
            fflush(stdout);
      }
   }
   printf(".done.\n");
   fclose(fp);
   return thesaurus;
}

/**
 * Simple question loop that prompts the user for a entry, and
 * then looks it up in the thesaurus.  If present, it prints the
 * list of synonyms found.
 */
static void query(CMap *thesaurus)
{
    while (true) {
        char response[1024];
        printf("\nEnter word (RETURN to exit): ");
        if (!read_line(stdin, response, sizeof(response))) break;
        CVector **found = (CVector **)cmap_get(thesaurus, response);
        if (found != NULL) {
            char **cur = cvec_first(*found);
            printf("%s: {%s", response, cur ? *cur : "");
            while (cur && (cur = cvec_next(*found, cur)))
                printf(", %s", *cur);
            printf("}\n");
        } else {
            printf("Nothing found for \"%s\". Try again.\n", response);
        }
    }
}

void thesaurus(const char *filename)
{
    printf("\n----------------- Testing thesaurus ------------------ \n");
    if (!filename) filename = "/afs/ir/class/cs107/samples/assign3/thesaurus.txt" ;
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) error(1, 0,"Could not open thesaurus file named \"%s\"", filename);
    CMap *thesaurus = read_thesaurus(fp);
    query(thesaurus);
    cmap_dispose(thesaurus);
}

int main(int argc, char *argv[])
{
    simple_cmap();
    frequency_test();
//    thesaurus(NULL);
    return 0;
}
