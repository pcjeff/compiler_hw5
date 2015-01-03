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
void genepilogue(char* functionName);
void genblock(AST_NODE* blockNode);
void genStmtNode(AST_NODE *stmtNode);
void gencheckFunctionCall(AST_NODE* functionCallNode);
void genWriteFunction(AST_NODE* functionCallNode);
void genReadFunction(AST_NODE* functionCallNode);


int scopelevel = 0, offset = 64;
int const_num = 0;



FILE* fptr = NULL;

void genCode(AST_NODE *root)
{
    fptr = fopen("output.s", "w");
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
            genStmtNode(traverseChildren);
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
        gendeclareFunction(declarationNode);//declareFunction(declarationNode);
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
        fprintf(fptr,".data\n"); //print  .data
    while(traverseIDList)
    {
        //SymbolAttribute* attribute = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
        //attribute->attributeKind = isVariableOrTypeAttribute;
        SymbolTableEntry* entry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
        switch(traverseIDList->semantic_value.identifierSemanticValue.kind)
        {
        case NORMAL_ID:
            if(typeNode->dataType == INT_TYPE)
                fprintf(fptr, "_g_%s: .word 0\n.text\n", traverseIDList->semantic_value.identifierSemanticValue.identifierName);
            else
                fprintf(fptr, "_g_%s: .float 0\n.text\n", traverseIDList->semantic_value.identifierSemanticValue.identifierName);
            break;
        case ARRAY_ID:
            //only one dimension array in this homework
            entry = retrieveSymbol(traverseIDList->semantic_value.identifierSemanticValue.identifierName);
            fprintf(fptr, "_g_%s: .space  %d\n.text\n"
                , traverseIDList->semantic_value.identifierSemanticValue.identifierName
                , entry->attribute->attr.typeDescriptor->properties.arrayProperties.sizeInEachDimension[0]
                );
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
    AST_NODE* parameterListNode = functionNameID->rightSibling;
    AST_NODE *blockNode = parameterListNode->rightSibling;
    
    genprologue(functionNameID->semantic_value.identifierSemanticValue.identifierName);
    genblock(blockNode);
    genepilogue(functionNameID->semantic_value.identifierSemanticValue.identifierName);

}
void genblock(AST_NODE* blockNode)
{
    scopelevel++;

    AST_NODE *traverseListNode = blockNode->child;
    while(traverseListNode)
    {
        genGeneralNode(traverseListNode);
        traverseListNode = traverseListNode->rightSibling;
    }

    if(scopelevel==0)  
    {
        printf("WTF? scopelevel = 0 at here\n");
    }
    else 
        scopelevel--;
    //print size of frame here
}
void genprologue(char* functionName)
{
    int i=0;

    fprintf(fptr, ".text\n");
    fprintf(fptr, "_start_%s:\n", functionName);
    fprintf(fptr, "str lr, [sp, #0]\n");
    fprintf(fptr, "str fp, [sp, #-4]\n");
    fprintf(fptr, "add fp, sp, #-4\n");
    fprintf(fptr, "add sp, sp, #-8\n");
    fprintf(fptr, "ldr lr, =_frameSize_%s\n", functionName);
    fprintf(fptr, "ldr lr, [lr, #0]\n");
    fprintf(fptr, "sub sp, sp, lr\n");

    for (i = 4; i <= 11; i++)
    {
        fprintf(fptr, "str r%d, [sp, #%d]\n",i, (i-3)*4);
    }
    for (i = 16; i <= 23; ++i)
    {
        fprintf(fptr, "vstr.f32 s%d, [sp, #%d]\n", i,(i-7)*4);
    }
}
void genepilogue(char* functionName)
{
    int i=0;
    fprintf(fptr, "_end_%s:\n", functionName);
    for (i = 4; i <= 11; i++)
    {
        fprintf(fptr, "ldr r%d, [sp, #%d]\n",i, (i-3)*4);
    }
    for (i = 16; i <= 23; ++i)
    {
        fprintf(fptr, "vldr.f32 s%d, [sp, #%d]\n", i,(i-7)*4);
    }
    fprintf(fptr, "ldr lr, [fp, #4]\n");
    fprintf(fptr, "mov sp, fp\n");
    fprintf(fptr, "add sp, sp, #4\n");
    fprintf(fptr, "ldr fp, [fp,#0]\n");
    fprintf(fptr, "bx lr\n");
    fprintf(fptr, "_frameSize_%s: .word %d\n", functionName, offset);
}
void genStmtNode(AST_NODE *stmtNode)
{
    if(stmtNode->nodeType == NUL_NODE)
    {
        return;
    }
    else if(stmtNode->nodeType == BLOCK_NODE)
    {
        genblock(stmtNode);
    }
    else
    {
        switch(stmtNode->semantic_value.stmtSemanticValue.kind)
        {
        case WHILE_STMT:
            //checkWhileStmt(stmtNode);
            break;
        case ASSIGN_STMT:
            //checkAssignmentStmt(stmtNode);
            break;
        case IF_STMT:
            //checkIfStmt(stmtNode);
            break;
        case FUNCTION_CALL_STMT:
            gencheckFunctionCall(stmtNode);
            break;
        case RETURN_STMT:
            //checkReturnStmt(stmtNode);
            break;
        default:
            printf("Unhandle case in void processStmtNode(AST_NODE* stmtNode)\n");
            break;
        }
    }
}
void gencheckFunctionCall(AST_NODE* functionCallNode)
{
    AST_NODE* functionIDNode = functionCallNode->child;

    //special case
    if(strcmp(functionIDNode->semantic_value.identifierSemanticValue.identifierName, "write") == 0)
    {
        genWriteFunction(functionCallNode);
        return;
    }
    if(strcmp(functionIDNode->semantic_value.identifierSemanticValue.identifierName, "read") == 0)
    {
        //genReadFunction(functionCallNode);
        return;
    }
    else
        fprintf(fptr, "\nbl %s\n", functionIDNode->semantic_value.identifierSemanticValue.identifierName);
}
void genWriteFunction(AST_NODE* functionCallNode)
{
    AST_NODE* functionIDNode = functionCallNode->child;
    AST_NODE* actualParameter = functionIDNode->rightSibling;

    if(actualParameter->dataType == CONST_STRING_TYPE)
    {
        fprintf(fptr, ".data\n");
        fprintf(fptr, "_CONSTANT_%d: .ascii ", const_num, 
            actualParameter->semantic_value.identifierSemanticValue.identifierName);
        fprintf(fptr, ".align 2\n");
        fprintf(fptr, "ldr r4, *_CONSTANT_%d\n", const_num);
        fprintf(fptr, "mov r0, r4\n");
        fprintf(fptr, "bl _write_str\n");
        const_num++;
    }
    else if(actualParameter->dataType == FLOAT_TYPE)
    {

    }
    else if(actualParameter->dataType == INT_TYPE)
    {

    }
    else 
        printf("ERROR type\n");

}