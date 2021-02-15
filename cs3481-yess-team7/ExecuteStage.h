class ExecuteStage: public Stage
{
    private:
        uint64_t dstE;
        uint64_t valE;
        bool M_bubble;
        uint64_t e_Cnd;
        void setMInput(M * mreg, uint64_t stat, uint64_t icode,
                       uint64_t Cnd, uint64_t valE, uint64_t valA,
                       uint64_t dstE, uint64_t dstM);
    public:
        uint64_t ALUCompA(uint64_t icode, uint64_t valA, uint64_t valC);
        uint64_t ALUCompB(uint64_t icode, uint64_t valB);
        uint64_t ALUCompFun(uint64_t icode, uint64_t ifun);
        bool setCC(uint64_t icode, uint64_t m_stat, uint64_t W_stat);
        uint64_t dstEComp(uint64_t icode, uint64_t Cnd, uint64_t dstE);
        uint64_t ALU(uint64_t alufun, uint64_t aluA, uint64_t aluB, bool &of);
        uint64_t get_dstE();
        uint64_t get_valE();
        uint64_t cond(uint64_t icode, uint64_t ifun);
        bool calculateControlSignals(uint64_t m_stat, uint64_t W_stat);
        uint64_t gete_Cnd();
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void normalM(PipeReg ** pregs);
        void bubbleM(PipeReg ** pregs);
        void doClockHigh(PipeReg ** pregs);
};
