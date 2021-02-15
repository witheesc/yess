class DecodeStage: public Stage
{
    private:
        uint64_t d_srcA;
        uint64_t d_srcB;
        bool E_bubble;
        void setEInput(E * ereg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                       uint64_t valC, uint64_t valA, uint64_t valB,
                       uint64_t dstE, uint64_t dstM, uint64_t srcA, uint64_t srcB);
    public:
        uint64_t srcAComponent(uint64_t icode, uint64_t rA);
        uint64_t srcBComponent(uint64_t icode, uint64_t rB);
        uint64_t dstEComponent(uint64_t icode, uint64_t rB);
        uint64_t dstMComponent(uint64_t icode, uint64_t rA);
        uint64_t selPlusFwdA(uint64_t srcA, uint64_t rvalA, uint64_t icode, uint64_t valP, Stage ** stages, PipeReg ** pregs);
        uint64_t fwdB(uint64_t srcB, uint64_t rvalB, Stage ** stages, PipeReg ** pregs);
        uint64_t getd_srcA();
        uint64_t getd_srcB();
        bool calculateControlSignals(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB, uint64_t e_Cnd);
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void bubbleE(PipeReg ** pregs);
        void normalE(PipeReg ** pregs);
        void doClockHigh(PipeReg ** pregs);
};
