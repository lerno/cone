/** Shared logic for namespace-based types
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "ir.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Initialize common fields
void iNsTypeInit(INsTypeNode *type, int nodecnt) {
    nodelistInit(&type->nodelist, nodecnt);
    namespaceInit(&type->namespace, nodecnt);
    // type->subtypes = newNodes(0);
}

// Add a function or potentially overloaded method
// If method is overloaded, add it to the link chain of same named methods
void iNsTypeAddFn(INsTypeNode *type, FnDclNode *fnnode) {
    NodeList *mnodes = &type->nodelist;
    FnDclNode *foundnode = (FnDclNode*)namespaceAdd(&type->namespace, fnnode->namesym, (INode*)fnnode);
    if (foundnode) {
        if (foundnode->tag != FnDclTag
            || !(foundnode->flags & FlagMethFld) || !(fnnode->flags & FlagMethFld)) {
            errorMsgNode((INode*)fnnode, ErrorDupName, "Duplicate name %s: Only methods can be overloaded.", &fnnode->namesym->namestr);
            return;
        }
        // Append to end of linked method list
        while (foundnode->nextnode)
            foundnode = foundnode->nextnode;
        foundnode->nextnode = fnnode;
    }
    nodelistAdd(mnodes, (INode*)fnnode);
}

// Find the named node (could be method or field)
// Return the node, if found or NULL if not found
INode *iNsTypeFindFnField(INsTypeNode *type, Name *name) {
    return namespaceFind(&type->namespace, name);
}

// Find method that best fits the passed arguments
FnDclNode *iNsTypeFindBestMethod(FnDclNode *firstmethod, Nodes *args, int isvref) {
    // Look for best-fit method
    FnDclNode *bestmethod = NULL;
    int bestnbr = 0x7fffffff; // ridiculously high number    
    for (FnDclNode *methnode = (FnDclNode *)firstmethod; methnode; methnode = methnode->nextnode) {
        int match;
        switch (match = fnSigMatchMethCall((FnSigNode *)methnode->vtype, args, isvref)) {
        case 0: continue;        // not an acceptable match
        case 1: return methnode;    // perfect match!
        default:                // imprecise match using conversions
            // If this will auto-ref, make sure the ref perm will match
            if (match >= 100 && 
                !refAutoRefCheck(nodesGet(args, 0), ((IExpNode*)nodesGet(((FnSigNode*)methnode->vtype)->parms, 0))->vtype))
                continue;
            if (match < bestnbr) {
                // Remember this as best found so far
                bestnbr = match;
                bestmethod = methnode;
            }
        }
    }
    return bestmethod;
}