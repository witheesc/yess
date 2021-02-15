#include <string>
#include <cstdint>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "E.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "ExecuteStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "ConditionCodes.h"
#include "Tools.h"
#include "MemoryStage.h"

// Method to Get the ALU Component A, to be used in our ALU.
uint64_t ExecuteStage::ALUCompA(uint64_t icode, uint64_t valA, uint64_t valC)
{
    if (icode == IRRMOVQ || icode == IOPQ) return valA;
    if (icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ) return valC;
    if (icode == ICALL || icode == IPUSHQ) return -8;
    if (icode == IRET || icode == IPOPQ) return 8;
    return 0;
}

// Method to Get the ALU Component B, to be used in our ALU.
uint64_t ExecuteStage::ALUCompB(uint64_t icode, uint64_t valB) 
{
    if (icode == IRMMOVQ || icode == IMRMOVQ || icode == IOPQ || icode == ICALL
        || icode == IPUSHQ || icode == IRET || icode == IPOPQ) return valB;
    else if (icode == IRRMOVQ || icode == IIRMOVQ) return 0;
    return 0;
}

// Method to to see if our function is an IOP, and return ifun if it is, else returns 0.
uint64_t ExecuteStage::ALUCompFun(uint64_t icode, uint64_t ifun) 
{
    if (icode == IOPQ) return ifun;
    else return ADDQ;
}

// Method to see if we need to Set the Condition Codes.
// Returns a boolean.
bool ExecuteStage::setCC(uint64_t icode, uint64_t m_stat, uint64_t W_stat)
{
    return (icode == IOPQ) && (m_stat != SADR && m_stat != SINS && m_stat != SHLT)
     && (W_stat != SADR && W_stat != SINS && W_stat != SHLT);
}

// Method to get a dstComponent if our Icode is an IRRMOVQ, returns RNONE
// Else returns dstE
uint64_t ExecuteStage::dstEComp(uint64_t icode, uint64_t Cnd, uint64_t dstE) 
{
    if (icode == IRRMOVQ && (!Cnd)) return RNONE;
    else return dstE;
}

// Method to do the Arithmetic logic in the ALU
// Takes ALUFun, values A and B, and a reference to an overflow boolean.
// Will return the result of the arithmetic operation
// Sets the reference of the of flag if the arithmetic operation resulted in an overflow.
uint64_t ExecuteStage::ALU(uint64_t alufun, uint64_t aluA, uint64_t aluB, bool &of) 
{
    if (alufun == ADDQ) 
    {
         of = Tools::addOverflow(aluA, aluB);
         return aluA + aluB;
    }
    else if (alufun == SUBQ) 
    {
         of = Tools::subOverflow(aluA, aluB);
         return aluB - aluA;
    }
    else if (alufun == XORQ) return aluA ^ aluB;
    else if (alufun == ANDQ) return aluA & aluB;
    else return 0;
}

// Getter for dstE
uint64_t ExecuteStage::get_dstE()
{
    return dstE;
}

// Getter for valE
uint64_t ExecuteStage::get_valE()
{
    return valE;
}

// Method for Condition.
// This method will set the value of (Cnd) to see if we need to take a jump/or not
uint64_t ExecuteStage::cond(uint64_t icode, uint64_t ifun)
{
    bool error = false;
    ConditionCodes * cc = ConditionCodes::getInstance();
    bool sf = cc->getConditionCode(SF, error);
    bool of = cc->getConditionCode(OF, error);
    bool zf = cc->getConditionCode(ZF, error);
    
    if (icode == IJXX || icode == ICMOVXX)
    {
        if (ifun == UNCOND) return 1;
        else if (ifun == LESSEQ) return ((sf ^ of) | zf);
        else if (ifun == LESS) return (sf ^ of);
        else if (ifun == EQUAL) return (zf);
        else if (ifun == NOTEQUAL) return (!zf);
        else if (ifun == GREATER) return (!(sf ^ of) & (!zf));
        else if (ifun == GREATEREQ) return (!(sf ^ of));
    }

    return 0;
}

// This method is used to return if Stat is valid for the instructions.
bool ExecuteStage::calculateControlSignals(uint64_t m_stat, uint64_t W_stat)
{
    return (m_stat == SADR || m_stat == SINS || m_stat == SHLT) ||
           (W_stat == SADR || W_stat == SINS || W_stat == SHLT);
}

// Getter for Cnd
uint64_t ExecuteStage::gete_Cnd()
{
    return e_Cnd;
}

// Method to Simulate the Logic of the Execute Stage (Main Method essentially).
// Will setup the variables to be written to the M register
// Will do arithmetic logic depending on the instruction
// Will set the condition codes.
bool ExecuteStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    E * ereg = (E *) pregs[EREG];
    M * mreg = (M *) pregs[MREG];

    uint64_t stat = ereg->getstat()->getOutput();
    uint64_t icode = ereg->geticode()->getOutput();
    uint64_t valA = ereg->getvalA()->getOutput();
    dstE = ereg->getdstE()->getOutput();
    uint64_t dstM = ereg->getdstM()->getOutput();
    uint64_t ifun = ereg->getifun()->getOutput();
    e_Cnd = 0;
    valE = 0;
    uint64_t valC = ereg-> getvalC()->getOutput();
    uint64_t valB = ereg->getvalB()->getOutput();
    // Start ALU Stuff
    bool of = false;
    bool error = false;
    ConditionCodes * cc = ConditionCodes::getInstance();
    uint64_t alufun = ALUCompFun(icode, ifun);
    uint64_t aluA = ALUCompA(icode, valA, valC);
    uint64_t aluB = ALUCompB(icode, valB);
    
       
    valE = ALU(alufun, aluA, aluB, of); 
    uint64_t signFlag = Tools::sign(valE);
    uint64_t zf = 0;
    if (valE == 0)
         zf = 1;

    MemoryStage * ms = (MemoryStage*) stages[MSTAGE];
    W * wreg = (W *) pregs[WREG];

    uint64_t E_icode = ereg->geticode()->getOutput();
    uint64_t m_stat = ms -> getm_stat();
    uint64_t W_stat = wreg->getstat()->getOutput();
    

    bool CCBool = setCC(E_icode, m_stat, W_stat);
    if (CCBool) {
        cc->setConditionCode(signFlag, SF, error);
        cc->setConditionCode(zf, ZF, error);
        cc->setConditionCode(of, OF, error); 
    }

    M_bubble = calculateControlSignals(m_stat, W_stat);
   
    e_Cnd = cond(icode, ifun); 
    dstE = dstEComp(icode, e_Cnd, dstE);   

    // Everything done!
    setMInput(mreg, stat, icode, e_Cnd, valE, valA, dstE, dstM);

    return false;
}

// Helper method for the ClockHigh to do set the Mregister normally.
void ExecuteStage::normalM(PipeReg ** pregs)
{
    M * mreg = (M *) pregs[MREG];

    mreg->getstat()->normal();
    mreg->geticode()->normal();
    mreg->getCnd()->normal();
    mreg->getvalE()->normal();
    mreg->getvalA()->normal();
    mreg->getdstE()->normal();
    mreg->getdstM()->normal();
}

// Helper method for the ClockHigh to set the M register to bubble (nop)
void ExecuteStage::bubbleM(PipeReg ** pregs)
{
    M * mreg = (M *) pregs[MREG];

    mreg->getstat()->bubble(SAOK);
    mreg->geticode()->bubble(INOP);
    mreg->getCnd()->bubble();
    mreg->getvalE()->bubble();
    mreg->getvalA()->bubble();
    mreg->getdstE()->bubble(RNONE);
    mreg->getdstM()->bubble(RNONE);
}

// Method to setup the the Memory Register with correct values
void ExecuteStage::doClockHigh(PipeReg ** pregs)
{
    if (!M_bubble)
    {
        normalM(pregs);
    }
    else
    {
        bubbleM(pregs);
    }
}

void ExecuteStage::setMInput(M * mreg, uint64_t stat, uint64_t icode,
                             uint64_t Cnd, uint64_t valE, uint64_t valA,
                             uint64_t dstE, uint64_t dstM)
{
    mreg->getstat()->setInput(stat);
    mreg->geticode()->setInput(icode);
    mreg->getCnd()->setInput(Cnd);
    mreg->getvalE()->setInput(valE);
    mreg->getvalA()->setInput(valA);
    mreg->getdstE()->setInput(dstE);
    mreg->getdstM()->setInput(dstM);
}
