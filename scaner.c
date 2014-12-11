#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "scaner.h"
#include "errors.h"
#include "debug.h"
/*
 * Very early pre-alpha release.
 *
 * Can operate only when:
 *  - no errors in syntax
 *
 * Author: Adam Samalik
 *
 */

/*
 * Get a keyword code from it's string representation.
 */

int convert_binary(int binary_number)
{
    int base = 1, reminder = 0, decimal = 0;

    while(binary_number > 0){
    reminder = binary_number % 10;
    decimal = decimal + reminder * base;
    binary_number = binary_number / 10;
    base = base * 2;

    }

    return decimal;
}

static bool token_register(Token *token, bool set)
{
    static Token saved_token;
    static bool saved = false;

    if (token == NULL)
        return false;

    if (set) {
        saved_token = *token;
        saved = true;
        return true;
    }

    if (!saved)
        return false;

    *token = saved_token;
    saved = false;

    return true;
}

static enum token_keyword _get_keyword(char *name) {
    // lowercase it before
    for(int i = 0; name[i]; i++){
        name[i] = tolower(name[i]);
    }
    switch (name[0]) {
    case 'a':
        if (!strcmp(name, "and"))
            return KEYWORD_AND;

        break;

    case 'b':
        if (!strcmp(name, "begin"))
            return KEYWORD_BEGIN;

        if (!strcmp(name, "boolean"))
            return KEYWORD_BOOLEAN;

        break;

    case 'd':
        if (!strcmp(name, "do"))
            return KEYWORD_DO;

        break;

    case 'e':
        if (!strcmp(name, "else"))
            return KEYWORD_ELSE;

        if (!strcmp(name, "end"))
            return KEYWORD_END;

        break;

    case 'f':
        if (!strcmp(name, "false"))
            return KEYWORD_FALSE;

        if (!strcmp(name, "for"))
            return KEYWORD_FOR;

        if (!strcmp(name, "forward"))
            return KEYWORD_FORWARD;

        if (!strcmp(name, "function"))
            return KEYWORD_FUNCTION;

        break;

    case 'i':
        if (!strcmp(name, "if"))
            return KEYWORD_IF;

        if (!strcmp(name, "integer"))
            return KEYWORD_INTEGER;

        break;

    case 'n':
        if (!strcmp(name, "not"))
            return KEYWORD_NOT;

        break;

    case 'o':
        if (!strcmp(name, "or"))
            return KEYWORD_OR;

        break;

    case 'r':
        if (!strcmp(name, "readln"))
            return KEYWORD_READLN;

        if (!strcmp(name, "real"))
            return KEYWORD_REAL;

        if (!strcmp(name, "repeat"))
            return KEYWORD_REPEAT;

        break;

    case 's':
        if (!strcmp(name, "string"))
            return KEYWORD_STRING;

        break;

    case 't':
        if (!strcmp(name, "then"))
            return KEYWORD_THEN;

        if (!strcmp(name, "true"))
            return KEYWORD_TRUE;

        if (!strcmp(name, "to"))
            return KEYWORD_TO;

        break;

    case 'u':
        if (!strcmp(name, "until"))
            return KEYWORD_UNTIL;

        break;

    case 'v':
        if (!strcmp(name, "var"))
            return KEYWORD_VAR;

        break;

    case 'w':
        if (!strcmp(name, "while"))
            return KEYWORD_WHILE;

        if (!strcmp(name, "write"))
            return KEYWORD_WRITE;

        break;

    case 'x':
        if (!strcmp(name, "xor"))
            return KEYWORD_XOR;

        break;
    }
    return NOT_A_KEYWORD;
}


/*
 * Append a character to a string
 */
static void strcatc(char * destination, char c) {
    char buffer[2] = {c};
    strcat(destination, buffer);
}

/*
 * Load (another) lexeme from the source file
 * and return it's Token representation
 */
int get_token(Token *token_ret, FILE *input) {
    int symbol = 0;
    enum lexer_state state = LEXER_START;

    int token_name_pos = 0;
    enum token_keyword keyword;

    char buffer[100] = {0};
    char *ptr;

    static Token token;

    if (token_register(&token, false)) {
        *token_ret = token;
        return SUCCESS;
    }
debug("LEXER\n");
    while (1) {
        
        symbol = getc(input);
        switch (state) {
        case LEXER_START:
            if (isspace(symbol))
                break;

            if (symbol == EOF) {
                debug("EOF\n");
                token.type = TOKEN_EOF;
                *token_ret = token;
                return SUCCESS;
            }

            if ((symbol >= 'a' && symbol <= 'z') ||
                (symbol >= 'A' && symbol <= 'Z') ||
                (symbol == '_')) {
                token.value.value_name[token_name_pos] = symbol;
                token_name_pos ++;
                state = LEXER_ID_KEYWORD;
                break;
            }

            if (symbol >= '0' && symbol <= '9') {
                strcatc(buffer, symbol);
                token.type = TOKEN_INT;
                state = LEXER_INT_LOADING;
                break;
            }

            switch (symbol) {
            case '+':
                token.type = TOKEN_SYMBOL;
                token.value.value_symbol = ADDITION;
                *token_ret = token;
                return SUCCESS;

            case '-':
                switch (token.type) {
                case TOKEN_KEYWORD:
                    token.value.value_keyword = (token.value.value_keyword
                                                == KEYWORD_TRUE ||
                                                token.value.value_keyword
                                                == KEYWORD_FALSE) ?
                                                SUBTRACTION : NEGATION;
                    break;
                case TOKEN_ID:
                case TOKEN_INT:
                case TOKEN_FLOAT:
                case TOKEN_STRING:
                    token.value.value_keyword = SUBTRACTION;
                    break;
                default:
                    token.value.value_keyword = NEGATION;
                    break;
                }
                token.type = TOKEN_SYMBOL;
                *token_ret = token;
                return SUCCESS;

            case '*':
                token.type = TOKEN_SYMBOL;
                token.value.value_symbol = MULTIPLICATION;
                *token_ret = token;
                return SUCCESS;

            case '/':
                token.type = TOKEN_SYMBOL;
                token.value.value_symbol = DIVISION;
                *token_ret = token;
                return SUCCESS;

            case '=':
                token.type = TOKEN_SYMBOL;
                token.value.value_symbol = COMPARISON;
                *token_ret = token;
                return SUCCESS;

            case ':':
                token.type = TOKEN_SYMBOL;
                state = LEXER_MAYBE_ASSIGNMENT;
                break; // can be ':=' or ':'

            case ';':
                token.type = TOKEN_SYMBOL;
                token.value.value_symbol = SEMICOLON;
                *token_ret = token;
                return SUCCESS;

            case '.':
                token.type = TOKEN_SYMBOL;
                state = LEXER_MAYBE_DOUBLE_DOT;
                break; // can be '..' or '.'

            case ',':
                token.type = TOKEN_SYMBOL;
                token.value.value_symbol = COMMA;
                *token_ret = token;
                return SUCCESS;

            case '(':
                token.type = TOKEN_SYMBOL;
                token.value.value_symbol = PARENTHESIS_L;
                *token_ret = token;
                return SUCCESS;

            case ')':
                token.type = TOKEN_SYMBOL;
                token.value.value_symbol = PARENTHESIS_R;
                *token_ret = token;
                return SUCCESS;

            case '>':
                token.type = TOKEN_SYMBOL;
                state = LEXER_MAYBE_GREATER_EQUAL;
                break; // can be '>=' or '>'

            case '<':
                token.type = TOKEN_SYMBOL;
                state = LEXER_MAYBE_LESS_EQUAL;
                break; // can be '<=' or '<>' or '<'


            case '\'':
                token.type = TOKEN_STRING;
                debug("LEXER_STR_START");
                state = LEXER_STR_START;
                break;

            case '{':
                state = LEXER_COMMENT;
                break;

            case '%':
                state = LEXER_BINARY_LOADING_FIRST;
                break;
            
            case '&':
                debug("AND OCTAL \n");
                state = LEXER_OCT_LOADING_FIRST;
                break;
            
            case '$':
                state = LEXER_HEX_LOADING_FIRST;
                break;

            default:
                return LEXICAL_ERROR;
                break;

            }
                break;


        case LEXER_COMMENT:
            if (symbol == '}') {
                state = LEXER_START;
                break;
            }
            break;


        case LEXER_STR_START:
            debug("lexer str start\n");
            if (symbol == '\'') {
                token.value.value_string = cstr_create_str("");
                state = LEXER_STR_AP;
                break;

            } else if(symbol == '#')
            {
                debug("vosiel som do hashtagu\n");
                token.value.value_string = cstr_create_str("");
                state = LEXER_STR_SPEC;
                break; 
            }
            else if (isprint(symbol)){ // is this right? any more valid chars?
                token.value.value_string = cstr_create_chr(symbol);
                state = LEXER_STR_LOAD;
                break;

            } else {
                return LEXICAL_ERROR;
            }


        case LEXER_STR_LOAD:
        debug("lexer str load \n");
            if (symbol == '\'') {
                state = LEXER_STR_AP;
                break;

            } 
                else if(isprint(symbol)) { // is this right? any more valid chars?
                cstr_append_chr(token.value.value_string, symbol);
                break;

            } else {
                return LEXICAL_ERROR;
            }


        case LEXER_STR_AP:
            debug("LEXER STR AP\n");
            if (symbol == '\'') {
                cstr_append_chr(token.value.value_string, symbol);
                state = LEXER_STR_LOAD;
                break;
            }

            else if (symbol =='#') {
                debug("special char #\n");
                 cstr_append_chr(token.value.value_string, symbol);
                state = LEXER_STR_SPEC;
                break;
            }

            else if(symbol == EOF)
                {debug("EOF\n");
                return LEXICAL_ERROR;}
            
            else {
                debug("KONIEC ELSE V STR_AP__\n");
                ungetc(symbol, input);
                *token_ret = token;
                state = LEXER_START;
                return SUCCESS;
            }


        case LEXER_STR_SPEC:
            debug("lexer str_spec\n");
            if (symbol == '\'') {
                // FIXME what if the int is too big?!
                // SOLUTION: allow max three characters and then check the
                // value. Else: LEXICAL_ERROR
                symbol = (int)atof(buffer);
                cstr_append_chr(token.value.value_string, symbol);
                state = LEXER_STR_LOAD;
                break;

            } else if (symbol == '%') {
                state = LEXER_STR_SPEC_BIN_FIRST;
                break;
            
            } else if (symbol == '&') {
                state = LEXER_STR_SPEC_OCT_FIRST;
                debug("SOM V AND\n");
                break;
            
            } else if (symbol == '$') {
                state = LEXER_STR_SPEC_HEX_FIRST;
                break;
            debug("WAAAAAAA\n");
            } else if(isdigit(symbol)) {
                strcatc(buffer, symbol);
                break; 

            } else {
                return LEXICAL_ERROR;
            }


        case LEXER_STR_SPEC_BIN_FIRST:
            if (symbol >= '0' && symbol <= '1') {
                strcatc(buffer, symbol);
                state = LEXER_STR_SPEC_BIN;
                break;

            } else {
                return LEXICAL_ERROR;
            }

        case LEXER_STR_SPEC_BIN:
            if (symbol == '\'') {
                // FIXME what if the int is too big?!
                // SOLUTION: allow max ?? characters (and check the value)
                // Else: LEXICAL_ERROR
                symbol = (int)strtol(buffer,&ptr,2);
                cstr_append_chr(token.value.value_string, symbol);
                state = LEXER_STR_LOAD;
                break;
            
            } else if(symbol >= '0' && symbol <= '1') {
                strcatc(buffer, symbol);
                break; 

            } else {
                return LEXICAL_ERROR;
            }

        case LEXER_STR_SPEC_OCT_FIRST:
            debug("spec oct first \n");
            if (symbol >= '0' && symbol <= '7') {
                
                strcatc(buffer, symbol);
                state = LEXER_STR_SPEC_OCT;
                debug("idem do spec oct a aktualny symbol mam %c\n",symbol);
                break;

            } else {
                debug("error v spec oct first\n");
                return LEXICAL_ERROR;
            }

        case LEXER_STR_SPEC_OCT:
            debug("som v spec oct\n");
            if (symbol == '\'') {
                debug("SPEC_OCT koniec stringu\n");
                // FIXME what if the int is too big?!
                // SOLUTION: allow max ?? characters (and check the value)
                // Else: LEXICAL_ERROR
                debug("string pred symbol strtol je %s\n",buffer);
                symbol = (int)strtol(buffer,&ptr,8);
                debug("posledny symbol je %d\n",symbol);
                cstr_append_chr(token.value.value_string, symbol);
                debug("spec_koniec za appendom\n");
                state = LEXER_STR_LOAD;
                debug("odchadzam do str_load z spec_oct\n");
                break;
             
            } else if(symbol >= '0' && symbol <= '7') {
                debug("VOSIEL SOM DO SPEC_OCT a mam symbol %c\n",symbol);
                strcatc(buffer, symbol);
                break; 

            } else {
                debug("SPEC_OCT lexical_error\n");
                return LEXICAL_ERROR;
            }


        case LEXER_STR_SPEC_HEX_FIRST:
            if ((symbol >= '0' && symbol <= '9') ||
                (symbol >= 'a' && symbol <= 'f') ||
                (symbol >= 'A' && symbol <= 'F')) {
                strcatc(buffer, symbol);
                state = LEXER_STR_SPEC_HEX;
                break;

            } else {
                return LEXICAL_ERROR;
            }

        case LEXER_STR_SPEC_HEX:
            if (symbol == '\'') {
                // FIXME what if the int is too big?!
                // SOLUTION: allow max ?? characters (and check the value)
                // Else: LEXICAL_ERROR
                symbol = (int)strtol(buffer,&ptr,16);
                cstr_append_chr(token.value.value_string, symbol);
                state = LEXER_STR_LOAD;
                break;
            
            } else if ((symbol >= '0' && symbol <= '9') ||
                (symbol >= 'a' && symbol <= 'f') ||
                (symbol >= 'A' && symbol <= 'F')) {
                strcatc(buffer, symbol);
                break; 

            } else {
                return LEXICAL_ERROR;
            }


        case LEXER_MAYBE_GREATER_EQUAL:
            if (symbol == '=') {
                token.value.value_symbol = GREATER_EQUAL;
                *token_ret = token;
                return SUCCESS;
            } else {
                ungetc(symbol, input);
                token.value.value_symbol = GREATER_THAN;
                *token_ret = token;
                return SUCCESS;
            }

            break;


        case LEXER_MAYBE_LESS_EQUAL:
            if (symbol == '=') {
                token.value.value_symbol = LESS_EQUAL;
                *token_ret = token;
                return SUCCESS;
            } else if (symbol == '>') {
                token.value.value_symbol = NOT_EQUAL;
                *token_ret = token;
                return SUCCESS;
            } else {
                ungetc(symbol, input);
                token.value.value_symbol = LESS_THAN;
                *token_ret = token;
                return SUCCESS;
            }

            break;


        case LEXER_MAYBE_ASSIGNMENT:
            if (symbol == '=') {
                token.value.value_symbol = ASSIGNMENT;
                *token_ret = token;
                return SUCCESS;
            } else {
                ungetc(symbol, input);
                token.value.value_symbol = COLON;
                *token_ret = token;
                return SUCCESS;
            }

            break;


        case LEXER_MAYBE_DOUBLE_DOT:
            if (symbol == '.') {
                token.value.value_symbol = DOUBLE_DOT;
                *token_ret = token;
                return SUCCESS;
            } else {
                ungetc(symbol, input);
                token.value.value_symbol = DOT;
                *token_ret = token;
                return SUCCESS;
            }

            break;

        case LEXER_BINARY_LOADING_FIRST:
            if(symbol == '0' || symbol == '1') {
                strcatc(buffer, symbol);
                state = LEXER_BINARY_LOADING;
                break;

            } else {
                return LEXICAL_ERROR;
            }

        case LEXER_BINARY_LOADING:
            if(symbol == '0' || symbol == '1') {
                strcatc(buffer, symbol);
                break;
            
            } else {
                token.type = TOKEN_INT;
                token.value.value_int = (int)strtol(buffer,&ptr,2);
                ungetc(symbol, input);
                *token_ret = token;
                return SUCCESS;
            }

            break;

        case LEXER_OCT_LOADING_FIRST:
            if(symbol >= '0' && symbol <= '7') {
                strcatc(buffer, symbol);
                state = LEXER_OCT_LOADING;
                break;

            } else {
                debug("JEBEM NA TEBA V OCT LOADING FIRST\n");
                return LEXICAL_ERROR;
            }
            debug("JEBEM NA TEBA V OCT LOADING FIRST\n");
            break;

        case LEXER_OCT_LOADING:
            if(symbol >= '0' && symbol <= '7') {
                strcatc(buffer, symbol);
                break;
            
            } else {
                token.type = TOKEN_INT;
                token.value.value_int = (int)strtol(buffer,&ptr,8);
                ungetc(symbol, input);
                *token_ret = token;
                return SUCCESS;
            }

            break;

        case LEXER_HEX_LOADING_FIRST:
            if ((symbol >= '0' && symbol <= '9') ||
                (symbol >= 'a' && symbol <= 'f') ||
                (symbol >= 'A' && symbol <= 'F')) {
                strcatc(buffer, symbol);
                state = LEXER_HEX_LOADING;
                break;

            } else {
                return LEXICAL_ERROR;
            }

        case LEXER_HEX_LOADING:
            if ((symbol >= '0' && symbol <= '9') ||
                (symbol >= 'a' && symbol <= 'f') ||
                (symbol >= 'A' && symbol <= 'F')) {
                strcatc(buffer, symbol);
                break;
            
            } else {
                token.type = TOKEN_INT;
                token.value.value_int = (int)strtol(buffer,&ptr,16);
                ungetc(symbol, input);
                *token_ret = token;
                return SUCCESS;
            }

            break;

        case LEXER_INT_LOADING:
            if (symbol >= '0' && symbol <= '9') {
                strcatc(buffer, symbol);
                break;

            } else if (symbol == '.') {
                strcatc(buffer, symbol);
                state = LEXER_FLOAT_LOADING_FIRST;
                token.type = TOKEN_FLOAT;
                token.value.value_float = (double)token.value.value_int;
                break;

            } else if ((symbol == 'e') || (symbol == 'E')) {
                strcatc(buffer, symbol);
                state = LEXER_FLOAT_EXP_LOADING_FIRST;
                token.type = TOKEN_FLOAT;
                token.value.value_float = (double)token.value.value_int;
                break;

            } else {
                token.value.value_int = (int)atof(buffer);
                ungetc(symbol, input);
                *token_ret = token;
                return SUCCESS;
            }

            break;

        case LEXER_FLOAT_LOADING_FIRST:
            if (! ((symbol >= '0' && symbol <= '9'))) {
                return LEXICAL_ERROR;
            }

            state = LEXER_FLOAT_LOADING;
            strcatc(buffer, symbol);
            break;

        case LEXER_FLOAT_LOADING:
            if ((symbol == 'e') || (symbol == 'E')) {
                strcatc(buffer, symbol);
                state = LEXER_FLOAT_EXP_LOADING_FIRST;
                token.type = TOKEN_FLOAT;
                token.value.value_float = (double)token.value.value_int;
                break;

            } else if (! ((symbol >= '0' && symbol <= '9'))) {
                token.value.value_float = atof(buffer);
                ungetc(symbol, input);
                *token_ret = token;
                return SUCCESS;
            }

            strcatc(buffer, symbol);
            break;

        case LEXER_FLOAT_EXP_LOADING_FIRST:
            if (! (symbol >= '0' && symbol <= '9')) {
                return LEXICAL_ERROR;
            }

            state = LEXER_FLOAT_EXP_LOADING;
            strcatc(buffer, symbol);
            break;

        case LEXER_FLOAT_EXP_LOADING:
            if (! (symbol >= '0' && symbol <= '9')) {
                token.value.value_float = atof(buffer);
                ungetc(symbol, input);
                *token_ret = token;
                return SUCCESS;
            }

            strcatc(buffer, symbol);
            break;

        case LEXER_ID_KEYWORD:
            if (!((symbol >= 'a' && symbol <= 'z') ||
                  (symbol >= 'A' && symbol <= 'Z') ||
                  (symbol >= '0' && symbol <= '9') ||
                  (symbol == '_'))) {
                token.value.value_name[token_name_pos] = 0;
                token_name_pos = 0;
                keyword = _get_keyword(token.value.value_name);
                if (!keyword) {
                    token.type = TOKEN_ID;
                } else {
                    token.type = TOKEN_KEYWORD;
                    token.value.value_keyword = keyword;
                }
                ungetc(symbol, input);
                *token_ret = token;
                return SUCCESS;

            } else if (symbol == EOF) {
                token.type = TOKEN_EOF;
                *token_ret = token;
                return SUCCESS;

            } else {
                token.value.value_name[token_name_pos] = tolower(symbol);
                token_name_pos ++;
                break;
            }

            default:
                return LEXICAL_ERROR;
                break;
        }
    }
}

void unget_token(Token *token)
{
    token_register(token, true);
}

/*
 * Just for testing purpose
 */
/*
int main() {
    FILE *input = fopen("test", "r");
    Token *token = malloc(sizeof(Token *));
    int err;

    while(1) {
        err = get_token(token, input);
        if (err) {
            printf ("FUCK IT !!!!!!!!!!!!!!!!!!!!!!! \nehm... Error: Lexical Error.\n");
            exit(1);
        }
        if (token->type == TOKEN_ID)
            printf (" TOKEN_ID          %s", token->value.value_name);
        else if (token->type == TOKEN_KEYWORD)
            printf (" TOKEN_KEYWORD     %d", token->value.value_keyword);
        else if (token->type == TOKEN_INT)
            printf (" TOKEN_INT         %d", token->value.value_int);
        else if (token->type == TOKEN_FLOAT)
            printf (" TOKEN_FLOAT       %f", token->value.value_float);
        else if (token->type == TOKEN_SYMBOL)
            printf (" TOKEN_SYMBOL      %d", token->value.value_int);
        else if (token->type == TOKEN_STRING)
            printf (" TOKEN_STRING      %s", token->value.value_string->str);
        else
            break;
        printf("\n");
    }


    //fclose(input); segfault.. why?

    return 0;
} */
