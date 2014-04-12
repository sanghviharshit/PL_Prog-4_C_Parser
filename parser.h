//
//  parser.h
//  PL_Prog-4_C_Parser
//
//  Created by Harshit Sanghvi on 4/12/14.
//  Copyright (c) 2014 Harshit. All rights reserved.
//

#ifndef PL_Prog_4_C_Parser_parser_h
#define PL_Prog_4_C_Parser_parser_h

typedef enum { T_PROG, T_GLOBAL, T_HOST, T_HOSTID, T_OBRACE, T_CBRACE, T_VAL,
    T_EQ, T_KEY, T_INT, T_FLOAT, T_STR, T_QUOTE, T_EOF, T_ERROR
} tokenType;

typedef enum { LEX, PARSE } errorType;

typedef struct tokenDataStruct {
    tokenType          tType;
    char           *tValue;
} tokenData;

typedef struct {
    tokenData *topToken;
    tokenData *prevToken;
    char   *lexInput;
} curParseState;

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


tokenData *ParseProg (curParseState *s);
tokenData *ParseGlobalBlock (curParseState *s);
tokenData *ParseHostBlock (curParseState *s);
tokenData *ParseKeyValuePairs (curParseState *s);

int ScanNumber (char *p, curParseState *state);
int ScanHostId (char *p, curParseState *state);
int ScanKey (char *p, curParseState *state);
int ScanString (char *p, curParseState *state);
int ScanQuote (char *p, curParseState *state);



KeyValTree* CreateKeyValPair (char *keyNamePassed, char *keyValuePassed, char *keyTypePassed);
void CreateGroupTree (int groupTypePassed, char *hostIdPassed);
void PrintGroupTree ();




#endif
