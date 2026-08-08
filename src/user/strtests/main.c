#include "wyrd.h"
#include "sys.h"
#include "syscall.h"
#include "string.h"
#include "io.h"

static void _startTest(const char* msg)
{
   printf("       %s\r", msg);
}

static bool _isEqual(bool cond)
{
   if (cond) {
      printf(" [OK]\n");
   } else {
      printf("[FAIL]\n");
   }
   return cond;
}

void test_printf()
{
   _startTest("printf ... ");
   printf("Expected Formatter: %% [test string] [10]\n");
   printf("  Output Formatter: %% [%s] [%u]\n", "test string", 10);

   const i32 expect = 27;  // "+ return len test: 1234"
   i32 ret = printf("      return len test: %u", 1234);
   printf("\r");
   if (!_isEqual(expect == ret))
      printf(" :: expected %u, got %u", expect, ret);

   _startTest("formatting tests");
   printf("       : format test (%%i): %i\n", 69);
   printf("       : format test (%%i): %i\n", -678);
   printf("       : format test (%%i): %i\n", -2147483648);  // INT_MIN
   printf("       : format test (%%u): %u\n", 1234);
   printf("       : format test (%%s): %s\n", "TEST");
   printf("       : format test (%%c): %c\n", '!');
   printf("       : format test (%%x): %x\n", 0x7F4C);
   printf("       : format test (%%X): %X\n", 0x7F4C);
   printf("       : format test (%%p): %p\n", 0xd3adb33f);
}

void test_strlen()
{
   _startTest("strlen");
   const u32 expect = 11;
   const char* str = "hello world\0";
   u32 len = strlen(str);
   if (!_isEqual(expect == len))
      printf(" :: expected %u, got %u\n", expect, len);
}

void test_strcmp_success()
{
   _startTest("strcmp (equ)");
   const char* str1 = "WYRD_TEST";
   const char* str2 = "WYRD_TEST";
   i32 ret = strcmp(str1, str2);
   if (!_isEqual(ret == 0))
      printf(" :: '%s' ?= '%s', expected 0, got %i\n", str1, str2, ret);
}

void test_strcmp_fail() 
{
   _startTest("strcmp (noeq)");
   const char* str1 = "WYRD_TEST";
   const char* str2 = "WYRD_DIFF";
   i32 ret = strcmp(str1, str2);
   if (!_isEqual(ret != 0))
      printf(" :: '%s' ?= '%s', expected -1, got %i\n", str1, str2, ret);
}

void test_strncmp_success()
{
   _startTest("strncmp (equ)");
   const char* str1 = "WYRD_TEST";
   const char* str2 = "WYRD_DIFF";
   i32 ret = strncmp(str1, str2, 5);
   if (!_isEqual(ret == 0))
      printf(" :: '%s' ?= '%s', expected 0, got %i\n", str1, str2, ret);
}

void test_strncmp_fail()
{
   _startTest("strncmp (not eq)");
   const char* str1 = "WYRD_TEST";
   const char* str2 = "WYRD_DIFF";
   i32 ret = strncmp(str1, str2, 10);
   if (!_isEqual(ret != 0))
      printf(":: '%s' ?= '%s', expected -1, got %i\n", str1, str2, ret);
}

void test_strcpy()
{
   _startTest("strcpy");
   const char* src = "hello world";
   char        dst[64];

   char* p = strcpy(dst, src);
   if (!_isEqual(p == dst && strcmp(src, dst) == 0)) {
      if (p != dst)
         printf(":: return value doesn't match dst pointer\n");
      else if (strcmp(src, dst) != 0)
         printf(":: dst doesn't match src (%s,%s)\n", src, dst);
   }
}

void test_strncpy()
{
   const char* src = "hello world";
   char*       p   = nil;

   _startTest("strncpy [exact size]");
   char buffer[12];
   p = strncpy(buffer, src, 12);
   if (!_isEqual(p == buffer && strcmp(src, buffer) == 0)) {
      if (p != buffer)
         printf(":: return value doesn't match dst pointer\n");
      else if (strcmp(src, buffer) != 0)
         printf(":: dst doesn't match src (%s,%s)\n", src, buffer);
   }

   _startTest("strncpy [big buffer]");
   char bigbuffer[64];
   p = strncpy(bigbuffer, src, 64);
   if (!_isEqual(p == bigbuffer && strcmp(src, bigbuffer) == 0)) {
      if (p != bigbuffer)
         printf(":: return value doesn't match dst pointer\n");
      else if (strcmp(src, bigbuffer) != 0)
         printf(":: dst doesn't match src (%s,%s)\n", src, bigbuffer);
      else if (bigbuffer[60] != '\0')
         printf(":: strncpy didn't continue filling up to COUNT. [60] = %c\n", bigbuffer[60]);
   }
   
   _startTest("strncpy [small buffer]");
   char smallbuffer[5];
   p = strncpy(smallbuffer, src, 5);
   if (!_isEqual(p == smallbuffer && strncmp(src, smallbuffer, 5) == 0)) {
      if (p != smallbuffer)
         printf(":: return value doesn't match dst pointer\n");
      else if (strncmp(src, smallbuffer, 5) != 0)
         printf(":: dst doesn't match (first 5) src (%s,%s)\n", src, smallbuffer);
      else if (smallbuffer[4] == '\0')
         printf(":: dst buffer null terminated %s\n", smallbuffer);
   }
}

void test_strchr()
{
   const char* p;
   const char* str = "hello world";
   _startTest("strchr to find 'e'");
   p = strchr(str, 'e');
   if (!_isEqual(p == str + 1))
      printf(":: strchr() returned %p (%s, %c)\n", p, str, 'e');
   
   _startTest("strchr to find 'w'");
   p = strchr(str, 'w');
   if (!_isEqual(p == str + 6))
      printf(":: strchr() returned %p (%s, %c)\n", p, str, 'w');
   
   _startTest("strchr to find '@' (fail)");
   p = strchr(str, '@');
   if (!_isEqual(p == nil))
      printf(":: strchr() returned %p (%s, %c)\n", p, str, '@');
}

void test_strcat()
{
   const char* answer = "hello world";

   _startTest("strcat");
   char buffer[64];
   strcpy(buffer, "hello");
   
   const char* suffix = " world";
   char* p = strcat(buffer, suffix);
   if (!_isEqual(p == buffer && strcmp(buffer, answer) == 0)) {
      if (p != buffer)
         printf(":: return value doesn't match dst pointer\n");
      else if (strcmp(buffer, answer) != 0)
         printf(":: dst doesn't match src (%s,%s)\n", buffer, answer);
   }
}

int main()
{
   printf("== START STRING.H TESTS ==\n");
   test_printf();
   test_strlen();   
   test_strcmp_success();
   test_strcmp_fail();
   test_strncmp_success();
   test_strncmp_fail();
   test_strcpy();
   test_strncpy();
   test_strchr();
   test_strcat();
   printf("== END STRING.H TESTS ==\n");

   return 0;
}