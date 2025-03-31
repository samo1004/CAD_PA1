#include <bits/stdc++.h>
using namespace std;

struct Gate
{
    string name;           // 例: "gat1"
    string type;           // 例: "nand"
    string output;         // 例: "net10"
    vector<string> inputs; // 例: {"net1", "net3"}
};

struct Net
{
    string netName;           // 例: "net1"
    vector<string> pinList;   // 連接到此 Net 的各個 pin
    bool isTopInput = false;  // 是否為頂層 input (inptX)
    bool isTopOutput = false; // 是否為頂層 output (outptX)
};

struct Circuit
{
    string moduleName;  // 模組名稱, e.g. "c17"
    vector<Gate> gates; // 所有 Gate
    vector<Net> nets;   // 所有 Net
};

// 刪頭尾空白
string trim(const string &s)
{
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start]))
    {
        start++;
    }

    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1]))
    {
        end--;
    }
    return s.substr(start, end - start);
}

void printCircuit(const Circuit &circuit)
{
    // 把所有變數都印出來 用 "struct的變數名稱: " << 變數 << endl;
    cout << "Module: " << circuit.moduleName << endl;
    cout << "Gates:" << endl;
    for (auto &g : circuit.gates)
    {
        cout << "  name: " << g.name << " type: " << g.type << " output: " << g.output << " inputs: ";
        for (auto &inp : g.inputs)
        {
            cout << inp << " ";
        }
        cout << endl;
    }
    cout << "Nets:" << endl;
    for (auto &n : circuit.nets)
    {
        cout << "  netName: " << n.netName << " pinList: ";
        for (auto &pin : n.pinList)
        {
            cout << pin << " ";
        }
        cout << " isTopInput: " << n.isTopInput << " isTopOutput: " << n.isTopOutput << endl;
    }
}

bool parseNetlist(const string &inputFileName, Circuit &circuit)
{
    ifstream fin(inputFileName.c_str());
    if (!fin.is_open())
    {
        cerr << "ERROR: cannot open file: " << inputFileName << "\n";
        return false;
    }

    string line;
    while (getline(fin, line))
    {
        line = trim(line);
        if (line.empty())
            continue; // 跳過空行
        if (line[0] == '*')
            continue; // 跳過註解行

        // 例: "Inst gat1 nand"
        if (line.rfind("Inst ", 0) == 0)
        {
            istringstream iss(line);
            string dummy, gateName, gateType;
            iss >> dummy >> gateName >> gateType;

            Gate g;
            g.name = gateName;
            g.type = gateType;
            circuit.gates.push_back(g);
        }
        // 例: "NET net1 2"
        else if (line.rfind("NET ", 0) == 0)
        {
            istringstream iss(line);
            string dummy, netName;
            int pinNum = 0;
            iss >> dummy >> netName >> pinNum; // "NET", "net1", 2

            Net netObj;
            netObj.netName = netName;

            int pinsRead = 0;
            bool done = false;

            // 讀 pin 資訊
            while (!done && pinsRead < pinNum && getline(fin, line)) // NET net11 3 \n PIN gat2/OUT1, gat3/IN2, gat4/IN1
            {
                line = trim(line);
                cout << "line: " << line << endl;
                if (line.empty() || line[0] == '*')
                    continue;

                size_t pinPos = line.find("PIN ");
                if (pinPos != string::npos)
                {
                    // 去掉 "PIN "
                    line = line.substr(pinPos + 4);
                    cout << "line: " << line << endl;
                }

                stringstream sspin(line);
                string token;
                while (getline(sspin, token, ','))
                {
                    token = trim(token);
                    cout << "token: " << token << endl;
                    if (!token.empty()) // 如果不是只有空白
                    {
                        netObj.pinList.push_back(token);
                        pinsRead++;
                        if (pinsRead == pinNum)
                        {
                            done = true;
                            break;
                        }
                    }
                }
            }
            circuit.nets.push_back(netObj);
        }
        // else: 其他不處理
    }
    fin.close();

    // -----用來輸入一個gate name e.g. "gat1"，找出這個gate在circuit.gates的index-----
    // 建立 gateName -> index
    vector<string> gateNames;
    gateNames.reserve(circuit.gates.size());
    for (size_t i = 0; i < circuit.gates.size(); i++)
    {
        cout << "i: " << i << endl;
        cout << "circuit.gates[i].name: " << circuit.gates[i].name << endl;
        gateNames.push_back(circuit.gates[i].name);
    }

    // lambda 傳進來一個str return int, pass by ref
    // 查找 gateIndex
    auto findGateIndex = [&](const string &gName) -> int
    {
        for (size_t i = 0; i < gateNames.size(); i++)
        {
            if (gateNames[i] == gName)
                return (int)i;
        }
        return -1;
    };
    // ---------------------------------------------------------------------------

    // 映射 pinList 到 Gate 的 inputs / output
    for (size_t i = 0; i < circuit.nets.size(); i++)
    {
        Net &netObj = circuit.nets[i];
        for (auto &pinStr : netObj.pinList) // e.g. pinStr -> gat1/In1
        {
            size_t slashPos = pinStr.find("/"); // 先處裡不是inptX or outptX的pin
            if (slashPos != string::npos)
            {
                // gateName/pinName
                string gName = pinStr.substr(0, slashPos);    // gat1
                string pinName = pinStr.substr(slashPos + 1); // e.g. IN1 or OUT1
                int idx = findGateIndex(gName);

                if (idx >= 0) // 有這個gate
                {
                    Gate &g = circuit.gates[idx];
                    // INx
                    if (pinName.rfind("IN", 0) == 0)
                    {
                        // e.g. "IN2"
                        string numStr = pinName.substr(2);    // 拿出2
                        int inNum = atoi(numStr.c_str());     // 轉成int
                        int vecIndex = inNum - 1;             // 2會存在inputs[1] (1會存在inputs[0]...
                        if (vecIndex >= (int)g.inputs.size()) // 如果原本的inputs不夠大(原本還沒存過比較後面的pin e.g. 先存過IN2那IN1有index 0可以放了 反之IN3就要resize)
                        {
                            g.inputs.resize(vecIndex + 1);
                        }
                        g.inputs[vecIndex] = netObj.netName;
                    }
                    // OUTx
                    else if (pinName.rfind("OUT", 0) == 0)
                    {
                        g.output = netObj.netName;
                    }
                }
            }
            else
            {
                // 可能是 inptX or outptX
                if (pinStr.rfind("inpt", 0) == 0)
                {
                    netObj.isTopInput = true;
                }
                else if (pinStr.rfind("outpt", 0) == 0)
                {
                    netObj.isTopOutput = true;
                }
            }
        }
    }
    return true;
}

bool generateVerilog(const Circuit &circuit, const string &outputFileName)
{
    ofstream fout(outputFileName.c_str());

    // timescale
    fout << "`timescale 1ns/1ps\n";

    // 先存pin有Input Output的net
    vector<string> topInputs, topOutputs; // inptX outptX
    for (auto &net : circuit.nets)
    {
        if (net.isTopInput)
            topInputs.push_back(net.netName);
        if (net.isTopOutput)
            topOutputs.push_back(net.netName);
    }

    // 3. module宣告 //只會有in out
    fout << "module " << circuit.moduleName << " (";
    {
        bool firstIO = true;

        // 輸入
        for (size_t i = 0; i < topInputs.size(); i++)
        {
            if (!firstIO)
                fout << ", "; // 第一個以外的input前面加逗號
            // cout << "topInputs[i]: " << topInputs[i] << endl;
            fout << "N" << topInputs[i].substr(3); // net1 -> N1
            firstIO = false;
        }
        // 輸出
        for (size_t i = 0; i < topOutputs.size(); i++)
        {
            if (!firstIO)
                fout << ", ";
            // cout << "topOutputs[i]: " << topOutputs[i] << endl;
            fout << "N" << topOutputs[i].substr(3);
            firstIO = false;
        }
    }
    fout << ");\n\n";

    // 4. input / output 宣告
    if (!topInputs.empty())
    {
        fout << "input ";
        for (size_t i = 0; i < topInputs.size(); i++)
        {
            if (i > 0)
                fout << ", ";
            if (topInputs[i].size() > 3 && topInputs[i].rfind("net", 0) == 0)
            {
                fout << "N" << topInputs[i].substr(3);
            }
            else
            {
                fout << topInputs[i];
            }
        }
        fout << ";\n";
    }
    if (!topOutputs.empty())
    {
        fout << "output ";
        for (size_t i = 0; i < topOutputs.size(); i++)
        {
            if (i > 0)
                fout << ", ";
            if (topOutputs[i].size() > 3 && topOutputs[i].rfind("net", 0) == 0)
            {
                fout << "N" << topOutputs[i].substr(3);
            }
            else
            {
                fout << topOutputs[i];
            }
        }
        fout << ";\n";
    }
    fout << "\n";

    // 5. 宣告 wire (不是頂層in/out的 net)
    vector<string> internalNets;
    for (auto &net : circuit.nets)
    {
        if (!net.isTopInput && !net.isTopOutput)
        {
            internalNets.push_back(net.netName);
        }
    }
    if (!internalNets.empty())
    {
        fout << "wire ";
        for (size_t i = 0; i < internalNets.size(); i++)
        {
            if (i > 0)
                fout << ", ";
            if (internalNets[i].size() > 3 && internalNets[i].rfind("net", 0) == 0)
            {
                fout << "N" << internalNets[i].substr(3);
            }
            else
            {
                fout << internalNets[i];
            }
        }
        fout << ";\n\n";
    }

    // 6. Gate
    //    e.g. nand NAND_gat1 (N10, N1, N3);
    for (auto &g : circuit.gates)
    {
        // 大寫 type
        string upType = g.type;
        for (char &c : upType)
            c = toupper((unsigned char)c);

        // e.g. "nand NAND_GAT1 (N10, N1, N3);"
        fout << g.type << " " << upType << "_" << g.name << " (";

        // 先輸出 output
        if (g.output.size() > 3 && g.output.rfind("net", 0) == 0)
        {
            fout << "N" << g.output.substr(3);
        }
        else
        {
            fout << g.output;
        }

        // 再輸出 inputs
        for (auto &inpNet : g.inputs)
        {
            fout << ", ";
            if (inpNet.size() > 3 && inpNet.rfind("net", 0) == 0)
            {
                fout << "N" << inpNet.substr(3);
            }
            else
            {
                fout << inpNet;
            }
        }
        fout << ");\n";
    }

    fout << "\nendmodule\n";
    fout.close();
    return true;
}

int main(int argc, char *argv[])
{
    string inputFileName = argv[1];
    string outputFileName = argv[2];

    Circuit circuit;

    // 從檔名移除 ".netlist" 當作 moduleName
    {
        size_t pos = inputFileName.rfind(".netlist");
        if (pos != string::npos)
        {
            circuit.moduleName = inputFileName.substr(0, pos);
        }
        else
        {
            circuit.moduleName = inputFileName;
        }
    }

    if (!parseNetlist(inputFileName, circuit))
    {
        cerr << "Failed to parse netlist.\n";
        return 1;
    }
    printCircuit(circuit);
    if (!generateVerilog(circuit, outputFileName))
    {
        cerr << "Failed to generate Verilog.\n";
        return 1;
    }

    cout << "Conversion done! Output: " << outputFileName << endl;
    return 0;
}
