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
#include "WritebackStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"

// Lastily, just checks if the Writeback register was filled out correctly.
bool WritebackStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    W * wreg = (W *) pregs[WREG];
    uint64_t stat = wreg->getstat()->getOutput();
    if (stat != SAOK)
    {
        return true;
    }

    return false;
}

// Writes the final values to the Register File. 
void WritebackStage::doClockHigh(PipeReg ** pregs)
{
    RegisterFile * rf = RegisterFile::getInstance();
    W * wreg = (W *) pregs[WREG];  
    uint64_t valE = wreg->getvalE()->getOutput();
    uint64_t dstE = wreg->getdstE()->getOutput();
    uint64_t valM = wreg->getvalM()->getOutput();  
    uint64_t dstM = wreg->getdstM()->getOutput();
    bool error = false;
    
    rf->writeRegister(valE, dstE, error);
    rf->writeRegister(valM, dstM, error);
}
