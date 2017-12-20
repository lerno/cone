/** Parser
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#ifndef parser_h
#define parser_h

#include "../ast/ast.h"

// parser.c
PgmAstNode *parse();
void parseSemi();
void parseRCurly();
void parseLCurly();
void parseRParen();

// parsestmt.c
void parseStmtBlock(Nodes **nodes);

// parseexp.c
AstNode *parseExp();

// parsetype.c
NameDclAstNode *parseVarDcl(PermAstNode *defperm);
AstNode *parseFnSig();
AstNode *parseVtype();

#endif
