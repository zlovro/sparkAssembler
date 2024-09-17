#pragma once

#include <algorithm>
#include <types.hpp>
#include <utility>

#include "cpu.hpp"
#include "SafeList.hpp"
#include "log.hpp"

using namespace std;

namespace SPARK::Assembler::Analysis
{
    bool humanChar(unsigned char pChar)
    {
        return pChar >= 0x20 && pChar <= 0x82;
    }

    int stringToInt(Cpu::SparkAssemblerContext* pCtx, const string& pStr, size_t* pOffset = nullptr, int pBase = 10)
    {
        try
        {
            return stoi(pStr, pOffset, pBase);
        }
        catch (...)
        {
            pCtx->error(format("Could not convert string '{0}' to integer.\n", pStr));
            return -1;
        }
    }

    Reg stringToOperandValue(Cpu::SparkAssemblerContext* pCtx, const std::string& pString)
    {
        int base = 10;
        int offset = 0;

        Cpu::ESparkExternalRegister regVal = Cpu::stringRegisterToRegisterValue(pString);
        if (regVal != Cpu::INVREG)
        {
            return regVal;
        }

        if (pString.contains("0x"))
        {
            base = 16;
            offset = 2;
        }

        if (pString.contains("0b"))
        {
            base = 2;
            offset = 2;
        }

        string toInt = pString.substr(offset, pString.length() - offset);
        return stringToInt(pCtx, toInt, nullptr, base);
    }

    string operandValueToString(Reg pValue, Cpu::ESparkOperandType pOperandType, size_t pBitLength)
    {
        string operandStr;

        switch (pOperandType)
        {
        case Cpu::REGISTER:
            {
                operandStr = Cpu::gRegisterNameTable[pValue];
            }
            break;

        case Cpu::IMMEDIATE:
            {
                switch (pBitLength)
                {
                case 1:
                    {
                        operandStr = format("0x{0:X}", pValue);
                    }
                    break;
                case 8:
                    {
                        string prefix;
                        auto valueSigned = static_cast<int8_t>(pValue);
                        if (valueSigned < 0)
                        {
                            valueSigned = -valueSigned;
                            prefix = "-";
                        }
                        operandStr = format("{0}0x{1:X}", prefix, valueSigned);
                    }
                    break;
                case 16:
                    {
                        string prefix;
                        auto valueSigned = static_cast<int8_t>(pValue);
                        if (valueSigned < 0)
                        {
                            valueSigned = -valueSigned;
                            prefix = "-";
                        }
                        operandStr = format("{0}0x{1:X}", prefix, valueSigned);
                    }
                    break;
                case 32:
                    {
                        string prefix;
                        auto valueSigned = static_cast<int8_t>(pValue);
                        if (valueSigned < 0)
                        {
                            valueSigned = -valueSigned;
                            prefix = "-";
                        }
                        operandStr = format("{0}0x{1:X}", prefix, valueSigned);
                    }
                    break;
                default:
                    {
                        LOGWRN("The immediate 0x{0:X} has a bit length of {1}, which is not supported (8, 16, 32).\n", pValue, pBitLength);
                    }
                    break;
                }
            }
            break;
        }

        return operandStr;
    }

    bool currentAssemblyLineHasLabel(Cpu::SparkAssemblerContext* pCtx)
    {
        return static_cast<int>(pCtx->currentLine->cleanLineContentsPtr->find(':')) > 0;
    }

    void parseLabelFromCurrentAssemblyLine(Cpu::SparkAssemblerContext* pCtx)
    {
        string cleanLine = *pCtx->currentLine->cleanLineContentsPtr;

        int idx = cleanLine.find(':');
        string labelName = cleanLine.substr(0, idx);
        pCtx->labels.add(new Cpu::SparkAssemblerLabel(*pCtx->currentLine->cpuLineNumberPtr * 4, labelName));
    }

    bool currentAssemblyLineHasInclude(Cpu::SparkAssemblerContext* pCtx)
    {
        return pCtx->currentLine->cleanLineContentsPtr->starts_with("#include ");
    }

    bool currentAssemblyLineHasIncludePath(Cpu::SparkAssemblerContext* pCtx)
    {
        return pCtx->currentLine->cleanLineContentsPtr->starts_with("#includePath");
    }

    bool isValidStringRegister(const string& pRegisterStr)
    {
        return std::ranges::any_of(Cpu::gRegisterNameTable, [pRegisterStr](const string& pX) { return pX == pRegisterStr; });
    }

    bool currentAssemblyLineHasRegisterMacro(Cpu::SparkAssemblerContext* pCtx)
    {
        string cleanLine = *pCtx->currentLine->cleanLineContentsPtr;
        int equalsSignIndex = cleanLine.find('=');

        if (equalsSignIndex <= 0)
        {
            return false;
        }

        if (std::ranges::count(cleanLine, '=') != 1)
        {
            return false;
        }

        string registerStr = cleanLine.substr(equalsSignIndex + 1);

        if (!isValidStringRegister(registerStr))
        {
            return false;
        }

        return true;
    }

    void parseRegisterMacroFromCurrentLine(Cpu::SparkAssemblerContext* pCtx)
    {
        string cleanLine = *pCtx->currentLine->cleanLineContentsPtr;
        int equalsSignIndex = cleanLine.find('=');

        string registerStr = cleanLine.substr(equalsSignIndex + 1);
        string representation = cleanLine.substr(0, equalsSignIndex);

        pCtx->setRegisterMacro(representation, Cpu::stringRegisterToRegisterValue(registerStr));

        if (pCtx->isError())
        {
            return;
        }
    }

    enum EAssemblyLineType
    {
        INV_LINE_TYPE = -1,
        EMPTY,
        EXECUTABLE,
        LABEL,
        REGISTER_MACRO,
    };

    EAssemblyLineType getCurrentLineType(Cpu::SparkAssemblerContext* pCtx)
    {
        string lineClean = *pCtx->currentLine->cleanLineContentsPtr;

        if (currentAssemblyLineHasLabel(pCtx))
        {
            return LABEL;
        }

        if (currentAssemblyLineHasRegisterMacro(pCtx))
        {
            return REGISTER_MACRO;
        }

        return EXECUTABLE;
    }


    string cleanupAssemblyLine(const string& pLine)
    {
        string tempLine = pLine;
        int off = pLine.find(';');

        if (off != -1)
        {
            tempLine = pLine.substr(0, off);
        }

        SafeList<char> trimmed(tempLine);

        size_t index = 0;
        size_t spaceCount = 0;

        char previousChar = '\0';

        while (true)
        {
            char nextChar;
            if (index == 1 && trimmed.count() == 1)
            {
                return "";
            }

            if (index >= trimmed.count())
            {
                break;
            }

            if (index > 0)
            {
                previousChar = trimmed[index - 1];
            }

            if (index < trimmed.count() - 1)
            {
                nextChar = trimmed[index + 1];
            }
            else
            {
                nextChar = '\0';
            }

            if (trimmed[0] == ' ')
            {
                trimmed.removeAt(0);
                continue;
            }

            char currentChar = trimmed[index];

            if (currentChar == ',' && nextChar == ' ')
            {
                trimmed.removeAt(index + 1);
                continue;
            }
            
            if (currentChar == ' ')
            {
                spaceCount++;

                if (previousChar == ' ')
                {
                    trimmed.removeAt(index);
                    continue;
                }

                if (nextChar == '=')
                {
                    trimmed.removeAt(index);
                    continue;
                }

                if (previousChar == '=')
                {
                    trimmed.removeAt(index);
                    continue;
                }

                if (index + 1 == trimmed.count() - 1)
                {
                    trimmed.removeAt(index);
                    continue;
                }
            }
            if (!humanChar(currentChar))
            {
                trimmed.removeAt(index);
                continue;
            }

            index++;
        }

        return {
            trimmed.begin(),
            trimmed.end()
        };
    }


    void getOpcodeFromCurrentAssemblyLine(Cpu::SparkAssemblerContext* pCtx, string* pOpcodeOut)
    {
        Cpu::AssemblyLine* line = pCtx->currentLine;

        if (!line)
        {
            pCtx->error("pCtx->currentLine was null.\n");
            return;
        }

        string* cleanLineStr = line->cleanLineContentsPtr;

        if (!cleanLineStr->contains(' '))
        {
            *pOpcodeOut = *cleanLineStr;
            pCtx->success();
            return;
        }

        for (size_t i = 0; i < cleanLineStr->size(); i++)
        {
            char currentChar = cleanLineStr->at(i);

            if (currentChar == ' ')
            {
                *pOpcodeOut = cleanLineStr->substr(0, i);
                return;
            }
        }

        pCtx->error(format("No opcode found in line '{0}'.\n", *line->rawLineContentsPtr));
    }

    size_t getOperandCountFromCurrentAssemblyLine(Cpu::SparkAssemblerContext* pCtx)
    {
        string cleanLineContents = *pCtx->currentLine->cleanLineContentsPtr;

        bool hasSpace = static_cast<int>(cleanLineContents.find(' ')) > 0;
        if (!hasSpace)
        {
            return 0;
        }

        return 1 + std::ranges::count(cleanLineContents, ',');
    }

    void getOperandsFromCurrentAssemblyLine(Cpu::SparkAssemblerContext* pCtx, SafeList<Reg>* pOutOperands, SafeList<string>* pRawOperands)
    {
        string cleanLine = *pCtx->currentLine->cleanLineContentsPtr;
        int spaceOffset = cleanLine.find(' ') + 1;

        size_t operandCount = getOperandCountFromCurrentAssemblyLine(pCtx);

        size_t commaOffset = 0;
        for (size_t i = 0; i < operandCount; i++)
        {
            string operandsStr = cleanLine.substr(spaceOffset);

            int newCommaOffset = operandsStr.find(',', commaOffset + 1);

            if (commaOffset != 0)
            {
                commaOffset++;
            }
            string rawOperand = operandsStr.substr(commaOffset, newCommaOffset == -1 ? string::npos : newCommaOffset - commaOffset);

            if (rawOperand.contains('\''))
            {
                pRawOperands->add(rawOperand.substr(1, rawOperand.size() - 2));
                continue;
            }

            if (newCommaOffset > 0)
            {
                commaOffset = newCommaOffset;
            }
            else
            {
                commaOffset += rawOperand.size();
            }

            pRawOperands->add(rawOperand);

            Reg operandValue;
            if (pCtx->registerMacroExists(rawOperand))
            {
                operandValue = pCtx->getRegisterFromRegisterMacroRepresentation(rawOperand);
            }
            else
            {
                operandValue = stringToOperandValue(pCtx, rawOperand);
            }

            if (pCtx->isError())
            {
                return;
            }

            pOutOperands->add(operandValue);
        }
    }


    void parseInstructionFromCurrentAssemblyLine(Cpu::SparkAssemblerContext* pCtx, Cpu::SparkInstructionInstance** pOutInstructionInstance)
    {
        string opcodeStr;
        string currentPart;

        SafeList<Reg> operands;
        SafeList<string> rawOperands;

        string lineContents = *pCtx->currentLine->cleanLineContentsPtr;

        if (lineContents.empty())
        {
            pCtx->ignore("The line is too short.");
            return;
        }

        getOpcodeFromCurrentAssemblyLine(pCtx, &opcodeStr);

        if (pCtx->isError())
        {
            return;
        }

        getOperandsFromCurrentAssemblyLine(pCtx, &operands, &rawOperands);

        if (pCtx->isError())
        {
            return;
        }

        Cpu::ESparkInstructionOpcodeId opcodeId = Cpu::getOpcodeIdFromOpcodeStr(opcodeStr);

        if (opcodeId == Cpu::INVOP)
        {
            Cpu::ESparkInstructionMacroOpcodeId macroOpcodeId = Cpu::getMacroOpcodeIdFromOpcodeStr(opcodeStr);

            if (macroOpcodeId != Cpu::INVMACRO)
            {
                Cpu::SparkInstructionMacroType* macroType = getMacroTypeFromId(macroOpcodeId);
                Cpu::ESparkInstructionOpcodeId baseOpcodeId = macroType->baseOpcodeId;

                pCtx->currentInstruction = new Cpu::SparkInstructionInstance(baseOpcodeId, operands, rawOperands);
                *pOutInstructionInstance = new Cpu::SparkInstructionInstance(baseOpcodeId, macroType->parserFunction(pCtx), rawOperands);

                pCtx->success();
                return;
            }

            pCtx->error(format("Opcode '{0}' was not found in the opcode database nor the macro database.", opcodeStr));
            return;
        }

        pCtx->success();
        *pOutInstructionInstance = new Cpu::SparkInstructionInstance(opcodeId, operands, rawOperands);
    }

    string getIncludeFileName(const string& pCleanLine)
    {
        string fileName = pCleanLine.substr(pCleanLine.find('\''));
        fileName = fileName.substr(1, fileName.size() - 2);
        return fileName;
    }

    string getIncludePathName(const string& pCleanLine)
    {
        return getIncludeFileName(pCleanLine);
    }

    void expandRawIncludeRecursively(Cpu::SparkAssemblerContext* pCtx, const string& pFileName, SafeList<string>* pOutLines)
    {
        std::ifstream file(pFileName);

        string line;

        pCtx->setCurrentFile(pFileName);

        while (std::getline(file, line))
        {
            pCtx->incrementAssemblerLineNumber();

            string cleanLine = cleanupAssemblyLine(line);
            if (cleanLine.contains("#include "))
            {
                string fileName = getIncludeFileName(cleanLine);
                expandRawIncludeRecursively(pCtx, fileName, pOutLines);

                if (pCtx->isError())
                {
                    return;
                }

                break;
            }
            if (cleanLine.contains("#includePath"))
            {
                pCtx->addIncludePath(getIncludePathName(cleanLine));
                continue;
            }

            pOutLines->insert(0, line);
        }
    }

    void expandCurrentIncludeRecursively(Cpu::SparkAssemblerContext* pCtx, SafeList<string>* pOutLines)
    {
        string includeFileName = getIncludeFileName(*pCtx->currentLine->rawLineContentsPtr);

        expandRawIncludeRecursively(pCtx, includeFileName, pOutLines);
    }
}

namespace SPARK::Assembler
{
    Reg assembleOperands(SafeList<size_t>* pOperandBitLengths, SafeList<Reg>* pOperandValues, Cpu::SparkAssemblerContext* pCtx)
    {
        Reg customData = 0;
        size_t position = 6;

        if (pOperandBitLengths->count() != pOperandValues->count())
        {
            pCtx->error(format("The number of provided operands ({0}) is not equal to the expected number of operands ({1}).", pOperandValues->count(), pOperandBitLengths->count()));

            return 0;
        }

        for (size_t i = 0; i < pOperandBitLengths->count(); i++)
        {
            Reg value = pOperandValues->at(i);
            size_t bitLength = pOperandBitLengths->at(i);

            customData |= value << 32 - bitLength - position;
            position += bitLength;
        }

        pCtx->success();

        return customData;
    }

    string disasemble(Reg pAssembled, Cpu::SparkAssemblerContext* pCtx)
    {
        string line;

        auto opcodeId = static_cast<Cpu::ESparkInstructionOpcodeId>((pAssembled & 0b111111 << 26) >> 26);
        Cpu::SparkInstructionType* instructionType = getInstructionTypeFromOpcodeId(opcodeId);

        if (instructionType == nullptr)
        {
            pCtx->error(format("No instruction type structure was found under opcode '{0}' (binary '{1:06B}') extracted from assembled instruction '{2:08X}'.", static_cast<Reg>(opcodeId), static_cast<Reg>(opcodeId), pAssembled));
            return "";
        }

        line = getOpcodeStrFromOpcodeId(opcodeId);

        size_t position = 26;
        SafeList<Reg> operandValues;

        for (size_t i = 0; i < instructionType->operandCount; i++)
        {
            size_t operandLength = instructionType->operandLengths[i];
            Cpu::ESparkOperandType operandType = instructionType->operandTypes[i];

            position -= operandLength;

            Reg mask = (2 << operandLength - 1) - 1;
            Reg operandValue = (pAssembled & mask << position) >> position;
            operandValues.add(operandValue);

            string operandPrefix = i > 0 ? ", " : " ";
            string operandStr = operandPrefix + Analysis::operandValueToString(operandValue, operandType, operandLength);

            line += operandStr;
        }

        pCtx->success();
        return line;
    }

#define OPTYPE(operandType, bitLength) Cpu::##operandType, (size_t)##bitLength

    void initAssembler()
    {
        Cpu::SparkInstructionType::create("liw", Cpu::LIW, 3, OPTYPE(REGISTER, 5), OPTYPE(IMMEDIATE, 16), OPTYPE(IMMEDIATE, 1));
        Cpu::SparkInstructionType::create("addi", Cpu::ADDI, 3, OPTYPE(REGISTER, 5), OPTYPE(REGISTER, 5), OPTYPE(IMMEDIATE, 16));
        Cpu::SparkInstructionType::create("add", Cpu::ADD, 3, OPTYPE(REGISTER, 5), OPTYPE(REGISTER, 5), OPTYPE(REGISTER, 5));
        Cpu::SparkInstructionType::create("mov", Cpu::MOV, 2, OPTYPE(REGISTER, 5), OPTYPE(REGISTER, 5));
        Cpu::SparkInstructionType::create("cmpr", Cpu::CMPR, 2, OPTYPE(REGISTER, 5), OPTYPE(REGISTER, 5));
        Cpu::SparkInstructionType::create("cmpi", Cpu::CMPI, 2, OPTYPE(REGISTER, 5), OPTYPE(IMMEDIATE, 16));
        Cpu::SparkInstructionType::create("jmpcr", Cpu::JMPCR, 2, OPTYPE(REGISTER, 5), OPTYPE(IMMEDIATE, 16));
        Cpu::SparkInstructionType::create("jmp", Cpu::JMP, 1, OPTYPE(REGISTER, 5));

        Cpu::SparkInstructionMacroType::create("inc", Cpu::INC, Cpu::ADDI, SPARK_INSTRUCTION_MACRO_EXPAND(OP(0), OP(0), 1));

        Cpu::SparkInstructionMacroType::create("liwl", Cpu::LIWL, Cpu::LIW, SPARK_INSTRUCTION_MACRO_EXPAND(OP(0), OP(1), 0));
        Cpu::SparkInstructionMacroType::create("liwh", Cpu::LIWH, Cpu::LIW, SPARK_INSTRUCTION_MACRO_EXPAND(OP(0), OP(1), 1));

        Cpu::SparkInstructionMacroType::create("jmpeq", Cpu::JMPEQ, Cpu::JMPCR, SPARK_INSTRUCTION_MACRO_EXPAND(OP(0), Cpu::EQUAL));
        Cpu::SparkInstructionMacroType::create("jmpl", Cpu::JMPL, Cpu::JMPCR, SPARK_INSTRUCTION_MACRO_EXPAND(OP(0), Cpu::LESS));
        Cpu::SparkInstructionMacroType::create("jmpleq", Cpu::JMPLEQ, Cpu::JMPCR, SPARK_INSTRUCTION_MACRO_EXPAND(OP(0), Cpu::LESS_OR_EQUAL));
        Cpu::SparkInstructionMacroType::create("jmpg", Cpu::JMPG, Cpu::JMPCR, SPARK_INSTRUCTION_MACRO_EXPAND(OP(0), Cpu::GREATER));
        Cpu::SparkInstructionMacroType::create("jmpgeq", Cpu::JMPGEQ, Cpu::JMPCR, SPARK_INSTRUCTION_MACRO_EXPAND(OP(0), Cpu::GREATER_OR_EQUAL));

        Cpu::SparkInstructionMacroType::create("labreg", Cpu::LABREG, Cpu::ADDI, SPARK_INSTRUCTION_MACRO_EXPAND(OP(0), Cpu::PC, x->findLabel(RAWOP(1))->offset - static_cast<Reg>(*x->currentLine->cpuLineNumberPtr - 1) * 4));
        Cpu::SparkInstructionMacroType::create("labjmp", Cpu::LABJMP, Cpu::ADDI, SPARK_INSTRUCTION_MACRO_EXPAND(Cpu::JR, Cpu::PC, x->findLabel(RAWOP(0))->offset - static_cast<Reg>(*x->currentLine->cpuLineNumberPtr - 1) * 4));

        Cpu::SparkInstructionMacroType::create("ret", Cpu::RET, Cpu::JMP, SPARK_INSTRUCTION_MACRO_EXPAND(Cpu::RETADDR));
    }
}
