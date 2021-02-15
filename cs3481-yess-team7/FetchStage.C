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
#include "FetchStage.h"
#include "DecodeStage.h"
#include "ExecuteStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"
#include "Tools.h"

// Implements the logic for selecting a PC
// If icode is a jump, will set the PC accordingly.
// If icode is a ret, withh set the PC accordingly.
uint64_t FetchStage::selectPC(F * freg, M * mreg, W * wreg) 
{
    if (mreg ->geticode()->getOutput() == IJXX && (!mreg->getCnd()->getOutput()))
        return mreg->getvalA()->getOutput(); // returns M_valA
    if (wreg ->geticode()->getOutput() == IRET)
        return wreg->getvalM()->getOutput(); // returns W_valM;
    else
        return freg->getpredPC()->getOutput(); // returns F_predPC;
}

// Returns a boolean for if we need register ids in our instruction.
bool FetchStage::needRegIds(uint64_t icode) {
    // Does this instruction have a register byte?
    if (icode == IRRMOVQ || icode == IOPQ
         || icode == IPUSHQ || icode == IPOPQ 
         || icode == IIRMOVQ || icode == IRMMOVQ 
         || icode == IMRMOVQ)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Returns a boolean for if we need a destination (valC) in our instruction.
bool FetchStage::needValC(uint64_t icode)
{
    // Does this instruction have a constant word?
    if (icode == IIRMOVQ || icode == IRMMOVQ 
        || icode == IMRMOVQ || icode == IJXX 
        || icode == ICALL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Implements the logic for selecting the PC.
// If jump, will return valC (destination)
// Else, return next sequential instruction.
uint64_t FetchStage::predictPC(uint64_t icode, uint64_t valC, uint64_t valP) 
{
    if (icode == IJXX || icode == ICALL)
    {
        return valC;
    }
    else
    {
        return valP;
    }
}

// Implements the logic for incrementing the PC.
// Takes current PC, need reg ids, and needValc.
// 
uint64_t FetchStage::PCincrement(uint64_t f_pc, bool needRegIds, bool needValC)
{
    if (needRegIds && needValC)
    {
        return f_pc + 10;
    }      
    else if (needRegIds && !needValC)
    {
        return f_pc + 2;
    }
    else if (!needRegIds && needValC)
    {
        return f_pc + 9;
    }
    else
    {
        return f_pc + 1;
    }
}

// Implements the logic for recieving the register Ids from an instruction
void FetchStage::getRegIds(uint64_t &rA, uint64_t &rB, uint64_t f_pc)
{
    Memory * mem = Memory::getInstance();
    bool error = false;

    rA = mem->getByte(f_pc + 1, error);
    rA = rA >> 4;
    
    rB = mem->getByte(f_pc + 1, error);
    rB = rB & 0x0F;   
          
}

// Method to build the valC from an instruction (if necessary)
uint64_t FetchStage::buildValC(uint64_t f_pc, bool RegIDBool)
{
    Memory * mem = Memory::getInstance();
    bool error = false;
    uint8_t bytes[LONGSIZE];
    
    if (RegIDBool)
    {
        bytes[0] = mem->getByte(f_pc + 2, error);
        bytes[1] = mem->getByte(f_pc + 3, error);
        bytes[2] = mem->getByte(f_pc + 4, error);
        bytes[3] = mem->getByte(f_pc + 5, error);
        bytes[4] = mem->getByte(f_pc + 6, error);
        bytes[5] = mem->getByte(f_pc + 7, error);
        bytes[6] = mem->getByte(f_pc + 8, error);
        bytes[7] = mem->getByte(f_pc + 9, error);
    }
    else
    {
        bytes[0] = mem->getByte(f_pc + 1, error);
        bytes[1] = mem->getByte(f_pc + 2, error);
        bytes[2] = mem->getByte(f_pc + 3, error);
        bytes[3] = mem->getByte(f_pc + 4, error);
        bytes[4] = mem->getByte(f_pc + 5, error);
        bytes[5] = mem->getByte(f_pc + 6, error);
        bytes[6] = mem->getByte(f_pc + 7, error);
        bytes[7] = mem->getByte(f_pc + 8, error);
    }

    return Tools::buildLong(bytes);

}

// Method to return if icode is a valid icode for all instructions
// In the Y86 Instruction Set.
bool FetchStage::instr_valid(uint64_t icode)
{
    return (icode == INOP || icode == IHALT || icode == IRRMOVQ 
    || icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ 
    || icode == IOPQ || icode == IJXX || icode == ICALL 
    || icode == IRET || icode == IPUSHQ || icode == IPOPQ);
}

// Method to determine the Fetch_Stat
// Returns if the stat is valid or invalid and sets it to an error value,
// Or sets it to a valid stat value for instructions.
uint64_t FetchStage::f_stat(bool mem_error, bool instr_valid, uint64_t icode)
{
    if (mem_error) return SADR;
    else if (!instr_valid) return SINS;
    else if (icode == IHALT) return SHLT;
    else return SAOK;
}

// Method to determine if we need to stall the Fetch Reg
// Returns boolean to see if need to or not.
bool FetchStage::set_F_stall(uint64_t D_icode, uint64_t M_icode, uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB)
{
    return ((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA || E_dstM == d_srcB)) || 
           (D_icode == IRET || E_icode == IRET || M_icode == IRET);
}

// Method to determine if we need to stall the Decode Register
// Returns boolean to see if we need to stall 
bool FetchStage::set_D_stall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB)
{
    return (E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA || E_dstM == d_srcB);
}

// Method to determine if we need to bubble the D register or not.
// Returns a boolean to see if we need to bubble.
bool FetchStage::set_D_bubble(uint64_t D_icode, uint64_t M_icode, uint64_t E_icode, uint64_t e_Cnd, uint64_t d_srcA, uint64_t d_srcB, uint64_t E_dstM)
{
    return (E_icode == IJXX && !e_Cnd) || (!((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA || E_dstM == d_srcB)) &&
           (D_icode == IRET || E_icode == IRET || M_icode == IRET));
}

// This method will setup the booleans to see if we should stall or bubble in DoClockHigh.
void FetchStage::calculateControlSignals(PipeReg ** pregs, Stage ** stages)
{
    DecodeStage * ds = (DecodeStage*) stages[DSTAGE];
    ExecuteStage * es = (ExecuteStage*) stages[ESTAGE];
    E * ereg = (E *) pregs[EREG];
    M * mreg = (M *) pregs[MREG];
    D * dreg = (D *) pregs[DREG];

    uint64_t E_icode = ereg->geticode()->getOutput();
    uint64_t M_icode = mreg->geticode()->getOutput();
    uint64_t D_icode = dreg->geticode()->getOutput();
    uint64_t E_dstM = ereg->getdstM()->getOutput();
    uint64_t d_srcA = ds->getd_srcA();
    uint64_t d_srcB = ds->getd_srcB();
    uint64_t e_Cnd = es->gete_Cnd();

    F_stall = set_F_stall(D_icode, M_icode, E_icode, E_dstM, d_srcA, d_srcB);
    D_stall = set_D_stall(E_icode, E_dstM, d_srcA, d_srcB);
    D_bubble = set_D_bubble(D_icode, M_icode, E_icode, e_Cnd, d_srcA, d_srcB, E_dstM);
}

/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool FetchStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
   F * freg = (F *) pregs[FREG];
   D * dreg = (D *) pregs[DREG];
   M * mreg = (M *) pregs[MREG];
   W * wreg = (W *) pregs[WREG];
   
   uint64_t f_pc = 0, icode = 0, ifun = 0, valC = 0, valP = 0;
   uint64_t rA = RNONE, rB = RNONE, stat = SAOK;

   //code missing here to select the value of the PC
   //and fetch the instruction from memory
   //Fetching the instruction will allow the icode, ifun,
   //rA, rB, and valC to be set.
   //The lab assignment describes what methods need to be
   //written.
   bool error = false;
   f_pc = selectPC(freg, mreg, wreg);
   Memory * mem = Memory::getInstance();  
   uint64_t byte = mem->getByte(f_pc, error);

   ifun = Tools::getBits(byte, 0, 3);
   icode = Tools::getBits(byte, 4, 7);

   if (error)
   {
       icode = INOP;
       ifun = FNONE;
   }
   
   
   bool regIDBool = needRegIds(icode);
   bool valCBool = needValC(icode);
   
   if (regIDBool)
   {
       getRegIds(rA, rB, f_pc);
   }

   if (valCBool)
   {
       valC = buildValC(f_pc, regIDBool);
   }
   
   bool instrvalid = instr_valid(icode);
   stat = f_stat(error, instrvalid, icode);

   valP = PCincrement(f_pc, regIDBool, valCBool);
   uint64_t predPC = predictPC(icode, valC, valP);
   freg->getpredPC()->setInput(predPC);

   calculateControlSignals(pregs, stages);

   //provide the input values for the D register
   setDInput(dreg, stat, icode, ifun, rA, rB, valC, valP);
   return false;
}

// Method to Bubble the D Register
void FetchStage::bubbleD(PipeReg ** pregs)
{
    D * dreg = (D *) pregs[DREG];

    dreg->getstat()->bubble(SAOK);
    dreg->geticode()->bubble(INOP);
    dreg->getifun()->bubble();
    dreg->getrA()->bubble(RNONE);
    dreg->getrB()->bubble(RNONE);
    dreg->getvalC()->bubble();
    dreg->getvalP()->bubble();
       
}

// Method to Treat the D Register normal.
void FetchStage::normalD(PipeReg ** pregs)
{
    D * dreg = (D *) pregs[DREG];

    dreg->getstat()->normal();
    dreg->geticode()->normal();
    dreg->getifun()->normal();
    dreg->getrA()->normal();
    dreg->getrB()->normal();
    dreg->getvalC()->normal();
    dreg->getvalP()->normal();

}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void FetchStage::doClockHigh(PipeReg ** pregs)
{
   F * freg = (F *) pregs[FREG];
   
   if (!F_stall)
   {
        freg->getpredPC()->normal();
   }
   
   if (D_bubble)
   {
       bubbleD(pregs);
   }
   else if (!D_stall)
   {
       normalD(pregs);
   }
}

/* setDInput
 * provides the input to potentially be stored in the D register
 * during doClockHigh
 *
 * @param: dreg - pointer to the D register instance
 * @param: stat - value to be stored in the stat pipeline register within D
 * @param: icode - value to be stored in the icode pipeline register within D
 * @param: ifun - value to be stored in the ifun pipeline register within D
 * @param: rA - value to be stored in the rA pipeline register within D
 * @param: rB - value to be stored in the rB pipeline register within D
 * @param: valC - value to be stored in the valC pipeline register within D
 * @param: valP - value to be stored in the valP pipeline register within D
*/
void FetchStage::setDInput(D * dreg, uint64_t stat, uint64_t icode, 
                           uint64_t ifun, uint64_t rA, uint64_t rB,
                           uint64_t valC, uint64_t valP)
{
   dreg->getstat()->setInput(stat);
   dreg->geticode()->setInput(icode);
   dreg->getifun()->setInput(ifun);
   dreg->getrA()->setInput(rA);
   dreg->getrB()->setInput(rB);
   dreg->getvalC()->setInput(valC);
   dreg->getvalP()->setInput(valP);
}
