/*
    This is a basic five segment MIPS pipeline simulator designed by Windigal.
    The simulator currently only supports five instructions: Load,Store,Add,Beqz,Nop
    To prevent Chinese display errors caused by coding issues, all annotations are in English.
    Use fr command to read file. Please input the absolute path. The file should have one binary instruction (32-bit) per line.
*/

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <list>
#include <iomanip>
#include <fstream>

using namespace std;

// Define instruction types.
enum InstructionType
{
    Load = 1,
    Store,
    Add,
    Beqz,
    Nop
};

// Define pipline stages.
enum PiplineStage
{
    If = 0,
    Id,
    Ex,
    Mem,
    Wb
};

// Define instructions in instruction memory.
struct Instruction
{
    InstructionType type;
    string rs = "r0";
    string rt = "r0";
    string rd = "r0";
    int imm = -1;
};

// Define instructions in pipline.
struct Instructions_in_pipeline
{
    Instruction ir;
    int pc = 0;
    int stage = 0;
    int order = 0;
};

// Define registers.
struct Register
{
    string name;
    int value;
    bool writing = false;
};

// Define the IF_ID pipeline register.
struct IF_ID
{
    int npc = 0;
    Instruction ir;
} if_id;

// Define the ID_EX pipeline register.
struct ID_EX
{
    int alu_a = 0;
    int alu_b = 0;
    int imm = 0;
    Instruction ir;
} id_ex;

// Define the EX_MEM pipeline register.
struct EX_MEM
{
    int alu_o = 0;
    int alu_b = 0;
    Instruction ir;
} ex_mem;

// Define the MEM_WB pipeline register.
struct MEM_WB
{
    int lmd = 0;
    int alu_o = 0;
    Instruction ir;
} mem_wb;

string stagename[5] = {"IF", "ID", "EX", "MEM", "WB"};
string ClockCycles_Diagram[1000][1000];     // Clockcycles diagram
vector<Register> RegisterFile(32, {"", 0}); // Register file
vector<Instruction> InstructionMemory;      // Instruction memory
vector<Register> RegisterFile_else;         // Used for data staging in forwarding
list<Instructions_in_pipeline> pipline;     // The pipline
int DataMemory[1000] = {0};                 // Data memory
string file_path;

int pc = 0;
int Instruction_num = 1; // Number of instructions that have already flowed out
int ClockCycles = 0;     // Number of clockcycles that have already occurred
int StallCycles = 0;     // Number of clockcycles paused on the pipline
bool has_end = false;    // Whether the program has ended
bool stall = false;      // Whether the pipline is paused now
bool Forwarding = false; // Whether to enable forwarding

// Convert binary to decimal
int to_decimalism(string a)
{
    int n = stoi(a);
    int decimalNumber = 0, base = 1;
    while (n)
    {
        decimalNumber += (n % 10) * base;
        n /= 10;
        base *= 2;
    }
    return decimalNumber;
}

// Binary instruction processing
void Instruction_read(string Binary_instruction)
{
    if (Binary_instruction.substr(0, 6) == "100000")
    {
        Instruction lw{Load, "r" + to_string(to_decimalism(Binary_instruction.substr(6, 5))), "r" + to_string(to_decimalism(Binary_instruction.substr(11, 5))), "r0", to_decimalism(Binary_instruction.substr(16, 16))};
        InstructionMemory.push_back(lw);
    }
    if (Binary_instruction.substr(0, 6) == "101000")
    {
        Instruction sw{Store, "r" + to_string(to_decimalism(Binary_instruction.substr(6, 5))), "r" + to_string(to_decimalism(Binary_instruction.substr(11, 5))), "r0", to_decimalism(Binary_instruction.substr(16, 16))};
        InstructionMemory.push_back(sw);
    }
    if (Binary_instruction.substr(0, 6) == "000000" && Binary_instruction.substr(26, 6) == "100000")
    {
        Instruction add{Add, "r" + to_string(to_decimalism(Binary_instruction.substr(6, 5))), "r" + to_string(to_decimalism(Binary_instruction.substr(11, 5))), "r" + to_string(to_decimalism(Binary_instruction.substr(16, 5))), 0};
        InstructionMemory.push_back(add);
    }
    if (Binary_instruction.substr(0, 6) == "000001" && Binary_instruction.substr(11, 5) == "00010")
    {
        Instruction beqz{Beqz, "r" + to_string(to_decimalism(Binary_instruction.substr(6, 5))), "r0", "r0", to_decimalism(Binary_instruction.substr(16, 16))};
        InstructionMemory.push_back(beqz);
    }
    if (Binary_instruction == "00000000000000000000000000000000")
    {
        Instruction nop{Nop};
        InstructionMemory.push_back(nop);
    }
}

// Program initialization
void program_Init()
{
    for (int i = 0; i < 1000; i++)
        for (int j = 0; j < 1000; j++)
            ClockCycles_Diagram[i][j] = "";
    ClockCycles_Diagram[0][0] = "Instruction/Cycles";
    for (int i = 0; i < 32; i++)
    {
        string r = "r";
        string num = to_string(i);
        RegisterFile[i] = {r + num, 0, false};
    }
    RegisterFile_else.clear();
    pipline.clear();
    memset(DataMemory, 0, sizeof(DataMemory));

    pc = 0;
    Instruction_num = 1;
    ClockCycles = 0;
    StallCycles = 0;
    has_end = false;
    stall = false;

    Instruction Ir;
    if_id = {0, Ir};
    id_ex = {0, 0, 0, Ir};
    ex_mem = {0, 0, Ir};
    mem_wb = {0, 0, Ir};

    RegisterFile[1].value = 1;
    RegisterFile[2].value = 2;
}

// Instruction standard representation
string Standard_Instruction(Instruction ir)
{
    string S_ir = "";
    switch (ir.type)
    {
    case Load:
        S_ir = "lw " + ir.rt + "," + to_string(ir.imm) + "(" + ir.rs + ")";
        break;
    case Store:
        S_ir = "sw " + to_string(ir.imm) + "(" + ir.rs + ")" + "," + ir.rt;
        break;
    case Beqz:
        S_ir = "beqz " + ir.rs + "," + to_string(ir.imm);
        break;
    case Add:
        S_ir = "add " + ir.rd + "," + ir.rs + "," + ir.rt;
        break;
    case Nop:
        S_ir = "nop";
    default:
        break;
    }
    return S_ir;
}

// Register read
int readRegister(string regName)
{
    for (int i = 0; i < (int)RegisterFile.size(); i++)
    {
        if (RegisterFile[i].name == regName)
        {
            return RegisterFile[i].value;
        }
    }
    if (Forwarding)
    {
        for (int i = 0; i < (int)RegisterFile_else.size(); i++)
        {
            if (RegisterFile_else[i].name == regName)
            {
                return RegisterFile_else[i].value;
            }
        }
    }

    return -1;
}

// Register write
void writeRegister(string regName, int value)
{
    for (int i = 0; i < (int)RegisterFile.size(); i++)
    {
        if (RegisterFile[i].name == regName)
        {
            RegisterFile[i].value = value;
            return;
        }
    }
}

// Instructions flowing out to the pipline
void Instruction_outflow()
{
    Instructions_in_pipeline I = {InstructionMemory[pc / 4], pc, 0, Instruction_num};
    Instruction_num++;
    pipline.push_back(I);
    ClockCycles_Diagram[I.order][0] = Standard_Instruction(I.ir);
}

/*
The instruction format
load：rt  <-  rs+offset
op-code:100000    rs=base(5 bit)     rt(5 bit)      offset(16 bit)

store: rt  ->  rs+offset
op-code:101000    rs=base(5 bit)     rt(5 bit)      offset(16 bit)

add: rd  <-  rs+rt
op_code:000000    rs(5 bit)    rt(5 bit)   rd(5 bit)   0(5 bit)   func:100000

beqz: if(rs==0)  pc=offset
op_code:000001    rs(5 bit)    beqz:00010   offset(16 bit)


*/

// Operations of the IF stage.
void IF()
{
    if (stall)
        return;
    if_id.ir = InstructionMemory[pc / 4];
    if (if_id.ir.type == Beqz && readRegister(if_id.ir.rs) == 0)
    {
        pc = pc + if_id.ir.imm;
        if_id.npc = pc;
    }
    else
    {
        pc += 4;
        if_id.npc = pc;
    }
    switch (if_id.ir.type)
    {
    case Load:
        for (int i = 0; i < 32; i++)
        {
            if (RegisterFile[i].name == if_id.ir.rs && RegisterFile[i].writing == true)
                stall = true;
        }
        break;
    case Store:
        for (int i = 0; i < 32; i++)
        {
            if (RegisterFile[i].name == if_id.ir.rt && RegisterFile[i].writing == true)
                stall = true;
        }
        break;
    case Add:
        for (int i = 0; i < 32; i++)
        {
            if (RegisterFile[i].name == if_id.ir.rs && RegisterFile[i].writing == true)
                stall = true;
            if (RegisterFile[i].name == if_id.ir.rt && RegisterFile[i].writing == true)
                stall = true;
        }
        break;
    default:
        break;
    }
}

// Operations of the ID stage.
void ID()
{
    if (!Forwarding)
    {
        id_ex.alu_a = readRegister(if_id.ir.rs);
        id_ex.alu_b = readRegister(if_id.ir.rt);
        id_ex.ir = if_id.ir;
        id_ex.imm = if_id.ir.imm;
    }
    else
    {
        switch (if_id.ir.type)
        {
        case Load:
        {
            id_ex.alu_b = readRegister(if_id.ir.rt);
            id_ex.ir = if_id.ir;
            id_ex.imm = if_id.ir.imm;
            for (int i = 0; i < 32; i++)
            {
                if (RegisterFile[i].name == if_id.ir.rs && RegisterFile[i].writing == true)
                    id_ex.alu_a = readRegister(if_id.ir.rs + "_else");
                else
                    id_ex.alu_a = readRegister(if_id.ir.rs);
            }
            break;
        }

        case Store:
        {
            id_ex.ir = if_id.ir;
            id_ex.imm = if_id.ir.imm;
            for (int i = 0; i < 32; i++)
            {
                if (RegisterFile[i].name == if_id.ir.rs && RegisterFile[i].writing == true)
                    id_ex.alu_a = readRegister(if_id.ir.rs + "_else");
                else
                    id_ex.alu_a = readRegister(if_id.ir.rs);
                if (RegisterFile[i].name == if_id.ir.rt && RegisterFile[i].writing == true)
                    id_ex.alu_b = readRegister(if_id.ir.rt + "_else");
                else
                    id_ex.alu_b = readRegister(if_id.ir.rt);
            }
            break;
        }

        case Add:
        {
            id_ex.ir = if_id.ir;
            id_ex.imm = if_id.ir.imm;
            for (int i = 0; i < 32; i++)
            {
                if (RegisterFile[i].name == if_id.ir.rs)
                {
                    if (RegisterFile[i].writing == true)
                        id_ex.alu_a = readRegister(if_id.ir.rs + "_else");
                    else
                        id_ex.alu_a = readRegister(if_id.ir.rs);
                }
                if (RegisterFile[i].name == if_id.ir.rt)
                {
                    if (RegisterFile[i].writing == true)
                        id_ex.alu_b = readRegister(if_id.ir.rt + "_else");
                    else
                        id_ex.alu_b = readRegister(if_id.ir.rt);
                }
            }
            break;
        }

        case Beqz:
        {
            id_ex.alu_b = readRegister(if_id.ir.rt);
            id_ex.ir = if_id.ir;
            id_ex.imm = if_id.ir.imm;
            for (int i = 0; i < 32; i++)
            {
                if (RegisterFile[i].name == if_id.ir.rs && RegisterFile[i].writing == true)
                    id_ex.alu_a = readRegister(if_id.ir.rs + "_else");
                else
                    id_ex.alu_a = readRegister(if_id.ir.rs);
            }
            break;
        }

        default:
            break;
        }
    }
    if (if_id.ir.type == Load)
    {
        for (int i = 0; i < 32; i++)
        {
            if (RegisterFile[i].name == if_id.ir.rt)
                RegisterFile[i].writing = true;
        }
    }
    if (if_id.ir.type == Add)
    {
        for (int i = 0; i < 32; i++)
        {
            if (RegisterFile[i].name == if_id.ir.rd)
                RegisterFile[i].writing = true;
        }
    }
}

// Operations of the EX stage.
void EX()
{
    ex_mem.ir = id_ex.ir;
    if (id_ex.ir.type == Add)
    {
        ex_mem.alu_o = id_ex.alu_a + id_ex.alu_b;
        if (Forwarding)
        {
            Register Reg;
            Reg.name = ex_mem.ir.rd + "_else";
            Reg.value = ex_mem.alu_o;
            RegisterFile_else.push_back(Reg);
            stall = false;
        }
    }
    else if (id_ex.ir.type == Load || id_ex.ir.type == Store)
    {
        ex_mem.alu_o = id_ex.alu_a + id_ex.imm;
        ex_mem.alu_b = id_ex.alu_b;
    }
}

// Operations of the MEM stage.
void MEM()
{
    mem_wb.ir = ex_mem.ir;
    if (ex_mem.ir.type == Add)
    {
        mem_wb.alu_o = ex_mem.alu_o;
    }
    else if (ex_mem.ir.type == Load)
    {
        mem_wb.lmd = DataMemory[ex_mem.alu_o];
        if (Forwarding)
        {
            Register Reg;
            Reg.name = mem_wb.ir.rt + "_else";
            Reg.value = mem_wb.lmd;
            RegisterFile_else.push_back(Reg);
            stall = false;
        }
    }
    else if (ex_mem.ir.type == Store)
    {
        DataMemory[ex_mem.alu_o] = ex_mem.alu_b;
    }
}

// Operations of the WB stage.
void WB()
{
    if (mem_wb.ir.type == Add)
    {
        writeRegister(mem_wb.ir.rd, mem_wb.alu_o);
        for (int i = 0; i < 32; i++)
        {
            if (RegisterFile[i].name == mem_wb.ir.rd && RegisterFile[i].writing == true)
                RegisterFile[i].writing = false;
        }
    }
    else if (mem_wb.ir.type == Load)
    {
        writeRegister(mem_wb.ir.rt, mem_wb.lmd);
        for (int i = 0; i < 32; i++)
        {
            if (RegisterFile[i].name == mem_wb.ir.rt && RegisterFile[i].writing == true)
                RegisterFile[i].writing = false;
        }
    }
    if (!Forwarding)
        stall = false;
}

// Instruction fr
// Read the file
void File_read()
{
    program_Init();
    getline(cin, file_path);
    file_path.erase(0, 1);
    ifstream infile;
    infile.open(file_path, ios::in);
    if (!infile.is_open())
    {
        cout << "Failed to read the file." << endl;
        return;
    }
    string buf;
    while (getline(infile, buf))
        Instruction_read(buf);
}

// Instruction n
// Single step execution
void Single_step_execution()
{
    if (InstructionMemory.empty())
    {
        cout << "Please load the program." << endl;
        return;
    }
    if (has_end)
    {
        cout << "This program has completed execution." << endl;
        return;
    }
    bool has_add = false;
    bool true_stall = false;
    if (pipline.empty() && pc / 4 < (int)InstructionMemory.size())
    {
        Instruction_outflow();
        has_add = true;
    }
    ClockCycles++;
    ClockCycles_Diagram[0][ClockCycles] = to_string(ClockCycles);
    for (auto i = pipline.begin(); i != pipline.end();)
    {
        switch (i->stage)
        {
        case If:
        {
            IF();
            ClockCycles_Diagram[i->order][ClockCycles] = stagename[i->stage];
            i->stage++;
            i++;
            break;
        }

        case Id:
        {
            if (!stall)
            {
                ID();
                ClockCycles_Diagram[i->order][ClockCycles] = stagename[i->stage];
                i->stage++;
            }
            else
            {
                ClockCycles_Diagram[i->order][ClockCycles] = "Stall";
                true_stall = true;
            }

            if (!has_add && !stall && pc / 4 < (int)InstructionMemory.size())
            {
                Instruction_outflow();
                has_add = true;
            }
            i++;
            break;
        }

        case Ex:
        {
            EX();
            ClockCycles_Diagram[i->order][ClockCycles] = stagename[i->stage];
            i->stage++;
            if (!has_add && !stall && pc / 4 < (int)InstructionMemory.size())
            {
                Instruction_outflow();
                has_add = true;
            }
            i++;
            break;
        }

        case Mem:
        {
            MEM();
            ClockCycles_Diagram[i->order][ClockCycles] = stagename[i->stage];
            i->stage++;
            if (!has_add && !stall && pc / 4 < (int)InstructionMemory.size())
            {
                Instruction_outflow();
                has_add = true;
            }
            i++;
            break;
        }

        case Wb:
        {
            WB();
            ClockCycles_Diagram[i->order][ClockCycles] = stagename[i->stage];
            i->stage++;
            auto j = ++i;
            pipline.erase(--i);
            i = j;
            if (!has_add && !stall && pc / 4 < (int)InstructionMemory.size())
            {
                Instruction_outflow();
                has_add = true;
            }
            break;
        }

        default:
            break;
        }
    }
    if (true_stall)
        StallCycles++;
    if (pipline.empty())
        has_end = true;
}

// Instruction b
// Sets and executes to a breakpoint
void Execute_to_breakpoint()
{
    if (InstructionMemory.empty())
    {
        cout << "Please load the program." << endl;
        return;
    }
    int breakpoint;
    int stage;
    cin >> breakpoint >> stage;
    if (breakpoint % 4 != 0 || breakpoint < 0 || breakpoint / 4 >= (int)InstructionMemory.size())
    {
        cout << "The breakpoint position must be a multiple of 4 that is not less than 0 and does not exceed the boundary" << endl;
        return;
    }
    if (stage != 0 && stage != 1 && stage != 2 && stage != 3 && stage != 4)
    {
        cout << "The stage must be an integer not less than 0 but less than 5" << endl;
        return;
    }
    while (1)
    {
        for (auto i = pipline.begin(); i != pipline.end(); i++)
        {
            if (i->pc == breakpoint && i->stage == stage)
            {
                cout << stagename[stage] << "-Stage: Reached at the breakpoint" << endl;
                return;
            }
        }
        Single_step_execution();
    }
}

// Instruction e
// Execute to the end of the program
void Execute_to_end()
{
    if (InstructionMemory.empty())
    {
        cout << "Please load the program." << endl;
        return;
    }
    while (!has_end)
        Single_step_execution();
    cout << "This program has completed execution." << endl;
}

// Instruction sr
// Output the register status
void Show_Register()
{
    for (int i = 0; i < 32; i++)
    {
        if (i != 0 && i % 4 == 0)
            cout << endl;
        cout << RegisterFile[i].name << ": " << RegisterFile[i].value << " ";
    }
    cout << endl;
}

// Instruction sd
// Output clockcycle diagram
void Show_Diagram()
{
    for (int i = 0; i < Instruction_num; i++)
    {
        cout << setw(25) << setiosflags(ios::left) << ClockCycles_Diagram[i][0];
        for (int j = 1; j <= ClockCycles; j++)
        {
            cout << setw(7) << setiosflags(ios::left) << ClockCycles_Diagram[i][j];
        }
        cout << endl;
    }
    cout << endl;
}

// Instruction ss
// Outputs statisticas
void Show_Stastistics()
{
    cout << "ClockCycles: " << ClockCycles << endl;
    cout << "StallCycles: " << StallCycles << endl;
}

// Instruction f
// Changes the forwarding status
void Forwarding_Change()
{
    Forwarding = !Forwarding;
    if (Forwarding)
        cout << "Enable Forwarding. The program will stop running and reinitialize." << endl;
    else
        cout << "Disable Forwarding. The program will stop running and reinitialize." << endl;
    program_Init();
}

// Instruction h
// Outputs instruction help information
void Help()
{
    cout << "fr file_path   File Read." << endl;
    cout << "n              Single step execution." << endl;
    cout << "b  pc  stage   Set and execute to breakpoint." << endl;
    cout << "e              Execute to end." << endl;
    cout << "sr             Show registers." << endl;
    cout << "sd             Show cycle diagram." << endl;
    cout << "ss             Show stastistic." << endl;
    cout << "f              Forwarding change." << endl;
    cout << "q              Quit." << endl;
}

// Command line interaction
void interaction()
{
    /*
    1.n: Single step execution
    2.b: Execute to breakpoint
    3.e: Execute to end
    4.sr: Show Registers
    5.sd: Show Cycle Diagram
    6.ss: Show Stastistic
    7.f: Forwarding change
    8.h: print the commands help
    9.quit
    */

    while (1)
    {
        string input;
        cout << "[MIPS_pipline @Windigal] ";
        cin >> input;
        if (input == "fr")
            File_read();

        else if (input == "n")
            Single_step_execution();

        else if (input == "b")
            Execute_to_breakpoint();

        else if (input == "e")
            Execute_to_end();

        else if (input == "sr")
            Show_Register();

        else if (input == "sd")
            Show_Diagram();

        else if (input == "ss")
            Show_Stastistics();

        else if (input == "f")
            Forwarding_Change();

        else if (input == "h")
            Help();

        else if (input == "q")
            break;
        else
            cout << "Invalid command, type h for help." << endl;
    }
}

int main()
{
    interaction();
    return 0;
}
