/*
 * The C program is fairly similar to Python, and I've tried to make this listing
 * track with the Python one as much as possible.
 *
 * Of course, there are major differences, including:
 *   - Obviously, C doesn't have classes, but we use similar data structures.
 *   - We have to do our own memory allocation.
 *   - It is much easier to read tokens when the parser needs them, instead of
 *     all at once.  The parser's reference to the token stream gets updated
 *     by the tokenizer.
 *   - switch() statements!
 *   - No exception handling, so we bail on our first hard error.
 *   - The use of explicit references.
 *
 */
#include <stdio.h>   // For definition of printf()
#include <stdlib.h>  // For definition of exit()
#include <string.h>  // For definition of memcpy()
#include <ctype.h>   // For definition of isalpha()
#include <stdio.h>   // For FILE *, fread, stdin

typedef enum { T_PROG, T_GLOBAL, T_HOST, T_HOSTID, T_OBRACE, T_CBRACE, T_KEYVAL, T_EQ, T_VAR, T_INT, T_FLOAT, T_STR, T_QUOTE, T_EOF } type_t;

typedef struct node_st {
    type_t          type;
    char           *value;
} node_t;

typedef struct {
    node_t *top_token;
	node_t *prev_token;
    char   *input;
} state_t;


int lineNumber = 1;
int override=0;

node_t *prog  (state_t *s);
node_t *parseGlobalBlock(state_t *s);
node_t *parseHostBlock(state_t *s);
node_t *parseKeyValuePairs(state_t *s);

int scanNumber(char *p, state_t *state);
int scanHostId(char *p, state_t *state);
int scanKey(char *p, state_t *state);
int scanString(char *p, state_t *state);

typedef struct KeyValSt {
    char *keyName;
    char *keyValue;
    char *keyType;
    struct KeyValSt *nextKeyVal;
} KeyValTree;

typedef struct GroupSt {
    int groupType;
    char *hostId;
    KeyValTree *keyValPairsPtr;
    struct GroupSt *nextGroup;
} GroupTree;


KeyValTree* CreateKeyValPair(char *keyNamePassed, char *keyValuePassed, char *keyTypePassed);
void PrintGroupTree();
void CreateGroupTree(int groupTypePassed, char *hostIdPassed);

GroupTree global, *curGroupTree, *topGroupTree, *tmpGroupTree;
KeyValTree *curKeyValTree,*topKeyValTree, *tmpKeyValPtr, *tmpGloKeyValPtr, *tmpHostKeyValPtr;


void bail(const char *s) {
    printf("%s\n",s);
	printf("ERR:P:%d\n",lineNumber);
    exit(0); /* Don't worry about deallocating memory since we errored. */
}

node_t *new_node() {
    node_t *ret = (node_t *)malloc(sizeof(node_t));
    ret->type   = 0;
    ret->value  = 0;
    return ret;
}

void free_node(node_t *n) {
    if(!n) return;
    if (n->value) free(n->value);
    free(n);
}

/* Tokenizes a single token, updating top_token in the parse state,
 * and advances the input accordingly.
 */

void load_next_token(state_t *state) {
    char   *p;
    size_t  len;

    state->top_token = new_node();

    p = state->input;
//	printf("Scanning: %s\n",state->input);
    while (1) {
//		printf("Scanning: %c\n",*p);
        switch(*p) {
        case '\0': /* The null-terminator for the string */
            state->top_token->type = T_EOF;
            return;
        case ' ':
        case '\t':
			p++;
            state->input++; // Move the start of the input.
            continue; // Keep looking till the first non-space.
		case '\n':
				p++;
				state->input++;
				lineNumber++;
				continue;
		case '#':
				do
				{
					p++;
					state->input++;
				} while(*p != '\n');
				continue;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if(state->prev_token->type == T_EQ) {
//				printf("Lexing Number\n");
				len = scanNumber(p, state);
				p = p + len - 1;
				
			}
			else if(state->prev_token->type == T_HOST) {
//				printf("Lexing HostID\n");
				len = scanHostId(p, state);
				p = p + len -1;
			}
				
			break;  // p now points to the first char of the next token.
		case '_':
			if(state->prev_token->type == T_OBRACE || state->prev_token->type == T_INT || state->prev_token->type == T_FLOAT ||
			   state->prev_token->type == T_STR || state->prev_token->type == T_QUOTE ) {
//				printf("Lexing Key\n");
				len = scanKey(p, state);
				p = p + len - 1;
			
			}
			else if(state->prev_token->type == T_HOST) {
//				printf("Lexing HostID\n");
				len = scanHostId(p, state);
				p = p + len - 1;
			}
			else {
				bail("Unexpected '_'");
			}
			break;
				
		case '/':
			len = scanString(p, state);
			p = p + len - 1;
			break;
        case '=':
            state->top_token->type  = T_EQ;
            break;
        case '{':
            state->top_token->type  = T_OBRACE;
            break;
        case '}':
            state->top_token->type  = T_CBRACE;
            break;
        case 'g':
            if(*(p+1) == 'l' && *(p+2) == 'o' && *(p+3) == 'b' && *(p+4) == 'a' && *(p+5) == 'l' && !isalpha(*(p+6))) {
                state->top_token->type = T_GLOBAL;
                p+=6;
//                printf("Lexed GLOBAL\n");
                break;
            }
        case 'h':
            if(*(p+1) == 'o' && *(p+2) == 's' && *(p+3) == 't' && !isalpha(*(p+4))) {
                state->top_token->type = T_HOST;
                p+=4;
//                printf("Lexed HOST\n");
                break;
            } /* Otherwise, it's not the END token so it's a regular E */
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
		case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
		case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B':
		case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
		case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
			if(state->prev_token->type == T_EQ) {
				len = scanString(p, state);
			}
			else if(state->prev_token->type == T_HOST){
				len = scanHostId(p, state);
			}
			else {
				len = scanKey(p, state);
			}
			p = p + len-1;
            break;
        default:
//            printf ("Warning: invalid character, %c\n", *p++);
			bail("Lex Error.");
            continue;
        }
		p++;
        break;
    }
    state->input = p;
//	printf("returning <token:%d>\n",state->top_token->type);
    return;
}

int scanKey(char *p, state_t *state) {
	size_t  len;
	if(*p && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')) {
		p++;
	}
	else {
		bail("Not a key.");
	}
	
	while(*p && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')) p++;
	len                     = p-state->input;
	state->top_token->type  = T_VAR;
	state->top_token->value = (char *)malloc(len+1);
	memcpy(state->top_token->value, state->input, len);
	state->top_token->value[len] = 0;
	return len;
}
int scanNumber(char *p, state_t *state) {
	size_t  len;
	
//	printf("scanning number:%c\n",*p);
	while (*p && (*p >= '0' && *p <= '9')) {
		p++;
	};

	if(*p == '.') {
		p++;
		while (*p && (*p >= '0' && *p <= '9')) p++;
		state->top_token->type  = T_FLOAT;
	}
	else {
		state->top_token->type  = T_INT;
	}
		
	len                     = p-state->input;
	state->top_token->value = (char *)malloc(len+1);
	memcpy(state->top_token->value, state->input, len);
	state->top_token->value[len] = 0;

	return len;
}

int scanHostId(char *p, state_t *state) {
	size_t  len;
	
	//	printf("scanning number:%c\n",*p);
	while ( *p &&
		   ( (*p >= '0' && *p <= '9')
		   || (*p >= 'a' && *p <= 'z')
		   || (*p >= 'A' && *p <= 'Z')
		   || (*p == '.' || *p == '_' || *p == '-')))  {
		p++;
	};
	
	len                     = p-state->input;
	state->top_token->value = (char *)malloc(len+1);
	memcpy(state->top_token->value, state->input, len);
	state->top_token->value[len] = 0;

	state->top_token->type  = T_HOSTID;
//	printf("Lexed HostID: %s\n",state->top_token->value);
	return len;
}

int scanString(char *p, state_t *state) {
	size_t  len;
	
	//	printf("scanning number:%c\n",*p);
	p++;	//first char is either '/' or alphabet
	while ( *p &&
		   ( (*p >= '0' && *p <= '9')
			|| (*p >= 'a' && *p <= 'z')
			|| (*p >= 'A' && *p <= 'Z')
			|| (*p == '_' || *p == '/' || *p == '.' || *p == '-')))  {
			   p++;
		   };
	
	len                     = p-state->input;
	state->top_token->value = (char *)malloc(len+1);
	memcpy(state->top_token->value, state->input, len);
	state->top_token->value[len] = 0;
	
	state->top_token->type  = T_STR;
	//	printf("Lexed HostID: %s\n",state->top_token->value);
	return len;
}

int cur_type(state_t *s) {
    return s->top_token->type;
}

node_t *consume(state_t *s) {
    node_t *ret = s->top_token;
	s->prev_token = s->top_token;
//	printf("Current char: %c\n",*s->input);
    load_next_token(s);
    return ret;
}

node_t *parse(char *input) {
    state_t parse_state;

    parse_state.input = input;
    load_next_token(&parse_state);

    return prog(&parse_state);
}

node_t *prog(state_t *s) {
    node_t *ret = new_node();
    ret->type = T_PROG;
	
    switch (cur_type(s)) {

    case T_EOF:
		if(topGroupTree != 0) {
			break;
		}
		else {
			bail("File without Global block.");
		}
		break;
			
    case T_GLOBAL:
//        printf("Parsing Global\n");
		parseGlobalBlock(s);
//        printf("Checking if HOST\n");
        consume(s);
        parseHostBlock(s);
        break;

    default:
        printf("Unexpected token: %d\n",cur_type(s));
        bail("Unexpected token.");
    }
    return ret;
}

node_t *parseHostBlock(state_t *s) {
    node_t *ret = new_node();
    ret->type = cur_type(s);

    switch (cur_type(s)) {
    case T_EOF:
        break;
    case T_HOST:
		consume(s);
		if (cur_type(s) != T_HOSTID) {
//			printf("<token:%d>\n",cur_type(s));
			bail("Expected 'HostID'.");
		}
	
		CreateGroupTree(1,s->top_token->value);
//        printf("Parsing HOST\n");
        consume(s);
        if (cur_type(s) != T_OBRACE) {
            bail("Expected '{'.");
        }
		
//			printf("Checking if KeyVal\n");
		consume(s);
		
		if(cur_type(s) == T_VAR) {
			parseKeyValuePairs(s);
		}
		else if (cur_type(s) == T_CBRACE) {
        }
		else {
			bail("Expected '}'.");
		}
			
//        printf("Parsed HOST\n");
		PrintGroupTree();
			
        consume(s);
        parseHostBlock(s);
        break;

    default:
//        printf("Unexpected token: %d\n",cur_type(s));
        bail("Unexpected token.");
    }
    return ret;
}

node_t *parseGlobalBlock(state_t *s) {

    node_t *ret = new_node();
    ret->type = T_GLOBAL;

	CreateGroupTree(0,"\0");
	
    consume(s);
    if (cur_type(s) != T_OBRACE) {
        bail("Expected '{'.");
    }
	consume(s);
	
	if(cur_type(s) == T_VAR) {
		parseKeyValuePairs(s);
	}
	else if (cur_type(s) == T_CBRACE) {
	}
	else {
		bail("Expected '}'.");
	}

	PrintGroupTree();
	
    return ret;
}

node_t *parseKeyValuePairs(state_t *s) {
	char *keyName, *keyValue, *keyType;
	
    node_t *ret = new_node();
    ret->type = T_KEYVAL;

//	printf("<token:%d>\n",cur_type(s));
    if (cur_type(s) != T_VAR) {
        bail("Expected variable.");
    }
	keyName = s->top_token->value;
	
    consume(s);
    if (cur_type(s) != T_EQ) {
        bail("Expected '='.");
    }
    consume(s);
    if (cur_type(s) == T_INT) {
		keyValue = s->top_token->value;
		keyType = "I";
	}
	else if (cur_type(s) == T_FLOAT) {
		keyValue = s->top_token->value;
		keyType = "F";
	}
	else if (cur_type(s) == T_STR) {
		keyValue = s->top_token->value;
		keyType = "S";
	}
	else if (cur_type(s) == T_QUOTE) {
		keyValue = s->top_token->value;
		keyType = "Q";
	}
	else {
		bail("Expected Value.");
	}
	CreateKeyValPair(keyName, keyValue, keyType);
	
	consume(s);
	if (cur_type(s)==T_VAR) {
		parseKeyValuePairs(s);
	}
	else if (cur_type(s)==T_CBRACE) {
		return ret;
	}
	else {
		bail("Expected CBRACE/KEYVAL");
	}
	return ret;
	
    }

KeyValTree* CreateKeyValPair(char *keyNamePassed, char *keyValuePassed, char *keyTypePassed)
{
	KeyValTree *newKeyValPtr = (KeyValTree *)malloc(sizeof(KeyValTree));
    newKeyValPtr->keyName = keyNamePassed;
    newKeyValPtr->keyValue = keyValuePassed;
    newKeyValPtr->keyType = keyTypePassed;
    newKeyValPtr->nextKeyVal = 0;
	
	if(topKeyValTree==0)
    {
        topKeyValTree = newKeyValPtr;
        curKeyValTree = topKeyValTree;
    }
    else
    {
        curKeyValTree->nextKeyVal = newKeyValPtr;
        curKeyValTree = curKeyValTree->nextKeyVal;
    }
	return newKeyValPtr;
}

void PrintGroupTree()
{
//	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
	curGroupTree->keyValPairsPtr = topKeyValTree;
	//        printf(">>Linked Keys to %s<<\n",curGroupTree->hostId);
	
	tmpGroupTree = curGroupTree;
	tmpKeyValPtr = tmpGroupTree->keyValPairsPtr;
	if(tmpGroupTree->groupType==0)
	{
		printf("GLOBAL:\n");
	}
	else
	{
		printf("HOST %s:\n",tmpGroupTree->hostId);
	}
	while(tmpKeyValPtr)
	{
		printf("    %s:",tmpKeyValPtr->keyType);
		
		if(tmpGroupTree->groupType!=0)
		{
			tmpGloKeyValPtr = topGroupTree->keyValPairsPtr;
			
			while(tmpGloKeyValPtr)
			{
				if(!strcmp(tmpGloKeyValPtr->keyName,tmpKeyValPtr->keyName))
				{
					override = 1;
				}
				tmpGloKeyValPtr = tmpGloKeyValPtr->nextKeyVal;
			}
		}
		
		tmpHostKeyValPtr = curGroupTree->keyValPairsPtr;
		while(tmpHostKeyValPtr && tmpHostKeyValPtr!=tmpKeyValPtr)
		{
			if(!strcmp(tmpHostKeyValPtr->keyName,tmpKeyValPtr->keyName))
			{
				override = 1;
			}
			tmpHostKeyValPtr = tmpHostKeyValPtr->nextKeyVal;
		}
		
		if(override==1)
		{
			printf("O");
			override=0;
		}
		printf(":%s:",tmpKeyValPtr->keyName);
		
		if(strcmp(tmpKeyValPtr->keyType,"Q")==0)
		{
			printf("\"\"\"");
		}
		
		printf("%s",tmpKeyValPtr->keyValue);
		if(strcmp(tmpKeyValPtr->keyType,"Q")==0)
		{
			printf("\"\"\"");
		}
		printf("\n");
		tmpKeyValPtr = tmpKeyValPtr->nextKeyVal;
	};
}

void CreateGroupTree(int groupTypePassed, char *hostIdPassed)
{
	tmpGroupTree = (GroupTree *)malloc(sizeof(GroupTree));
    tmpGroupTree->groupType = groupTypePassed;
    tmpGroupTree->nextGroup = 0;
	
	topKeyValTree = 0;
	tmpGroupTree->keyValPairsPtr = topKeyValTree;
	
	if(groupTypePassed == 0)
	{
		topGroupTree = tmpGroupTree;
		curGroupTree = tmpGroupTree;
	}
	else
	{
        tmpGroupTree->hostId = hostIdPassed;
        curGroupTree->nextGroup = tmpGroupTree;
        curGroupTree = curGroupTree->nextGroup;
	}
//	printf("Created Group Tree for: %d\n",groupTypePassed);
}


/*
node_t *hostBlock(state_t *s) {

	node_t *ret = new_node();
	ret->type = T_HOST;

	ret->b1 = consume(s);
	if (cur_type(s) != T_OBRACE) {
		printf("Token Type: %d\n",ret->type);
		bail("Expected '{'.");
	}

	ret->b2 = consume(s);
	if (cur_type(s) != T_CBRACE) {
		bail("Expected '}'.");
	}

	return ret;
}
*/



#ifndef BUFSIZE
#define BUFSIZE 1024
#endif

char *read_all_of_stdin() {
    char  *ret = (char *)malloc(BUFSIZE);
    size_t total_len = 0, new_len;


    FILE *myfile = fopen("test.cfg", "r");
    // make sure it's valid:
    if (!myfile) {
        printf("ERR:F:\n");
        return 0;
    }

    /* This is an assignment as an expression, folks! */
    while ((new_len = fread(ret+total_len, 1, BUFSIZE, myfile))) {
        total_len += new_len;
        ret = (char *)realloc(ret, total_len + BUFSIZE); /* Grow the buffer. */
    }

//    printf("%s\n",ret);

    return ret;
}

int main (const int argc, const char **argv) {
    char *input  = read_all_of_stdin();

    node_t *tree = parse(input);
//    print_node(tree, 0);
//  find_assignments(tree);
//    free_node(tree);
//    free(input);
}
