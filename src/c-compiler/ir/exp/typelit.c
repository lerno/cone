/** Handling for list nodes (e.g., type literals)
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "../ir.h"

// Serialize a list node
void typeLitPrint(FnCallNode *node) {
    if (node->objfn)
        inodePrintNode(node->objfn);
    INode **nodesp;
    uint32_t cnt;
    inodeFprint("[");
    for (nodesFor(node->args, cnt, nodesp)) {
        inodePrintNode(*nodesp);
        if (cnt)
            inodeFprint(",");
    }
    inodeFprint("]");
}

// Check the type literal node (actually done by fncall)
void typeLitNameRes(NameResState *pstate, FnCallNode *arrlit) {
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(arrlit->args, cnt, nodesp))
        inodeNameRes(pstate, nodesp);
}

// Is the type literal actually a literal?
int typeLitIsLiteral(FnCallNode *node) {
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(node->args, cnt, nodesp)) {
        INode *arg = *nodesp;
        if (arg->tag == NamedValTag)
            arg = ((NamedValNode*)arg)->val;
        if (!litIsLiteral(arg))
            return 0;
    }
    return 1;
}

// Type check a number literal
void typeLitNbrCheck(TypeCheckState *pstate, FnCallNode *nbrlit, INode *type) {

    if (nbrlit->args->used != 1) {
        errorMsgNode((INode*)nbrlit, ErrorBadArray, "Number literal requires one value");
        return;
    }

    INode *first = nodesGet(nbrlit->args, 0);
    if (!isExpNode(first)) {
        errorMsgNode((INode*)first, ErrorBadArray, "Literal value must be typed");
        return;
    }
    INode *firsttype = itypeGetTypeDcl(((IExpNode*)first)->vtype);
    if (firsttype->tag != IntNbrTag && firsttype->tag != UintNbrTag && firsttype->tag != FloatNbrTag) 
        errorMsgNode((INode*)first, ErrorBadArray, "May only create number literal from another number");
}

// Type check an array literal
void typeLitArrayCheck(TypeCheckState *pstate, FnCallNode *arrlit) {

    if (arrlit->args->used == 0) {
        errorMsgNode((INode*)arrlit, ErrorBadArray, "Literal list may not be empty");
        return;
    }

    // Get element type from first element
    // Type of array literal is: array of elements whose type matches first value
    INode *first = nodesGet(arrlit->args, 0);
    if (!isExpNode(first)) {
        errorMsgNode((INode*)first, ErrorBadArray, "Array literal element must be a typed value");
        return;
    }
    INode *firsttype = ((IExpNode*)first)->vtype;
    ((ArrayNode*)arrlit->vtype)->elemtype = firsttype;

    // Ensure all elements are consistently typed (matching first element's type)
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(arrlit->args, cnt, nodesp)) {
        if (!itypeIsSame(((IExpNode*)*nodesp)->vtype, firsttype))
            errorMsgNode((INode*)*nodesp, ErrorBadArray, "Inconsistent type of array literal value");
    }
}

// Return true if desired named field is found and swapped into place
int typeLitGetName(Nodes *args, uint32_t argi, Name *name) {
    uint32_t nargs = args->used;
    uint32_t i = argi;
    for (; i < nargs; i++) {
        NamedValNode *node = (NamedValNode*)nodesGet(args, i);
        if (node->tag == NamedValTag && ((NameUseNode*)node->name)->namesym == name) {
            nodesMove(args, argi, i);
            return 1;
        }
    }
    return 0;
}

// Reorder the literal's field values to the same order as the type's fields
// Also prevent the specification of a value for a private field outside the type's methods
void typeLitStructReorder(FnCallNode *arrlit, StructNode *strnode, int private) {
    INode **nodesp;
    uint32_t cnt;
    uint32_t argi = 0;
    for (nodelistFor(&strnode->fields, cnt, nodesp)) {
        FieldDclNode *field = (FieldDclNode *)*nodesp;

        // If field represents a discriminated tag, inject struct's discriminant nbr
        if (field->flags & IsTagField) {
            ULitNode *tagnbrnode = newULitNode(strnode->tagnbr, field->vtype);
            nodesInsert(&arrlit->args, (INode*)tagnbrnode, argi++);
            continue;
        }

        // A field value has been specified...
        if (argi < arrlit->args->used) {
            // If we have a named value, insert the proper named value here where it belongs
            INode **litval = &nodesGet(arrlit->args, argi);
            if ((*litval)->tag == NamedValTag && !typeLitGetName(arrlit->args, argi, field->namesym)) {
                // Use default value for unmatched field, if the type defined one
                if (field->value)
                    nodesInsert(&arrlit->args, field->value, argi);
                else {
                    errorMsgNode((INode*)arrlit, ErrorBadArray, "Cannot find named value matching the field %s", &field->namesym->namestr);
                    ++argi;
                    continue;
                }
            }
            // Don't allow a value to be given for a private field outside of the type's methods
            if (!private && field->namesym->namestr == '_')
                errorMsgNode(*litval, ErrorNotTyped, "Only a method in the type may specify a value for the private field %s.", &field->namesym->namestr);
        }
        // Append default value if no value specified
        else if (field->value) {
            nodesAdd(&arrlit->args, field->value);
        }
        else {
            errorMsgNode((INode*)arrlit, ErrorBadArray, "Not enough values specified on type literal");
            while (cnt--)
                nodesAdd(&arrlit->args, (INode*)newULitNode(0,field->vtype));  // Put in fake nodes to pretend we are ok
            return;
        }
        ++argi;
    }
    if (argi < arrlit->args->used)
        errorMsgNode((INode*)arrlit, ErrorBadArray, "Too many values specified on type literal");
}

// Type check a struct literal
void typeLitStructCheck(TypeCheckState *pstate, FnCallNode *arrlit, StructNode *strnode) {
    INode **nodesp;

    // Ensure type has been type-checked, in case any rewriting/semantic analysis was needed
    inodeTypeCheck(pstate, &arrlit->vtype);

    // Reorder the literal's arguments to match the type's field order
    typeLitStructReorder(arrlit, strnode, (INode*)strnode == pstate->typenode);

    uint32_t cnt;
    uint32_t argi = 0;
    for (nodelistFor(&strnode->fields, cnt, nodesp)) {
        FieldDclNode *field = (FieldDclNode *)*nodesp;
        INode **litval = &nodesGet(arrlit->args, argi);
        if (!iexpSameType(*nodesp, litval))
            errorMsgNode((INode*)*litval, ErrorBadArray, "Literal value's type does not match expected field's type");
        ++argi;
    }
}

// Check the list node
void typeLitTypeCheck(TypeCheckState *pstate, FnCallNode *arrlit) {
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(arrlit->args, cnt, nodesp))
        inodeTypeCheck(pstate, nodesp);

    INode *littype = itypeGetTypeDcl(arrlit->vtype);
    if (!itypeIsConcrete(arrlit->vtype))
        errorMsgNode((INode*)arrlit, ErrorInvType, "Type must be concrete and instantiable.");
    else if (littype->tag == ArrayTag)
        typeLitArrayCheck(pstate, arrlit);
    else if (littype->tag == StructTag)
        typeLitStructCheck(pstate, arrlit, (StructNode*)littype);
    else if (littype->tag == IntNbrTag || littype->tag == UintNbrTag || littype->tag == FloatNbrTag)
        typeLitNbrCheck(pstate, arrlit, littype);
    else
        errorMsgNode((INode*)arrlit, ErrorBadArray, "Unknown type literal type for type checking");
}
