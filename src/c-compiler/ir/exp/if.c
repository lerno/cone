/** Handling for if nodes
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "../ir.h"

// Create a new If node
IfNode *newIfNode() {
    IfNode *ifnode;
    newNode(ifnode, IfNode, IfTag);
    ifnode->condblk = newNodes(4);
    ifnode->vtype = voidType;
    return ifnode;
}

// Serialize an if statement
void ifPrint(IfNode *ifnode) {
    INode **nodesp;
    uint32_t cnt;
    int firstflag = 1;

    for (nodesFor(ifnode->condblk, cnt, nodesp)) {
        if (firstflag) {
            inodeFprint("if ");
            firstflag = 0;
            inodePrintNode(*nodesp);
        }
        else {
            inodePrintIndent();
            if (*nodesp == voidType)
                inodeFprint("else");
            else {
                inodeFprint("elif ");
                inodePrintNode(*nodesp);
            }
        }
        inodePrintNL();
        inodePrintNode(*(++nodesp));
        cnt--;
    }
}

// Recursively strip 'returns' out of all block-ends in 'if' (see returnPass)
void ifRemoveReturns(IfNode *ifnode) {
    INode **nodesp;
    int16_t cnt;
    for (nodesFor(ifnode->condblk, cnt, nodesp)) {
        INode **laststmt;
        cnt--; nodesp++;
        laststmt = &nodesLast(((BlockNode*)*nodesp)->stmts);
        if ((*laststmt)->tag == ReturnTag)
            *laststmt = ((ReturnNode*)*laststmt)->exp;
        if ((*laststmt)->tag == IfTag)
            ifRemoveReturns((IfNode*)*laststmt);
    }
}

// if node name resolution
void ifNameRes(NameResState *pstate, IfNode *ifnode) {
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(ifnode->condblk, cnt, nodesp)) {
        inodeNameRes(pstate, nodesp);
    }
}

// Type check the if statement node
// - Every conditional expression must be a bool
// - if's vtype is specified/checked only when coerced by iexpCoerces
void ifTypeCheck(TypeCheckState *pstate, IfNode *ifnode) {
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(ifnode->condblk, cnt, nodesp)) {

        // Validate that conditional node is correct
        inodeTypeCheck(pstate, nodesp);
        if (*nodesp != voidType) {
            if (0 == iexpCoerces((INode*)boolType, nodesp))
                errorMsgNode(*nodesp, ErrorInvType, "Conditional expression must be coercible to boolean value.");
        }
        else if (cnt > 2) {
            errorMsgNode(*(nodesp+1), ErrorInvType, "match on everything should be last.");
        }

        ++nodesp; --cnt;
        inodeTypeCheck(pstate, nodesp);
    }
}

// Special type-checking for iexpChkType, where blk->vtype sets type expectations
// - Every conditional expression must be a bool
// - Type of every branch's value must match expected type and each other
void IfChkType(TypeCheckState *pstate, IfNode *ifnode) {
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(ifnode->condblk, cnt, nodesp)) {

        // Validate that conditional node is correct
        inodeTypeCheck(pstate, nodesp);
        if (*nodesp != voidType) {
            if (0 == iexpCoerces((INode*)boolType, nodesp))
                errorMsgNode(*nodesp, ErrorInvType, "Conditional expression must be coercible to boolean value.");
        }
        else if (cnt > 2) {
            errorMsgNode(*(nodesp + 1), ErrorInvType, "match on everything should be last.");
        }

        // Validate that all branches have matching types
        ++nodesp; --cnt;
        if (!iexpChkType(pstate, &ifnode->vtype, nodesp))
            errorMsgNode(*nodesp, ErrorInvType, "expression type does not match expected type");
    }
}

// Perform data flow analysis on an if expression
void ifFlow(FlowState *fstate, IfNode **ifnodep) {
    IfNode *ifnode = *ifnodep;
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(ifnode->condblk, cnt, nodesp)) {
        if (*nodesp != voidType)
            flowLoadValue(fstate, nodesp);
        nodesp++; cnt--;
        blockFlow(fstate, (BlockNode**)nodesp);
        flowAliasReset();
    }
}