#include <string>
#include <cstdint>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "M.h"
#include "W.h"
#include "E.h"
#include "Stage.h"
#include "DecodeStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "ExecuteStage.h"
#include "MemoryStage.h"

// Returns a certain component based on what ICODE is.
uint64_t DecodeStage::srcAComponent(uint64_t icode, uint64_t rA)
{
    if (icode == IRRMOVQ || icode == IRMMOVQ
         || icode == IOPQ || icode == IPUSHQ)
    {
        return rA;
    }
    else if (icode == IPOPQ || icode == IRET)
    {
        return RSP;
    }
    else
    {
        return RNONE;
    }

}

// Returns a certain component based on what icode is (rB)
uint64_t DecodeStage::srcBComponent(uint64_t icode, uint64_t rB)
{
    if (icode == IOPQ || icode == IRMMOVQ || icode == IMRMOVQ)
    {
        return rB;
    }
    else if (icode == IPUSHQ || icode == IPOPQ 
        || icode == ICALL || icode == IRET)
    {
        return RSP;
    }
    else 
    {
        return RNONE;
    }
}

// Returns DSTECompoent based on what icode is.
uint64_t DecodeStage::dstEComponent(uint64_t icode, uint64_t rB)
{
    if (icode == IRRMOVQ || icode == IIRMOVQ || icode == IOPQ)
    {
        return rB;
    }
    else if (icode == IPUSHQ || icode == IPOPQ 
        || icode == ICALL || icode == IRET)
    {
        return RSP;
    }
    else
    {
        return RNONE;
    }
}

// Returns DSTMCompoent based on what icode is.
uint64_t DecodeStage::dstMComponent(uint64_t icode, uint64_t rA)
{
    if (icode == IMRMOVQ || icode == IPOPQ)
    {
        return rA;
    }
    else
    {
        return RNONE;
    }
}

// Takes multiple parameters to implement the forwarding logic for srcA
uint64_t DecodeStage::selPlusFwdA(uint64_t srcA, uint64_t rvalA, uint64_t icode, uint64_t valP, Stage ** stages, PipeReg ** pregs)
{
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];
    ExecuteStage * es = (ExecuteStage*) stages[ESTAGE];
    MemoryStage * ms = (MemoryStage*) stages[MSTAGE];

    if (icode == ICALL || icode == IJXX)
    {
        return valP;
    }
    if (srcA == RNONE)
    {
        return 0;
    }
    if (srcA == es->get_dstE())
    {
        return es->get_valE();
    }
    if (srcA == ms->get_dstM())
    {
        return ms->get_valM();
    }
    if (srcA == mreg->getdstE()->getOutput())
    {
        return mreg->getvalE()->getOutput();
    }
    if (srcA == wreg->getdstM()->getOutput())
    {
        return wreg->getvalM()->getOutput();
    }
    if (srcA == wreg->getdstE()->getOutput())
    {
        return wreg->getvalE()->getOutput();
    }
    return rvalA;
}

// Implements the forwarding logic for srcB/destination.
uint64_t DecodeStage::fwdB(uint64_t srcB, uint64_t rvalB, Stage ** stages, PipeReg ** pregs)
{
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];
    ExecuteStage * es = (ExecuteStage*) stages[ESTAGE];
    MemoryStage * ms = (MemoryStage*) stages[MSTAGE];

    if (srcB == RNONE)
    {
        return 0;
    }
    if (srcB == es->get_dstE())
    {
        return es->get_valE();
    }
    if (srcB == ms->get_dstM())
    {
        return ms->get_valM();
    }
    if (srcB == mreg->getdstE()->getOutput())
    {
        return mreg->getvalE()->getOutput();
    }
    if (srcB == wreg->getdstM()->getOutput())
    {
        return wreg->getvalM()->getOutput();
    }
    if (srcB == wreg->getdstE()->getOutput())
    {
        return wreg->getvalE()->getOutput();
    }
    return rvalB;
}

// Getter for Decode Stage SRCA
uint64_t DecodeStage::getd_srcA()
{
    return d_srcA;
}

// Getter for Decode Stage SRCB
uint64_t DecodeStage::getd_srcB()
{
    return d_srcB;
}

// Returns a boolean based on if the eicode is a jump/irmov/pop.
bool DecodeStage::calculateControlSignals(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB, uint64_t e_Cnd)
{
    return (E_icode == IJXX && !e_Cnd) 
    || ((E_icode == IMRMOVQ 
    || E_icode == IPOPQ) && (E_dstM == d_srcA 
    || E_dstM == d_srcB));   
}

// Clock low method, the actual stage logic.
bool DecodeStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    D * dreg = (D *) pregs[DREG];
    E * ereg = (E *) pregs[EREG];
     

    uint64_t stat = dreg->getstat()->getOutput();
    uint64_t icode = dreg->geticode()->getOutput();
    uint64_t ifun = dreg->getifun()->getOutput();
    uint64_t valC = dreg->getvalC()->getOutput();
    uint64_t rA = dreg->getrA()->getOutput();
    uint64_t rB = dreg->getrB()->getOutput();
    uint64_t valP = dreg->getvalP()->getOutput();

    uint64_t valA = 0, valB = 0, rvalA = 0, rvalB = 0;
    uint64_t dstE = RNONE, dstM = RNONE;
    d_srcA = RNONE;
    d_srcB = RNONE;

    d_srcA = srcAComponent(icode, rA);
    d_srcB = srcBComponent(icode, rB);
    dstE = dstEComponent(icode, rB);
    dstM = dstMComponent(icode, rA);

    uint64_t E_icode = ereg->geticode()->getOutput();
    uint64_t E_dstM = ereg->getdstM()->getOutput();
    ExecuteStage * es = (ExecuteStage*) stages[ESTAGE];
    uint64_t e_Cnd = es->gete_Cnd();

    E_bubble = calculateControlSignals(E_icode, E_dstM, d_srcA, d_srcB, e_Cnd);

    RegisterFile * rf = RegisterFile::getInstance();
    bool error = false;
    rvalA = rf->readRegister(d_srcA, error);
    rvalB = rf->readRegister(d_srcB, error);   

    valA = selPlusFwdA(d_srcA, rvalA, icode, valP, stages, pregs);
    valB = fwdB(d_srcB, rvalB, stages, pregs);

    setEInput(ereg, stat, icode, ifun, valC, valA, valB, dstE, dstM, d_srcA, d_srcB);

    return false;
}

// Implements the bubbling part of ClockHigh if the e reg needs to be bubbled.
void DecodeStage::bubbleE(PipeReg ** pregs)
{
    E * ereg = (E *) pregs[EREG];

    ereg->getstat()->bubble(SAOK);
    ereg->geticode()->bubble(INOP);
    ereg->getifun()->bubble();
    ereg->getvalC()->bubble();
    ereg->getvalA()->bubble();
    ereg->getvalB()->bubble();
    ereg->getdstE()->bubble(RNONE);
    ereg->getdstM()->bubble(RNONE);
    ereg->getsrcA()->bubble(RNONE);
    ereg->getsrcB()->bubble(RNONE);
}

// Implements the normal logic of Clockhigh if the e reg needs to be treated normally.
void DecodeStage::normalE(PipeReg ** pregs)
{
    E * ereg = (E *) pregs[EREG];

    ereg->getstat()->normal();
    ereg->geticode()->normal();
    ereg->getifun()->normal();
    ereg->getvalC()->normal();
    ereg->getvalA()->normal();
    ereg->getvalB()->normal();
    ereg->getdstE()->normal();
    ereg->getdstM()->normal();
    ereg->getsrcA()->normal();
    ereg->getsrcB()->normal();

}

// Implements the logic for setting the ereg to certain values,
// Either normally or with a bubble.
void DecodeStage::doClockHigh(PipeReg ** pregs)
{
    if (E_bubble)
    {
        bubbleE(pregs);
    }
    else
    {
        normalE(pregs);
    }
}

// Sets the Eregister input.
void DecodeStage::setEInput(E * ereg, uint64_t stat, uint64_t icode,
                            uint64_t ifun, uint64_t valC, uint64_t valA,
                            uint64_t valB, uint64_t dstE, uint64_t dstM,
                            uint64_t srcA, uint64_t srcB)
{
    ereg->getstat()->setInput(stat);
    ereg->geticode()->setInput(icode);
    ereg->getifun()->setInput(ifun);    
    ereg->getvalC()->setInput(valC);
    ereg->getvalA()->setInput(valA); //valA cant be obtained from the d register, so set to 0
    ereg->getvalB()->setInput(valB);
    ereg->getdstE()->setInput(dstE); //dstE cant be obtained from the d register, so set to RNONE (Because dstE is a resister.)
    ereg->getdstM()->setInput(dstM);
    ereg->getsrcA()->setInput(srcA);
    ereg->getsrcB()->setInput(srcB);
}
