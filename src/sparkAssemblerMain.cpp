#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef DEBUG
#define DEBUG
#endif

#include <algorithm>
#include <cstdio>
#include <string>
#include <format>
#include <print>
#include <filesystem>
#include <fstream>

#include <assembler.hpp>
#include <cpu.hpp>
#include <log.hpp>

#define ASSEMBLERERR_EX(file, lineNumber, lineContents, reason) LOGERR("Assembler failed on file '{0}', line {1}: {2}'{3}' - {4}{5}\n", file, lineNumber, CLR_FRYEL, lineContents, CLR_FBRED, reason)
#define ASSEMBLERERR(ctx) ASSEMBLERERR_EX(ctx->currentFile.string(), assemblerLineNumber, *ctx->currentLine->rawLineContentsPtr, ctx->getReason())
#define ASSEMBLERDBG(lineNumber, lineContents, reason) LOGERR("Ignoring line {0}: {1}'{2}' - {3}{4}\n", lineNumber, CLR_FRYEL, lineContents, CLR_FBRED, reason)
#define ASSEMBLERERR_NOT_PROVIDED(argName, shortOption, longOption) LOGERR(##argName " was not provided, provide it using '-" ##shortOption "' or '--" ##longOption "'.\n")

#define DISASSEMBLERERR_EX(fileOffset, ctx) LOGERR("Disassembler failed on file offset 0x{0:X} - {1}{2}\n", fileOffset, CLR_FBRED, ctx->getReason())

enum EReturnCode
{
    RET_ERR = -1,
    RET_OK,
};

enum ESparkAssemblerOperation
{
    UNRECOGNIZEDASSEMBLEROP = -2,
    INVASSEMBLEROP,
    ASSEMBLE,
    DISASSEMBLE
};

ESparkAssemblerOperation argToAssemblerOperation(const string& pArg)
{
    if (pArg == "ASSEMBLE" || pArg == "A")
    {
        return ASSEMBLE;
    }

    if (pArg == "DISASSEMBLE" || pArg == "D")
    {
        return DISASSEMBLE;
    }

    return UNRECOGNIZEDASSEMBLEROP;
}

int main(int pArgumentCount, char* pArguments[])
{
    string inputFile, outputFile;
    ESparkAssemblerOperation operation = INVASSEMBLEROP;
    string stringOperation;
    bool disassemblerHexDumpEnabled = false;

    for (int i = 1; i < pArgumentCount; ++i)
    {
        string argument = pArguments[i];

        if (argument == "-i" || argument == "-inputFile")
        {
            inputFile = pArguments[i + 1];
        }

        else if (argument == "-o" || argument == "-outputFile")
        {
            outputFile = pArguments[i + 1];
        }

        else if (argument == "-op" || argument == "--operation")
        {
            stringOperation = pArguments[i + 1];
            operation = argToAssemblerOperation(stringOperation);
        }

        else if (argument == "-hexdump")
        {
            disassemblerHexDumpEnabled = true;
        }
    }

    if (inputFile.empty())
    {
        ASSEMBLERERR_NOT_PROVIDED("Input file", "i", "inputFile");
        return RET_ERR;
    }

    if (outputFile.empty())
    {
        ASSEMBLERERR_NOT_PROVIDED("Output file", "o", "outputFile");
        return RET_ERR;
    }

    if (operation == INVASSEMBLEROP)
    {
        ASSEMBLERERR_NOT_PROVIDED("No operation", "op", "operation");
        return RET_ERR;
    }

    if (operation == UNRECOGNIZEDASSEMBLEROP)
    {
        LOGERR("Operation '{0}' is invalid. Valid operations: ASSEMBLE (A), DISASSEMBLE (D)\n", stringOperation);
        return RET_ERR;
    }

    SPARK::Assembler::initAssembler();

    auto errCtx = new SPARK::Cpu::SparkAssemblerErrorContext();
    auto ctx = new SPARK::Cpu::SparkAssemblerContext(errCtx);

    FILE* fp;

    switch (operation)
    {
    case ASSEMBLE:
        {
            SafeList<Reg> outputFileData;
            SafeList<string> linesToParse;

            std::ifstream file(inputFile);

            if (!file.is_open())
            {
                LOGERR("Error opening file '{0}'\n.", inputFile);
                return RET_ERR;
            }

            std::string lineContentsClean;
            std::string lineContentsRaw;

            size_t cpuLineNumber = 0;
            size_t assemblerLineNumber = 0;

            ctx->setCurrentFile(inputFile);
            ctx->currentLine = new SPARK::Cpu::AssemblyLine(&cpuLineNumber, &assemblerLineNumber, &lineContentsRaw, &lineContentsClean);

            while (std::getline(file, lineContentsRaw))
            {
                lineContentsClean = SPARK::Assembler::Analysis::cleanupAssemblyLine(lineContentsRaw);

                if (SPARK::Assembler::Analysis::currentAssemblyLineHasIncludePath(ctx))
                {
                    string path = SPARK::Assembler::Analysis::getIncludePathName(lineContentsClean);
                    ctx->addIncludePath(path);
                    if (ctx->isError())
                    {
                        LOGERR("Failed to add include path '{0}'.\n", path);
                        return RET_ERR;
                    }

                    ctx->incrementAssemblerLineNumber();

                    continue;
                }

                if (SPARK::Assembler::Analysis::currentAssemblyLineHasInclude(ctx))
                {
                    SPARK::Assembler::Analysis::expandCurrentIncludeRecursively(ctx, &linesToParse);
                    ctx->incrementAssemblerLineNumber();
                    continue;
                }

                linesToParse.add(lineContentsRaw);
            }

            for (const auto& lineContents : linesToParse)
            {
                ctx->incrementAssemblerLineNumber();

                lineContentsRaw = lineContents;
                lineContentsClean = SPARK::Assembler::Analysis::cleanupAssemblyLine(lineContentsRaw);

                if (lineContentsClean.empty())
                {
                    continue;
                }

                switch (SPARK::Assembler::Analysis::getCurrentLineType(ctx))
                {
                case SPARK::Assembler::Analysis::EMPTY:
                    {
                    }
                    break;

                case SPARK::Assembler::Analysis::EXECUTABLE:
                    {
                        ctx->incrementCpuLineNumber();

                        SPARK::Cpu::SparkInstructionInstance* parsed;
                        SPARK::Assembler::Analysis::parseInstructionFromCurrentAssemblyLine(ctx, &parsed);

                        if (ctx->isError())
                        {
                            ASSEMBLERERR(ctx);
                            return RET_ERR;
                        }

                        if (ctx->isIgnore())
                        {
                            ASSEMBLERDBG(cpuLineNumber, lineContentsRaw, ctx->getReason());
                            continue;
                        }

                        Reg operandData = SPARK::Assembler::assembleOperands(&parsed->base->operandLengths, parsed->getOperandValues(), ctx);
                        if (ctx->isError())
                        {
                            ASSEMBLERERR(ctx);
                            return RET_ERR;
                        }

                        Reg assembled = parsed->base->opcodeId << 26 | operandData;
                        delete parsed;

                        // LOGDBG("Assembled line '{0}' --> '{1:08X}'\n", lineContentsRaw, assembled);

                        if (ctx->isError())
                        {
                            ASSEMBLERERR(ctx);
                            return RET_ERR;
                        }

                        Reg assembledBigEndian = _byteswap_ulong(assembled);

                        if (ctx->isSuccessful())
                        {
                            outputFileData.add(assembledBigEndian);
                        }
                    }
                    break;

                case SPARK::Assembler::Analysis::LABEL:
                    {
                        SPARK::Assembler::Analysis::parseLabelFromCurrentAssemblyLine(ctx);
                    }
                    break;

                case SPARK::Assembler::Analysis::EAssemblyLineType::REGISTER_MACRO:
                    {
                        SPARK::Assembler::Analysis::parseRegisterMacroFromCurrentLine(ctx);
                    }
                    break;

                default:
                    {
                        LOGERR("Could not determine line type from line '{0}'.\n", *ctx->currentLine->rawLineContentsPtr);
                    }
                    break;
                }
            }
            file.close();


            fp = fopen(outputFile.c_str(), "w");
            fwrite(outputFileData.data(), sizeof(Reg), outputFileData.count(), fp);
            fclose(fp);

            LOGINF("Successfully assembled.\n");

            break;
        }
    case DISASSEMBLE:
        {
            string outputFileData;
            size_t lineNumber = 0;
            size_t maxLineLength = 0;
            
            SafeList<size_t> newlineIndexes;
            SafeList<size_t> lineLenghts;
            
            ifstream file(inputFile, ios_base::in | ios::binary);

            size_t inBufferInstructionCount = filesystem::file_size(inputFile) / sizeof(Reg);
            SafeList<Reg> inBuffer(inBufferInstructionCount);

            file.read(inBuffer.data(), inBufferInstructionCount * sizeof(Reg));

            for (size_t i = 0; i < inBufferInstructionCount; i++)
            {
                lineNumber++;

                Reg instruction = inBuffer[i];
                instruction = _byteswap_ulong(instruction);

                string disassembled = SPARK::Assembler::disasemble(instruction, ctx);
                if (disassembled.length() > maxLineLength)
                {
                    maxLineLength = disassembled.length();
                }

                if (ctx->isSuccessful())
                {
                    outputFileData += disassembled + '\n';

                    lineLenghts.add(disassembled.length());
                    newlineIndexes.add(outputFileData.size() - 1);
                }

                if (ctx->isError())
                {
                    DISASSEMBLERERR_EX(i * 4, ctx);
                    return RET_ERR;
                }
            }

            if (disassemblerHexDumpEnabled)
            {
                size_t i = 0;
                size_t offset = 0;
                for (size_t newLineIndex : newlineIndexes)
                {
                    string filler(maxLineLength - lineLenghts[i], ' ');
                    Reg instruction = _byteswap_ulong(inBuffer[i]);
                    
                    string comment = format("{0} ; {1:08X}\t{2:08X}", filler, instruction, i * 4);
                    outputFileData.insert(newLineIndex + offset, comment);
                    
                    offset += comment.length();
                    
                    i++;
                }
            }

            fp = fopen(outputFile.c_str(), "w");
            fwrite(outputFileData.data(), 1, outputFileData.size(), fp);
            fclose(fp);

            LOGINF("Successfully disassembled.\n");

            break;
        }
    default: return RET_ERR;
    }

    return 0;
}
