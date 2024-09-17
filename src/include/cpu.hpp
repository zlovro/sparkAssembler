#pragma once

#include "types.hpp"
#include <map>
#include <utility>
#include <stdarg.h>

#include "SafeList.hpp"
#include "log.hpp"

namespace SPARK::Cpu
{
    // must fit into 5 bits, 32 possible registers
    enum ESparkExternalRegister
    {
        INVREG = -1,

        // function arguments
        A0,
        A1,
        A2,
        A3,
        A4,
        A5,
        A6,
        A7,

        // gprs
        R0,
        R1,
        R2,
        R3,
        R4,
        R5,
        R6,
        R7,
        R8,
        R9,
        R10,
        R11,
        R12,
        R13,
        R14,
        R15,

        // jump address register
        JR,

        // program counter
        PC,

        // return value register
        RETVAL,

        // return address register
        RETADDR,

        // stack pointer
        SP,

        // condition register
        CR,

        // hardware interface instruction
        HII,

        // hardware interface return value
        HIRV,
    };

    inline SafeList<string> gRegisterNameTable({
        "a0",
        "a1",
        "a2",
        "a3",
        "a4",
        "a5",
        "a6",
        "a7",
        "r0",
        "r1",
        "r2",
        "r3",
        "r4",
        "r5",
        "r6",
        "r7",
        "r8",
        "r9",
        "r10",
        "r11",
        "r12",
        "r13",
        "r14",
        "r15",
        "jr",
        "pc",
        "retval",
        "retaddr",
        "sp",
        "cr",
        "rtclo",
        "rtchi"
    });

    ESparkExternalRegister stringRegisterToRegisterValue(const string& pRegisterStr)
    {
        size_t registerCount = gRegisterNameTable.count();
        for (size_t i = 0; i < registerCount; i++)
        {
            if (gRegisterNameTable[i] == pRegisterStr)
            {
                return static_cast<ESparkExternalRegister>(i);
            }
        }
        
        return INVREG;
    }

    enum ESparkConditionRegisterValues
    {
        EQUAL,
        LESS,
        LESS_OR_EQUAL,
        GREATER,
        GREATER_OR_EQUAL,
    };

    enum ESparkOperandType
    {
        IMMEDIATE,
        REGISTER,
    };

    enum ESparkInstructionOpcodeId
    {
        INVOP = -1,
        // register1 = immediate
        LIW = 1,
        // registerDst = register1 + immediate
        ADDI,
        // registerDst = register1 + register2
        ADD,
        // registerDst = registerSrc
        MOV,
        // compare register1 and register2, store result in CR
        CMPR,
        // compare register1 and register
        CMPI,
        // jump to register1 if CR == immediate
        JMPCR,
        // jump to register1 unconditionally
        JMP,
        NOP = 63
    };

    enum ESparkInstructionMacroOpcodeId
    {
        INVMACRO = -1,
        // register1++
        INC,
        LIWL,
        LIWH,
        JMPEQ,
        JMPL,
        JMPLEQ,
        JMPG,
        JMPGEQ,
        // store label offset in register1
        LABREG,
        // store label offset in jr
        LABJMP,
        RET,
    };

    class SparkInstructionType;
    class SparkInstructionMacroType;

    inline map<string, ESparkInstructionOpcodeId> gOpcodeSet;
    inline map<ESparkInstructionOpcodeId, SparkInstructionType*> gInstructionSet;
    inline map<string, ESparkInstructionMacroOpcodeId> gMacroSet;
    inline map<ESparkInstructionMacroOpcodeId, SparkInstructionMacroType*> gMacroInstructionSet;
    inline map<ESparkInstructionOpcodeId, SafeList<SparkInstructionMacroType*>> gMacrosByInstructionOpcodes;

    typedef class SparkInstructionType
    {
    public:
        string opcodeStr;
        ESparkInstructionOpcodeId opcodeId;
        size_t operandCount;
        SafeList<ESparkOperandType> operandTypes;
        SafeList<size_t> operandLengths;

        static void create(string pOpcodeStr, ESparkInstructionOpcodeId pOpcodeId, size_t pOperandCount, ...)
        {
            auto* instance = new SparkInstructionType();

            instance->opcodeStr = std::move(pOpcodeStr);
            instance->opcodeId = pOpcodeId;
            instance->operandCount = pOperandCount;

            instance->operandTypes = SafeList<ESparkOperandType>();
            instance->operandLengths = SafeList<size_t>();

            va_list args;
            va_start(args, pOperandCount);

            size_t totalBitLength = 6;

            for (size_t i = 0; i < instance->operandCount; i++)
            {
                ESparkOperandType operandType = va_arg(args, ESparkOperandType);
                size_t bitLength = va_arg(args, size_t);

                totalBitLength += bitLength;

                instance->operandTypes.add(operandType);
                instance->operandLengths.add(bitLength);
            }

            if (totalBitLength > 32)
            {
                LOGERR("Instruction '{0}' exceeded the maximum bit length (32) by {1} bits (total {2}).\n", pOpcodeStr, totalBitLength - 32, totalBitLength);
            }

            va_end(args);

            gInstructionSet[instance->opcodeId] = instance;
            gOpcodeSet[instance->opcodeStr] = instance->opcodeId;
        }

        ~SparkInstructionType() = default;
    } SparkInstructionType;

    typedef class SparkInstructionInstance
    {
        SafeList<Reg> operandValues;

    public:
        SparkInstructionType* base;
        SafeList<string> rawOperandValues;

        SparkInstructionInstance()
        {
            base = gInstructionSet[INVOP];
            operandValues = SafeList<Reg>();
            rawOperandValues = SafeList<string>();
        }

        SparkInstructionInstance(ESparkInstructionOpcodeId pOpcodeId, SafeList<Reg> pOperandValues, const SafeList<string>& pRawOperandValues)
        {
            base = gInstructionSet[pOpcodeId];
            rawOperandValues = pRawOperandValues;

            for (size_t idx = 0; idx < pOperandValues.count(); idx++)
            {
                Reg operandValue = pOperandValues[idx];
                operandValues.add(operandValue & (1 << base->operandLengths[idx]) - 1);
            }
        }

        SafeList<Reg>* getOperandValues()
        {
            return &operandValues;
        }

        Reg getOperandValue(size_t pIdx)
        {
            return operandValues[pIdx];
        }
    } SparkInstructionInstance;


#define SPARK_INSTRUCTION_MACRO_EXPAND(...) [](Cpu::SparkAssemblerContext* x) \
    { \
        return SafeList<Reg>({__VA_ARGS__}); \
    }

#define OP(idx) x->currentInstruction->getOperandValue(idx)
#define RAWOP(idx) x->currentInstruction->rawOperandValues[idx]

    typedef struct SparkAssemblerLabel
    {
        Reg offset;
        string name;

        SparkAssemblerLabel(Reg pOffset, const string& pName)
        {
            offset = pOffset;
            name = pName;
        }
    } SparkAssemblerLabel;

    typedef struct AssemblyLine
    {
        size_t* cpuLineNumberPtr; // 1 onwards
        size_t* assemblerLineNumberPtr; // 1 onwards
        string* rawLineContentsPtr;
        string* cleanLineContentsPtr;

        AssemblyLine(size_t* pLineNumberPtr, size_t* pAssemblerLineNumberPtr, string* pRawLineContentsPtr, string* pCleanLineContentsPtr)
        {
            cpuLineNumberPtr = pLineNumberPtr;
            rawLineContentsPtr = pRawLineContentsPtr;
            cleanLineContentsPtr = pCleanLineContentsPtr;
            assemblerLineNumberPtr = pAssemblerLineNumberPtr;
        }

        void incrementCpuLineCounter()
        {
            *cpuLineNumberPtr = *cpuLineNumberPtr + 1;
        }

        void incrementAssemblerLineCounter()
        {
            *assemblerLineNumberPtr = *assemblerLineNumberPtr + 1;
        }
    } AssemblyLine;

    enum ESparkAssemblerResult
    {
        NONE = -1,
        ERROR,
        // 'addi r14, r14, r4455'
        IGNORE,
        // '# comment'
        SUCCESS,
        // 'inc r15'
    };

    typedef struct SparkAssemblerErrorContext
    {
        ESparkAssemblerResult result;
        string reason;

        void success()
        {
            result = SUCCESS;
        }

        void error(const string& pReason)
        {
            result = ERROR;
            reason = pReason;
        }

        void ignore(const string& pReason)
        {
            result = IGNORE;
            reason = pReason;
        }
    } SparkAssemblerErrorContext;

    typedef struct SparkAssemblerContext
    {
        SafeList<SparkAssemblerLabel*> labels;
        SafeList<string> absoluteIncludePaths;
        SparkInstructionInstance* currentInstruction;
        SparkAssemblerErrorContext* errorContext;
        AssemblyLine* currentLine;
        std::filesystem::path currentFile;

        map<string, ESparkExternalRegister> registerMacros;

        SparkAssemblerContext(const string& pCurrentFilePath, SparkInstructionInstance* pCurrentInstruction, SparkAssemblerErrorContext* pErrCtx, size_t* pLineNumberPtr, size_t* pAssemblerLineNumberPtr, string* pCurrentLineRaw, string* pCurrentLineClean)
        {
            setCurrentFile(pCurrentFilePath);

            labels = SafeList<SparkAssemblerLabel*>();

            currentInstruction = pCurrentInstruction;
            errorContext = pErrCtx;
            currentLine = new AssemblyLine(pLineNumberPtr, pAssemblerLineNumberPtr, pCurrentLineRaw, pCurrentLineClean);
        }

        explicit SparkAssemblerContext(SparkAssemblerErrorContext* pErrCtx) : SparkAssemblerContext("", nullptr, pErrCtx, nullptr, nullptr, nullptr, nullptr)
        {
        }

        ~SparkAssemblerContext()
        {
            delete currentLine;
            delete errorContext;
            delete currentInstruction;
        }

        void setCurrentFile(const string& pPath)
        {
            currentFile = std::filesystem::path(pPath);
        }

        void addIncludePath(const string& pPath)
        {
            try
            {
                string expanded = std::filesystem::canonical(pPath).generic_string();
                absoluteIncludePaths.add(expanded);

                success();
            }
            catch (exception& e)
            {
                error(format("Error adding include path '{0}': {1}", pPath, e.what()));
            }
        }

        string expandIncludeStatementPath(SparkAssemblerContext* pCtx, const string& pPath)
        {
            string conflicts;
            string finalPath;
            std::filesystem::path includeStatementPathObject(pPath);

            size_t foundMatches = 0;

            for (const auto& includePath : absoluteIncludePaths)
            {
                std::filesystem::path target = includeStatementPathObject.concat(includePath);
                if (exists(target))
                {
                    foundMatches++;

                    if (foundMatches > 1)
                    {
                        conflicts += (conflicts.empty() ? "" : ", ") + target.string();
                        continue;
                    }

                    finalPath = target.string();
                }
            }

            if (foundMatches == 0)
            {
                pCtx->error(format("Could not resolve include path '{0}' in file '{1}'.\n", includeStatementPathObject.string(), pCtx->currentFile.string()));
                return "";
            }

            if (foundMatches > 1)
            {
                pCtx->error(format("Found conflicts in include path '{0}'.\n", conflicts));
                return "";
            }

            return finalPath;
        }

        void setRegisterMacro(const string& pRepr, ESparkExternalRegister pRegister)
        {
            if (pRegister == INVREG)
            {
                error(format("Invalid register found in register macro '{0}'.\n", pRepr));
                return;
            }
            registerMacros[pRepr] = pRegister;
        }

        ESparkExternalRegister getRegisterFromRegisterMacroRepresentation(const string& pRegisterStr)
        {
            return registerMacros[pRegisterStr];
        }

        bool registerMacroExists(const string& pRepr)
        {
            return registerMacros.contains(pRepr);
        }

        void incrementCpuLineNumber()
        {
            currentLine->incrementCpuLineCounter();
        }

        void incrementAssemblerLineNumber()
        {
            currentLine->incrementAssemblerLineCounter();
        }

        void success()
        {
            errorContext->success();
        }

        void error(const string& pReason)
        {
            errorContext->error(pReason);
        }

        void ignore(const string& pReason)
        {
            errorContext->ignore(pReason);
        }

        bool isSuccessful()
        {
            return errorContext->result == SUCCESS;
        }

        bool isError()
        {
            return errorContext->result == ERROR;
        }

        bool isIgnore()
        {
            return errorContext->result == IGNORE;
        }

        string getReason()
        {
            return errorContext->reason;
        }

        SparkAssemblerLabel* findLabel(const string& pLabelName)
        {
            return labels.find<string>([](SparkAssemblerLabel* pLabel) { return pLabel->name; }, pLabelName);
        }
    } SparkAssemblerContext;

    typedef SafeList<Reg> (*SparkInstructionMacroExpander)(SparkAssemblerContext*);

    typedef class SparkInstructionMacroType
    {
    public:
        string opcode;
        ESparkInstructionMacroOpcodeId opcodeId;
        ESparkInstructionOpcodeId baseOpcodeId;
        SparkInstructionMacroExpander parserFunction;

        static void create(const string& pOpcode, ESparkInstructionMacroOpcodeId pMacroOpcodeId, ESparkInstructionOpcodeId pBaseOpcodeId, SparkInstructionMacroExpander pParserFunction)
        {
            auto* instance = new SparkInstructionMacroType();

            instance->opcode = pOpcode;
            instance->opcodeId = pMacroOpcodeId;
            instance->baseOpcodeId = pBaseOpcodeId;
            instance->parserFunction = pParserFunction;

            gMacroSet[instance->opcode] = instance->opcodeId;
            gMacroInstructionSet[instance->opcodeId] = instance;
            gMacrosByInstructionOpcodes[instance->baseOpcodeId].add(instance);
        }
    } SparkInstructionMacroType;

    ESparkInstructionOpcodeId getOpcodeIdFromOpcodeStr(const string& pOpcodeStr)
    {
        if (!gOpcodeSet.contains(pOpcodeStr))
        {
            return INVOP;
        }

        return gOpcodeSet[pOpcodeStr];
    }

    ESparkInstructionMacroOpcodeId getMacroOpcodeIdFromOpcodeStr(const string& pOpcodeStr)
    {
        if (!gMacroSet.contains(pOpcodeStr))
        {
            return INVMACRO;
        }

        return gMacroSet[pOpcodeStr];
    }

    SparkInstructionMacroType* getMacroTypeFromId(ESparkInstructionMacroOpcodeId pOpcodeId)
    {
        if (!gMacroInstructionSet.contains(pOpcodeId))
        {
            return nullptr;
        }

        return gMacroInstructionSet[pOpcodeId];
    }

    SafeList<SparkInstructionMacroType*> getMacrosFromInstructionOpcodeId(ESparkInstructionOpcodeId pInstructionOpcodeId)
    {
        return gMacrosByInstructionOpcodes[pInstructionOpcodeId];
    }

    SparkInstructionType* getInstructionTypeFromOpcodeId(ESparkInstructionOpcodeId pOpcodeId)
    {
        if (!gInstructionSet.contains(pOpcodeId))
        {
            return nullptr;
        }

        return gInstructionSet[pOpcodeId];
    }


    string getOpcodeStrFromOpcodeId(ESparkInstructionOpcodeId pOpcodeId)
    {
        SparkInstructionType* instructionType = getInstructionTypeFromOpcodeId(pOpcodeId);
        if (!instructionType)
        {
            return "";
        }

        return instructionType->opcodeStr;
    }
}
