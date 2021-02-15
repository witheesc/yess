class MemoryStage: public Stage
{
    private:
        uint64_t dstM;
        uint64_t valM;
        uint64_t stat;
        void setWInput(W * wreg, uint64_t stat, uint64_t icode,
        uint64_t vlaE, uint64_t valM, uint64_t dstE,
        uint64_t dstM);

    public:
        uint64_t memAddr(uint64_t icode, uint64_t valA, uint64_t valE);
        bool memRead(uint64_t icode);
        bool memWrite(uint64_t icode);
        uint64_t get_dstM();
        uint64_t get_valM();
        uint64_t getm_stat();
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);

};
