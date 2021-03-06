/**
 * @file    interpreter.c
 * @brief   Implementation of interpreter
 * @author  Vojtech Mašek (xmasek15), Tomas Paulus (xpaulu01)
 *******************************************************************/

#include <ctype.h>

#include "debug.h"
#include "interpreter.h"
#include "builtin.h"

#define GL_MIN_SIZE 16

static int get_line(char **buffer)
{
    static unsigned size = 0;
    int character;
    unsigned read = 0;
    char *temp;

    if (buffer == NULL)
        return INTERNAL_ERROR;

    while (true) {
        character = getchar();
        if (read == size) {
            if ((temp = realloc(*buffer, size + GL_MIN_SIZE)) == NULL)
                return INTERNAL_ERROR;
            *buffer = temp;
            size += GL_MIN_SIZE;
        }
        if (character == '\n' || character == EOF) {
            (*buffer)[read] = '\0';
            break;
        }
        (*buffer)[read] = character;
        read++;
    }

    return SUCCESS;
}

int interpret(Instruction *item, Variables *globals)
{
	if(!item || !globals)
	{
		debug("Null pointer in given params\n");
		return INTERNAL_ERROR;
	}

	int err_code;
	char *buffer = NULL;
	Stack calcs;
	stack_init(&calcs, VALUE_STACK);
	Stack locals;
	stack_init(&locals, VALUE_STACK);
	Stack instructions;
	stack_init(&instructions, INSTR_STACK);


	if((err_code = interpret_loop(item, &calcs, &locals, &instructions, globals, &buffer)) != SUCCESS)
	{
		debug("INTERPRET LOOP returned NON success errcode.\n");
	}
	debug("Number of items in calcs: %u, locals: %u, instrs: %u\n", calcs.count, locals.count, instructions.count);
	stack_free(&calcs);
	stack_free(&locals);
	stack_free(&instructions);
	free(buffer);

	return err_code; ///returns err_code given from interpreter_loop()
}


int interpret_loop(Instruction *item, Stack *calcs, Stack *locals, Stack *instructions, Variables *globals, char **buffer)
{

	Value values[2];
	Type  types[3];
	Value result = {.inited = true};
	char *endptr;

	while(item)
	{
		switch (item->instruction)
		{

		case I_ASSIGN:
			debug("I_ASSIGN\n");
			if(!globals)
			{
				debug("Globals field not given.\n");
				return INTERNAL_ERROR;
			}
			///                         Stack operations
			CALCS_STACK_OPERATIONS_RESULT();

			///                         Instruction operations
			if(item->index < 0)																		/// index indicates locals stack operation
			{
				debug("I_ASSIGN - locals stack\n");
				if (stack_update(locals, (-(item->index+1)), (int)types[0], &result) == INTERNAL_ERROR)
				{
					debug("I_ASSIGN - locals stack error.\n");
					return INTERNAL_ERROR;
				}
			}
			else
			{
				debug("I_ASSIGN - globals field\n");
				if (variables_value_write(globals, &result, item->index) == INTERNAL_ERROR)	/// index indicates global variables field operation
				{
					debug("I_ASSIGN - globals field error.\n");
					return INTERNAL_ERROR;
				}
			}
			debug("Assigning to index %d (type %u)\n", item->index, types[0]);
			item = item->next_instruction;
			break;

		case I_PUSH:
			debug("I_PUSH\n");
			///                         Value operations
			if(item->index < 0)																		/// index indicates local stack operation
			{
				debug("I_PUSH - locals stack\n");
				if (stack_index(locals, (-(item->index+1)), (int *)&types[0], &result) == INTERNAL_ERROR) ///
				{
					debug("I_PUSH - locals stack_index error.\n");
					return INTERNAL_ERROR;
				}
			}
			else
			{
				debug("I_PUSH - globals field\n");
				if (variables_value_read(globals, &types[0], &result, item->index) == INTERNAL_ERROR)		/// index indicates global variables field operation
				{
					debug("I_PUSH - globals variables_value_read error.\n");
					return INTERNAL_ERROR;
				}
			}
			debug("I_PUSH - index %d (type %u)\n", item->index, types[0]);
			///ACHTUNG! do not pop or change locals or globals
			if (!result.inited)
			{
				debug("I_PUSH - uninitialized\n");
				return RUNTIME_UNINITIALIZED;
			}
			///                         Instruction operations
			stack_push(calcs, types[0], &result);

			item = item->next_instruction;
			break;

		case I_PASS:
			debug("I_PASS\n");
			///                         Stack operations
			CALCS_STACK_OPERATIONS_RESULT();

			///                         Instruction operations
			debug("I_PASS - pushing to locals stack\n");
			if (stack_push(locals, (int)types[0], &result) == INTERNAL_ERROR)
			{
				debug("I_PASS - locals stack push error.\n");
				return INTERNAL_ERROR;
			}


			item = item->next_instruction;
			break;

		case I_PREP:
			debug("I_PREP - pushing to locals stack (type %u)\n", item->index);
			result.inited = false;
			if (stack_push(locals, item->index, &result) == INTERNAL_ERROR)
			{
				debug("I_PREP - locals stack push error.\n");
				return INTERNAL_ERROR;
			}


			item = item->next_instruction;
			break;

		case I_CALL:
			debug("I_CALL\n");

			if (stack_push(instructions, 0, &item->next_instruction) == INTERNAL_ERROR)
			{
				debug("I_CALL - instructions stack push error.\n");
				return INTERNAL_ERROR;
			}

			item = item->alt_instruction;
			break;

		case I_HALT:
			debug("I_HALT\n");

			if (stack_index(locals, 0, (int *)&types[0], &result) == INTERNAL_ERROR) ///get the top of locals (it is return code)
			{
				debug("I_HALT - locals stack_index (top) error.\n");
				return INTERNAL_ERROR;
			}

			if (stack_push(calcs, types[0], &result) == INTERNAL_ERROR)
			{
				debug("I_HALT - calcs stack push error.\n");
				return INTERNAL_ERROR;
			}
			
			if(stack_popping_spree(locals, item->index)==INTERNAL_ERROR) ///number of requied pop is in index
			{
				debug("I_HALT - Popping spree locals error.\n");
				return INTERNAL_ERROR;
			}

			if(stack_top(instructions, NULL, &item) == SUCCESS)
			{
				debug("I_HALT - continue.\n");
				stack_pop(instructions);
			}
			else
			{
				debug("I_HALT - Successful end of interpretation.\n");
				item = NULL;
			}

			break;

		case I_ADD:
			debug("I_ADD\n");
			///                         Stack operations
			CALCS_STACK_OPERATIONS();

			///                         Instruction operations
			if ((types[0] == TYPE_INT) && (types[1] == TYPE_INT))
			{
				debug("I_ADD for ints\n");
				result.data.integer = values[1].data.integer + values[0].data.integer;
				stack_push(calcs, TYPE_INT, &result);
			}
			else if ((types[0] == TYPE_REAL) && (types[1] == TYPE_REAL))
			{
				debug("I_ADD for reals\n");
				result.data.real = values[1].data.real + values[0].data.real;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else if ((types[0] == TYPE_INT) && (types[1] == TYPE_REAL))
			{
				debug("I_ADD for real + int\n");
				result.data.real = values[1].data.real + values[0].data.integer;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else if ((types[0] == TYPE_REAL) && (types[1] == TYPE_INT))
			{
				debug("I_ADD for int + real\n");
				result.data.real = values[1].data.integer + values[0].data.real;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else if ((types[0] == TYPE_STRING) && (types[1] == TYPE_STRING))				/// WARNING: !!!!!!!!!!!! TAKE BIG CARE WHEN RECASTING CSTRING* TO VOID*
			{
				debug("I_ADD for strings\n");
				result.data.string = cstr_append_cstr(cstr_create_cstr(values[1].data.string), values[0].data.string);
				stack_push(calcs, TYPE_STRING, &(result));
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_SUB:
			debug("I_SUB\n");
			///                         Stack operations
			CALCS_STACK_OPERATIONS();

			///                         Instruction operations
			if ((types[0] == TYPE_INT) && (types[1] == TYPE_INT))
			{
				debug("I_SUB for ints\n");
				result.data.integer = values[1].data.integer - values[0].data.integer;
				stack_push(calcs, TYPE_INT, &result);
			}
			else if ((types[0] == TYPE_REAL) && (types[1] == TYPE_REAL))
			{
				debug("I_SUB for reals\n");
				result.data.real = values[1].data.real - values[0].data.real;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else if ((types[0] == TYPE_REAL) && (types[1] == TYPE_INT))
			{
				debug("I_SUB for int - real\n");
				result.data.real = values[1].data.integer - values[0].data.real;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else if ((types[0] == TYPE_INT) && (types[1] == TYPE_REAL))
			{
				debug("I_SUB for real - int\n");
				result.data.real = values[1].data.real - values[0].data.integer;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_MUL:
			debug("I_MUL\n");
			///                         Stack operations
			CALCS_STACK_OPERATIONS();

			///                         Instruction operations
			if ((types[0] == TYPE_INT) && (types[1] == TYPE_INT))
			{
				debug("I_MULTIPLY - INT\n");
				result.data.integer = values[1].data.integer * values[0].data.integer;
				stack_push(calcs, TYPE_INT, &result);
			}
			else if ((types[0] == TYPE_REAL) && (types[1] == TYPE_REAL))
			{
				debug("I_MULTIPLY - DOUBLE\n");
				result.data.real = values[1].data.real * values[0].data.real;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else if ((types[0] == TYPE_INT) && (types[1] == TYPE_REAL))
			{
				debug("I_MULTIPLY for real * int\n");
				result.data.real = values[1].data.real * values[0].data.integer;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else if ((types[0] == TYPE_REAL) && (types[1] == TYPE_INT))
			{
				debug("I_MULTIPLY for int * real\n");
				result.data.real = values[1].data.integer * values[0].data.real;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else
			{
				debug("Invalid types passed to instruction: %u %u\n", types[0], types[1]);
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_DIV:
			debug("I_DIV\n");
			///                         Stack operations
			CALCS_STACK_OPERATIONS();

			///                         Instruction operations
			if ((types[0] == TYPE_INT) && (types[1] == TYPE_INT))
			{
				debug("I_DIV - INT\n");
				if(values[0].data.integer == 0)
				{
					debug("Dividing by zero !");
					return RUNTIME_DIV_BY_ZERO;
				}
				result.data.real = (double)values[1].data.integer / values[0].data.integer;
				stack_push(calcs, TYPE_REAL, &result);
			}
			else if ((types[0] == TYPE_REAL) && (types[1] == TYPE_REAL))
			{
				debug("I_DIV - REAL\n");
				if(values[0].data.real == 0.0)
				{
					debug("Dividing by zero !");
					return RUNTIME_DIV_BY_ZERO;
				}
				result.data.real = values[1].data.real / values[0].data.real;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else if ((types[0] == TYPE_INT) && (types[1] == TYPE_REAL))
			{
				debug("I_DIV - REAL / INT\n");
				if(values[0].data.integer == 0)
				{
					debug("Dividing by zero !");
					return RUNTIME_DIV_BY_ZERO;
				}
				result.data.real = values[1].data.real / values[0].data.integer;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else if ((types[0] == TYPE_REAL) && (types[1] == TYPE_INT))
			{
				debug("I_DIV - INT / REAL\n");
				if(values[0].data.real == 0.0)
				{
					debug("Dividing by zero !");
					return RUNTIME_DIV_BY_ZERO;
				}
				result.data.real = values[1].data.integer / values[0].data.real;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_NEG:
			debug("I_NEG\n");
			CALCS_STACK_OPERATIONS_RESULT();

			if ((types[0] == TYPE_INT))
			{
				debug("I_NEG - INT\n");
				result.data.integer = - result.data.integer;
				stack_push(calcs, TYPE_INT, &(result));
			}
			else if ((types[0] == TYPE_REAL))
			{
				debug("I_NEG - REAL\n");
				result.data.real = - result.data.real;
				stack_push(calcs, TYPE_REAL, &(result));
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_INC:
			debug("I_INC\n");

			///                         Instruction operations
			if(item->index < 0)																		/// index indicates locals stack operation
			{
				debug("I_INC - locals stack\n");
				if (stack_index(locals, (-(item->index+1)), (int *)&types[0], &result) == INTERNAL_ERROR) ///
				{
					debug("I_INC - locals stack_index error.\n");
					return INTERNAL_ERROR;
				}
				result.data.integer++; ///incrementation
				if (stack_update(locals, (-(item->index+1)), (int)types[0], &result) == INTERNAL_ERROR)
				{
					debug("I_INC - locals stack error.\n");
					return INTERNAL_ERROR;
				}
			}
			else
			{
				debug("I_INC - globals field\n");
				if (variables_value_read(globals, &types[0], &result, item->index) == INTERNAL_ERROR)		/// index indicates global variables field operation
				{
					debug("I_INC - globals variables_value_read error.\n");
					return INTERNAL_ERROR;
				}
				result.data.integer++; ///incrementation
				if (variables_value_write(globals, &result, item->index) == INTERNAL_ERROR)	/// index indicates global variables field operation
				{
					debug("I_INC - globals field error.\n");
					return INTERNAL_ERROR;
				}
			}

			item = item->next_instruction;
			break;

		case I_DEC:
			debug("I_DEC\n");

			///                         Instruction operations
			if(item->index < 0)																		/// index indicates locals stack operation
			{
				debug("I_DEC - locals stack\n");
				if (stack_index(locals, (-(item->index+1)), (int *)&types[0], &result) == INTERNAL_ERROR) ///
				{
					debug("I_DEC - locals stack_index error.\n");
					return INTERNAL_ERROR;
				}
				result.data.integer--; ///decrementation
				if (stack_update(locals, (-(item->index+1)), (int)types[0], &result) == INTERNAL_ERROR)
				{
					debug("I_DEC - locals stack error.\n");
					return INTERNAL_ERROR;
				}
			}
			else
			{
				debug("I_DEC - globals field\n");
				if (variables_value_read(globals, &types[0], &result, item->index) == INTERNAL_ERROR)		/// index indicates global variables field operation
				{
					debug("I_DEC - globals variables_value_read error.\n");
					return INTERNAL_ERROR;
				}
				result.data.integer--; ///decrementation
				if (variables_value_write(globals, &result, item->index) == INTERNAL_ERROR)	/// index indicates global variables field operation
				{
					debug("I_DEC - globals field error.\n");
					return INTERNAL_ERROR;
				}
			}

			item = item->next_instruction;
			break;

		case I_LESS:

			COMPARISON_INSTRUCTION("I_LESS",  < );

		case I_GREATER:

			COMPARISON_INSTRUCTION("I_GREATER",  > );

		case I_LESS_EQUAL:

			COMPARISON_INSTRUCTION("I_LESS_EQUAL", <= );

		case I_GREATER_EQUAL:

			COMPARISON_INSTRUCTION("I_GREATER_EQUAL", >= );

		case I_EQUAL:

			COMPARISON_INSTRUCTION("I_EQUAL", == );

		case I_NOT_EQUAL:

			COMPARISON_INSTRUCTION("I_NOT_EQUAL", != );


		case I_AND:

			LOGICAL_OPERATOR_INSTRUCTION("I_AND", && );

		case I_OR:

			LOGICAL_OPERATOR_INSTRUCTION("II_OR", || );

		case I_XOR:
			debug("I_XOR\n");
			CALCS_STACK_OPERATIONS();
			if ((types[0] == TYPE_INT) && (types[1] == TYPE_INT))
			{
				debug("I_XOR - INT\n");
				result.data.integer = MACRO_FOR_XOR(values[1].data.integer, values[0].data.integer);
				stack_push(calcs, TYPE_INT, &(result));
			}
			else if ((types[0] == TYPE_BOOL) && (types[1] == TYPE_BOOL))
			{
				debug("I_XOR - BOOL\n");
				result.data.boolean = MACRO_FOR_XOR(values[1].data.boolean, values[0].data.boolean);
				stack_push(calcs, TYPE_BOOL, &(result));
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}
			item = item->next_instruction;
			break;

		case I_NOT:
			debug("I_NOT\n");
			CALCS_STACK_OPERATIONS_RESULT();

			if ((types[0] == TYPE_BOOL))
			{
				debug("I_NOT - BOOL\n");
				result.data.boolean = !( result.data.boolean);
				stack_push(calcs, TYPE_BOOL, &(result));
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_WRITE:
			debug("I_WRITE\n");
			///                         Stack operations
			CALCS_STACK_OPERATIONS_RESULT();

			if (types[0] == TYPE_INT)
			{
				debug("I_WRITE - INT\n");
				printf("%d", result.data.integer);
			}
			else if (types[0] == TYPE_REAL)
			{
				debug("I_WRITE - REAL\n");
				printf("%g", result.data.real);
			}
			else if (types[0] == TYPE_STRING)
			{
				debug("I_WRITE - STRING\n");
				printf("%s", result.data.string->str);
			}
			else if (types[0] == TYPE_BOOL)
			{
				debug("I_WRITE - BOOL\n");
				printf("%s", result.data.boolean ? "TRUE" : "FALSE");
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_READLN:
			debug("I_READLN\n");

			if (get_line(buffer) != SUCCESS)
			{
				debug("I_READLN - error loading string\n");
				return INTERNAL_ERROR;
			}

			if(item->index < 0)																		/// index indicates local stack operation
			{
				debug("I_READLN - locals stack\n");
				if (stack_index(locals, (-(item->index+1)), (int *)&types[0], &result) == INTERNAL_ERROR) ///
				{
					debug("I_PUSH - locals stack_index error.\n");
					return INTERNAL_ERROR;
				}
			}
			else
			{
				debug("I_READLN - globals field\n");
				if (variables_value_read(globals, &types[0], &result, item->index) == INTERNAL_ERROR)		/// index indicates global variables field operation
				{
					debug("I_PUSH - globals variables_value_read error.\n");
					return INTERNAL_ERROR;
				}
			}

			if (types[0] == TYPE_INT)
			{
				debug("I_READLN - integer\n");
				result.data.integer = strtol(*buffer, &endptr, 10);
				if (*endptr != '\0' && !isspace(*endptr))
				{
					debug("I_READLN - error loading int\n");
					return RUNTIME_NUMERIC_IN;
				}
			}
			else if (types[0] == TYPE_REAL)
			{
				debug("I_READLN - double\n");
				result.data.real = strtod(*buffer, &endptr);
				if (*endptr != '\0' && !isspace(*endptr))
				{
					debug("I_READLN - error loading real\n");
					return RUNTIME_NUMERIC_IN;
				}
			}	
			else if (types[0] == TYPE_STRING)
			{
				debug("I_READLN - cstring\n");
				if ((result.data.string = cstr_create_str(*buffer)) == NULL)
					return INTERNAL_ERROR;
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			result.inited = true; ///mark as inited

			///                         Instruction operations
			if(item->index < 0)																		/// index indicates locals stack operation
			{
				debug("I_READLN - locals stack\n");
				if (stack_update(locals, (-(item->index+1)), types[0], &result) == INTERNAL_ERROR)
				{
					debug("I_READLN - locals stack error.\n");
					return INTERNAL_ERROR;
				}
			}
			else
			{
				debug("I_READLN - globals field\n");
				if (variables_value_write(globals, &result, item->index) == INTERNAL_ERROR)	/// index indicates global variables field operation
				{
					debug("I_READLN - globals field error.\n");
					return INTERNAL_ERROR;
				}
			}


			item = item->next_instruction;
			break;

		case I_LEN:
			debug("I_LEN\n");
			///                         Stack operations
			CALCS_STACK_OPERATIONS_RESULT();

			if (types[0] == TYPE_STRING)
			{
				debug("I_LEN - string\n");

				result.data.integer = length(result.data.string);
				stack_push(calcs, TYPE_INT, &result);
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_COPY:
			debug("I_COPY\n");

			if(stack_index(calcs, 0, (int*)&types[0], &values[0])==INTERNAL_ERROR)
			{
				debug("Invalid read from calcs.\n");
				return INTERNAL_ERROR;
			}
			if(stack_index(calcs, 1, (int*)&types[1], &values[1])==INTERNAL_ERROR)
			{
				debug("Invalid read from calcs.\n");
				return INTERNAL_ERROR;
			}
			if(stack_index(calcs, 2, (int *)&types[2], &result) == INTERNAL_ERROR)
			{
				debug("Invalid read from calcs.\n");
				return INTERNAL_ERROR;
			}
			if(stack_popping_spree(calcs, 3)==INTERNAL_ERROR)
			{
				debug("Popping spree calcs error.\n");
				return INTERNAL_ERROR;
			}

			if (types[2] == TYPE_STRING && types[1] == TYPE_INT && types[0] == TYPE_INT )
			{
				debug("I_COPY - string\n");
				result.data.string = copy(result.data.string, values[1].data.integer, values[0].data.integer);
				stack_push(calcs, TYPE_STRING, &result);
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_FIND:
			debug("I_FIND\n");
			///                         Stack operations
			CALCS_STACK_OPERATIONS();

			if (types[0] == TYPE_STRING && types[1] == TYPE_STRING)
			{
				debug("I_FIND - string\n");
				result.data.integer = find(values[1].data.string, values[0].data.string);
				stack_push(calcs, TYPE_INT, &result);
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_SORT:
			debug("I_SORT\n");
			///                         Stack operations
			CALCS_STACK_OPERATIONS_RESULT();

			if (types[0] == TYPE_STRING)
			{
				debug("I_FIND - string\n");

				if (sort(&result.data.string) != SUCCESS)
                    return INTERNAL_ERROR;

				stack_push(calcs, TYPE_STRING, &result);
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			item = item->next_instruction;
			break;

		case I_JMP:
			debug("I_JMP\n");
			///                         Stack operations
			CALCS_STACK_OPERATIONS_RESULT();

			///                         Instruction operations
			if (types[0] == TYPE_BOOL)
			{
				if (result.data.boolean)
				{
					debug("I_JMP - next_instruction\n");
					item = item->next_instruction;
				}
				else
				{
					debug("I_JMP - alt_instruction\n");
					item = item->alt_instruction;
				}
			}
			else
			{
				debug("Invalid type passed to instruction\n");
				return INTERNAL_ERROR;
			}

			break;

		case I_NOP:
			debug("I_NOP\n");
			debug("I_NOP - sets next instruction.\n");
			item = item->next_instruction;
			break;

		default:
			debug("ERROR: Instruction not found.\n");
			return RUNTIME_OTHER;

		}


	}
	return SUCCESS;
}
