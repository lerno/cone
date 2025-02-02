/** Handling for deref nodes
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "../ir.h"

// Create a new deref node
DerefNode *newDerefNode() {
    DerefNode *node;
    newNode(node, DerefNode, DerefTag);
    node->vtype = voidType;
    return node;
}

// Serialize deref
void derefPrint(DerefNode *node) {
    inodeFprint("*");
    inodePrintNode(node->exp);
}

// Name resolution of deref node
void derefNameRes(NameResState *pstate, DerefNode *node) {
    inodeNameRes(pstate, &node->exp);
}

// Type check deref node
void derefTypeCheck(TypeCheckState *pstate, DerefNode *node) {
    inodeTypeCheck(pstate, &node->exp);
    PtrNode *ptype = (PtrNode*)((IExpNode *)node->exp)->vtype;
    if (ptype->tag != RefTag && ptype->tag != PtrTag)
        errorMsgNode((INode*)node, ErrorNotPtr, "May only de-reference a simple reference or pointer.");
    node->vtype = ptype->pvtype;
}

// Insert automatic deref, if node is a ref
void derefAuto(INode **node) {
    if (iexpGetTypeDcl(*node)->tag != RefTag)
        return;
    DerefNode *deref = newDerefNode();
    deref->exp = *node;
    deref->vtype = ((RefNode*)((IExpNode *)*node)->vtype)->pvtype;
    *node = (INode*)deref;
}
