#include <string>
#include <cstdint>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "MemoryStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"

// Method to see if the Memory Address comes from depending on the instruction
// Answers the following question >>  Memory Address, where does it come from?
uint64_t MemoryStage::memAddr(uint64_t icode, uint64_t valA, uint64_t valE) 
{
    if (icode == IRMMOVQ || icode == IPUSHQ 
        || icode == ICALL || icode == IMRMOVQ)
    {
        return valE;
    }
    else if (icode == IPOPQ || icode == IRET)
    {
        return valA;
    }
    else
    {
        return 0;
    }
}   

// Boolean method to see if you're reading from memory.
bool MemoryStage::memRead(uint64_t icode) 
{
    return (icode == IMRMOVQ || icode == IPOPQ || icode == IRET);
}

// Boolean method to see if you're writing to memory.
bool MemoryStage::memWrite(uint64_t icode) 
{
    return (icode == IRMMOVQ || icode == IPUSHQ || icode == ICALL);
}

// Getter for dstM
uint64_t MemoryStage::get_dstM()
{
    return dstM;
}

// Getter for valM
uint64_t MemoryStage::get_valM()
{
    return valM;
}

// Getter for m_Stat
uint64_t MemoryStage::getm_stat()
{
    return stat;
}


// This method implements the logic of the Memory Stage.
// This method will read/write to memory depending on the instruction.
// Sets up the appropiate fields to be written to the W register.
bool MemoryStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];
    Memory * mem = Memory::getInstance();
    stat = mreg->getstat()->getOutput();
    uint64_t icode = mreg->geticode()->getOutput();
    uint64_t valE = mreg->getvalE()->getOutput();
    uint64_t dstE = mreg->getdstE()->getOutput();
    dstM = mreg->getdstM()->getOutput();
    uint64_t valA = mreg->getvalA()->getOutput();
    bool error = false;
    valM = 0;
    
    uint64_t addr = memAddr(icode, valA, valE);

    if (memWrite(icode))
    {
        mem->putLong(valA, addr, error);
    }
    
    if (memRead(icode)) 
    {
       valM = mem->getLong(addr, error);
    }

    if (error)
    {
        stat = SADR;
    }
    

    setWInput(wreg, stat, icode, valE, valM, dstE, dstM);

    return false;
}

// This method will setup the W register's values.
void MemoryStage::doClockHigh(PipeReg ** pregs)
{
    W * wreg = (W *) pregs[WREG];

    wreg->getstat()->normal();
    wreg->geticode()->normal();
    wreg->getvalE()->normal();
    wreg->getvalM()->normal();
    wreg->getdstE()->normal();
    wreg->getdstM()->normal();
}

// This method will set the imputs of the W register to the Memory Stage's variables.
void MemoryStage::setWInput(W * wreg, uint64_t stat, uint64_t icode,
                            uint64_t valE, uint64_t valM,
                            uint64_t dstE, uint64_t dstM)
{
    wreg->getstat()->setInput(stat);
    wreg->geticode()->setInput(icode);
    wreg->getvalE()->setInput(valE);
    wreg->getvalM()->setInput(valM);
    wreg->getdstE()->setInput(dstE);
    wreg->getdstM()->setInput(dstM);
}



