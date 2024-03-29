#include "fdbox.h"
#include "lib/applet.h"
#include "lib/args.h"
#include "lib/environ.h"
#include "lib/strextra.h"
#include "lib/tc202/stdextra.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef bool (*function_test)();

bool test(const char *message, function_test unittest);
bool test_str_list();
bool test_args();
bool test_args_split();
bool test_applets();
bool test_strings();
bool test_prompts();
bool test_var_expand();

int main(int argc, char *argv[]) {
        bool ok = true;

        ok &= test("applets", test_applets);
        ok &= test("args", test_args);
        ok &= test("strings", test_strings);
        ok &= test("prompts", test_prompts);
        ok &= test("variables expansion", test_var_expand);
        ok &= test("argument split", test_args_split);
        ok &= test_str_list();
        UNUSED(argc);
        UNUSED(argv);
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool test(const char *message, function_test unittest) {
        printf("Running test for %s:\n", message);
        if (!unittest()) {
                printf(" * %s -  FAIL\n", message);
                return false;
        }
        return true;
}

//////////////////////////
// assert like functions.
// TODO : if we convert this to a macro, we will have line numbers

bool verify_ptr_equals(const void *arg1, const void *arg2, const char *message) {
        printf(" * Checking %s: ", message);
        if (arg1 == arg2) {
                printf("OK\n");
                return true;
        }
        printf("FAIL (expecting %p, got %p)\n", arg1, arg2);
        return false;
}

bool verify_int_equals(int arg1, int arg2, const char *message) {
        printf(" * Checking %s: ", message);
        if (arg1 == arg2) {
                printf("OK\n");
                return true;
        }
        printf("FAIL (expecting %d, got %d)\n", arg1, arg2);
        return false;
}

bool verify_string_equals(const char *arg1, const char *arg2, const char *message) {
        printf(" * Checking %s: ", message);
        if (strcmp(arg1, arg2) == 0) {
                printf("OK\n");
                return true;
        }
        printf("FAIL (expecting %s, got %s)\n", arg1, arg2);
        return false;
}

///////////////////////////////////////////
bool test_args() {
#define MAX_ARGV 100
        bool ok = true;
        char c2[256], *c3;
        const char *argv[MAX_ARGV];
        size_t argc = 0;
        bool parsing_ok;

        // basic argument parsing - can we create argc/argv?
        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) */
        memset(c2, 'x', 256);
        c3 = "";
        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy) */
        strcpy(c2, c3);
        parsing_ok = command_split_args(c2, &argc, argv, MAX_ARGV);
        ok |= parsing_ok && verify_int_equals(0, argc, "no arguments");

        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) */
        memset(c2, 'x', 256);
        c3 = "asd asd";
        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy) */
        strcpy(c2, c3);
        parsing_ok = command_split_args(c2, &argc, argv, MAX_ARGV);
        ok |= parsing_ok && verify_int_equals(2, argc, "2 arguments");

        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) */
        memset(c2, 'x', 256);
        c3 = "111 222 333 44             555 666";
        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy) */
        strcpy(c2, c3);
        parsing_ok = command_split_args(c2, &argc, argv, MAX_ARGV);
        ok |= parsing_ok && verify_int_equals(6, argc, "6 arguments, with spaces");

        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) */
        memset(c2, 'x', 256);
        c3 = "dir /w";
        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy) */
        strcpy(c2, c3);
        parsing_ok = command_split_args(c2, &argc, argv, MAX_ARGV);
        ok |= parsing_ok && verify_int_equals(2, argc, "2 args - dir /w");

        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) */
        memset(c2, 'x', 256);
        c3 = "dir /w /w";
        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy) */
        strcpy(c2, c3);
        parsing_ok = command_split_args(c2, &argc, argv, MAX_ARGV);
        ok |= parsing_ok && verify_int_equals(3, argc, "3 args - dir /w /w");

        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) */
        memset(c2, 'x', 256);
        c3 = "dir /w /2";
        /* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy) */
        strcpy(c2, c3);
        parsing_ok = command_split_args(c2, &argc, argv, MAX_ARGV);
        ok |= parsing_ok && verify_int_equals(3, argc, "3 args - dir /w /2");
        return ok;
}

bool test_args_split() {
        bool ok = true;
        struct command_args args;
        int rc;

        rc = command_args_split("", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(0, args.argc, "no arguments");
        command_args_free(&args);

        rc = command_args_split("x", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(1, args.argc, "single argument");
        command_args_free(&args);

        rc = command_args_split("asd asd", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(2, args.argc, "2 arguments");
        command_args_free(&args);

        rc = command_args_split("one, two three!", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(3, args.argc, "3 words");
        command_args_free(&args);

        rc = command_args_split("111 222 333 44             555 666", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(6, args.argc, "6 arguments - spaces");
        command_args_free(&args);

        rc = command_args_split("'one', 'two'", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(2, args.argc, "2 args, quoting");
        command_args_free(&args);

        rc = command_args_split("\"1 2\", 'three'", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(2, args.argc, "2 args, double quoting");
        command_args_free(&args);

        rc = command_args_split("'t \" a", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(1, args.argc, "1 arg, containing open quote");
        command_args_free(&args);

        rc = command_args_split("'one', \"two's three \"", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(2, args.argc, "2 arg, containing quote");
        command_args_free(&args);

        rc = command_args_split("'one', \"two's three \" - four", &args);
        ok |= rc == EXIT_SUCCESS &&
              verify_int_equals(4, args.argc, "3 words and '-', containing quote");
        command_args_free(&args);

        rc = command_args_split("dir /w", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(2, args.argc, "2 args - dir /w");
        command_args_free(&args);

        rc = command_args_split("dir /w /w", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(3, args.argc, "3 args - dir /w /w");
        command_args_free(&args);

        rc = command_args_split("dir /w /2", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(3, args.argc, "3 args - dir /w /2");
        command_args_free(&args);

        rc = command_args_split("a=12", &args);
        ok |= rc == EXIT_SUCCESS && verify_int_equals(3, args.argc, "3 args - a=12");
        command_args_free(&args);

        return ok;
}

bool test_var_expand() {
        char orig[256];
        char parsed[256];
        bool ok = true;

        strcpy(orig, "");
        expand_string(orig, parsed, sizeof(parsed));
        ok |= verify_string_equals(orig, parsed, "empty string");

        strcpy(orig, "hello");
        expand_string(orig, parsed, sizeof(parsed));
        ok |= verify_string_equals(orig, parsed, "no vars");

        strcpy(orig, "[%%]");
        expand_string(orig, parsed, sizeof(parsed));
        ok |= verify_string_equals("[]", parsed, "empty var name");

        setenv("FOOBAR", "***", 1);
        strcpy(orig, "%FOOBAR%");
        expand_string(orig, parsed, sizeof(parsed));
        ok |= verify_string_equals("***", parsed, "normal var");

        setenv("ZOIP", "abc", 1);
        strcpy(orig, "%FOOBAR%%ZOIP%");
        expand_string(orig, parsed, sizeof(parsed));
        ok |= verify_string_equals("***abc", parsed, "2 normal vars");

        strcpy(orig, "%FOOBAR%[%ZOIP%]");
        expand_string(orig, parsed, sizeof(parsed));
        ok |= verify_string_equals("***[abc]", parsed, "2 normal vars (mixed)");

        strcpy(orig, "%FOOBAR%");
        expand_string(orig, parsed, 1);
        ok |= verify_string_equals("", parsed, "1 var, truncated");

        strcpy(orig, "%FOOBAR%%ZOIP%");
        expand_string(orig, parsed, 7);
        ok |= verify_string_equals("***abc", parsed, "2 vars un-truncated");

        expand_string(orig, parsed, 6);
        ok |= verify_string_equals("***", parsed, "2 vars truncated");

        return ok;
}

/////////// applets

/* clang-format off */
int applet1(int argc, char *argv[]) { return EXIT_SUCCESS; UNUSED(argc); UNUSED(argv); }

int applet2(int argc, char *argv[]) { return EXIT_SUCCESS; UNUSED(argc); UNUSED(argv); }

int applet3(int argc, char *argv[]) { return EXIT_SUCCESS; UNUSED(argc); UNUSED(argv); }
/* clang-format on */

bool test_applets() {
        /* clang-format off */
        struct applet commands[] = {
                {NULL, &applet1, "applet1"},
                {NULL, &applet2, "applet2"},
                {NULL, NULL, NULL}
        };
        /* clang-format on */
        bool ok = true;
        struct applet *c;

        c = find_applet(CASE_INSENSITVE, "applet1", commands);
        ok &= verify_ptr_equals(c->handler, applet1, "applet 1 available");
        c = find_applet(CASE_SENSITVE, "applet2", commands);
        ok &= verify_ptr_equals(c->handler, applet2, "applet 2 available");
        c = find_applet(CASE_INSENSITVE, "applet3", commands);
        ok &= verify_ptr_equals(c, NULL, "applet 3 un-available");
        c = find_applet(CASE_INSENSITVE, "ApPleT2", commands);
        ok &= verify_ptr_equals(c->handler, applet2, "applet 2 (ApPleT2) found (case insensitive)");
        return ok;
}

bool test_string_lower() {
        const char *c;
        char str[100];
        bool ok = true;

        c = str_to_lower(strcpy(str, "NULL"));
        ok &= verify_string_equals(c, "null", "normal lower");
        c = str_to_lower(strcpy(str, "qwertyuiopQWERTYUIOP"));
        ok &= verify_string_equals(c, "qwertyuiopqwertyuiop", "normal lower");
        c = str_to_lower(strcpy(str, ""));
        ok &= verify_string_equals(c, strcpy(str, ""), "lower to empty string");
        c = str_to_lower(strcpy(str, "123456"));
        ok &= verify_string_equals(c, "123456", "numbers");

        return ok;
}

bool test_str_prefix() {
        const char *c;
        char str[100];
        bool ok = true;

        c = strcpy(str, "Lorem Ipsum");
        ok &= verify_int_equals(str_is_prefix(c, "L"), 1, "string starts with string (1)");
        ok &= verify_int_equals(str_is_prefix(c, "Lorem"), 1, "string starts with string (5)");
        ok &= verify_int_equals(str_is_prefix(c, "XXX"), 0, "string does not start with string");
        ok &= verify_int_equals(str_is_prefix(c, "Lorem Ipsum"), 1, "string starts with itself");
        ok &= verify_int_equals(str_is_prefix(c, ""), 1, "string starts with itself");

        return ok;
}

bool test_str_del() {
        bool ok = true;
        char c[1000];

        strcpy(c, "hello");
        str_del_char(c, 0);
        ok &= verify_string_equals("ello", c, "Deleting first char");

        strcpy(c, "hello");
        str_del_char(c, 0);
        ok &= verify_string_equals("ello", c, "Deleting first char");

        strcpy(c, "hello");
        str_del_char(c, strlen(c) - 1);
        ok &= verify_string_equals("hell", c, "Deleting last char");

        strcpy(c, "hello");
        str_del_char(c, 3);
        ok &= verify_string_equals("helo", c, "Deleting some char");

        return ok;
}

bool test_str_ins() {
        bool ok = true;
        char c[1000];

        c[0] = 0;
        str_ins_char(c, 1000, 'Y', 0);
        ok &= verify_string_equals("Y", c, "insert a char to empty string");

        strcpy(c, "hello");
        str_ins_char(c, 1000, '!', 5);
        ok &= verify_string_equals("hello!", c, "insert a char at the end");

        strcpy(c, "hello");
        str_ins_char(c, 1000, '!', 0);
        ok &= verify_string_equals("!hello", c, "insert a char at the beginnig");

        strcpy(c, "hello");
        str_ins_char(c, 1000, 'A', 1);
        ok &= verify_string_equals("hAello", c, "insert in the middle");

        strcpy(c, "hello");
        str_ins_char(c, 6, '!', 10000);
        ok &= verify_string_equals("hello", c, "insert an out of bound char - silently fail");

        strcpy(c, "124");
        str_ins_char(c, 5, '3', 2);
        ok &= verify_string_equals("1234", c, "insert a char in the middle (exact buffer size)");

        strcpy(c, "13");
        str_ins_char(c, 3, '2', 1);
        ok &= verify_string_equals("123", c, "insert a char and the end (truncated)");

        return ok;
}

bool test_str_char_suffix() {
        bool ok = true;

        ok &= verify_int_equals(str_ends_with("123", '3'), 1, "String ends (digit)");
        ok &= verify_int_equals(str_ends_with("3", '3'), 1, "String ends (digit)");
        ok &= verify_int_equals(str_ends_with("333", '3'), 1, "String ends (digit)");
        ok &= verify_int_equals(str_ends_with("aaa", 'a'), 1, "String ends (alpha)");
        ok &= verify_int_equals(str_ends_with("cba", 'a'), 1, "String ends (alpha)");
        ok &= verify_int_equals(str_ends_with("a", 'a'), 1, "String ends (alpha)");
        ok &= verify_int_equals(str_ends_with("A", 'a'), 0, "String not ends (alpha)");
        ok &= verify_int_equals(str_ends_with("Aa-", 'a'), 0, "String not ends (alpha)");
        ok &= verify_int_equals(str_ends_with("", 'a'), 0, "String not ends (alpha)");
        return ok;
}

bool test_file_basename() {
        char str[100];
        const char *c;
        bool ok = true;

        strcpy(str, "file.txt");
        c = file_base_name(str);
        ok &= verify_string_equals(c, "file.txt", "full file name");

        strcpy(str, "c:\\windows\\drivers\\file.txt");
        c = file_base_name(str);
        ok &= verify_string_equals(c, "file.txt", "windows path");

        strcpy(str, "\\windows\\drivers\\file.txt");
        c = file_base_name(str);
        ok &= verify_string_equals(c, "file.txt", "windows path (no drive)");

        strcpy(str, "windows\\drivers\\file.txt");
        c = file_base_name(str);
        ok &= verify_string_equals(c, "file.txt", "windows path (relative)");

        strcpy(str, "..\\..\\windows\\file.txt");
        c = file_base_name(str);
        ok &= verify_string_equals(c, "file.txt", "windows path (relative, parent)");

        strcpy(str, "/tmp/file.txt");
        c = file_base_name(str);
        ok &= verify_string_equals(c, "file.txt", "unix path");

        strcpy(str, "tmp/file.txt");
        c = file_base_name(str);
        ok &= verify_string_equals(c, "file.txt", "unix path (relative)");

        strcpy(str, "../../../tmp/file.txt");
        c = file_base_name(str);
        ok &= verify_string_equals(c, "file.txt", "unix path (relative, parent)");

        strcpy(str, "c:/test/messedup/..\\windows.exe.manifest");
        c = file_base_name(str);
        ok &= verify_string_equals(c, "windows.exe.manifest", "mixed path (unix/windows, parent)");

        return ok;
}

bool test_file_extensions() {
        char str[100];
        const char *c;
        bool ok = true;

        strcpy(str, "file.txt");
        c = file_get_extension(str);
        ok &= verify_string_equals(c, "txt", "getting normal 8.3 extention");

        strcpy(str, "/var/lib/file.txt");
        c = file_get_extension(str);
        ok &= verify_string_equals(c, "txt", "getting normal 8.3 extention + path");

        strcpy(str, "/var/lib/file.text");
        c = file_get_extension(str);
        ok &= verify_string_equals(c, "text", "getting normal long extention + path");

        strcpy(str, "/var/lib/file.txt.text.file.blabla");
        c = file_get_extension(str);
        ok &= verify_string_equals(c, "blabla", "getting very long extention + path");

        return ok;
}

bool test_strings() {
        bool ok = true;
        ok &= test_string_lower();
        ok &= test_str_prefix();
        ok &= test_str_del();
        ok &= test_str_ins();
        ok &= test_str_char_suffix();
        ok &= test_file_basename();
        ok &= test_file_extensions();
        return ok;
}

bool test_str_list() {
        bool ok = true;
        struct str_list list;
        char *c1;
        const char *c2;

        str_list_init(&list, 5);
        c1 = str_list_pop(&list);
        ok &= verify_ptr_equals(c1, NULL, "pop from empty list");

        str_list_push(&list, "aaa");
        c1 = str_list_pop(&list);
        ok &= verify_string_equals(c1, "aaa", "push and pop");
        free(c1);
        c1 = str_list_pop(&list);
        ok &= verify_ptr_equals(c1, NULL, "pop after push");

        str_list_push(&list, "bbb");
        c2 = str_list_get(&list, 0);
        ok &= verify_string_equals(c2, "bbb", "get from a single item list");
        c1 = str_list_pop(&list);
        ok &= verify_string_equals(c1, "bbb", "pop after fetch");
        free(c1);
        c1 = str_list_pop(&list);
        ok &= verify_ptr_equals(c1, NULL, "pop after push, and get");

        str_list_push(&list, "111");
        str_list_push(&list, "222");
        str_list_push(&list, "333");
        str_list_push(&list, "444");
        c1 = str_list_pop(&list);
        free(c1);
        c1 = str_list_pop(&list);
        free(c1);
        c1 = str_list_pop(&list);
        free(c1);
        c1 = str_list_pop(&list);
        ok &= verify_string_equals(c1, "111", "pop after fetch");
        free(c1);
        c1 = str_list_pop(&list);
        ok &= verify_ptr_equals(c1, NULL, "pop after 4  pushes");

        str_list_push(&list, "111");
        str_list_push(&list, "222");
        str_list_push(&list, "333");
        str_list_push(&list, "444");
        str_list_push(&list, "555");

        c2 = str_list_get(&list, 0);
        ok &= verify_string_equals(c2, "555", "fetch from 1/5 list");
        c2 = str_list_get(&list, 1);
        ok &= verify_string_equals(c2, "444", "fetch from 2/5 list");
        c2 = str_list_get(&list, 2);
        ok &= verify_string_equals(c2, "333", "fetch from 3/5 list");
        c2 = str_list_get(&list, 3);
        ok &= verify_string_equals(c2, "222", "fetch from 4/5 list");
        c2 = str_list_get(&list, 4);
        ok &= verify_string_equals(c2, "111", "fetch from 5/5 list");

        str_list_push(&list, "666");
        c2 = str_list_get(&list, 0);
        ok &= verify_string_equals(c2, "666", "fetch from 6/5 list");
        c2 = str_list_get(&list, 1);
        ok &= verify_string_equals(c2, "555", "fetch from 6/5 list");
        c2 = str_list_get(&list, 2);
        ok &= verify_string_equals(c2, "444", "fetch from 6/5 list");
        c2 = str_list_get(&list, 3);
        ok &= verify_string_equals(c2, "333", "fetch from 6/5 list");
        c2 = str_list_get(&list, 4);
        ok &= verify_string_equals(c2, "222", "fetch from 6/5 list");
        c2 = str_list_get(&list, 5);
        ok &= verify_ptr_equals(c2, NULL, "fetch out of bounds");

        c1 = str_list_pop(&list);
        ok &= verify_string_equals(c1, "666", "pop from 6/5 list");
        free(c1);
        c1 = str_list_pop(&list);
        ok &= verify_string_equals(c1, "555", "pop from 6/5 list");
        free(c1);
        c1 = str_list_pop(&list);
        ok &= verify_string_equals(c1, "444", "pop from 6/5 list");
        free(c1);
        c1 = str_list_pop(&list);
        ok &= verify_string_equals(c1, "333", "pop from 6/5 list");
        free(c1);
        c1 = str_list_pop(&list);
        ok &= verify_string_equals(c1, "222", "pop from 6/5 list");
        free(c1);
        c1 = str_list_pop(&list);
        ok &= verify_ptr_equals(c1, NULL, "pop after 6/5 pushes");
        free(c1);

        str_list_free(&list);

        return ok;
}

bool test_prompt_letter(char l, char k) {
        bool ok = true;
        char p1[16], p2[16];
        char message[100];
        int i;

        snprintf(p1, 16, "$%c", l);
        get_prompt(p1, p2, 256);
        i = strlen(p2);
        snprintf(message, 100, "prompt size for $%c", l);
        ok &= verify_int_equals(i, 1, message);
        snprintf(message, 100, "char $%c = %d", l, k);
        ok &= verify_int_equals(p2[0], k, message);
        return ok;
}

bool test_prompts() {
        bool ok = true;
        char prompt[100], message[200];

        ok &= test_prompt_letter('$', '$');
        ok &= test_prompt_letter('_', '\n');
        ok &= test_prompt_letter('a', '&');
        ok &= test_prompt_letter('b', '|');
        ok &= test_prompt_letter('c', '(');
        ok &= test_prompt_letter('e', 27);
        ok &= test_prompt_letter('g', '>');
        ok &= test_prompt_letter('h', 8);
        ok &= test_prompt_letter('l', '<');
        ok &= test_prompt_letter('q', '=');
        ok &= test_prompt_letter('s', ' ');

        /* TODO */
        /* date */
        /* time */
        /* drive */

        get_prompt("$V", prompt, 100);
        ok &= verify_int_equals(strlen(prompt), strlen(FDBOX_VERSION_STR),
                                "testing prompt version length");
        snprintf(message, 200, "testing for prompt version, get %s, expected %s", prompt,
                 FDBOX_VERSION_STR);
        ok &= verify_string_equals(prompt, FDBOX_VERSION_STR, message);

        return ok;
}
