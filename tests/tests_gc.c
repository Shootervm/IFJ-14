/**
 *  @name   tests_cstring.c
 *  @brief  Tests for cstring
 *  @author Vojtech Mašek (xmasek15)
 ****************************************************************************/

#include "tests.h"



int test_gc(void)
{
	return test_gc_complex();
}


int test_gc_complex(void)
{
	static int errors = 1;


do{ /// do{...}while(0); 	is for testing hack

    int *pepa_int = gc_malloc("pepik", sizeof(int));
    float *pepa_float = gc_malloc("pepik", sizeof(float));

    int *karel_int = gc_malloc("karlik", sizeof(int));
    float *karel_float = gc_malloc("karlik", sizeof(float));

    if (pepa_int == NULL || pepa_float == NULL ||
        karel_int == NULL || karel_float == NULL)
        goto cleanup;   // SOME OF THEM COULD'VE BEEN ALLOCATED SUCCESSFULLY

    gc_free("karlik");  // KAREL GAVE UP ALL MEMORY HE HAS ALLOCATED
    *pepa_int = 42;     // PEPA CAN STILL USE HIS MEMORY

cleanup:
    gc_free("pepik");
    gc_free("karlik");  // ( TRYING TO DEALLOCATE NEVER-ALLOCATED OR ALREADY
    gc_free("jarda");   //   DEALLOCATED MEMORY DOESN'T DO HARM )

    cstring *cstr = gc_malloc("test_vojto", sizeof(cstring));
    cstr->str = (char*)"0123";
    cstr->size = cstr->tab_size = strlen(cstr->str);
    gc_free("test_vojto");

    cstring *cstr1 = cstr_create_str("0123");

    printf("\n\nGC Printing cstrings:\n\n");
    print_cstr(cstr1);
    printf("\n");
    print_cstr_all(cstr1);
	printf("\n");

errors=0;
}while(0);

	if(errors)
	{
		printf("\n\n!!! ERROR has occured during testing !!!\n\n");

	}
	else
	{
		printf("\n\nOK. Everything is fine.\n\n");
	}


	return errors;
}
