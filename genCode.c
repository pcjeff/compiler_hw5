#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "symbolTable.h"


void genCode(AST_NODE *root);
void genProgramNode(AST_NODE *programNode);
void genGeneralNode(AST_NODE *node);
void genDeclarationNode(AST_NODE* declarationNode);
void gendeclareIdList(AST_NODE* typeNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize);
void gendeclareFunction(AST_NODE* declarationNode);
void genprologue(char* functionName);
void genepilogue();

int scopelevel = 0;


void genCode(AST_NODE *root)
{
    genProgramNode(root);
    return;
}
void genProgramNode(AST_NODE *programNode)
{
    AST_NODE *traverseDeclaration = programNode->child;
    //printf(".data\n");
    while(traverseDeclaration)
    {
        if(traverseDeclaration->nodeType == VARIABLE_DECL_LIST_NODE)
        {
            genGeneralNode(traverseDeclaration);
        }
        else
        {
            //function declaration
            genDeclarationNode(traverseDeclaration);
        }

        traverseDeclaration = traverseDeclaration->rightSibling;
    }
    return;
}
void genGeneralNode(AST_NODE *node)
{
    AST_NODE *traverseChildren = node->child;
    switch(node->nodeType)
    {
    case VARIABLE_DECL_LIST_NODE:
        while(traverseChildren)
        {
            genDeclarationNode(traverseChildren);
            traverseChildren = traverseChildren->rightSibling;
        }
        break;
    case STMT_LIST_NODE:
        while(traverseChildren)
        {
            //genStmtNode(traverseChildren);
            traverseChildren = traverseChildren->rightSibling;
        }
        break;
    case NONEMPTY_ASSIGN_EXPR_LIST_NODE:
        while(traverseChildren)
        {
            //checkAssignOrExpr(traverseChildren);
            traverseChildren = traverseChildren->rightSibling;
        }
        break;
    case NONEMPTY_RELOP_EXPR_LIST_NODE:
        while(traverseChildren)
        {
            //genExprRelatedNode(traverseChildren);
            traverseChildren = traverseChildren->rightSibling;
        }
        break;
    case NUL_NODE:
        break;
    default:
        printf("Unhandle case in void genGeneralNode(AST_NODE *node)\n");
        break;
    }
}
void genDeclarationNode(AST_NODE* declarationNode)
{
    AST_NODE *typeNode = declarationNode->child;


    switch(declarationNode->semantic_value.declSemanticValue.kind)
    {
    case VARIABLE_DECL:
        gendeclareIdList(declarationNode, VARIABLE_ATTRIBUTE, 0);
        break;
    /*case TYPE_DECL:
        gendeclareIdList(declarationNode, TYPE_ATTRIBUTE, 0);
        break;
    */
    case FUNCTION_DECL:
        //declareFunction(declarationNode);
        break;
    /*case FUNCTION_PARAMETER_DECL:
        gendeclareIdList(declarationNode, VARIABLE_ATTRIBUTE, 1);
        break;
    */
    default:

        break;
    }
    return;
}
void gendeclareIdList(AST_NODE* declarationNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize)
{
    AST_NODE* typeNode = declarationNode->child;
    TypeDescriptor *typeDescriptorOfTypeNode = typeNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor;
 
    AST_NODE* traverseIDList = typeNode->rightSibling;

    if (scopelevel == 0) 
        printf(".data\n"); //print  .data
    while(traverseIDList)
    {
        //SymbolAttribute* attribute = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
        //attribute->attributeKind = isVariableOrTypeAttribute;
        switch(traverseIDList->semantic_value.identifierSemanticValue.kind)
        {
        case NORMAL_ID:
            //attribute->attr.typeDescriptor = typeNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor;
            printf("_g_%s: .word 0\n", traverseIDList->semantic_value.identifierSemanticValue.identifierName);
            break;
        case ARRAY_ID:
            /*printf("_g_%s: .word %s\n", 
                traverseIDList->semantic_value.identifierSemanticValue.identifierName,
                traverseIDList->semantic_value.identifierSemanticValue.identifierName,
                );
            */
            break;
        default:
            printf("Unhandle case in void gendeclareIdList(AST_NODE* typeNode)\n");
            break;
        }
        traverseIDList = traverseIDList->rightSibling;
    }

}
void gendeclareFunction(AST_NODE* declarationNode)
{
    AST_NODE* returnTypeNode = declarationNode->child;
    AST_NODE* functionNameID = returnTypeNode->rightSibling;
    printf(".text\n");
    printf("_start_%s:\n", functionNameID->semantic_value.identifierSemanticValue.identifierName);
    genprologue(functionNameID->semantic_value.identifierSemanticValue.identifierName);
    scopelevel++;
    /*while(traverseListNode)
    {
        genGeneralNode(traverseListNode);
        traverseListNode = traverseListNode->rightSibling;
    }*/
    if(scopelevel==0)  
    {
        printf("WTF? scopelevel = 0 at here\n");
    }
    else 
        scopelevel--;
    genepilogue();
}
void genprologue(char* functionName)
{
    int i=0;

    printf("str lr, [sp, #0]\n");
    printf("str fp, [sp, #-4]\n");
    printf("str fp, [sp, #-4]\n");
    printf("add fp, sp, #-4\n");
    printf("add sp, sp, #-8\n");
    printf("ldr lr, =_frameSize_%s\n", functionName);
    printf("ldr lr, [lr, #0]\n");
    printf("sub sp, sp, lr\n");

    for (i = 4; i <= 11; i++)
    {
        printf("str r%d, [sp, #%d]\n",i, (i-3)*4);
    }
    for (i = 16; i <= 23; ++i)
    {
        printf("vstr.f32 s%d, [sp, #%d]\n", i,(i-7*4));
    }
}
void genepilogue()
{
    int i=0;

    for (i = 4; i <= 11; i++)
    {
        printf("ldr r%d, [sp, #%d]\n",i, (i-3)*4);
    }
    for (i = 16; i <= 23; ++i)
    {
        printf("vldr.f32 s%d, [sp, #%d]\n", i,(i-7*4));
    }
    printf("ldr lr, [fp, #4]\n");
    printf("mov sp, fp\n");
    printf("add sp, sp, #4\n");
    printf("ldr fp, [fp,#0]\n");
    printf("bx lr\n");
}