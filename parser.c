//
//  parser.c
//  PL_Prog-4_C_Parser
//
//  Created by Harshit Sanghvi on 4/1/14.
//  Copyright (c) 2014 Harshit. All rights reserved.
//

#include <stdio.h>   // For definition of printf()
#include <stdlib.h>  // For definition of exit()
#include <string.h>  // For definition of memcpy()
#include <ctype.h>   // For definition of isalpha()
#include <stdio.h>   // For FILE *, fread, stdin
#include "parser.h"

int lineNumber = 1;
int override = 0;
int sColon = 0, newLine = 0;	//Patch for allowing only 1 Scolon, and condition that keyval pair cant span multiple lines
int totalLen = 0, newLen = 0;
char *startInput = 0;


GroupTree global, *curGroupTree, *topGroupTree, *tmpGroupTree;
KeyValTree *curKeyValTree,*topKeyValTree, *tmpKeyValPtr, *tmpGloKeyValPtr, *tmpHostKeyValPtr;



void bail(const char *s, errorType error) {
//    printf("\n%s\n",s);
    if(error == LEX) {
        printf("ERR:L:%d\n",lineNumber);

    }
    else {
        printf("ERR:P:%d\n",lineNumber);
    }

    exit(0);
}

tokenData *CreateNewNode() {
    tokenData *ret = (tokenData *)malloc(sizeof(tokenData));
    ret->tType   = 0;
    ret->tValue  = 0;
    return ret;
}

void FreeNode(tokenData *n) {
    if(!n) return;
    if (n->tValue) free(n->tValue);
    free(n);
}

/* Tokenizes a single token, updating top_token in the parse state,
 * and advances the input accordingly.
 */

void LoadNextToken(curParseState *parseState) {
    char   *p;
    int  len;

    parseState->topToken = CreateNewNode();

    p = parseState->lexInput;
//	printf("Scanning: %s\n",state->input);
    while (1) {
//		printf("Scanning: %c\n",*p);
        switch(*p) {
        case '\0': /* The null-terminator for the string */
            parseState->topToken->tType = T_EOF;
            if(p-startInput < totalLen) {
//				printf("Len: %ld\n",p-startInput);
                bail("Null char in unexpected place", LEX);
            }
            p--;
            break;
        /*
        if(*(p+1)!='\0')
        	bail("Null char in unexpected place");
        else
        	return;
         */
        case ' ':
        case '\t':
            p++;
            parseState->lexInput++; // Move the start of the input.
            continue; // Keep looking till the first non-space.
        case '\n':
            if(parseState->prevToken->tType == T_KEY || parseState->prevToken->tType == T_EQ)
            {
                parseState->topToken->tType  = T_ERROR;
                return;
            }
            newLine = 1;
            p++;
            parseState->lexInput++;
            lineNumber++;
            continue;
        case '#':
            do
            {
                p++;
                parseState->lexInput++;
            } while(*p != '\n');
            //lineNumber++;
            continue;
        case '-':	//For signed numbers.
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(parseState->prevToken->tType == T_EQ) {
//				printf("Lexing Number\n");
                len = ScanNumber(p, parseState);
                p = p + len - 1;

            }
            else if(parseState->prevToken->tType == T_HOST) {
//				printf("Lexing HostID\n");
                len = ScanHostId(p, parseState);
                p = p + len -1;
            }
            else
                bail("Unexpected Place for digit", PARSE);

            break;  // p now points to the first char of the next token.
        case '_':
            if(parseState->prevToken->tType == T_OBRACE || parseState->prevToken->tType == T_INT || parseState->prevToken->tType == T_FLOAT ||
                    parseState->prevToken->tType == T_STR || parseState->prevToken->tType == T_QUOTE ) {
//				printf("Lexing Key\n");
                len = ScanKey(p, parseState);
                p = p + len - 1;

            }
            else if(parseState->prevToken->tType == T_HOST) {
//				printf("Lexing HostID\n");
                len = ScanHostId(p, parseState);
                p = p + len - 1;
            }
            else {
                bail("Unexpected '_'", PARSE);
            }
            break;

        case '/':
            len = ScanString(p, parseState);
            p = p + len - 1;
            break;
        case '"':
            len = ScanQuote(p, parseState);
            p = p + len - 1;
            break;
        case '=':
            parseState->topToken->tType  = T_EQ;
            break;
        case '{':
            parseState->topToken->tType  = T_OBRACE;
            break;
        case '}':
            parseState->topToken->tType  = T_CBRACE;
            break;
        case 'g':
            if(topGroupTree == 0 &&
                    *(p+1) == 'l' && *(p+2) == 'o' && *(p+3) == 'b' && *(p+4) == 'a' && *(p+5) == 'l' && !isalnum(*(p+6))) {
                parseState->topToken->tType = T_GLOBAL;
                p+=5;
//                printf("Lexed GLOBAL\n");
                break;
            }
            else if(parseState->prevToken->tType == T_EQ) {
                len = ScanString(p, parseState);
                p = p + len - 1;
                break;
            }
            else {
                len = ScanKey(p, parseState);
                p = p + len - 1;
                break;
            }


        case 'h':
            if(parseState->prevToken->tType == T_CBRACE &&
                    *(p+1) == 'o' && *(p+2) == 's' && *(p+3) == 't' && !isalnum(*(p+4))) {
                parseState->topToken->tType = T_HOST;
                p+=3;
//                printf("Lexed HOST\n");
                sColon = 0;
                break;
            }
            else if(parseState->prevToken->tType == T_EQ) {
                len = ScanString(p, parseState);
                p = p + len - 1;
                break;
            }
            else {
                len = ScanKey(p, parseState);
                p = p + len - 1;
                break;
            }

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
            if(parseState->prevToken->tType == T_EQ) {
                len = ScanString(p, parseState);
            }
            else if(parseState->prevToken->tType == T_HOST) {
                len = ScanHostId(p, parseState);
            }
            else {
                len = ScanKey(p, parseState);
            }
            p = p + len-1;
            break;
        case ';':
            if(parseState->prevToken->tType == T_CBRACE && sColon != 1) {
                sColon = 1;
                p++;
                parseState->lexInput++; // Move the start of the input.
                continue;
            }
            else
                bail("Wrong place for SCOLON",PARSE);
            break;

        default:
//            printf ("Warning: invalid character, %c\n", *p++);
            bail("Lex Error.", LEX);
            break;
        }
        p++;
        break;
    }
    parseState->lexInput = p;
//	printf("returning <token:%d>\n",state->top_token->type);
    return;
}

int ScanKey(char *p, curParseState *state) {
    int  len;
//	if(*p && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')) {

    if (newLine == 0) {
        state->topToken->tType  = T_ERROR;
        return 0;
    }
    else {
        newLine = 0;
    }

    p++;	//Skip first letter, checked in LoadNextToken()

    while(*p && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')) p++;
    len                     = p-state->lexInput;
    state->topToken->tType  = T_KEY;
    state->topToken->tValue = (char *)malloc(len+1);
    memcpy(state->topToken->tValue, state->lexInput, len);
    state->topToken->tValue[len] = 0;
    return len;
}

int ScanNumber(char *p, curParseState *state) {
    int  len;

//	printf("scanning number:%c\n",*p);
    if(*p && *p == '-') {
        p++;	//signed number
    }
    while (*p && (*p >= '0' && *p <= '9')) {
        p++;
    };

    if(*p == '.') {
        p++;
        while (*p && (*p >= '0' && *p <= '9')) p++;
        state->topToken->tType  = T_FLOAT;
    }
    else {
        state->topToken->tType  = T_INT;
    }

    len                     = p-state->lexInput;
    state->topToken->tValue = (char *)malloc(len+1);
    memcpy(state->topToken->tValue, state->lexInput, len);
    state->topToken->tValue[len] = 0;

    return len;
}

int ScanHostId(char *p, curParseState *state) {
    int  len;

    //	printf("scanning number:%c\n",*p);
    while ( *p &&
            ( (*p >= '0' && *p <= '9')
              || (*p >= 'a' && *p <= 'z')
              || (*p >= 'A' && *p <= 'Z')
              || (*p == '.' || *p == '_' || *p == '-')))  {
        p++;
    };

    len                     = p-state->lexInput;
    state->topToken->tValue = (char *)malloc(len+1);
    memcpy(state->topToken->tValue, state->lexInput, len);
    state->topToken->tValue[len] = 0;

    state->topToken->tType  = T_HOSTID;
//	printf("Lexed HostID: %s\n",state->top_token->value);
    return len;
}

int ScanString(char *p, curParseState *state) {
    int  len;

    //	printf("scanning number:%c\n",*p);
    p++;	//first char is either '/' or alphabet
    while ( *p &&
            ( (*p >= '0' && *p <= '9')
              || (*p >= 'a' && *p <= 'z')
              || (*p >= 'A' && *p <= 'Z')
              || (*p == '_' || *p == '/' || *p == '.' || *p == '-')))  {
        p++;
    };

    len                     = p-state->lexInput;
    state->topToken->tValue = (char *)malloc(len+1);
    memcpy(state->topToken->tValue, state->lexInput, len);
    state->topToken->tValue[len] = 0;

    state->topToken->tType  = T_STR;
    //	printf("Lexed HostID: %s\n",state->top_token->value);
    return len;
}

int ScanQuote (char *p, curParseState *state) {
    int  len;
    int qEnd = 0;

    //	printf("scanning number:%c\n",*p);
    p++;	//first char is either '/' or alphabet
    char strBuf[1000];
    char *qStr, *sStr;

    qStr = strBuf;
    sStr = qStr;



    while (!qEnd) {
//		printf("%d->",*p);


        if( *p <= 127 && *p >0 && *p != '\\' && *p != '"' && *p != '\n') {
            *qStr++ = *p;
            p++;
//			printf("\n");
        }
//		printf("current qStr: %s. Scanning: %c\n",strBuf, *p);
        else if(*p == '\\') {
            if(*(p+1) == 'n') {
                *qStr++ = '\n';
                p = p + 2;
            }
            else if(*(p+1) == 'r') {
                *qStr++ = '\r';
                p = p + 2;
            }
            else if(*(p+1) == '\\') {
                *qStr++ = '\\';
                p = p + 2;
            }
            else if(*(p+1) == '"') {
                *qStr++ = '"';
                p = p + 2;
            }
            else if(*(p+1)<=127 && *(p+1)>0) {
                p++;
                *qStr++ = *p;
                p++;
            }
            else {
                bail("Invalid char in quoted string", LEX);
            }
        }
        else if(*p == '\n') {
            bail("New line not expected in quoted string",LEX);
        }
        else if(*p == '"') {
//			printf("End of Quoted String\n");
            *qStr = 0;
            p++;
            qEnd = 1;
            break;
        }
        else {
            bail("Invalid char in quoted string", LEX);
        }
    }


    len                     = qStr-sStr;
//		   printf("Length: %d\n",len);
    state->topToken->tValue = (char *)malloc(len+1);
    memcpy(state->topToken->tValue, strBuf, len);
//		   state->top_token->value[len] = 0;

    state->topToken->tType  = T_QUOTE;
//			printf("Lexed Quote: %s\n",state->topToken->tValue);
    len                     = p-state->lexInput;
    return len;
}

int CurTokenType(curParseState *s) {
    return s->topToken->tType;
}

tokenData *Consume(curParseState *s) {
    tokenData *ret = s->topToken;
    s->prevToken = s->topToken;
//	printf("Current char: %c\n",*s->input);
    LoadNextToken(s);
    return ret;
}

tokenData *Parse(char *input) {
    curParseState parse_state;
    startInput = input;
    parse_state.lexInput = input;
    LoadNextToken(&parse_state);

    return ParseProg(&parse_state);
}

tokenData *ParseProg(curParseState *s) {
    tokenData *ret = CreateNewNode();
    ret->tType = T_PROG;

    switch (CurTokenType(s)) {

    case T_EOF:
        if(topGroupTree != 0) {
            break;
        }
        else {
            bail("File without Global block.",PARSE);
        }
        break;

    case T_GLOBAL:
//        printf("Parsing Global\n");
        ParseGlobalBlock(s);
//        printf("Checking if HOST\n");
        Consume(s);
        ParseHostBlock(s);
        break;

    default:
        printf("Unexpected token: %d\n",CurTokenType(s));
        bail("Unexpected token",PARSE);
    }
    return ret;
}

tokenData *ParseHostBlock(curParseState *s) {
    tokenData *ret = CreateNewNode();
    ret->tType = CurTokenType(s);

    switch (CurTokenType(s)) {
    case T_EOF:
        break;
    case T_HOST:
        Consume(s);
        if (CurTokenType(s) != T_HOSTID) {
//			printf("<token:%d>\n",cur_type(s));
            bail("Expected 'HostID'.", PARSE);
        }

        CreateGroupTree(1,s->topToken->tValue);
//        printf("Parsing HOST\n");
        Consume(s);
        if (CurTokenType(s) != T_OBRACE) {
            bail("Expected '{'.", PARSE);
        }

//			printf("Checking if KeyVal\n");
        Consume(s);

        if(CurTokenType(s) == T_KEY) {
            ParseKeyValuePairs(s);
        }
        else if (CurTokenType(s) == T_CBRACE) {
        }
        else {
            bail("Expected '}'.", PARSE);
        }

//        printf("Parsed HOST\n");
        PrintGroupTree();

        Consume(s);
        ParseHostBlock(s);
        break;

    default:
//        printf("Unexpected token: %d\n",cur_type(s));
        bail("Unexpected token.", PARSE);
    }
    return ret;
}

tokenData *ParseGlobalBlock(curParseState *s) {

    tokenData *ret = CreateNewNode();
    ret->tType = T_GLOBAL;

    CreateGroupTree(0,"\0");

    Consume(s);
    if (CurTokenType(s) != T_OBRACE) {
        bail("Expected '{'.", PARSE);
    }
    Consume(s);

    if(CurTokenType(s) == T_KEY) {
        ParseKeyValuePairs(s);
    }
    else if (CurTokenType(s) == T_CBRACE) {
    }
    else {
        bail("Expected '}'.", PARSE);
    }

    PrintGroupTree();

    return ret;
}

tokenData *ParseKeyValuePairs(curParseState *s) {
    char *keyName, *keyValue, *keyType;

    tokenData *ret = CreateNewNode();
    ret->tType = T_VAL;

//	printf("<token:%d>\n",cur_type(s));
    if (CurTokenType(s) != T_KEY) {
        bail("Expected variable.", PARSE);
    }
    keyName = s->topToken->tValue;

    Consume(s);
    if (CurTokenType(s) != T_EQ) {
        bail("Expected '='.", PARSE);
    }
    Consume(s);
    if (CurTokenType(s) == T_INT) {
        keyValue = s->topToken->tValue;
        keyType = "I";
    }
    else if (CurTokenType(s) == T_FLOAT) {
        keyValue = s->topToken->tValue;
        keyType = "F";
    }
    else if (CurTokenType(s) == T_STR) {
        keyValue = s->topToken->tValue;
        keyType = "S";
    }
    else if (CurTokenType(s) == T_QUOTE) {
        keyValue = s->topToken->tValue;
        keyType = "Q";
    }
    else {
        bail("Expected Value.", PARSE);
    }
    CreateKeyValPair(keyName, keyValue, keyType);

    Consume(s);
    if (CurTokenType(s)==T_KEY) {
        ParseKeyValuePairs(s);
    }
    else if (CurTokenType(s)==T_CBRACE) {
        return ret;
    }
    else {
        bail("Expected CBRACE/KEYVAL", PARSE);
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


#ifndef BUFSIZE
#define BUFSIZE 1024
#endif

char *ReadFile() {
    char  *fileInput = (char *)malloc(BUFSIZE);

    FILE *myfile = fopen("t.cfg", "r");
    if (!myfile) {
        printf("ERR:F:\n");
        return 0;
    }

    while ((newLen = fread(fileInput+totalLen, 1, BUFSIZE, myfile))) {
        totalLen += newLen;
        fileInput = (char *)realloc(fileInput, totalLen + BUFSIZE); /* Grow the buffer. */
    }

//    printf("total_length: %ld\n",total_len);

    return fileInput;
}

int main (const int argc, const char **argv) {
    char *input  = ReadFile();
    tokenData *tree = Parse(input);

    return 0;
}
