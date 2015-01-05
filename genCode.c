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
void genAssignmentStmt(AST_NODE* assignmentNode);
void gencheckFunctionCall(AST_NODE* functionCallNode);
void genWriteFunction(AST_NODE* functionCallNode);
void genReadFunction(AST_NODE* functionCallNode);
void genVariableLValue(AST_NODE* idNode);
void genVariableRValue(AST_NODE* idNode);
void genExprRelatedNode(AST_NODE* exprRelatedNode);
void genConstValueNode(AST_NODE* constValueNode);

int get_reg(int float_or_int);
void free_reg(int reg_num, int float_or_int);


int reg_use[2][8] = {0};

int scopelevel = 0, AR_offset = 0, reg_num = 0;
int const_num = 0;



FILE* fptr = NULL;

int get_reg(int float_or_int)
{
    int i=0;

    for(i=0 ; i<8 ; i++)
    {
        if(reg_use[float_or_int][i] == 0)
        {
            reg_use[float_or_int][i] = 1;
            if(float_or_int == INT_TYPE)
                return i+4;
            else 
                return i+16;
        }
    }   
    return -1;
    /*if(i == 8) //out of register need str
    {
        //implement it later
    }*/
}
void free_reg(int reg_num,int float_or_int)
{
    if(float_or_int == INT_TYPE)
        reg_use[float_or_int][reg_num-4] = 0;
    else
        reg_use[float_or_int][reg_num-16] = 0;
}



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
            entry = traverseIDList->semantic_value.identifierSemanticValue.symbolTableEntry;
            fprintf(fptr, "_g_%s: .space  %d\n.text\n"
                , traverseIDList->semantic_value.identifierSemanticValue.identifierName
                , entry->attribute->attr.typeDescriptor->properties.arrayProperties.sizeInEachDimension[0]
                *4
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
    fprintf(fptr, "_frameSize_%s: .word %d\n", functionName, AR_offset*-1+64);
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
            genAssignmentStmt(stmtNode);
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
void genVariableLValue(AST_NODE* idNode)
{   
    if(idNode->dataType == INT_TYPE)
        AR_offset -= 4;
    else 
        AR_offset -= 8;
    idNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset = AR_offset;
}
void genVariableRValue(AST_NODE* idNode)
{//袋檢查
    if(idNode->dataType == INT_TYPE)
    {
        idNode->place = get_reg(INT_TYPE);
    }
    else 
    {
        idNode->place = get_reg(FLOAT_TYPE);
    }
}
void genConstValueNode(AST_NODE* constValueNode)
{
    int reg = 0;
    if(constValueNode->dataType == INT_TYPE)
    {
        reg = get_reg(INT_TYPE);
        constValueNode->place = reg;
        fprintf(fptr, "mov r%d, #%d\n", reg, constValueNode->semantic_value.const1->const_u.intval);
    }
    else 
    {
        reg = get_reg(FLOAT_TYPE);
        constValueNode->place = get_reg(FLOAT_TYPE);
        fprintf(fptr, "vmov s%d, #%f\n", reg, constValueNode->semantic_value.const1->const_u.fval);
    }

}
void genExprRelatedNode(AST_NODE* exprRelatedNode)
{

    switch(exprRelatedNode->nodeType)
    {
    case EXPR_NODE:
        //processExprNode(exprRelatedNode);
        break;
    case STMT_NODE:
        //function call
        //gencheckFunctionCall(exprRelatedNode);
        //exprRelatedNode->place = 0;
        break;
    case IDENTIFIER_NODE:
        //genVariableRValue(exprRelatedNode);
        break;
    case CONST_VALUE_NODE:
        genConstValueNode(exprRelatedNode);
        break;
    default:
        printf("Unhandle case in void processExprRelatedNode(AST_NODE* exprRelatedNode)\n");
        exprRelatedNode->dataType = ERROR_TYPE;
        break;
    }//

}
void genAssignmentStmt(AST_NODE* assignmentNode)
{
    int reg, offset;

    AST_NODE* leftOp = assignmentNode->child;
    AST_NODE* rightOp = leftOp->rightSibling;

    genVariableLValue(leftOp);
    genExprRelatedNode(rightOp);
    SymbolTableEntry* left_entry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
    left_entry = leftOp->semantic_value.identifierSemanticValue.symbolTableEntry;

    reg = rightOp->place;
    offset = left_entry->offset;

    if(left_entry->nestingLevel != 0)
    {
        if(leftOp->dataType == INT_TYPE)
        {
            fprintf(fptr, "str r%d, [fp, #%d]\n", reg, -1*offset);
            free_reg(reg, INT_TYPE);
        }
        else if(leftOp->dataType == FLOAT_TYPE)
        {
            fprintf(fptr, "vstr.f32 s%d, [fp, #%d]\n", reg, -1*offset);
            free_reg(reg, FLOAT_TYPE);
        }
    }
    else
    {
        int left_reg = get_reg(leftOp->dataType);

        
        if(leftOp->dataType == INT_TYPE)
        {
            fprintf(fptr, "ldr r%d, =_g_%s\n", left_reg, leftOp->semantic_value.identifierSemanticValue.identifierName);
            if(leftOp->semantic_value.identifierSemanticValue.kind == NORMAL_ID)
                fprintf(fptr, "str r%d, [r%d, #0]\n", reg, left_reg);
            else if(leftOp->semantic_value.identifierSemanticValue.kind == ARRAY_ID)
                fprintf(fptr, "str r%d, [r%d, #%d]\n", reg, left_reg, 
                    leftOp->child->semantic_value.exprSemanticValue.constEvalValue.iValue*4);//global array
            free_reg(reg, INT_TYPE);
            free_reg(left_reg, INT_TYPE);
        }
        else if(leftOp->dataType == FLOAT_TYPE)
        {
            fprintf(fptr, "ldr s%d, =_g_%s\n", left_reg, leftOp->semantic_value.identifierSemanticValue.identifierName);
            if(leftOp->semantic_value.identifierSemanticValue.kind == NORMAL_ID)
                fprintf(fptr, "vstr.f32 s%d, [r%d, #0]\n", reg, left_reg);
            else if(leftOp->semantic_value.identifierSemanticValue.kind == ARRAY_ID)
                fprintf(fptr, "vstr.f32 s%d, [r%d, #%d]\n", reg, left_reg, 
                    leftOp->child->semantic_value.exprSemanticValue.constEvalValue.iValue*4);//global array
            free_reg(reg, FLOAT_TYPE);
            free_reg(left_reg, INT_TYPE);
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
    AST_NODE* actualParameterList = functionIDNode->rightSibling;
    AST_NODE* actualParameter = actualParameterList->child;

    if(actualParameter->dataType == CONST_STRING_TYPE)
    {
        fprintf(fptr, ".data\n");
        fprintf(fptr, "_CONSTANT_%d: .ascii %s\n", const_num, 
            actualParameter->semantic_value.const1->const_u.sc);
        fprintf(fptr, ".align 2\n.text\n");
        fprintf(fptr, "ldr r4, =_CONSTANT_%d\n", const_num);
        fprintf(fptr, "mov r0, r4\n");
        fprintf(fptr, "bl _write_str\n");
        const_num++;
    }
    else if(actualParameter->dataType == FLOAT_TYPE)
    {
        /*fprintf(fptr, "vldr.f32 s16, [fp, #-8]\n");
        fprintf(fptr, "vmov s0, s16\n");
        fprintf(fptr, "vmov s0, s16\n");*/
    }
    else if(actualParameter->dataType == INT_TYPE)
    {
        /*fprintf(fptr, "ldr r4, [fp, #-4]\n");
        fprintf(fptr, "mov r0, r4\n");
        fprintf(fptr, "bl _write_int\n");*/
    }
    else 
        printf("ERROR type: %d\n", actualParameter->dataType);
}