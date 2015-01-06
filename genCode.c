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
void genReadFunction(AST_NODE* functionCallNode);
void genFreadFunction(AST_NODE* functionCallNode);
void genVariableLValue(AST_NODE* idNode);
void genVariableRValue(AST_NODE* idNode);
void genExprRelatedNode(AST_NODE* exprRelatedNode);
void genConstValueNode(AST_NODE* constValueNode);
void genExprNode(AST_NODE* exprNode);
void genevaluateExprValue(AST_NODE* exprNode);
void genReturnStmt(AST_NODE* returnNode);

int get_reg(int float_or_int);
void free_reg(int reg_num, int float_or_int);


int reg_use[2][8] = {0};

int scopelevel = 0, AR_offset = 0, reg_num = 0;
int const_num = 0, label_num = 0;
int float_label_count;
int fp_num = 0;


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
    fclose(fptr);
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

    if (traverseIDList->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0) 
    {
        fprintf(fptr,".data\n"); //print  .data
        while(traverseIDList)
        {
            SymbolTableEntry* entry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
            switch(traverseIDList->semantic_value.identifierSemanticValue.kind)
            {
            case NORMAL_ID:
                if(typeNode->dataType == INT_TYPE)
                    fprintf(fptr, "_g_%s: .word 0\n", traverseIDList->semantic_value.identifierSemanticValue.identifierName);
                else
                    fprintf(fptr, "_g_%s: .float 0\n", traverseIDList->semantic_value.identifierSemanticValue.identifierName);
                break;
            case ARRAY_ID:
                //only one dimension array in this homework
                entry = traverseIDList->semantic_value.identifierSemanticValue.symbolTableEntry;
                fprintf(fptr, "_g_%s: .space  %d\n\n"
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
        fprintf(fptr, ".text\n");
    }

}
void gendeclareFunction(AST_NODE* declarationNode)
{
    AST_NODE* returnTypeNode = declarationNode->child;
    AST_NODE* functionNameID = returnTypeNode->rightSibling;
    AST_NODE* parameterListNode = functionNameID->rightSibling;
    AST_NODE *blockNode = parameterListNode->rightSibling;
    AR_offset = 0;
    genprologue(functionNameID->semantic_value.identifierSemanticValue.identifierName);
    genblock(blockNode);
    genepilogue(functionNameID->semantic_value.identifierSemanticValue.identifierName);

}
void genblock(AST_NODE* blockNode)
{
    AST_NODE *traverseListNode = blockNode->child;
    while(traverseListNode)
    {
        genGeneralNode(traverseListNode);
        traverseListNode = traverseListNode->rightSibling;
    }
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
    int i=0, output_offset = 0;
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
    fprintf(fptr, ".data\n");
    output_offset =  AR_offset*-1+64;
    while(output_offset%8)
    {
        output_offset++;
    } 
    fprintf(fptr, "_frameSize_%s: .word %d\n", functionName, output_offset);
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
            genReturnStmt(stmtNode);
            break;
        default:
            printf("Unhandle case in void processStmtNode(AST_NODE* stmtNode)\n");
            break;
        }
    }
}
void genReturnStmt(AST_NODE* returnNode)
{
    int reg = -1;
    if(returnNode->dataType == INT_TYPE)
    {
        reg = get_reg(INT_TYPE);
        fprintf(fptr, "mov r%d, #%d\n", reg, returnNode->child->semantic_value.exprSemanticValue.constEvalValue.iValue);
        fprintf(fptr, "mov r0, r%d\n", reg);
        free_reg(reg, INT_TYPE);
    }
    else
    {
        reg = get_reg(FLOAT_TYPE);
        int temp_reg = get_reg(INT_TYPE);
        fprintf(fptr, ".data\n");
        fprintf(fptr, "_fp_%d: .float ", fp_num);
        fprintf(fptr, "%f\n", returnNode->child->semantic_value.exprSemanticValue.constEvalValue.fValue);
        fprintf(fptr, ".text\n");
        fprintf(fptr, "ldr r%d, =_fp_%d\n", temp_reg, fp_num);
        fprintf(fptr, "vldr.f32 s%d, [r%d, #0]\n", reg, temp_reg);
        free_reg(temp_reg, INT_TYPE);
        fp_num++;
        fprintf(fptr, "vmov s0, s%d\n", reg);
        free_reg(temp_reg, INT_TYPE);
        free_reg(reg, FLOAT_TYPE);
    }
}
void genVariableLValue(AST_NODE* idNode)
{   
    AR_offset -= 4;
    idNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset = AR_offset;
}
void genVariableRValue(AST_NODE* idNode)
{
    int reg = 0;
    if(idNode->dataType == INT_TYPE)
    {
        reg = get_reg(INT_TYPE);
        fprintf(fptr, "ldr r%d, [fp, #%d]\n", reg, 
            idNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
        idNode->place = reg;
    }
    else 
    {
        reg = get_reg(FLOAT_TYPE);
        fprintf(fptr, "vldr.f32 s%d, [fp, #%d]\n", reg, 
            idNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
        idNode->place = reg;
    }
}
void genConstValueNode(AST_NODE* constValueNode)
{
    int reg = 0;
    if(constValueNode->dataType == INT_TYPE)
    {
        reg = get_reg(INT_TYPE);
        constValueNode->place = reg;
        fprintf(fptr, "mov r%d, #%d\n", reg, constValueNode->semantic_value.exprSemanticValue.constEvalValue.iValue);
    }
    else 
    {
        reg = get_reg(FLOAT_TYPE);
        int temp_reg = get_reg(INT_TYPE);
        constValueNode->place = reg;
        fprintf(fptr, ".data\n");
        fprintf(fptr, "_fp_%d: .float ", fp_num);
        fprintf(fptr, "%f\n", constValueNode->semantic_value.exprSemanticValue.constEvalValue.fValue);
        fprintf(fptr, ".text\n");
        fprintf(fptr, "ldr r%d, =_fp_%d\n", temp_reg, fp_num);
        fprintf(fptr, "vldr.f32 s%d, [r%d, #0]\n", reg, temp_reg);
        free_reg(temp_reg, INT_TYPE);
        fp_num++;
        //fprintf(fptr, "vmov.f32 s%d, #%f\n", reg, constValueNode->semantic_value.const1->const_u.fval);
    }

}
void genevaluateExprValue(AST_NODE* exprNode)
{
    int left_reg = -1;
    int right_reg = -1;
    int reg = -1;
    if(exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION)
    {
        AST_NODE* leftOp = exprNode->child;
        AST_NODE* rightOp = leftOp->rightSibling;
        if(leftOp->dataType == INT_TYPE && rightOp->dataType == INT_TYPE)
        {
            exprNode->dataType = INT_TYPE;
            left_reg = leftOp->place;
            right_reg = rightOp->place;
            reg = get_reg(INT_TYPE);
            exprNode->place = reg;
            
            switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp)
            {
            case BINARY_OP_ADD:
                fprintf(fptr, "add r%d, r%d, r%d\n", reg, left_reg, right_reg);
                break;
            case BINARY_OP_SUB:
                fprintf(fptr, "sub r%d, r%d, r%d\n", reg, left_reg, right_reg);
                break;
            case BINARY_OP_MUL:
                fprintf(fptr, "mul r%d, r%d, r%d\n", reg, left_reg, right_reg);
                break;
            case BINARY_OP_DIV:
                fprintf(fptr, "sdiv r%d, r%d, r%d\n", reg, left_reg, right_reg);
                break;
            case BINARY_OP_EQ:
                fprintf(fptr, "cmp r%d, r%d\n", left_reg, right_reg);
                fprintf(fptr, "beq _LABEL_%d\n", label_num);
                fprintf(fptr, "mov r%d, #0\n", reg);
                fprintf(fptr, "b _LABELEXIT_%d\n", label_num);
                fprintf(fptr, "_LABEL_%d:\n", label_num);
                fprintf(fptr, "mov r%d, #1\n", reg);
                fprintf(fptr, "_LABELEXIT_%d:\n", label_num);
                label_num++;
                break;
            case BINARY_OP_GE:
                fprintf(fptr, "cmp r%d, r%d\n", left_reg, right_reg);
                fprintf(fptr, "bge _LABEL_%d\n", label_num);
                fprintf(fptr, "mov r%d, #0\n", reg);
                fprintf(fptr, "b _LABELEXIT_%d\n", label_num);
                fprintf(fptr, "_LABEL_%d:\n", label_num);
                fprintf(fptr, "mov r%d, #1\n", reg);
                fprintf(fptr, "_LABELEXIT_%d:\n", label_num);
                label_num++;
                break;
            case BINARY_OP_LE:
                fprintf(fptr, "cmp r%d, r%d\n", left_reg, right_reg);
                fprintf(fptr, "ble _LABEL_%d\n", label_num);
                fprintf(fptr, "mov r%d, #0\n", reg);
                fprintf(fptr, "b _LABELEXIT_%d\n", label_num);
                fprintf(fptr, "_LABEL_%d:\n", label_num);
                fprintf(fptr, "mov r%d, #1\n", reg);
                fprintf(fptr, "_LABELEXIT_%d:\n", label_num);
                label_num++;
                break;
            case BINARY_OP_NE:
                fprintf(fptr, "cmp r%d, r%d\n", left_reg, right_reg);
                fprintf(fptr, "bne _LABEL_%d\n", label_num);
                fprintf(fptr, "mov r%d, #0\n", reg);
                fprintf(fptr, "b _LABELEXIT_%d\n", label_num);
                fprintf(fptr, "_LABEL_%d:\n", label_num);
                fprintf(fptr, "mov r%d, #1\n", reg);
                fprintf(fptr, "_LABELEXIT_%d:\n", label_num);
                label_num++;
                break;
            case BINARY_OP_GT:
                fprintf(fptr, "cmp r%d, r%d\n", left_reg, right_reg);
                fprintf(fptr, "bgt _LABEL_%d\n", label_num);
                fprintf(fptr, "mov r%d, #0\n", reg);
                fprintf(fptr, "b _LABELEXIT_%d\n", label_num);
                fprintf(fptr, "_LABEL_%d:\n", label_num);
                fprintf(fptr, "mov r%d, #1\n", reg);
                fprintf(fptr, "_LABELEXIT_%d:\n", label_num);
                label_num++;
                break;
            case BINARY_OP_LT:
                fprintf(fptr, "cmp r%d, r%d\n", left_reg, right_reg);
                fprintf(fptr, "blt _LABEL_%d\n", label_num);
                fprintf(fptr, "mov r%d, #0\n", reg);
                fprintf(fptr, "b _LABELEXIT_%d\n", label_num);
                fprintf(fptr, "_LABEL_%d:\n", label_num);
                fprintf(fptr, "mov r%d, #1\n", reg);
                fprintf(fptr, "_LABELEXIT_%d:\n", label_num);
                label_num++;
                break;
            case BINARY_OP_AND:
                //fprintf(fptr, "and r%d, r%d, r%d\n", reg, left_reg, right_reg);
                
                break;
            case BINARY_OP_OR:
                //fprintf(fptr, "orr r%d, r%d, r%d\n", reg, left_reg, right_reg);
                break;
            default:
                printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
                break;
            }
            free_reg(left_reg, INT_TYPE);
            free_reg(right_reg, INT_TYPE);
            return ;
        }
        else //float binary
        {
            left_reg = leftOp->place;
            right_reg = rightOp->place;
            reg = get_reg(FLOAT_TYPE);
            exprNode->place = reg;
            switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp)
            {
            case BINARY_OP_ADD:
                fprintf(fptr, "vadd.f32 s%d, s%d, s%d\n", reg, left_reg, right_reg);
                break;
            case BINARY_OP_SUB:
                fprintf(fptr, "vsub.f32 s%d, s%d, s%d\n", reg, left_reg, right_reg);
                break;
            case BINARY_OP_MUL:
                fprintf(fptr, "vmul.f32 s%d, s%d, s%d\n", reg, left_reg, right_reg);
                break;
            case BINARY_OP_DIV:
                fprintf(fptr, "vdiv.f32 s%d, s%d, s%d\n", reg, left_reg, right_reg);
                break;
            case BINARY_OP_EQ:
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = leftValue == rightValue;
                free_reg(reg ,FLOAT_TYPE);
                reg = get_reg(INT_TYPE);
                exprNode->place = reg;
                fprintf(fptr, "\tldr r%d, =1\n", reg);
                fprintf(fptr, "\tvcmp.f32 s%d, s%d\n", left_reg, right_reg);
                fprintf(fptr, "\tVMRS APSR_nzcv, FPSCR\n");
                fprintf(fptr, "\tbeq FLOAT_LABEL%d\n", float_label_count);
                fprintf(fptr, "\tldr r%d, =0\n", reg);
                fprintf(fptr, "FLOAT_LABEL%d:\n", float_label_count);
                float_label_count++;
                break;
            case BINARY_OP_GE:
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = leftValue >= rightValue;
                free_reg(reg ,FLOAT_TYPE);
                reg = get_reg(INT_TYPE);
                exprNode->place = reg;
                fprintf(fptr, "\tldr r%d, =1\n", reg);
                fprintf(fptr, "\tvcmp.f32 s%d, s%d\n", left_reg, right_reg);
                fprintf(fptr, "\tVMRS APSR_nzcv, FPSCR\n");
                fprintf(fptr, "\tbge FLOAT_LABEL%d\n", float_label_count);
                fprintf(fptr, "\tldr r%d, =0\n", reg);
                fprintf(fptr, "FLOAT_LABEL%d:\n", float_label_count);
                float_label_count++;
                break;
            case BINARY_OP_LE:
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = leftValue <= rightValue;
                free_reg(reg ,FLOAT_TYPE);
                reg = get_reg(INT_TYPE);
                exprNode->place = reg;
                fprintf(fptr, "\tldr r%d, =1\n", reg);
                fprintf(fptr, "\tvcmp.f32 s%d, s%d\n", left_reg, right_reg);
                fprintf(fptr, "\tVMRS APSR_nzcv, FPSCR\n");
                fprintf(fptr, "\tble FLOAT_LABEL%d\n", float_label_count);
                fprintf(fptr, "\tldr r%d, =0\n", reg);
                fprintf(fptr, "FLOAT_LABEL%d:\n", float_label_count);
                float_label_count++;
                break;
            case BINARY_OP_NE:
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = leftValue != rightValue;
                free_reg(reg ,FLOAT_TYPE);
                reg = get_reg(INT_TYPE);
                exprNode->place = reg;
                fprintf(fptr, "\tldr r%d, =1\n", reg);
                fprintf(fptr, "\tvcmp.f32 s%d, s%d\n", left_reg, right_reg);
                fprintf(fptr, "\tVMRS APSR_nzcv, FPSCR\n");
                fprintf(fptr, "\tbne FLOAT_LABEL%d\n", float_label_count);
                fprintf(fptr, "\tldr r%d, =0\n", reg);
                fprintf(fptr, "FLOAT_LABEL%d:\n", float_label_count);
                float_label_count++;
                break;
            case BINARY_OP_GT:
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = leftValue > rightValue;
                free_reg(reg ,FLOAT_TYPE);
                reg = get_reg(INT_TYPE);
                exprNode->place = reg;
                fprintf(fptr, "ldr r%d, =1\n", reg);
                fprintf(fptr, "vcmp.f32 s%d, s%d\n", left_reg, right_reg);
                fprintf(fptr, "VMRS APSR_nzcv, FPSCR\n");
                fprintf(fptr, "bgt FLOAT_LABEL%d\n", float_label_count);
                fprintf(fptr, "ldr r%d, =0\n", reg);
                fprintf(fptr, "FLOAT_LABEL%d:\n", float_label_count);
                float_label_count++;
                break;
            case BINARY_OP_LT:
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = leftValue < rightValue;
                free_reg(reg ,FLOAT_TYPE);
                reg = get_reg(INT_TYPE);
                exprNode->place = reg;
                fprintf(fptr, "ldr r%d, =1\n", reg);
                fprintf(fptr, "vcmp.f32 s%d, s%d\n", left_reg, right_reg);
                fprintf(fptr, "VMRS APSR_nzcv, FPSCR\n");
                fprintf(fptr, "blt FLOAT_LABEL%d\n", float_label_count);
                fprintf(fptr, "ldr r%d, =0\n", reg);
                fprintf(fptr, "FLOAT_LABEL%d:\n", float_label_count);
                float_label_count++;
                break;
            case BINARY_OP_AND:
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = leftValue && rightValue;
                
                break;
            case BINARY_OP_OR:
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = leftValue || rightValue;
                break;
            default:
                printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
                break;
            }
            free_reg(left_reg, FLOAT_TYPE);
            free_reg(right_reg, FLOAT_TYPE);
            return ;
        }
    }
    else //unary
    {
        AST_NODE* operand = exprNode->child;
        if(operand->dataType == INT_TYPE)
        {
            left_reg = operand->place;
            reg = get_reg(INT_TYPE);
            exprNode->place = reg;
            switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp)
            {
            case UNARY_OP_POSITIVE:
                fprintf(fptr, "ldr r%d, r%d\n", reg, left_reg);
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = operandValue;
                break;
            case UNARY_OP_NEGATIVE:
                fprintf(fptr, "neg r%d, r%d\n", reg, left_reg);
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = -operandValue;
                break;
            case UNARY_OP_LOGICAL_NEGATION:
                //fprintf(fptr, "mvn r%d, r%d\n", reg, left_reg);
                fprintf(fptr, "ldr r%d, =1\n", reg);
                fprintf(fptr, "cmp r%d, #0\n", left_reg);
                fprintf(fptr, "beq  INT_LABEL%d\n", label_num);
                
                fprintf(fptr, "ldr r%d, =0\n", reg);
                fprintf(fptr, "INT_LABEL%d:\n", label_num);
                label_num++;
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = !operandValue;
                break;
            default:
                printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
                break;
            }
            free_reg(left_reg, INT_TYPE);
        }
        else //float unary
        {
            left_reg = operand->place;
            reg = get_reg(FLOAT_TYPE);
            exprNode->place = reg;
            switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp)
            {
            case UNARY_OP_POSITIVE:
                fprintf(fptr, "vldr.f32 s%d, s%d\n", reg, left_reg);
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = operandValue;
                break;
            case UNARY_OP_NEGATIVE:
                fprintf(fptr, "vneg.f32 s%d, s%d\n", reg, left_reg);
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = -operandValue;
                break;
            case UNARY_OP_LOGICAL_NEGATION:
                //會當掉==
                fprintf(fptr, "vldr.f32 s%d, =1\n", reg);
                fprintf(fptr, "vcmp.f32 s%d, #0\n", left_reg);
                fprintf(fptr, "VMRS APSR_nzcv, FPSCR\n");
                fprintf(fptr, "beq  FLOAT_LABEL%d\n", float_label_count);
                
                fprintf(fptr, "vldr.f32 s%d, =0\n", reg);
                fprintf(fptr, "FLOAT_LABEL%d:\n", float_label_count);
                float_label_count++;
                //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = !operandValue;
                break;
            default:
                printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
                break;
            }
        }
    }
}
void genExprNode(AST_NODE* exprNode)
{//加上constant folding 以外的地方
    int reg = 0;
    if(exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION)
    {
        AST_NODE* leftOp = exprNode->child;
        AST_NODE* rightOp = leftOp->rightSibling;
        genExprRelatedNode(leftOp);
        genExprRelatedNode(rightOp);

        genevaluateExprValue(exprNode);

        free_reg(leftOp->place, leftOp->dataType);
        free_reg(rightOp->place, leftOp->dataType);
    }
    else
    {
        AST_NODE* operand = exprNode->child;
        genExprRelatedNode(operand);
        genevaluateExprValue(exprNode);
            //同上
        free_reg(operand->place, operand->dataType);
    }
}
void genExprRelatedNode(AST_NODE* exprRelatedNode)
{

    switch(exprRelatedNode->nodeType)
    {
    case EXPR_NODE:
        genExprNode(exprRelatedNode);
        break;
    case STMT_NODE:
        //function call
        gencheckFunctionCall(exprRelatedNode);
        int reg = 0;
        if(exprRelatedNode->dataType == INT_TYPE)
        {
            reg = get_reg(INT_TYPE);
            fprintf(fptr, "mov r%d, r0\n", reg);
        }
        else
        {
            reg = get_reg(FLOAT_TYPE);
            fprintf(fptr, "vmov s%d, s0\n", reg);
        }
        exprRelatedNode->place = reg; //s0 or r0
        break;
    case IDENTIFIER_NODE:
        genVariableRValue(exprRelatedNode);
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

    
    genExprRelatedNode(rightOp);
    SymbolTableEntry* left_entry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
    left_entry = leftOp->semantic_value.identifierSemanticValue.symbolTableEntry;

    reg = rightOp->place;
    
    if(left_entry->nestingLevel != 0)
    {
        genVariableLValue(leftOp);//local variable 才需要用到,global 不用
        offset = left_entry->offset;
        if(leftOp->dataType == INT_TYPE)
        {
            fprintf(fptr, "str r%d, [fp, #%d]\n", reg, offset);
            free_reg(reg, INT_TYPE);
        }
        else if(leftOp->dataType == FLOAT_TYPE)
        {
            fprintf(fptr, "vstr.f32 s%d, [fp, #%d]\n", reg, offset);
            free_reg(reg, FLOAT_TYPE);
        }
    }
    else
    {
        int left_reg = get_reg(INT_TYPE);
        
        if(leftOp->dataType == INT_TYPE)
        {
            fprintf(fptr, "ldr r%d, =_g_%s\n", left_reg, leftOp->semantic_value.identifierSemanticValue.identifierName);
            if(leftOp->semantic_value.identifierSemanticValue.kind == NORMAL_ID)
                fprintf(fptr, "str r%d, [r%d, #0]\n", reg, left_reg);
            else if(leftOp->semantic_value.identifierSemanticValue.kind == ARRAY_ID)
                fprintf(fptr, "str r%d, [r%d, #%d]\n", reg, left_reg, 
                    leftOp->child->semantic_value.exprSemanticValue.constEvalValue.iValue*(-4));//global array
            free_reg(reg, INT_TYPE);
            free_reg(left_reg, INT_TYPE);
        }
        else if(leftOp->dataType == FLOAT_TYPE)
        {
            fprintf(fptr, "ldr r%d, =_g_%s\n", left_reg, leftOp->semantic_value.identifierSemanticValue.identifierName);
            if(leftOp->semantic_value.identifierSemanticValue.kind == NORMAL_ID)
                fprintf(fptr, "vstr.f32 s%d, [r%d, #0]\n", reg, left_reg);
            else if(leftOp->semantic_value.identifierSemanticValue.kind == ARRAY_ID)
                fprintf(fptr, "vstr.f32 s%d, [r%d, #%d]\n", reg, left_reg, 
                    leftOp->child->semantic_value.exprSemanticValue.constEvalValue.iValue*(-4));//global array
            free_reg(reg, FLOAT_TYPE);
            free_reg(left_reg, FLOAT_TYPE);
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
    else if(strcmp(functionIDNode->semantic_value.identifierSemanticValue.identifierName, "read") == 0)
    {
        genReadFunction(functionCallNode);
        return;
    }
    else if(strcmp(functionIDNode->semantic_value.identifierSemanticValue.identifierName, "fread") == 0)
    {
        genFreadFunction(functionCallNode);
        return;
    }    
    else
        fprintf(fptr, "\nbl _start_%s\n", functionIDNode->semantic_value.identifierSemanticValue.identifierName);
}
void genWriteFunction(AST_NODE* functionCallNode)
{
    int reg = 0, offset = 0;
    //參數沒辦法吃進一個算式
    AST_NODE* functionIDNode = functionCallNode->child;
    AST_NODE* actualParameterList = functionIDNode->rightSibling;
    AST_NODE* actualParameter = actualParameterList->child;

    if(actualParameter->dataType == CONST_STRING_TYPE)
    {
        reg = get_reg(INT_TYPE);
        fprintf(fptr, ".data\n");
        fprintf(fptr, "_CONSTANT_%d: .ascii %s\n", const_num, 
            actualParameter->semantic_value.const1->const_u.sc);
        fprintf(fptr, ".align 2\n.text\n");
        fprintf(fptr, "ldr r%d, =_CONSTANT_%d\n", reg, const_num);
        fprintf(fptr, "mov r0, r%d\n", reg);
        fprintf(fptr, "bl _write_str\n");
        free_reg(reg, INT_TYPE);
        const_num++;
    }
    else if(actualParameter->dataType == FLOAT_TYPE)
    {
        reg = get_reg(FLOAT_TYPE);
        int temp_reg = get_reg(INT_TYPE);
        if(actualParameter->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0)
        {
            fprintf(fptr, "ldr r%d, =_g_%s\n", temp_reg, 
                actualParameter->semantic_value.identifierSemanticValue.identifierName);
            fprintf(fptr, "vldr.f32 s%d, [r%d, #0]\n", reg, temp_reg);
        }
        else
        {
            fprintf(fptr, "vldr.f32 s%d, [fp, #%d]\n", reg, 
            actualParameter->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
        }
        fprintf(fptr, "vmov.f32 s0, s%d\n", reg);
        fprintf(fptr, "bl _write_float\n");
        free_reg(reg, FLOAT_TYPE);
        free_reg(temp_reg, INT_TYPE);
    }
    else if(actualParameter->dataType == INT_TYPE)
    {
        reg = get_reg(INT_TYPE);
        int temp_reg = get_reg(INT_TYPE);
        if(actualParameter->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0)
        {
            fprintf(fptr, "ldr r%d, =_g_%s\n", temp_reg, 
                actualParameter->semantic_value.identifierSemanticValue.identifierName);
            fprintf(fptr, "ldr r%d, [r%d, #0]\n", reg, temp_reg);   
        }
        else
        {
            fprintf(fptr, "ldr r%d, [fp, #%d]\n", reg,
            actualParameter->semantic_value.identifierSemanticValue.symbolTableEntry->offset);
        }
        fprintf(fptr, "mov r0, r%d\n", reg);
        fprintf(fptr, "bl _write_int\n");
        free_reg(reg, INT_TYPE);
        free_reg(temp_reg, INT_TYPE);
    }
    else 
        printf("ERROR type: %d\n", actualParameter->dataType);
}
void genReadFunction(AST_NODE* functionCallNode)
{
    fprintf(fptr, "bl _read_int\n");
}
void genFreadFunction(AST_NODE* functionCallNode)
{
    fprintf(fptr, "bl _read_float\n");
}