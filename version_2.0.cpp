#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <cstdlib>

using namespace std;

// ============================================================================
// Message struct (replaces Python namedtuple)
// ============================================================================
struct Message {
    string ErrorType;
    string Location;
    string ErrorMessage;

    Message() : ErrorType(""), Location(""), ErrorMessage("") {}
    Message(const string& type, const string& loc, const string& msg)
        : ErrorType(type), Location(loc), ErrorMessage(msg) {}

    bool hasError() const {
        return !ErrorType.empty();
    }
};

// ============================================================================
// Variable struct (replaces Python namedtuple)
// ============================================================================
struct Variable {
    string name;
    string type;
    int addr;

    Variable() : name(""), type(""), addr(0) {}
    Variable(const string& n, const string& t, int a)
        : name(n), type(t), addr(a) {}
};

// ============================================================================
// Token struct (replaces Python namedtuple)
// ============================================================================
struct Token {
    string type;
    int index;
    string val;
    int cur_line;
    int id;

    Token() : type(""), index(0), val(""), cur_line(0), id(0) {}
    Token(const string& t, int idx, const string& v, int line, int token_id)
        : type(t), index(idx), val(v), cur_line(line), id(token_id) {}
};

// ============================================================================
// SecTable class (Symbol Table Base Class)
// ============================================================================
class SecTable {
public:
    map<string, Variable> variableDict;
    int totalSize;

    SecTable() : totalSize(0) {
        cout << "[DEBUG] SecTable constructor called" << endl;
    }

    virtual ~SecTable() {}

    virtual Message addPaVariable(const Token& token, const string& typ) {
        cout << "[DEBUG] SecTable::addPaVariable - not implemented" << endl;
        return Message();
    }

    Message addVariable(const Token& token, const string& typ, int s) {
        cout << "[DEBUG] SecTable::addVariable - Adding variable: " << token.val
            << ", type: " << typ << ", size: " << s << endl;

        Message message = checkHasDefine(token);
        if (message.hasError()) {
            cout << "[DEBUG] Variable already defined: " << token.val << endl;
            return message;
        }

        Variable tmp(token.val, typ, totalSize);
        totalSize += s;
        variableDict[token.val] = tmp;

        cout << "[DEBUG] Variable added successfully. Total size now: " << totalSize << endl;
        return Message();
    }

    Message checkHasDefine(const Token& token) {
        cout << "[DEBUG] SecTable::checkHasDefine - Checking: " << token.val << endl;

        if (variableDict.find(token.val) != variableDict.end()) {
            stringstream ss;
            ss << "Location:line " << (token.cur_line + 1);
            string location = ss.str();

            string errorMsg = "variable '" + token.val + "' duplicate definition";
            cout << "[DEBUG] Duplicate definition found: " << token.val << endl;
            return Message("Duplicate identified", location, errorMsg);
        }
        return Message();
    }

    Message checkDoDefine(const Token& token) {
        cout << "[DEBUG] SecTable::checkDoDefine - Checking: " << token.val << endl;

        if (variableDict.find(token.val) == variableDict.end()) {
            stringstream ss;
            ss << "Location:line " << token.cur_line;
            string location = ss.str();

            string errorMsg = "variable '" + token.val + "' has no definition";
            cout << "[DEBUG] Variable not defined: " << token.val << endl;
            return Message("Unknown identifier", location, errorMsg);
        }
        return Message();
    }
};

// ============================================================================
// Struct class
// ============================================================================
class Struct : public SecTable {
public:
    string structName;

    Struct(const string& name) : SecTable(), structName(name) {
        cout << "[DEBUG] Struct constructor - name: " << name << endl;
    }
};

// ============================================================================
// Function class
// ============================================================================
class Function : public SecTable {
public:
    string functionName;
    int numOfParameters;
    map<string, Variable> parametersDict;
    vector<string> typeOfParametersList;
    string returnType;

    // �������洢��������
    vector<string> parameterTypes;

    Function(const string& name, const string& retType)
        : SecTable(), functionName(name), numOfParameters(0), returnType(retType) {
        cout << "[DEBUG] Function constructor - name: " << name
            << ", return type: " << retType << endl;
    }

    virtual Message addPaVariable(const Token& token, const string& typ) {
        cout << "[DEBUG] Function::addPaVariable - Adding parameter: "
            << token.val << ", type: " << typ << endl;

        Message message = checkHasDefine(token);
        if (message.hasError()) {
            return message;
        }

        Variable tmp(token.val, typ, -2);

        // �������б�����ַ
        for (map<string, Variable>::iterator it = variableDict.begin();
            it != variableDict.end(); ++it) {
            Variable& var = it->second;
            var.addr -= 2;
        }

        variableDict[token.val] = tmp;
        parametersDict[token.val] = tmp;
        numOfParameters++;

        // ��������¼��������
        parameterTypes.push_back(typ);

        cout << "[DEBUG] Parameter added. Total parameters: " << numOfParameters << endl;
        return Message();
    }
};

// ============================================================================
// SYMBOL class (Global Symbol Table Manager)
// ============================================================================
class SYMBOL {
public:
    vector<Function*> functionList;
    vector<Struct*> structList;
    vector<SecTable*> allList;
    vector<string> globalNameList;
    vector<string> structNameList;
    vector<string> functionNameList;
    map<string, SecTable*> symDict;
    vector<string> symbolTableInfo;

    // ��������ǰ�����ĺ���
    Function* currentFunction;

    // ���������������б�
    set<string> basicTypes;

    SYMBOL() : currentFunction(NULL) {
        cout << "[DEBUG] SYMBOL constructor called" << endl;

        // ��ʼ����������
        basicTypes.insert("int");
        basicTypes.insert("float");
        basicTypes.insert("char");
        basicTypes.insert("void");
    }

    ~SYMBOL() {
        // ����̬������ڴ�
        for (size_t i = 0; i < functionList.size(); ++i) {
            delete functionList[i];
        }
        for (size_t i = 0; i < structList.size(); ++i) {
            delete structList[i];
        }
    }

    // ���������ͼ����Լ��
    bool isTypeCompatible(const string& expected, const string& actual) {
        // ��ͬ�������Ǽ���
        if (expected == actual) return true;

        // �������ͼ��ݹ���
        if (expected == "float" && actual == "int") return true;
        if (expected == "double" && (actual == "int" || actual == "float")) return true;

        // �ṹ�����ͱ�����ȫƥ��
        if (find(structNameList.begin(), structNameList.end(), expected) != structNameList.end()) {
            return expected == actual;
        }

        // ���������Ϊ������
        return false;
    }

    // ��������ȡ������ʵ������
    string getVariableType(const Token& token) {
        // ��鵱ǰ�����еľֲ�����
        if (currentFunction) {
            map<string, Variable>::iterator it = currentFunction->variableDict.find(token.val);
            if (it != currentFunction->variableDict.end()) {
                return it->second.type;
            }
        }

        // ���ȫ�ַ��ű�
        map<string, SecTable*>::iterator symIt = symDict.find(token.val);
        if (symIt != symDict.end()) {
            SecTable* table = symIt->second;
            if (Function* func = dynamic_cast<Function*>(table)) {
                return "function";
            }
            else if (Struct* st = dynamic_cast<Struct*>(table)) {
                return st->structName;
            }
        }

        // ���ȫ�ֱ���������ȫ�ֱ����洢��ĳ�����У�
        // ������Ҫ��������ʵ�ֲ���
        return "unknown";
    }

    Message checkHasDefine(const Token& token) {
        cout << "[DEBUG] SYMBOL::checkHasDefine - Checking global: " << token.val << endl;

        if (find(globalNameList.begin(), globalNameList.end(), token.val) != globalNameList.end()) {
            stringstream ss;
            ss << "Location:line " << (token.cur_line + 1);
            string location = ss.str();

            string errorMsg = "variable '" + token.val + "' duplicate definition";
            cout << "[DEBUG] Global duplicate definition: " << token.val << endl;
            return Message("Duplicate identified", location, errorMsg);
        }
        return Message();
    }

    Message addFunction(const Token& token, const string& returnType) {
        cout << "[DEBUG] SYMBOL::addFunction - Adding function: "
            << token.val << ", return type: " << returnType << endl;

        Message message = checkHasDefine(token);
        if (message.hasError()) {
            return message;
        }

        Function* function = new Function(token.val, returnType);
        functionList.push_back(function);
        allList.push_back(function);
        globalNameList.push_back(token.val);
        functionNameList.push_back(token.val);
        symDict[token.val] = function;

        // ���õ�ǰ����
        currentFunction = function;

        cout << "[DEBUG] Function added successfully. Total functions: "
            << functionList.size() << endl;
        return Message();
    }

    Message addStruct(const Token& token) {
        cout << "[DEBUG] SYMBOL::addStruct - Adding struct: " << token.val << endl;

        Message message = checkHasDefine(token);
        if (message.hasError()) {
            return message;
        }

        Struct* st = new Struct(token.val);
        structList.push_back(st);
        allList.push_back(st);
        globalNameList.push_back(token.val);
        structNameList.push_back(token.val);
        symDict[token.val] = st;

        cout << "[DEBUG] Struct added successfully. Total structs: "
            << structList.size() << endl;
        return Message();
    }

    Message addVariableToTable(const Token& token, const string& varType, bool doseParameter = false) {
        cout << "[DEBUG] SYMBOL::addVariableToTable - Adding variable: "
            << token.val << ", type: " << varType << ", parameter: " << doseParameter << endl;

        if (allList.empty()) {
            cout << "[ERROR] No current scope for variable definition" << endl;
            return Message("No scope", "", "No current scope for variable definition");
        }

        SecTable* tmp = allList.back();
        Message message;

        if (doseParameter) {
            message = tmp->addPaVariable(token, varType);
            // ��ɾ���ظ���Ӳ������͵Ĵ���
        }
        else {
            int s = 0;
            if (varType == "int") {
                s = 2;
            }
            if (find(structNameList.begin(), structNameList.end(), varType) != structNameList.end()) {
                s = symDict[varType]->totalSize;
            }
            if (token.val.find('[') != string::npos) {
                // ��������
                size_t start = token.val.find('[');
                size_t end = token.val.find(']');
                if (start != string::npos && end != string::npos && end > start) {
                    string numStr = token.val.substr(start + 1, end - start - 1);
                    int num = atoi(numStr.c_str());
                    s *= num;
                }
            }
            message = tmp->addVariable(token, varType, s);
        }

        return message;
    }

    Message checkDoDefineInFunction(const Token& token) {
        cout << "[DEBUG] SYMBOL::checkDoDefineInFunction - Checking: " << token.val << endl;

        if (functionList.empty()) {
            cout << "[ERROR] No current function scope" << endl;
            return Message("No function scope", "", "No current function scope");
        }

        Function* function = functionList.back();
        return function->checkDoDefine(token);
    }

    // �����޸ĺ��checkFunction����
    Message checkFunction(const vector<Token>& RES_TOKEN, int id) {
        cout << "[DEBUG] SYMBOL::checkFunction - Checking function call at id: " << id << endl;

        if (id >= static_cast<int>(RES_TOKEN.size())) {
            return Message("Invalid token id", "", "Invalid token id");
        }

        const Token& token = RES_TOKEN[id];
        string funcName = token.val;

        // ��麯���Ƿ���
        if (find(globalNameList.begin(), globalNameList.end(), funcName) == globalNameList.end()) {
            stringstream ss;
            ss << "Location:line " << token.cur_line;
            string location = ss.str();

            string errorMsg = "function '" + funcName + "' has no definition";
            cout << "[DEBUG] Function not defined: " << funcName << endl;
            return Message("Unknown identifier", location, errorMsg);
        }

        // ���Һ�������
        vector<string>::iterator it = find(functionNameList.begin(), functionNameList.end(), funcName);
        if (it == functionNameList.end()) {
            stringstream ss;
            ss << "Location:line " << token.cur_line;
            string location = ss.str();

            string errorMsg = "'" + funcName + "' is not a function";
            return Message("Not a function", location, errorMsg);
        }

        int funcIndex = distance(functionNameList.begin(), it);
        Function* func = functionList[funcIndex];

        // ����ʵ�ʲ�������
        int para_num = 0;
        int temp_id = id + 1; // ����������
        vector<Token> argTokens;
        vector<string> argTypes;

        // ȷ����һ��token�� '('
        if (temp_id >= static_cast<int>(RES_TOKEN.size()) || RES_TOKEN[temp_id].val != "(") {
            stringstream ss;
            ss << "Location:line " << token.cur_line;
            string location = ss.str();
            return Message("Syntax error", location, "expected '(' after function name");
        }
        temp_id++; // ���� '('

        // �ռ�����ֱ������ ')'
        while (temp_id < static_cast<int>(RES_TOKEN.size()) && RES_TOKEN[temp_id].val != ")") {
            if (RES_TOKEN[temp_id].val == ",") {
                temp_id++;
                continue;
            }

            para_num++;
            argTokens.push_back(RES_TOKEN[temp_id]);
            temp_id++;
        }

        // ����������
        if (func->numOfParameters != para_num) {
            stringstream ss;
            ss << "Location:line " << token.cur_line;
            string location = ss.str();

            string errorMsg = "function '" + funcName + "' expects " +
                toString(func->numOfParameters) + " arguments, but " +
                toString(para_num) + " were provided";
            return Message("Argument count mismatch", location, errorMsg);
        }

        // �������������ͼ��
        for (int i = 0; i < para_num; i++) {
            string expectedType = func->parameterTypes[i];
            string actualType = "unknown";
            const Token& argToken = argTokens[i];

            // ȷ��ʵ�ʲ�������
            if (argToken.type == "con") {
                // ������Ĭ��Ϊint����
                actualType = "int";
            }
            else if (argToken.type == "i") {
                // ��ʶ�������ұ�������
                actualType = getVariableType(argToken);
            }
            else if (argToken.type == "s") {
                // �ַ���������
                actualType = "string";
            }
            else if (argToken.type == "c") {
                // �ַ�������
                actualType = "char";
            }

            // ������ͼ�����
            if (!isTypeCompatible(expectedType, actualType)) {
                stringstream ss;
                ss << "Location:line " << argToken.cur_line;
                string location = ss.str();

                string errorMsg = "argument " + toString(i + 1) + " of function '" + funcName +
                    "' has incompatible type: expected '" + expectedType +
                    "', got '" + actualType + "'";
                return Message("Type mismatch", location, errorMsg);
            }
        }

        return Message();
    }

    void showTheInfo() {
        cout << "[DEBUG] SYMBOL::showTheInfo - Generating symbol table info" << endl;

        symbolTableInfo.clear();

        for (size_t i = 0; i < functionList.size(); ++i) {
            Function* fun = functionList[i];

            stringstream ss;
            ss << "FuncName:" << fun->functionName
                << " ReturnType:" << fun->returnType
                << " NumOfParameters:" << fun->numOfParameters
                << " Size:" << fun->totalSize;
            symbolTableInfo.push_back(ss.str());

            // ��������ʾ��������
            ss.str("");
            ss << "    ParameterTypes:";
            for (size_t j = 0; j < fun->parameterTypes.size(); ++j) {
                if (j > 0) ss << ", ";
                ss << fun->parameterTypes[j];
            }
            symbolTableInfo.push_back(ss.str());

            for (map<string, Variable>::iterator it = fun->variableDict.begin();
                it != fun->variableDict.end(); ++it) {
                const Variable& v = it->second;
                stringstream vss;
                vss << "    VariableName:" << v.name
                    << " Type:" << v.type
                    << " Addr:" << v.addr;
                symbolTableInfo.push_back(vss.str());
            }
        }

        for (size_t i = 0; i < structList.size(); ++i) {
            Struct* st = structList[i];

            stringstream ss;
            ss << "StructName:" << st->structName
                << " Size:" << st->totalSize;
            symbolTableInfo.push_back(ss.str());

            for (map<string, Variable>::iterator it = st->variableDict.begin();
                it != st->variableDict.end(); ++it) {
                const Variable& v = it->second;
                stringstream vss;
                vss << "    VariableName:" << v.name
                    << " Type:" << v.type
                    << " Addr:" << v.addr;
                symbolTableInfo.push_back(vss.str());
            }
        }

        cout << "[DEBUG] Symbol table info generated with "
            << symbolTableInfo.size() << " entries" << endl;
    }

private:
    // �������������� - ����ת�ַ���
    string toString(int value) {
        stringstream ss;
        ss << value;
        return ss.str();
    }
};

// ============================================================================
// Lexer class
// ============================================================================
class Lexer {
private:
    map<string, map<string, int> > DICT; // ģ��Python��Ƕ���ֵ�
    vector<string> INPUT;
    vector<Token> TokenList;
    int CUR_ROW;
    int CUR_LINE;
    int id;
    vector<string> k_list; // �ؼ����б�
    vector<string> p_list; // ����б�

    int incId() {
        id++;
        return id;
    }

    int getId(const string& demo, const string& typ) {
        if (DICT[typ].find(demo) == DICT[typ].end()) {
            DICT[typ][demo] = DICT[typ].size();
        }
        return DICT[typ][demo];
    }

public:
    Lexer() : CUR_ROW(-1), CUR_LINE(0), id(-1) {
        cout << "[DEBUG] Lexer constructor called" << endl;

        // ��ʼ���ֵ�
        DICT["k"] = map<string, int>();
        DICT["p"] = map<string, int>();
        DICT["con"] = map<string, int>();
        DICT["c"] = map<string, int>();
        DICT["s"] = map<string, int>();
        DICT["i"] = map<string, int>();

        // ��ȡ�ؼ����б�
        ifstream keywordFile("C://Users/��/Desktop/Project1/keyword_list.txt");
        if (keywordFile.is_open()) {
            string line;
            while (getline(keywordFile, line)) {
                if (!line.empty()) {
                    k_list.push_back(line);
                }
            }
            keywordFile.close();
            cout << "[DEBUG] Loaded " << k_list.size() << " keywords" << endl;
        }
        else {
            cout << "[WARNING] Could not open keyword_list.txt, using default keywords" << endl;
            // Ĭ�Ϲؼ���
            k_list.push_back("int");
            k_list.push_back("void");
            k_list.push_back("struct");
            k_list.push_back("return");
            k_list.push_back("if");
            k_list.push_back("else");
            k_list.push_back("while");
            k_list.push_back("for");
        }

        // ��ȡ����б�
        ifstream pFile("C://Users/��/Desktop/Project1/p_list.txt");
        if (pFile.is_open()) {
            string line;
            while (getline(pFile, line)) {
                if (!line.empty()) {
                    p_list.push_back(line);
                }
            }
            pFile.close();
            cout << "[DEBUG] Loaded " << p_list.size() << " delimiters" << endl;
        }
        else {
            cout << "[WARNING] Could not open p_list.txt, using default delimiters" << endl;
            // Ĭ�Ͻ��
            p_list.push_back("+");
            p_list.push_back("-");
            p_list.push_back("*");
            p_list.push_back("/");
            p_list.push_back("=");
            p_list.push_back("==");
            p_list.push_back("!=");
            p_list.push_back("<");
            p_list.push_back(">");
            p_list.push_back("<=");
            p_list.push_back(">=");
            p_list.push_back("(");
            p_list.push_back(")");
            p_list.push_back("{");
            p_list.push_back("}");
            p_list.push_back("[");
            p_list.push_back("]");
            p_list.push_back(";");
            p_list.push_back(",");
            p_list.push_back(".");
        }
    }

    void getInput(const vector<string>& input_list) {
        cout << "[DEBUG] Lexer::getInput - Processing " << input_list.size() << " lines" << endl;
        INPUT = input_list;
        CUR_ROW = -1;
        CUR_LINE = 0;
    }

    string getNextChar() {
        CUR_ROW++;
        if (CUR_LINE >= static_cast<int>(INPUT.size())) {
            return "END";
        }

        while (CUR_ROW >= static_cast<int>(INPUT[CUR_LINE].length())) {
            CUR_ROW = 0;
            CUR_LINE++;
            if (CUR_LINE >= static_cast<int>(INPUT.size())) {
                return "END";
            }
        }

        return string(1, INPUT[CUR_LINE][CUR_ROW]);
    }

    void backOneStep() {
        CUR_ROW--;
    }

    Token scanner() {
        string item = getNextChar();

        // �����հ��ַ�
        while (item == " " || item == "\t" || item == "\r" || item == "\n") {
            item = getNextChar();
        }

        if (item == "END") {
            return Token("END", 0, "END", CUR_LINE, incId());
        }

        string demo = "";
        string typ = "";
        int id_val = 0;

        if (isalpha(item[0]) || item == "_") {
            // ��ʶ����ؼ���
            demo = "";
            while (isalnum(item[0]) || item == "_" || item == "." || item == "[" || item == "]") {
                demo += item;
                if (CUR_ROW >= static_cast<int>(INPUT[CUR_LINE].length()) - 1) {
                    CUR_LINE++;
                    CUR_ROW = 0;
                    break;
                }
                else {
                    item = getNextChar();
                }
            }
            backOneStep();

            if (find(k_list.begin(), k_list.end(), demo) != k_list.end()) {
                id_val = getId(demo, "k");
                typ = "k";
            }
            else {
                id_val = getId(demo, "i");
                typ = "i";
            }
        }
        else if (isdigit(item[0])) {
            // ���ֳ���
            demo = "";
            while (isdigit(item[0]) || item == ".") {
                demo += item;
                if (CUR_ROW >= static_cast<int>(INPUT[CUR_LINE].length()) - 1) {
                    CUR_LINE++;
                    CUR_ROW = 0;
                    break;
                }
                else {
                    item = getNextChar();
                }
            }
            backOneStep();
            id_val = getId(demo, "con");
            typ = "con";
        }
        else if (item == "\"") {
            // �ַ���
            demo = "\"";
            item = getNextChar();
            demo += item;
            while (item != "\"") {
                item = getNextChar();
                demo += item;
            }
            id_val = getId(demo, "s");
            typ = "s";
        }
        else if (item == "'") {
            // �ַ�
            demo = "'";
            for (int i = 0; i < 2; i++) {
                item = getNextChar();
                demo += item;
            }
            id_val = getId(demo, "c");
            typ = "c";
        }
        else {
            // ���
            string item_next = getNextChar();
            string two_char = item + item_next;

            if (find(p_list.begin(), p_list.end(), two_char) != p_list.end()) {
                demo = two_char;
                id_val = getId(demo, "p");
            }
            else if (find(p_list.begin(), p_list.end(), item) != p_list.end()) {
                demo = item;
                id_val = getId(demo, "p");
                backOneStep();
            }
            typ = "p";
        }

        cout << "[DEBUG] Lexer::scanner - Token: " << demo << ", Type: " << typ << endl;
        return Token(typ, id_val, demo, CUR_LINE, incId());
    }

    vector<Token> analyse() {
        cout << "[DEBUG] Lexer::analyse - Starting lexical analysis" << endl;

        TokenList.clear();
        while (true) {
            Token tmp = scanner();
            if (tmp.type == "END") {
                break;
            }
            if (!tmp.val.empty()) {
                TokenList.push_back(tmp);
            }
        }

        cout << "[DEBUG] Lexer::analyse - Generated " << TokenList.size() << " tokens" << endl;
        return TokenList;
    }
};

// ============================================================================
// GrammarParser class
// ============================================================================
class GrammarParser {
protected:
    map<string, vector<vector<string> > > GRAMMAR_DICT;
    vector<pair<string, vector<string> > > P_LIST;
    vector<set<string> > FIRST;
    map<string, set<string> > FIRST_VT;
    map<string, set<string> > FOLLOW;
    set<string> VN; // ���ս��
    set<string> VT; // �ս��
    string Z; // ��ʼ����
    vector<set<string> > SELECT;
    map<string, map<string, int> > analysis_table;

public:
    GrammarParser(const string& path = "") {
        cout << "[DEBUG] GrammarParser constructor called with path: " << path << endl;

        if (!path.empty()) {
            loadGrammar(path);
        }
    }

    void initList() {
        cout << "[DEBUG] GrammarParser::initList - Computing FIRST, FOLLOW, SELECT sets" << endl;

        calFirstvt();
        calFirst();
        calFollow();
        calSelect();
        calAnalysisTable();

        // ����������ļ�
        string tableName = getAnalysisTableName();
        saveAnalysisTable(tableName);
    }

protected:
    virtual string getAnalysisTableName() {
        return "AnalysisTable.txt";
    }

    void loadGrammar(const string& path) {
        cout << "[DEBUG] Loading grammar from: " << path << endl;

        ifstream grammarFile(path.c_str());
        if (!grammarFile.is_open()) {
            cout << "[ERROR] Could not open grammar file: " << path << endl;
            return;
        }

        string line;
        bool first = true;

        // �����������
        GRAMMAR_DICT.clear();
        P_LIST.clear();
        VN.clear();
        VT.clear();

        while (getline(grammarFile, line)) {
            if (line.empty()) continue;

            size_t arrow_pos = line.find("->");
            if (arrow_pos == string::npos) continue;

            string vn = line.substr(0, arrow_pos);
            string right = line.substr(arrow_pos + 2);

            if (first) {
                Z = vn;
                first = false;
            }

            // �ָ��Ҳ�
            vector<string> tmp;
            stringstream ss(right);
            string token;
            while (ss >> token) {
                tmp.push_back(token);
            }

            P_LIST.push_back(make_pair(vn, tmp));
            GRAMMAR_DICT[vn].push_back(tmp);
            VN.insert(vn);
        }

        grammarFile.close();

        // ��ȡ�ս��
        for (size_t i = 0; i < P_LIST.size(); ++i) {
            const vector<string>& right = P_LIST[i].second;
            for (size_t j = 0; j < right.size(); ++j) {
                if (VN.find(right[j]) == VN.end() && right[j] != "$" &&
                    (right[j].empty() || right[j][0] != '@')) {  // �ų����嶯��
                    VT.insert(right[j]);
                }
            }
        }

        cout << "[DEBUG] Grammar loaded: " << VN.size() << " non-terminals, "
            << VT.size() << " terminals, " << P_LIST.size() << " productions" << endl;
    }

    void calFirstvt() {
        cout << "[DEBUG] Computing FIRST_VT sets" << endl;

        // ��ʼ��
        for (set<string>::iterator it = VN.begin(); it != VN.end(); ++it) {
            FIRST_VT[*it] = set<string>();
        }

        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t i = 0; i < P_LIST.size(); ++i) {
                const string& vn = P_LIST[i].first;
                const vector<string>& right = P_LIST[i].second;

                for (size_t j = 0; j < right.size(); ++j) {
                    const string& symbol = right[j];

                    // �������嶯��
                    if (!symbol.empty() && symbol[0] == '@') continue;

                    if (VT.find(symbol) != VT.end()) {
                        if (FIRST_VT[vn].find(symbol) == FIRST_VT[vn].end()) {
                            FIRST_VT[vn].insert(symbol);
                            changed = true;
                        }
                        break;
                    }
                    else if (VN.find(symbol) != VN.end()) {
                        size_t oldSize = FIRST_VT[vn].size();
                        for (set<string>::iterator it = FIRST_VT[symbol].begin();
                            it != FIRST_VT[symbol].end(); ++it) {
                            FIRST_VT[vn].insert(*it);
                        }
                        if (FIRST_VT[vn].size() > oldSize) {
                            changed = true;
                        }

                        // ����Ƿ��пղ���ʽ
                        bool hasEmpty = false;
                        for (size_t k = 0; k < GRAMMAR_DICT[symbol].size(); ++k) {
                            if (GRAMMAR_DICT[symbol][k].size() == 1 &&
                                GRAMMAR_DICT[symbol][k][0] == "$") {
                                hasEmpty = true;
                                break;
                            }
                        }
                        if (!hasEmpty) break;
                    }
                }
            }
        }
    }

    void calFirst() {
        cout << "[DEBUG] Computing FIRST sets for productions" << endl;

        for (size_t i = 0; i < P_LIST.size(); ++i) {
            const vector<string>& right = P_LIST[i].second;
            set<string> demo;

            for (size_t j = 0; j < right.size(); ++j) {
                const string& symbol = right[j];

                // �������嶯��
                if (!symbol.empty() && symbol[0] == '@') continue;

                if (VT.find(symbol) != VT.end()) {
                    demo.insert(symbol);
                    break;
                }
                else if (VN.find(symbol) != VN.end()) {
                    for (set<string>::iterator it = FIRST_VT[symbol].begin();
                        it != FIRST_VT[symbol].end(); ++it) {
                        demo.insert(*it);
                    }

                    // ����Ƿ��пղ���ʽ
                    bool hasEmpty = false;
                    for (size_t k = 0; k < GRAMMAR_DICT[symbol].size(); ++k) {
                        if (GRAMMAR_DICT[symbol][k].size() == 1 &&
                            GRAMMAR_DICT[symbol][k][0] == "$") {
                            hasEmpty = true;
                            break;
                        }
                    }
                    if (!hasEmpty) break;
                }
            }

            FIRST.push_back(demo);
        }
    }

    void calFollow() {
        cout << "[DEBUG] Computing FOLLOW sets" << endl;

        // ��ʼ��
        for (set<string>::iterator it = VN.begin(); it != VN.end(); ++it) {
            FOLLOW[*it] = set<string>();
        }

        // ��ʼ���ŵ�FOLLOW������#
        FOLLOW[Z].insert("#");

        bool changed = true;
        while (changed) {
            changed = false;

            for (size_t i = 0; i < P_LIST.size(); ++i) {
                const string& left = P_LIST[i].first;
                const vector<string>& right = P_LIST[i].second;

                for (size_t j = 0; j < right.size(); ++j) {
                    const string& symbol = right[j];

                    // �������嶯��
                    if (!symbol.empty() && symbol[0] == '@') continue;

                    if (VN.find(symbol) != VN.end()) {
                        // ���ڷ��ս��
                        bool shouldAddFollow = true;

                        for (size_t k = j + 1; k < right.size(); ++k) {
                            const string& nextSymbol = right[k];

                            // �������嶯��
                            if (!nextSymbol.empty() && nextSymbol[0] == '@') continue;

                            if (VT.find(nextSymbol) != VT.end()) {
                                if (FOLLOW[symbol].find(nextSymbol) == FOLLOW[symbol].end()) {
                                    FOLLOW[symbol].insert(nextSymbol);
                                    changed = true;
                                }
                                shouldAddFollow = false;
                                break;
                            }
                            else if (VN.find(nextSymbol) != VN.end()) {
                                size_t oldSize = FOLLOW[symbol].size();
                                for (set<string>::iterator it = FIRST_VT[nextSymbol].begin();
                                    it != FIRST_VT[nextSymbol].end(); ++it) {
                                    FOLLOW[symbol].insert(*it);
                                }
                                if (FOLLOW[symbol].size() > oldSize) {
                                    changed = true;
                                }

                                // ����Ƿ��пղ���ʽ
                                bool hasEmpty = false;
                                for (size_t l = 0; l < GRAMMAR_DICT[nextSymbol].size(); ++l) {
                                    if (GRAMMAR_DICT[nextSymbol][l].size() == 1 &&
                                        GRAMMAR_DICT[nextSymbol][l][0] == "$") {
                                        hasEmpty = true;
                                        break;
                                    }
                                }
                                if (!hasEmpty) {
                                    shouldAddFollow = false;
                                    break;
                                }
                            }
                        }

                        if (shouldAddFollow) {
                            size_t oldSize = FOLLOW[symbol].size();
                            for (set<string>::iterator it = FOLLOW[left].begin();
                                it != FOLLOW[left].end(); ++it) {
                                FOLLOW[symbol].insert(*it);
                            }
                            if (FOLLOW[symbol].size() > oldSize) {
                                changed = true;
                            }
                        }
                    }
                }
            }
        }
    }

    void calSelect() {
        cout << "[DEBUG] Computing SELECT sets" << endl;

        for (size_t i = 0; i < P_LIST.size(); ++i) {
            if (!FIRST[i].empty()) {
                SELECT.push_back(FIRST[i]);
            }
            else {
                SELECT.push_back(FOLLOW[P_LIST[i].first]);
            }
        }
    }

    void calAnalysisTable() {
        cout << "[DEBUG] Computing LL(1) analysis table" << endl;

        // ��ʼ��������
        for (set<string>::iterator vn_it = VN.begin(); vn_it != VN.end(); ++vn_it) {
            for (set<string>::iterator vt_it = VT.begin(); vt_it != VT.end(); ++vt_it) {
                analysis_table[*vn_it][*vt_it] = -1;
            }
            analysis_table[*vn_it]["#"] = -1;
        }

        // ��������
        for (size_t i = 0; i < P_LIST.size(); ++i) {
            const string& left = P_LIST[i].first;
            for (set<string>::iterator it = SELECT[i].begin(); it != SELECT[i].end(); ++it) {
                analysis_table[left][*it] = static_cast<int>(i);
            }
        }
    }

    void saveAnalysisTable(const string& filename) {
        cout << "[DEBUG] Saving analysis table to " << filename << endl;

        ofstream file(filename.c_str());
        if (!file.is_open()) {
            cout << "[ERROR] Could not save analysis table" << endl;
            return;
        }

        // ����VN
        file << "VN:";
        for (set<string>::iterator it = VN.begin(); it != VN.end(); ++it) {
            file << *it << " ";
        }
        file << endl;

        // ����VT
        file << "VT:";
        for (set<string>::iterator it = VT.begin(); it != VT.end(); ++it) {
            file << *it << " ";
        }
        file << "# " << endl;

        // ���������
        for (set<string>::iterator vn_it = VN.begin(); vn_it != VN.end(); ++vn_it) {
            file << *vn_it << ":";
            for (set<string>::iterator vt_it = VT.begin(); vt_it != VT.end(); ++vt_it) {
                file << analysis_table[*vn_it][*vt_it] << " ";
            }
            file << analysis_table[*vn_it]["#"] << endl;
        }

        file.close();
    }

    void loadAnalysisTable(const string& filename) {
        cout << "[DEBUG] Loading analysis table from " << filename << endl;

        ifstream file(filename.c_str());
        if (!file.is_open()) {
            cout << "[WARNING] Could not load analysis table, will compute it" << endl;
            initList();
            return;
        }

        string line;

        // ��ȡVN
        if (getline(file, line) && line.substr(0, 3) == "VN:") {
            stringstream ss(line.substr(3));
            string vn;
            VN.clear();
            while (ss >> vn) {
                VN.insert(vn);
            }
        }

        // ��ȡVT
        if (getline(file, line) && line.substr(0, 3) == "VT:") {
            stringstream ss(line.substr(3));
            string vt;
            VT.clear();
            while (ss >> vt) {
                VT.insert(vt);
            }
        }

        // ��ȡ������
        while (getline(file, line)) {
            size_t colon_pos = line.find(':');
            if (colon_pos == string::npos) continue;

            string vn = line.substr(0, colon_pos);
            stringstream ss(line.substr(colon_pos + 1));

            analysis_table[vn] = map<string, int>();

            int value;
            set<string>::iterator vt_it = VT.begin();
            while (ss >> value && vt_it != VT.end()) {
                analysis_table[vn][*vt_it] = value;
                ++vt_it;
            }
            if (ss >> value) {
                analysis_table[vn]["#"] = value;
            }
        }

        file.close();
    }
};

// ============================================================================
// LL1 class (LL1 Parser)
// ============================================================================
class LL1 : public GrammarParser {
private:
    Lexer lex;
    vector<Token> RES_TOKEN;
    SYMBOL syn_table;
    vector<vector<Token> > funcBlocks;

protected:
    virtual string getAnalysisTableName() {
        return "SyntaxAnalysisTable.txt";
    }

public:
    // ��ȡ���ű�ָ��
    SYMBOL* getSymbolTable() {
        return &syn_table;
    }
    // ��ȡ�������б�
    vector<vector<Token> > getFuncBlocks() {
        return funcBlocks;
    }

    LL1(const string& path, bool doseIniList = false) : GrammarParser(path) {
        cout << "[DEBUG] LL1 constructor called" << endl;

        if (doseIniList) {
            initList();
        }
        else {
            loadAnalysisTable(getAnalysisTableName());
        }
    }

    void getInput(const vector<string>& INPUT) {
        cout << "[DEBUG] LL1::getInput - Processing input" << endl;
        lex.getInput(INPUT);
        RES_TOKEN = lex.analyse();

        cout << "[DEBUG] Generated tokens:" << endl;
        for (size_t i = 0; i < RES_TOKEN.size(); ++i) {
            cout << "  [" << i << "] " << RES_TOKEN[i].val
                << " (type: " << RES_TOKEN[i].type << ")" << endl;
        }
    }

    Message analyzeInputString() {
        cout << "[DEBUG] LL1::analyzeInputString - Starting syntax analysis" << endl;

        vector<string> stack;
        stack.push_back("#");
        stack.push_back(Z);

        vector<Token> TokenList = RES_TOKEN;
        if (TokenList.empty()) {
            return Message("Empty input", "", "Input is empty");
        }

        Token token = TokenList[0];
        TokenList.erase(TokenList.begin());

        vector<Token> funcBlock;
        funcBlock.push_back(token);

        string w = getTokenVal(token);

        while (!stack.empty()) {
            string x = stack.back();
            stack.pop_back();

            cout << "[DEBUG] Stack top: " << x << ", Current token: " << w << endl;

            if (x != w) {
                if (VN.find(x) == VN.end()) {
                    // x���ս������ƥ��
                    stringstream ss;
                    ss << "Location:line " << token.cur_line;
                    string location = ss.str();

                    string errorMsg = "expect '" + x + "' before '" + w + "'";
                    return Message("Identifier Expected", location, errorMsg);
                }

                // x�Ƿ��ս�����������
                if (analysis_table.find(x) == analysis_table.end() ||
                    analysis_table[x].find(w) == analysis_table[x].end()) {
                    return Message("Analysis table error", "", "Invalid entry in analysis table");
                }

                int id = analysis_table[x][w];
                if (id == -1) {
                    // ������
                    vector<string> keys;
                    for (map<string, int>::iterator it = analysis_table[x].begin();
                        it != analysis_table[x].end(); ++it) {
                        if (it->second != -1 && it->first != "#") {
                            keys.push_back(it->first);
                        }
                    }

                    stringstream ss;
                    ss << "Location:line " << token.cur_line;
                    string location = ss.str();

                    string errorMsg = "error:expect tokens after '" +
                        (token.id > 0 ? RES_TOKEN[token.id - 1].val : "") + "' token";
                    return Message("syntax error", location, errorMsg);
                }

                // ��ȡ����ʽ�Ҳ�
                const vector<string>& tmp = P_LIST[id].second;
                if (tmp.size() != 1 || tmp[0] != "$") {
                    // ����ѹջ
                    for (int i = static_cast<int>(tmp.size()) - 1; i >= 0; --i) {
                        stack.push_back(tmp[i]);
                    }

                    // ���ű���
                    Message message = editSymTable(x, w, token);
                    if (message.hasError()) {
                        return message;
                    }
                }
            }
            else {
                // ƥ��ɹ�
                if (w == "#") {
                    funcBlocks.push_back(funcBlock);
                    syn_table.showTheInfo();
                    return Message(); // �����ɹ�
                }

                try {
                    if (!TokenList.empty()) {
                        token = TokenList[0];
                        TokenList.erase(TokenList.begin());

                        if (!stack.empty() && stack.back() == "Funcs") {
                            funcBlocks.push_back(funcBlock);
                            funcBlock.clear();
                        }

                        w = getTokenVal(token);
                        funcBlock.push_back(token);
                    }
                    else {
                        w = "#";
                    }
                }
                catch (...) {
                    w = "#";
                }
            }
        }

        return Message("Analysis incomplete", "", "Analysis did not complete successfully");
    }

private:
    string getTokenVal(const Token& token) {
        if (find(syn_table.structNameList.begin(), syn_table.structNameList.end(), token.val)
            != syn_table.structNameList.end()) {
            return "st"; // �ṹ������
        }
        if (token.type == "con") {
            return "NUM";
        }
        if (token.type == "i") {
            return "ID";
        }
        return token.val;
    }

    Message editSymTable(const string& x, const string& w, const Token& token) {
        cout << "[DEBUG] LL1::editSymTable - Processing: " << x << " -> " << w << endl;

        if (x == "Funcs") {
            // ��������
            if (static_cast<int>(token.id + 1) < static_cast<int>(RES_TOKEN.size())) {
                return syn_table.addFunction(RES_TOKEN[token.id + 1], token.val);
            }
        }

        if (x == "Struct") {
            // �ṹ�嶨��
            if (static_cast<int>(token.id + 1) < static_cast<int>(RES_TOKEN.size())) {
                return syn_table.addStruct(RES_TOKEN[token.id + 1]);
            }
        }

        if (x.find("FormalParameters") == 0) {
            // ������������
            if (token.val != ",") {
                // ��������-������
                if (static_cast<int>(token.id + 1) < static_cast<int>(RES_TOKEN.size())) {
                    return syn_table.addVariableToTable(RES_TOKEN[token.id + 1], token.val, true);
                }
            }
            else {
                // ���ź�Ӧ�����µ���������
                if (static_cast<int>(token.id + 2) < static_cast<int>(RES_TOKEN.size())) {
                    Token typeToken = RES_TOKEN[token.id + 1];
                    Token varToken = RES_TOKEN[token.id + 2];

                    // ȷ������token����Ч�����ͱ�ʶ��
                    if (typeToken.type == "k" ||
                        find(syn_table.structNameList.begin(),
                            syn_table.structNameList.end(),
                            typeToken.val) != syn_table.structNameList.end()) {
                        return syn_table.addVariableToTable(varToken, typeToken.val, true);
                    }
                    else {
                        stringstream ss;
                        ss << "Location:line " << typeToken.cur_line;
                        string location = ss.str();
                        string errorMsg = "invalid type '" + typeToken.val + "' in parameter list";
                        return Message("Invalid type", location, errorMsg);
                    }
                }
            }
        }

        if (x == "LocalVarDefine") {
            // �ֲ���������
            if (static_cast<int>(token.id + 1) < static_cast<int>(RES_TOKEN.size())) {
                return syn_table.addVariableToTable(RES_TOKEN[token.id + 1], token.val);
            }
        }

        if (x == "NormalStatement" || (x == "F" && w == "ID")) {
            // ����ʹ�ü��
            if (token.val.find('.') == string::npos && token.val.find('[') == string::npos) {
                return syn_table.checkDoDefineInFunction(token);
            }
        }

        // �����ϸ�ֵ������
        if (x == "CompoundAssignOp") {
            // ���ڸ��ϸ�ֵ����������Ҫ��������ǰ��ı����Ƿ��Ѷ���
            if (token.id > 0) {
                Token varToken = RES_TOKEN[token.id - 1];
                if (varToken.type == "i") {
                    return syn_table.checkDoDefineInFunction(varToken);
                }
            }
        }

        // �����׺������
        if (x == "PostfixOp") {
            // ���ں�׺++/--����Ҫ��������ǰ��ı����Ƿ��Ѷ���
            if (token.id > 0) {
                Token varToken = RES_TOKEN[token.id - 1];
                if (varToken.type == "i") {
                    return syn_table.checkDoDefineInFunction(varToken);
                }
            }
        }

        // ����ǰ׺������
        if (x == "UnaryOp") {
            // ����ǰ׺++/--����Ҫ������������ı����Ƿ��Ѷ���
            if (static_cast<int>(token.id + 1) < static_cast<int>(RES_TOKEN.size())) {
                Token varToken = RES_TOKEN[token.id + 1];
                if (varToken.type == "i") {
                    return syn_table.checkDoDefineInFunction(varToken);
                }
            }
        }

        if (x == "FuncCallFollow") {
            // �������ü��
            int id = token.id;
            if (token.val == "=") {
                id += 1;
            }
            else {
                id -= 1;
            }
            return syn_table.checkFunction(RES_TOKEN, id);
        }

        return Message();
    }

public:
    void printSymbolTable() {
        syn_table.showTheInfo();
        cout << "\n[DEBUG] Symbol Table Information:" << endl;
        for (size_t i = 0; i < syn_table.symbolTableInfo.size(); ++i) {
            cout << syn_table.symbolTableInfo[i] << endl;
        }
    }
};

// ============================================================================
// Quaternion struct (��Ԫʽ�ṹ)
// ============================================================================
struct Quaternion {
    string op;
    string arg1;
    string arg2;
    string result;

    Quaternion(const string& o, const string& a1, const string& a2, const string& r)
        : op(o), arg1(a1), arg2(a2), result(r) {}
};
// ============================================================================
// QtGen class (��ȫ������Ƶ���Ԫʽ������)
// ============================================================================
class QtGen : public GrammarParser {
private:
    SYMBOL* syn_table;
    int t_id;
    vector<vector<Quaternion> > qt_res;

protected:
    virtual string getAnalysisTableName() {
        return "TranslationAnalysisTable.txt";
    }

public:
    QtGen(SYMBOL* syn, const string& grammarPath = "")
        : GrammarParser(grammarPath), syn_table(syn), t_id(0) {
        cout << "[DEBUG] QtGen constructor called" << endl;

        // ���û���ṩ�ķ�·����ʹ��Ĭ�ϵķ����ķ�
        if (grammarPath.empty()) {
            loadEmbeddedTranslationGrammar();
        }

        // ����������
        initList();
    }

    // ��ȡ��Ԫʽ���
    vector<vector<Quaternion> > getQtRes() const {
        return qt_res;
    }

    // ������Ԫʽ
    void genQt(const vector<Token>& funcBlock) {
        cout << "[DEBUG] QtGen::genQt - Generating quaternions for function block" << endl;

        vector<Quaternion> qtList;
        vector<string> SEM_STACK;     // ����ջ
        vector<string> SYMBOL_STACK;  // �������ջ
        vector<string> SYN;           // �﷨ջ

        // ������tokenջ�����ڸ���ƥ���tokens
        vector<Token> MATCHED_TOKENS;

        SYN.push_back("#");
        SYN.push_back(Z);

        // ����token�б���
        vector<Token> TokenList = funcBlock;
        Token endToken("p", 0, "#", 0, static_cast<int>(funcBlock.size()));
        TokenList.push_back(endToken);

        t_id = 0;

        if (TokenList.empty()) {
            cout << "[WARNING] Empty token list" << endl;
            return;
        }

        size_t tokenIndex = 0;
        Token token = TokenList[tokenIndex++];
        string w = getTokenVal(token);

        while (!SYN.empty()) {
            string x = SYN.back();
            SYN.pop_back();

            cout << "[DEBUG] Stack top: " << x << ", Current token: " << w << " (val: " << token.val << ")" << endl;

            // �������嶯��
            if (!x.empty() && x[0] == '@') {
                catchAction(x, MATCHED_TOKENS, SYMBOL_STACK, SEM_STACK, qtList);
                continue;
            }

            if (x != w) {
                // x�Ƿ��ս�������Ҳ���ʽ
                if (VN.find(x) != VN.end()) {
                    if (analysis_table.find(x) != analysis_table.end() &&
                        analysis_table[x].find(w) != analysis_table[x].end()) {

                        int id = analysis_table[x][w];
                        if (id >= 0 && id < static_cast<int>(P_LIST.size())) {
                            const vector<string>& tmp = P_LIST[id].second;

                            cout << "[DEBUG] Using production " << id << ": " << x << " -> ";
                            for (size_t i = 0; i < tmp.size(); ++i) {
                                cout << tmp[i] << " ";
                            }
                            cout << endl;

                            if (!(tmp.size() == 1 && tmp[0] == "$")) {
                                // ����ѹջ
                                for (int i = static_cast<int>(tmp.size()) - 1; i >= 0; --i) {
                                    SYN.push_back(tmp[i]);
                                }
                            }
                        }
                        else {
                            cout << "[ERROR] Invalid production id: " << id << endl;
                        }
                    }
                    else {
                        cout << "[ERROR] No entry in analysis table for " << x << " with " << w << endl;
                    }
                }
                else {
                    cout << "[ERROR] Expected " << x << " but got " << w << endl;
                }
            }
            else {
                // ƥ��ɹ�
                cout << "[DEBUG] Matched: " << x << " with token: " << token.val << endl;

                // ��ƥ���token��ӵ�ջ��
                MATCHED_TOKENS.push_back(token);

                if (w == "#") {
                    cout << "[DEBUG] Analysis completed" << endl;
                    break;
                }

                if (tokenIndex < TokenList.size()) {
                    token = TokenList[tokenIndex++];
                    w = getTokenVal(token);
                }
                else {
                    w = "#";
                }
            }
        }

        qt_res.push_back(qtList);
        cout << "[DEBUG] Generated " << qtList.size() << " quaternions" << endl;
    }

    // ��ӡ��Ԫʽ
    void printQuaternions() {
        cout << "\n=== Quaternions ===" << endl;
        for (size_t i = 0; i < qt_res.size(); ++i) {
            cout << "Function " << i << ":" << endl;
            for (size_t j = 0; j < qt_res[i].size(); ++j) {
                const Quaternion& q = qt_res[i][j];
                cout << "  [" << j << "] (" << q.op << ", "
                    << q.arg1 << ", " << q.arg2 << ", "
                    << q.result << ")" << endl;
            }
        }
    }

private:
    // ��ȡToken��ֵ�������﷨������
    string getTokenVal(const Token& token) {
        // ����Ƿ��ǽṹ����
        if (find(syn_table->structNameList.begin(),
            syn_table->structNameList.end(),
            token.val) != syn_table->structNameList.end()) {
            return "st";
        }

        if (token.type == "con") {
            return "NUM";
        }

        if (token.type == "i") {
            return "ID";
        }

        // ���⴦��CALL
        if (token.val == "CALL") {
            return "CALL";
        }

        return token.val;
    }

    // ������Ƶ����嶯������
    void catchAction(const string& x, vector<Token>& MATCHED_TOKENS,
        vector<string>& SYMBOL_STACK,
        vector<string>& SEM_STACK,
        vector<Quaternion>& qtList) {

        cout << "[DEBUG] Semantic action: " << x << endl;

        // ���� @PUSH_VAL
        if (x == "@PUSH_VAL") {
            // ��ƥ���tokenջ�л�ȡ���ƥ���token
            if (!MATCHED_TOKENS.empty()) {
                Token lastMatched = MATCHED_TOKENS.back();
                SEM_STACK.push_back(lastMatched.val);
                cout << "[DEBUG] Pushed to SEM_STACK: " << lastMatched.val << endl;
            }
            return;
        }

        // ���� @PUSH_ID - �������ƥ���ID token
        if (x == "@PUSH_ID") {
            // �Ӻ���ǰ�������ID token
            for (int i = static_cast<int>(MATCHED_TOKENS.size()) - 1; i >= 0; --i) {
                if (MATCHED_TOKENS[i].type == "i") {
                    SEM_STACK.push_back(MATCHED_TOKENS[i].val);
                    cout << "[DEBUG] Pushed ID to SEM_STACK: " << MATCHED_TOKENS[i].val << endl;
                    break;
                }
            }
            return;
        }

        // ���� @PUSH_NUM - �������ƥ���NUM token
        if (x == "@PUSH_NUM") {
            // �Ӻ���ǰ�������NUM token
            for (int i = static_cast<int>(MATCHED_TOKENS.size()) - 1; i >= 0; --i) {
                if (MATCHED_TOKENS[i].type == "con") {
                    SEM_STACK.push_back(MATCHED_TOKENS[i].val);
                    cout << "[DEBUG] Pushed NUM to SEM_STACK: " << MATCHED_TOKENS[i].val << endl;
                    break;
                }
            }
            return;
        }

        // ���� @SAVE_X ��ʽ
        if (x.find("@SAVE_") == 0) {
            string symbol = x.substr(6);  // ��ȡ @SAVE_ ����ķ���
            SYMBOL_STACK.push_back(symbol);
            cout << "[DEBUG] Pushed to SYMBOL_STACK: " << symbol << endl;
            return;
        }

        // ���� @GEQ_X ��ʽ
        if (x.find("@GEQ_") == 0) {
            string action = x.substr(5);  // ��ȡ @GEQ_ ����Ķ���

            if (action == "G") {
                // ͨ����Ԫʽ����
                if (!SYMBOL_STACK.empty()) {
                    string op = SYMBOL_STACK.back();
                    SYMBOL_STACK.pop_back();

                    cout << "[DEBUG] Processing @GEQ_G with operator: " << op << endl;
                    cout << "[DEBUG] SEM_STACK size: " << SEM_STACK.size() << ", content: ";
                    for (size_t i = 0; i < SEM_STACK.size(); ++i) {
                        cout << SEM_STACK[i] << " ";
                    }
                    cout << endl;

                    if (op == "FUN") {
                        // ��������
                        if (!SEM_STACK.empty()) {
                            string funcName = SEM_STACK.back();
                            SEM_STACK.pop_back();
                            qtList.push_back(Quaternion("FUN", funcName, "_", "_"));
                        }
                    }
                    else if (op == "=") {
                        // ��ֵ����
                        if (SEM_STACK.size() >= 2) {
                            string rvalue = SEM_STACK.back(); SEM_STACK.pop_back();
                            string lvalue = SEM_STACK.back(); SEM_STACK.pop_back();
                            qtList.push_back(Quaternion("=", rvalue, "_", lvalue));
                        }
                    }
                    else if (op == "+=" || op == "-=" || op == "*=" || op == "/=") {
                        // ���ϸ�ֵ
                        if (SEM_STACK.size() >= 2) {
                            string rvalue = SEM_STACK.back(); SEM_STACK.pop_back();
                            string lvalue = SEM_STACK.back(); SEM_STACK.pop_back();

                            // �ȼ���
                            string binOp = op.substr(0, 1);
                            stringstream ss;
                            ss << "@t" << t_id++;
                            string temp = ss.str();
                            qtList.push_back(Quaternion(binOp, lvalue, rvalue, temp));

                            // �ٸ�ֵ
                            qtList.push_back(Quaternion("=", temp, "_", lvalue));
                        }
                    }
                    else if (op == "++" || op == "--") {
                        // ǰ׺�����Լ�
                        if (!SEM_STACK.empty()) {
                            string var = SEM_STACK.back(); SEM_STACK.pop_back();

                            stringstream ss;
                            ss << "@t" << t_id++;
                            string temp = ss.str();

                            string binOp = (op == "++") ? "+" : "-";
                            qtList.push_back(Quaternion(binOp, var, "1", temp));
                            qtList.push_back(Quaternion("=", temp, "_", var));
                        }
                    }
                    else if (op == "p++" || op == "p--") {
                        // ��׺�����Լ�
                        if (!SEM_STACK.empty()) {
                            string var = SEM_STACK.back(); SEM_STACK.pop_back();

                            stringstream ss;
                            ss << "@t" << t_id++;
                            string temp = ss.str();

                            // �ȱ���ԭֵ
                            qtList.push_back(Quaternion("=", var, "_", temp));

                            // ������/�Լ�
                            string binOp = (op == "p++") ? "+" : "-";
                            stringstream ss2;
                            ss2 << "@t" << t_id++;
                            string temp2 = ss2.str();

                            qtList.push_back(Quaternion(binOp, var, "1", temp2));
                            qtList.push_back(Quaternion("=", temp2, "_", var));

                            // ����ԭֵ
                            SEM_STACK.push_back(temp);
                        }
                    }
                    else if (op == "+" || op == "-" || op == "*" || op == "/" ||
                        op == ">" || op == "<" || op == ">=" || op == "<=" ||
                        op == "==" || op == "!=") {
                        // ��Ԫ����
                        if (SEM_STACK.size() >= 2) {
                            string arg2 = SEM_STACK.back(); SEM_STACK.pop_back();
                            string arg1 = SEM_STACK.back(); SEM_STACK.pop_back();

                            stringstream ss;
                            ss << "@t" << t_id++;
                            string temp = ss.str();

                            SEM_STACK.push_back(temp);
                            qtList.push_back(Quaternion(op, arg1, arg2, temp));
                        }
                    }
                    else if (op == "return") {
                        // return���
                        if (!SEM_STACK.empty()) {
                            string retVal = SEM_STACK.back(); SEM_STACK.pop_back();
                            qtList.push_back(Quaternion("return", retVal, "_", "_"));
                        }
                    }
                    else if (op == "if" || op == "elif") {
                        // �������
                        if (!SEM_STACK.empty()) {
                            string condition = SEM_STACK.back(); SEM_STACK.pop_back();
                            qtList.push_back(Quaternion(op, condition, "_", "_"));
                        }
                    }
                    else if (op == "do") {
                        // whileѭ������
                        if (!SEM_STACK.empty()) {
                            string condition = SEM_STACK.back(); SEM_STACK.pop_back();
                            qtList.push_back(Quaternion("do", condition, "_", "_"));
                        }
                    }
                    else if (op == "push") {
                        // ��������
                        if (!SEM_STACK.empty()) {
                            string arg = SEM_STACK.back(); SEM_STACK.pop_back();
                            qtList.push_back(Quaternion("push", arg, "_", "_"));
                        }
                    }
                }
            }
            else if (action == "break") {
                qtList.push_back(Quaternion("break", "_", "_", "_"));
            }
            else if (action == "continue") {
                qtList.push_back(Quaternion("continue", "_", "_", "_"));
            }
            else if (action == "el") {
                qtList.push_back(Quaternion("el", "_", "_", "_"));
            }
            else if (action == "ie") {
                qtList.push_back(Quaternion("ie", "_", "_", "_"));
            }
            else if (action == "wh") {
                qtList.push_back(Quaternion("wh", "_", "_", "_"));
            }
            else if (action == "we") {
                qtList.push_back(Quaternion("we", "_", "_", "_"));
            }
            else if (action == "c1") {
                // �з���ֵ�ĺ�������
                if (SEM_STACK.size() >= 2) {
                    string funcName = SEM_STACK.back(); SEM_STACK.pop_back();
                    string retVar = SEM_STACK.back(); SEM_STACK.pop_back();
                    qtList.push_back(Quaternion("callr", funcName, "_", retVar));
                }
            }
            else if (action == "c2") {
                // �޷���ֵ�ĺ�������
                if (!SEM_STACK.empty()) {
                    string funcName = SEM_STACK.back(); SEM_STACK.pop_back();
                    qtList.push_back(Quaternion("call", funcName, "_", "_"));
                }
            }
        }
    }

    // ������Ƶķ����ķ�
    void loadEmbeddedTranslationGrammar() {
        // �����ķ����ķ����� - ʹ�ø���ȷ�����嶯��
        string grammar[] = {
            "Program->Struct Funcs",
            "Funcs->FuncsHead { CodeBody } Funcs",
            "FuncsHead->Type ID @PUSH_ID @SAVE_FUN @GEQ_G ( FormalParameters )",
            "Funcs->$",
            "FormalParameters->Type ID FormalParametersFollow",
            "FormalParameters->$",
            "FormalParametersFollow->, Type ID FormalParametersFollow",
            "FormalParametersFollow->$",
            "Type->int",
            "Type->void",
            "Type->float",
            "Type->char",
            "Type->st",
            "CodeBody->$",
            "CodeBody->LocalDefineList CodeList",
            "LocalDefineList->LocalVarDefine LocalDefineList",
            "LocalDefineList->$",
            "LocalVarDefine->Type ID ;",
            "CodeList->Code CodeList",
            "CodeList->$",
            "Code->NormalStatement",
            "Code->PrefixStatement",
            "Code->IfStatement",
            "Code->LoopStatement",
            "Code->break @GEQ_break ;",
            "Code->continue @GEQ_continue ;",
            "Code->return @SAVE_return Operation @GEQ_G ;",
            "Code->FuncCall",
            "PrefixStatement->++ @SAVE_++ ID @PUSH_ID @GEQ_G ;",
            "PrefixStatement->-- @SAVE_-- ID @PUSH_ID @GEQ_G ;",
            "NormalStatement->ID @PUSH_ID NormalStatementFollow",
            "NormalStatementFollow->= @SAVE_= Operation @GEQ_G ;",
            "NormalStatementFollow->+= @SAVE_+= Operation @GEQ_G ;",
            "NormalStatementFollow->-= @SAVE_-= Operation @GEQ_G ;",
            "NormalStatementFollow->*= @SAVE_*= Operation @GEQ_G ;",
            "NormalStatementFollow->/= @SAVE_/= Operation @GEQ_G ;",
            "NormalStatementFollow->++ @SAVE_p++ @GEQ_G ;",
            "NormalStatementFollow->-- @SAVE_p-- @GEQ_G ;",
            "Operation->T A",
            "A->M T @GEQ_G A",
            "A->$",
            "T->F B",
            "B->N F @GEQ_G B",
            "B->$",
            "F->ID @PUSH_ID",
            "F->NUM @PUSH_NUM",
            "F->( Operation )",
            "F->++ @SAVE_++ ID @PUSH_ID @GEQ_G",
            "F->-- @SAVE_-- ID @PUSH_ID @GEQ_G",
            "M->+ @SAVE_+",
            "M->- @SAVE_-",
            "N->* @SAVE_*",
            "N->/ @SAVE_/",
            "IfStatement->if @SAVE_if ( JudgeStatement ) @GEQ_G { CodeBody } @GEQ_el IFStatementFollow @GEQ_ie",
            "IFStatementFollow->$",
            "IFStatementFollow->ElseIFPart ElsePart",
            "ElsePart->$",
            "ElsePart->else { CodeBody }",
            "ElseIFPart->elif @SAVE_elif ( JudgeStatement ) @GEQ_G { CodeBody } @GEQ_el ElseIFPart",
            "ElseIFPart->$",
            "JudgeStatement->Operation JudgeStatementFollow",
            "JudgeStatementFollow->CompareSymbol Operation @GEQ_G",
            "JudgeStatementFollow->$",
            "CompareSymbol->== @SAVE_==",
            "CompareSymbol-><= @SAVE_<=",
            "CompareSymbol->>= @SAVE_>=",
            "CompareSymbol->< @SAVE_<",
            "CompareSymbol->> @SAVE_>",
            "CompareSymbol->!= @SAVE_!=",
            "LoopStatement->while @GEQ_wh @SAVE_do ( JudgeStatement ) @GEQ_G { CodeBody } @GEQ_we",
            "FuncCall->CALL ID @PUSH_ID FuncCallFollow ;",
            "FuncCallFollow->= ID @PUSH_ID ( Args ) @SAVE_callr @GEQ_c1",
            "FuncCallFollow->( Args ) @SAVE_call @GEQ_c2",
            "Args->F @SAVE_push @GEQ_G ArgsFollow",
            "Args->$",
            "ArgsFollow->, F @SAVE_push @GEQ_G ArgsFollow",
            "ArgsFollow->$",
            "Struct->struct st { LocalDefineList } ; Struct",
            "Struct->$"
        };

        int numRules = sizeof(grammar) / sizeof(grammar[0]);

        for (int i = 0; i < numRules; ++i) {
            string line = grammar[i];
            size_t arrow_pos = line.find("->");
            if (arrow_pos == string::npos) continue;

            string vn = line.substr(0, arrow_pos);
            string right = line.substr(arrow_pos + 2);

            if (i == 0) {
                Z = vn;
            }

            // ��ӷ��ս��
            VN.insert(vn);

            // �ָ��Ҳ�
            vector<string> tmp;
            stringstream ss(right);
            string token;
            while (ss >> token) {
                tmp.push_back(token);
                // ��ȡ�ս��
                if (VN.find(token) == VN.end() && token != "$" && token[0] != '@') {
                    VT.insert(token);
                }
            }

            P_LIST.push_back(make_pair(vn, tmp));
        }

        cout << "[DEBUG] Loaded " << P_LIST.size() << " embedded translation productions" << endl;
    }
};

// ============================================================================
// ��չ�������԰�����Ԫʽ����
// ============================================================================
int main() {
    cout << "=== C++ Compiler Frontend with Quaternion Generation ===" << endl;

    // ��ȡ�����ļ�
    string filename = "C://Users/��/Desktop/c_input.txt";
    ifstream inputFile(filename.c_str());
    vector<string> INPUT;

    if (!inputFile.is_open()) {
        cerr << "[ERROR] Could not open input file: " << filename << endl;
        return 1;
    }

    cout << "\n[DEBUG] Reading input from: " << filename << endl;
    string line;
    while (getline(inputFile, line)) {
        INPUT.push_back(line);
    }
    inputFile.close();

    try {
        // ����LL1�������������﷨������
        LL1 ll1("C://Users/��/Desktop/Project2/c_like_grammar.txt", true);

        // ��������
        ll1.getInput(INPUT);

        // �����﷨���������
        cout << "\n[DEBUG] Starting syntax and semantic analysis..." << endl;
        Message result = ll1.analyzeInputString();

        cout << "\n=== Analysis Result ===" << endl;
        if (result.hasError()) {
            cout << "Error Type: " << result.ErrorType << endl;
            cout << "Location: " << result.Location << endl;
            cout << "Error Message: " << result.ErrorMessage << endl;
            return 1;
        }
        else {
            cout << "Analysis completed successfully!" << endl;
        }

        // ��ӡ���ű�
        cout << "\n=== Symbol Table ===" << endl;
        ll1.printSymbolTable();

        // ������Ԫʽ
        cout << "\n[DEBUG] Starting quaternion generation..." << endl;

        // ����QtGen��ʹ�÷����ķ�
        QtGen qtGen(ll1.getSymbolTable(),
            "C://Users/��/Desktop/Project2/transalation_grammar.txt");

        // ��ȡ�������б�
        vector<vector<Token> > funcBlocks = ll1.getFuncBlocks();

        // Ϊÿ��������������Ԫʽ
        for (size_t i = 0; i < funcBlocks.size(); ++i) {
            cout << "\n[DEBUG] Generating quaternions for function " << i << endl;
            qtGen.genQt(funcBlocks[i]);
        }

        // ��ӡ��Ԫʽ
        qtGen.printQuaternions();

    }
    catch (const exception& e) {
        cout << "[ERROR] Exception occurred: " << e.what() << endl;
        return 1;
    }
    catch (...) {
        cout << "[ERROR] Unknown exception occurred" << endl;
        return 1;
    }

    return 0;
}