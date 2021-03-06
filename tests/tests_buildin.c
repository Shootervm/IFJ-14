#include "tests.h"

void test_buildin()
{
	int failures=0;

    printf("*** Testing of builtin ***\n");
    failures += test_length();
    failures += test_copy();
    failures += test_find();
    failures += test_sort();


    if(failures)
	{
		printf("\n\n!!! ERROR has occured during testing !!!\n\n");
	}
	else
	{
		printf("\n\nOK. Everything is fine.\n\n");
	}
}

int test_length()
{
    int failures = 0;
    const int test_count = 4;

    cstring *Vojto = cstr_create_str("Ahoj Karle");
   printf("\n*** Testing buildin length ***\n");

    if (length(Vojto) != 10) {
        print_cstr_all(Vojto);
        failures++;
    }

    cstr_append_str(Vojto, "1234");

    if (length(Vojto) != 14) {
        print_cstr_all(Vojto);
        failures++;
    }

    cstr_assign_str(Vojto, "Your penis smells like shit.");


    if (length(Vojto) != 28) {
        print_cstr_all(Vojto);
        failures++;
    }

    cstr_assign_str(Vojto, "");

    if (length(Vojto) != 0) {
        print_cstr_all(Vojto);
        failures++;
    }

    print_result(test_count, failures);

    return failures;
}

int test_copy()
{
    int failures = 0;
    const int test_count = 4;

    cstring *Vojto = cstr_create_str("Ahoj Karle");
    cstring *Albert = copy(Vojto, 2, 6);

    printf("\n*** Testing buildin copy ***\n");

    if (strcmp(Albert->str, "hoj Ka") != 0) {
        print_cstr_all(Albert);
        failures++;
    }

    cstr_clear(Albert);
    Albert = copy(Vojto, 1, 2);

    if (strcmp(Albert->str, "Ah") != 0) {
        print_cstr_all(Albert);
        failures++;
    }

    cstr_clear(Albert);
    Albert = copy(Vojto, 6, 2);

    if (strcmp(Albert->str, "Ka") != 0) {
        print_cstr_all(Albert);
        failures++;
    }

    cstr_clear(Vojto);
    cstr_clear(Albert);
    Albert = copy(Vojto, 1, 10);
    if (strcmp(Albert->str, "") != 0) {
        print_cstr_all(Albert);
        failures++;
    }

    print_result(test_count, failures);

    return failures;

}

//Mem leaks in kmp_substring

int test_find()
{
    int failures = 0;
    const int test_count = 5;
    printf("\n*** Testing buildin find ***\n");

    cstring *Vojto = cstr_create_str("Ahoj Karle      1");
    cstring *Albert = cstr_create_str("le");

    if (find(Vojto, Albert) != 9) {
		printf("prve\n");
        print_cstr_all(Albert);
        failures++;
    }

    debug("\n*** za prvym ***\n");
    cstr_assign_str(Albert, " ");

    if (find(Vojto, Albert) != 5) {
		printf("druhe\n");
        print_cstr_all(Albert);
        failures++;
    }

    debug("\n*** za druhym ***\n");

    cstr_assign_str(Albert, "1");
    if (find(Vojto, Albert) != 17) {
		printf("tretie\n");
        print_cstr_all(Albert);
        failures++;
    }

    debug("\n*** za tretim ***\n");

    cstr_assign_str(Vojto, "");

    if (find(Vojto, Albert) != 0) {
	printf("stvrte\n");
        print_cstr_all(Albert);
        failures++;
    }

    debug("\n*** za stvrtym ***\n");

    cstr_assign_str(Vojto,"Ahoj Kikus, nasi isli na vikend, prec!");
    cstr_assign_str(Albert, "");

    if (find(Vojto, Albert) != 1) {
        printf("piate\n");
        print_cstr_all(Albert);
        failures++;
    }

    debug("\n*** za piatym ***\n");

    print_result(test_count, failures);

    return failures;
}

int test_sort()
{
    int failures = 0;
    const int test_count = 4;
    printf("\n*** Testing buildin sort ***\n");
    cstring *Vojto = cstr_create_str("abcABCdefDEF");

    cstring *Albert = Vojto;
    sort(&Albert);

    if (strcmp(Albert->str, "ABCDEFabcdef") != 0) {
        print_cstr_all(Vojto);
        failures++;
    }

    cstr_clear(Vojto);
    cstr_append_str(Vojto, "123987");
    cstring *Pavel = Vojto;
    sort(&Pavel);

    if ((strcmp(Pavel->str, "123789") != 0)) {
        print_cstr_all(Vojto);
        failures++;
    }

    cstr_clear(Vojto);
    cstr_append_str(Vojto, "1234abCDefGH");
    cstring *Adam = Vojto;
    sort(&Adam);

    if ((strcmp(Adam->str, "1234CDGHabef") != 0)) {
        print_cstr_all(Vojto);
        failures++;
    }

    print_result(test_count, failures);


    return failures;
}
