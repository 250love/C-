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
// Message struct  
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
// Variable struct 
// ============================================================================
struct Variable {
    string name;
    string type;        // 基本类型 (int, float, char, struct等)
    string fullType;    // 完整类型 (包含数组信息，如 int[], int[10] 等)
    int addr;
    bool isArray;       // 是否为数组
    int arraySize;      // 数组大小

    Variable() : name(""), type(""), fullType(""), addr(0), isArray(false), arraySize(0) {}

    Variable(const string& n, const string& t, int a)
        : name(n), type(t), fullType(t), addr(a), isArray(false), arraySize(0) {}

    Variable(const string& n, const string& t, const string& ft, int a, bool arr, int size)
        : name(n), type(t), fullType(ft), addr(a), isArray(arr), arraySize(size) {}
};


// ============================================================================
// Token struct  
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

    // 修改：增加数组支持
    Message addVariable(const Token& token, const string& typ, int s, bool isArray = false, int arraySize = 0) {
        cout << "[DEBUG] SecTable::addVariable - Adding variable: " << token.val
            << ", type: " << typ << ", size: " << s
            << ", isArray: " << isArray << ", arraySize: " << arraySize << endl;

        Message message = checkHasDefine(token);
        if (message.hasError()) {
            cout << "[DEBUG] Variable already defined: " << token.val << endl;
            return message;
        }

        // 提取变量名（去除数组部分）
        string varName = token.val;
        size_t bracketPos = varName.find('[');
        if (bracketPos != string::npos) {
            varName = varName.substr(0, bracketPos);
        }

        // 构建完整类型字符串
        string fullType = typ;
        if (isArray) {
            stringstream ss;
            ss << typ << "[" << arraySize << "]";
            fullType = ss.str();
        }

        Variable tmp(varName, typ, fullType, totalSize, isArray, arraySize);
        totalSize += s;
        variableDict[varName] = tmp;

        cout << "[DEBUG] Variable added successfully. Name: " << varName
            << ", Full type: " << fullType << ", Total size now: " << totalSize << endl;
        return Message();
    }

    Message checkHasDefine(const Token& token) {
        cout << "[DEBUG] SecTable::checkHasDefine - Checking: " << token.val << endl;

        // 提取变量名（去除数组部分）
        string varName = token.val;
        size_t bracketPos = varName.find('[');
        if (bracketPos != string::npos) {
            varName = varName.substr(0, bracketPos);
        }

        if (variableDict.find(varName) != variableDict.end()) {
            stringstream ss;
            ss << "Location:line " << (token.cur_line + 1);
            string location = ss.str();

            string errorMsg = "variable '" + varName + "' duplicate definition";
            cout << "[DEBUG] Duplicate definition found: " << varName << endl;
            return Message("Duplicate identified", location, errorMsg);
        }
        return Message();
    }

    Message checkDoDefine(const Token& token) {
        cout << "[DEBUG] SecTable::checkDoDefine - Checking: " << token.val << endl;

        // 提取变量名（去除数组部分）
        string varName = token.val;
        size_t bracketPos = varName.find('[');
        if (bracketPos != string::npos) {
            varName = varName.substr(0, bracketPos);
        }

        if (variableDict.find(varName) == variableDict.end()) {
            stringstream ss;
            ss << "Location:line " << token.cur_line;
            string location = ss.str();

            string errorMsg = "variable '" + varName + "' has no definition";
            cout << "[DEBUG] Variable not defined: " << varName << endl;
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

    // 新增：存储参数类型
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

        // 检查是否为数组参数
        string varName = token.val;
        bool isArray = false;
        int arraySize = 0;
        string fullType = typ;

        size_t bracketPos = varName.find('[');
        if (bracketPos != string::npos) {
            isArray = true;
            varName = varName.substr(0, bracketPos);

            // 提取数组大小（如果有）
            size_t endBracket = token.val.find(']');
            if (endBracket != string::npos && endBracket > bracketPos + 1) {
                string sizeStr = token.val.substr(bracketPos + 1, endBracket - bracketPos - 1);
                arraySize = atoi(sizeStr.c_str());
            }

            // 构建完整类型
            stringstream ss;
            ss << typ << "[]";  // 参数中的数组通常不指定大小
            fullType = ss.str();
        }

        Variable tmp(varName, typ, fullType, -2, isArray, arraySize);

        // 调整现有变量地址
        for (map<string, Variable>::iterator it = variableDict.begin();
            it != variableDict.end(); ++it) {
            Variable& var = it->second;
            var.addr -= 2;
        }

        variableDict[varName] = tmp;
        parametersDict[varName] = tmp;
        numOfParameters++;

        // 记录参数类型（使用完整类型）
        parameterTypes.push_back(fullType);

        cout << "[DEBUG] Parameter added. Name: " << varName
            << ", Full type: " << fullType << ", Total parameters: " << numOfParameters << endl;
        return Message();
    }
};


// ============================================================================
// 增强的SYMBOL类 - 改进了类型兼容性检查
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

    // 新增：当前分析的函数
    Function* currentFunction;

    // 新增：基本类型列表
    set<string> basicTypes;

    SYMBOL() : currentFunction(NULL) {
        cout << "[DEBUG] SYMBOL constructor called" << endl;

        // 初始化基本类型
        basicTypes.insert("int");
        basicTypes.insert("float");
        basicTypes.insert("char");
        basicTypes.insert("void");
        basicTypes.insert("double");
    }

    ~SYMBOL() {
        // 清理动态分配的内存
        for (size_t i = 0; i < functionList.size(); ++i) {
            delete functionList[i];
        }
        for (size_t i = 0; i < structList.size(); ++i) {
            delete structList[i];
        }
    }

    // 改进的类型兼容性检查
    bool isTypeCompatible(const string& expected, const string& actual) {
        cout << "[DEBUG] SYMBOL::isTypeCompatible - Checking: " << expected << " vs " << actual << endl;

        // 相同类型总是兼容
        if (expected == actual) return true;

        // 提取基本类型（去除数组标记）
        string expectedBase = extractBaseType(expected);
        string actualBase = extractBaseType(actual);

        // 改进的基本类型兼容规则 - 更严格
        // 只允许向上的隐式类型转换
        if (expectedBase == "float" && actualBase == "int") {
            return true; // int可以隐式转换为float
        }
        if (expectedBase == "double" && (actualBase == "int" || actualBase == "float")) {
            return true; // int和float可以隐式转换为double
        }

        // 不允许向下的隐式类型转换（如float到int）
        if (expectedBase == "int" && (actualBase == "float" || actualBase == "double")) {
            return false; // 这是你想要捕获的错误情况
        }

        // char类型的特殊处理
        if (expectedBase == "int" && actualBase == "char") {
            return true; // char可以隐式转换为int
        }
        if (expectedBase == "char" && actualBase == "int") {
            return false; // int不能隐式转换为char
        }

        // 数组类型检查
        bool expectedIsArray = isArrayType(expected);
        bool actualIsArray = isArrayType(actual);

        // 数组和非数组类型不兼容（除非是函数参数）
        if (expectedIsArray != actualIsArray) {
            // 函数参数中，数组可以退化为指针
            if (expectedIsArray && !actualIsArray) {
                return false;  // 期望数组但提供了非数组
            }
            if (!expectedIsArray && actualIsArray) {
                return true;   // 数组可以作为指针使用
            }
        }

        // 结构体类型必须完全匹配
        if (find(structNameList.begin(), structNameList.end(), expectedBase) != structNameList.end()) {
            return expectedBase == actualBase;
        }

        // 其他情况视为不兼容
        return false;
    }

    // 提取基本类型（去除数组标记）
    string extractBaseType(const string& fullType) {
        size_t bracketPos = fullType.find('[');
        if (bracketPos != string::npos) {
            return fullType.substr(0, bracketPos);
        }
        return fullType;
    }

    // 检查是否为数组类型
    bool isArrayType(const string& type) {
        return type.find('[') != string::npos;
    }

    // 获取变量的实际类型（包含数组信息）
    string getVariableType(const Token& token) {
        string varName = token.val;

        // 去除数组索引部分（如果有）
        size_t bracketPos = varName.find('[');
        if (bracketPos != string::npos) {
            varName = varName.substr(0, bracketPos);
        }

        // 检查当前函数中的局部变量
        if (currentFunction) {
            map<string, Variable>::iterator it = currentFunction->variableDict.find(varName);
            if (it != currentFunction->variableDict.end()) {
                // 返回基本类型（不包含数组信息，因为使用时已经解引用）
                return it->second.type;
            }
        }

        // 检查全局符号表
        map<string, SecTable*>::iterator symIt = symDict.find(varName);
        if (symIt != symDict.end()) {
            SecTable* table = symIt->second;
            if (Function* func = dynamic_cast<Function*>(table)) {
                return "function";
            }
            else if (Struct* st = dynamic_cast<Struct*>(table)) {
                return st->structName;
            }
        }

        // 检查全局变量（如果有全局作用域表）
        if (!allList.empty()) {
            for (vector<SecTable*>::iterator it = allList.begin(); it != allList.end(); ++it) {
                SecTable* table = *it;
                map<string, Variable>::iterator varIt = table->variableDict.find(varName);
                if (varIt != table->variableDict.end()) {
                    return varIt->second.type;
                }
            }
        }

        return "unknown";
    }

    // 其余方法保持不变...
    Message checkHasDefine(const Token& token) {
        cout << "[DEBUG] SYMBOL::checkHasDefine - Checking global: " << token.val << endl;

        // 提取实际的标识符名（去除数组部分）
        string varName = token.val;
        size_t bracketPos = varName.find('[');
        if (bracketPos != string::npos) {
            varName = varName.substr(0, bracketPos);
        }

        if (find(globalNameList.begin(), globalNameList.end(), varName) != globalNameList.end()) {
            stringstream ss;
            ss << "Location:line " << (token.cur_line + 1);
            string location = ss.str();

            string errorMsg = "variable '" + varName + "' duplicate definition";
            cout << "[DEBUG] Global duplicate definition: " << varName << endl;
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

        // 设置当前函数
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
        }
        else {
            // 检查是否为数组声明
            string varName = token.val;
            bool isArray = false;
            int arraySize = 0;

            size_t bracketPos = varName.find('[');
            if (bracketPos != string::npos) {
                isArray = true;

                // 提取数组大小
                size_t endBracket = varName.find(']');
                if (endBracket != string::npos && endBracket > bracketPos + 1) {
                    string sizeStr = varName.substr(bracketPos + 1, endBracket - bracketPos - 1);
                    arraySize = atoi(sizeStr.c_str());

                    if (arraySize <= 0) {
                        stringstream ss;
                        ss << "Location:line " << token.cur_line;
                        string location = ss.str();
                        string errorMsg = "array size must be positive";
                        return Message("Invalid array size", location, errorMsg);
                    }
                }
                else {
                    // 数组声明必须有大小（除非是参数）
                    stringstream ss;
                    ss << "Location:line " << token.cur_line;
                    string location = ss.str();
                    string errorMsg = "array size missing in declaration";
                    return Message("Array size missing", location, errorMsg);
                }
            }

            // 计算变量大小
            int s = 0;
            if (varType == "int") {
                s = 2;
            }
            else if (varType == "float") {
                s = 4;
            }
            else if (varType == "double") {
                s = 8;
            }
            else if (varType == "char") {
                s = 1;
            }
            else if (find(structNameList.begin(), structNameList.end(), varType) != structNameList.end()) {
                s = symDict[varType]->totalSize;
            }
            else {
                // 未知类型
                stringstream ss;
                ss << "Location:line " << token.cur_line;
                string location = ss.str();
                string errorMsg = "unknown type '" + varType + "'";
                return Message("Unknown type", location, errorMsg);
            }

            // 如果是数组，乘以数组大小
            if (isArray && arraySize > 0) {
                s *= arraySize;
            }

            message = tmp->addVariable(token, varType, s, isArray, arraySize);
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

    // 完整修改后的checkFunction函数
    Message checkFunction(const vector<Token>& RES_TOKEN, int id) {
        cout << "[DEBUG] SYMBOL::checkFunction - Checking function call at id: " << id << endl;

        if (id >= static_cast<int>(RES_TOKEN.size())) {
            return Message("Invalid token id", "", "Invalid token id");
        }

        const Token& token = RES_TOKEN[id];
        string funcName = token.val;

        // 检查函数是否定义
        if (find(globalNameList.begin(), globalNameList.end(), funcName) == globalNameList.end()) {
            stringstream ss;
            ss << "Location:line " << token.cur_line;
            string location = ss.str();

            string errorMsg = "function '" + funcName + "' has no definition";
            cout << "[DEBUG] Function not defined: " << funcName << endl;
            return Message("Unknown identifier", location, errorMsg);
        }

        // 查找函数定义
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

        // 计算实际参数数量
        int para_num = 0;
        int temp_id = id + 1; // 跳过函数名
        vector<Token> argTokens;
        vector<string> argTypes;

        // 确保下一个token是 '('
        if (temp_id >= static_cast<int>(RES_TOKEN.size()) || RES_TOKEN[temp_id].val != "(") {
            stringstream ss;
            ss << "Location:line " << token.cur_line;
            string location = ss.str();
            return Message("Syntax error", location, "expected '(' after function name");
        }
        temp_id++; // 跳过 '('

        // 收集参数直到遇到 ')'
        while (temp_id < static_cast<int>(RES_TOKEN.size()) && RES_TOKEN[temp_id].val != ")") {
            if (RES_TOKEN[temp_id].val == ",") {
                temp_id++;
                continue;
            }

            para_num++;
            argTokens.push_back(RES_TOKEN[temp_id]);
            temp_id++;
        }

        // 检查参数数量
        if (func->numOfParameters != para_num) {
            stringstream ss;
            ss << "Location:line " << token.cur_line;
            string location = ss.str();

            string errorMsg = "function '" + funcName + "' expects " +
                toString(func->numOfParameters) + " arguments, but " +
                toString(para_num) + " were provided";
            return Message("Argument count mismatch", location, errorMsg);
        }

        // 新增：参数类型检查
        for (int i = 0; i < para_num; i++) {
            string expectedType = func->parameterTypes[i];
            string actualType = "unknown";
            const Token& argToken = argTokens[i];

            // 确定实际参数类型
            if (argToken.type == "con") {
                // 常量：检查是否为浮点数
                if (argToken.val.find('.') != string::npos) {
                    actualType = "float";
                }
                else {
                    actualType = "int";
                }
            }
            else if (argToken.type == "i") {
                // 标识符：查找变量类型
                actualType = getVariableType(argToken);
            }
            else if (argToken.type == "s") {
                // 字符串字面量
                actualType = "char[]";  // 字符串是字符数组
            }
            else if (argToken.type == "c") {
                // 字符字面量
                actualType = "char";
            }

            // 检查类型兼容性
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

        // 输出函数信息
        for (size_t i = 0; i < functionList.size(); ++i) {
            Function* fun = functionList[i];

            stringstream ss;
            ss << "FuncName:" << fun->functionName
                << " ReturnType:" << fun->returnType
                << " NumOfParameters:" << fun->numOfParameters
                << " Size:" << fun->totalSize;
            symbolTableInfo.push_back(ss.str());

            // 显示参数类型
            if (fun->numOfParameters > 0) {
                ss.str("");
                ss << "    ParameterTypes:";
                for (size_t j = 0; j < fun->parameterTypes.size(); ++j) {
                    if (j > 0) ss << ", ";
                    ss << fun->parameterTypes[j];
                }
                symbolTableInfo.push_back(ss.str());
            }

            // 显示变量信息
            for (map<string, Variable>::iterator it = fun->variableDict.begin();
                it != fun->variableDict.end(); ++it) {
                const Variable& v = it->second;
                stringstream vss;
                vss << "    VariableName:" << v.name
                    << " Type:" << v.type
                    << " FullType:" << v.fullType
                    << " Addr:" << v.addr;
                if (v.isArray) {
                    vss << " IsArray:true Size:" << v.arraySize;
                }
                symbolTableInfo.push_back(vss.str());
            }
        }

        // 输出结构体信息
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
                    << " FullType:" << v.fullType
                    << " Addr:" << v.addr;
                if (v.isArray) {
                    vss << " IsArray:true Size:" << v.arraySize;
                }
                symbolTableInfo.push_back(vss.str());
            }
        }

        cout << "[DEBUG] Symbol table info generated with "
            << symbolTableInfo.size() << " entries" << endl;
    }

    // 新增：结束当前函数的分析
    void endCurrentFunction() {
        currentFunction = NULL;
    }

    // 新增：获取当前作用域
    SecTable* getCurrentScope() {
        if (!allList.empty()) {
            return allList.back();
        }
        return NULL;
    }

    // 新增：进入新作用域
    void enterScope(SecTable* scope) {
        allList.push_back(scope);
    }

    // 新增：退出当前作用域
    void exitScope() {
        if (!allList.empty()) {
            allList.pop_back();
        }
    }

private:
    // 辅助函数 - 整数转字符串
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
    map<string, map<string, int> > DICT; // 模拟Python的嵌套字典
    vector<string> INPUT;
    vector<Token> TokenList;
    int CUR_ROW;
    int CUR_LINE;
    int id;
    vector<string> k_list; // 关键字列表
    vector<string> p_list; // 界符列表

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

        // 初始化字典
        DICT["k"] = map<string, int>();
        DICT["p"] = map<string, int>();
        DICT["con"] = map<string, int>();
        DICT["c"] = map<string, int>();
        DICT["s"] = map<string, int>();
        DICT["i"] = map<string, int>();

        // 读取关键字列表
        ifstream keywordFile("C://Users/汤/Desktop/Project1/keyword_list.txt");
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
            // 默认关键字
            k_list.push_back("int");
            k_list.push_back("void");
            k_list.push_back("struct");
            k_list.push_back("return");
            k_list.push_back("if");
            k_list.push_back("else");
            k_list.push_back("while");
            k_list.push_back("for");
        }

        // 读取界符列表
        ifstream pFile("C://Users/汤/Desktop/Project1/p_list.txt");
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
            // 默认界符
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

        // 跳过空白字符
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
            // 标识符或关键字
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
            // 数字常量
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
            // 字符串
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
            // 字符
            demo = "'";
            for (int i = 0; i < 2; i++) {
                item = getNextChar();
                demo += item;
            }
            id_val = getId(demo, "c");
            typ = "c";
        }
        else {
            // 界符
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
    set<string> VN; // 非终结符
    set<string> VT; // 终结符
    string Z; // 开始符号
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

        // 保存分析表到文件
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

        // 清空现有数据
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

            // 分割右部
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

        // 提取终结符
        for (size_t i = 0; i < P_LIST.size(); ++i) {
            const vector<string>& right = P_LIST[i].second;
            for (size_t j = 0; j < right.size(); ++j) {
                if (VN.find(right[j]) == VN.end() && right[j] != "$" &&
                    (right[j].empty() || right[j][0] != '@')) {  // 排除语义动作
                    VT.insert(right[j]);
                }
            }
        }

        cout << "[DEBUG] Grammar loaded: " << VN.size() << " non-terminals, "
            << VT.size() << " terminals, " << P_LIST.size() << " productions" << endl;
    }

    void calFirstvt() {
        cout << "[DEBUG] Computing FIRST_VT sets" << endl;

        // 初始化
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

                    // 跳过语义动作
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

                        // 检查是否有空产生式
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

                // 跳过语义动作
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

                    // 检查是否有空产生式
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

        // 初始化
        for (set<string>::iterator it = VN.begin(); it != VN.end(); ++it) {
            FOLLOW[*it] = set<string>();
        }

        // 开始符号的FOLLOW集包含#
        FOLLOW[Z].insert("#");

        bool changed = true;
        while (changed) {
            changed = false;

            for (size_t i = 0; i < P_LIST.size(); ++i) {
                const string& left = P_LIST[i].first;
                const vector<string>& right = P_LIST[i].second;

                for (size_t j = 0; j < right.size(); ++j) {
                    const string& symbol = right[j];

                    // 跳过语义动作
                    if (!symbol.empty() && symbol[0] == '@') continue;

                    if (VN.find(symbol) != VN.end()) {
                        // 对于非终结符
                        bool shouldAddFollow = true;

                        for (size_t k = j + 1; k < right.size(); ++k) {
                            const string& nextSymbol = right[k];

                            // 跳过语义动作
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

                                // 检查是否有空产生式
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

        // 初始化分析表
        for (set<string>::iterator vn_it = VN.begin(); vn_it != VN.end(); ++vn_it) {
            for (set<string>::iterator vt_it = VT.begin(); vt_it != VT.end(); ++vt_it) {
                analysis_table[*vn_it][*vt_it] = -1;
            }
            analysis_table[*vn_it]["#"] = -1;
        }

        // 填充分析表
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

        // 保存VN
        file << "VN:";
        for (set<string>::iterator it = VN.begin(); it != VN.end(); ++it) {
            file << *it << " ";
        }
        file << endl;

        // 保存VT
        file << "VT:";
        for (set<string>::iterator it = VT.begin(); it != VT.end(); ++it) {
            file << *it << " ";
        }
        file << "# " << endl;

        // 保存分析表
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

        // 读取VN
        if (getline(file, line) && line.substr(0, 3) == "VN:") {
            stringstream ss(line.substr(3));
            string vn;
            VN.clear();
            while (ss >> vn) {
                VN.insert(vn);
            }
        }

        // 读取VT
        if (getline(file, line) && line.substr(0, 3) == "VT:") {
            stringstream ss(line.substr(3));
            string vt;
            VT.clear();
            while (ss >> vt) {
                VT.insert(vt);
            }
        }

        // 读取分析表
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
// 增强的LL1类 - 添加了赋值类型检查
// ============================================================================
class LL1 : public GrammarParser {
private:
    Lexer lex;
    vector<Token> RES_TOKEN;
    SYMBOL syn_table;
    vector<vector<Token> > funcBlocks;

    // 新增：表达式类型推导结果缓存
    map<int, string> exprTypeCache;

protected:
    virtual string getAnalysisTableName() {
        return "SyntaxAnalysisTable.txt";
    }

public:
    // 获取符号表指针
    SYMBOL* getSymbolTable() {
        return &syn_table;
    }

    // 获取函数块列表
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

    // 新增：推导表达式类型
    string inferExpressionType(int startId, int endId) {
        cout << "[DEBUG] LL1::inferExpressionType - Inferring type from " << startId << " to " << endId << endl;

        if (startId >= static_cast<int>(RES_TOKEN.size()) || endId >= static_cast<int>(RES_TOKEN.size())) {
            return "unknown";
        }

        // 检查缓存
        if (exprTypeCache.find(startId) != exprTypeCache.end()) {
            return exprTypeCache[startId];
        }

        // 简单情况：单个token
        if (startId == endId) {
            Token& token = RES_TOKEN[startId];
            string type = "unknown";

            if (token.type == "con") {
                // 常量
                if (token.val.find('.') != string::npos) {
                    type = "float";
                }
                else {
                    type = "int";
                }
            }
            else if (token.type == "i") {
                // 标识符
                type = syn_table.getVariableType(token);
            }
            else if (token.type == "s") {
                type = "char[]";
            }
            else if (token.type == "c") {
                type = "char";
            }

            exprTypeCache[startId] = type;
            return type;
        }

        // 复杂表达式：需要分析运算符和操作数
        // 这里简化处理，实际应该构建表达式树
        string resultType = "unknown";
        bool hasFloat = false;
        bool hasDouble = false;

        for (int i = startId; i <= endId; i++) {
            Token& token = RES_TOKEN[i];

            if (token.type == "con" || token.type == "i") {
                string tokenType = inferExpressionType(i, i);
                if (tokenType == "float") hasFloat = true;
                else if (tokenType == "double") hasDouble = true;
            }
        }

        // 类型提升规则
        if (hasDouble) {
            resultType = "double";
        }
        else if (hasFloat) {
            resultType = "float";
        }
        else {
            resultType = "int"; // 默认为int
        }

        exprTypeCache[startId] = resultType;
        return resultType;
    }

    // 新增：查找赋值语句的右侧表达式范围
    pair<int, int> findAssignmentRHS(int assignOpId) {
        int start = assignOpId + 1;
        int end = start;

        // 查找到分号或其他语句终止符
        while (end < static_cast<int>(RES_TOKEN.size())) {
            if (RES_TOKEN[end].val == ";" || RES_TOKEN[end].val == "," ||
                RES_TOKEN[end].val == ")") {
                end--;
                break;
            }
            end++;
        }

        if (end >= static_cast<int>(RES_TOKEN.size())) {
            end = RES_TOKEN.size() - 1;
        }

        return make_pair(start, end);
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
                    // x是终结符但不匹配
                    stringstream ss;
                    ss << "Location:line " << token.cur_line;
                    string location = ss.str();

                    string errorMsg = "expect '" + x + "' before '" + w + "'";
                    return Message("Identifier Expected", location, errorMsg);
                }

                // x是非终结符，查分析表
                if (analysis_table.find(x) == analysis_table.end() ||
                    analysis_table[x].find(w) == analysis_table[x].end()) {
                    return Message("Analysis table error", "", "Invalid entry in analysis table");
                }

                int id = analysis_table[x][w];
                if (id == -1) {
                    // 错误处理
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

                // 获取产生式右部
                const vector<string>& tmp = P_LIST[id].second;
                if (tmp.size() != 1 || tmp[0] != "$") {
                    // 逆序压栈
                    for (int i = static_cast<int>(tmp.size()) - 1; i >= 0; --i) {
                        stack.push_back(tmp[i]);
                    }

                    // 符号表处理
                    Message message = editSymTable(x, w, token);
                    if (message.hasError()) {
                        return message;
                    }
                }
            }
            else {
                // 匹配成功
                if (w == "#") {
                    funcBlocks.push_back(funcBlock);
                    syn_table.showTheInfo();
                    return Message(); // 分析成功
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
            return "st"; // 结构体类型
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
            // 函数定义
            if (static_cast<int>(token.id + 1) < static_cast<int>(RES_TOKEN.size())) {
                return syn_table.addFunction(RES_TOKEN[token.id + 1], token.val);
            }
        }

        if (x == "Struct") {
            // 结构体定义
            if (static_cast<int>(token.id + 1) < static_cast<int>(RES_TOKEN.size())) {
                return syn_table.addStruct(RES_TOKEN[token.id + 1]);
            }
        }

        if (x.find("FormalParameters") == 0) {
            // 函数参数处理
            if (token.val != ",") {
                // 正常类型-变量对
                if (static_cast<int>(token.id + 1) < static_cast<int>(RES_TOKEN.size())) {
                    return syn_table.addVariableToTable(RES_TOKEN[token.id + 1], token.val, true);
                }
            }
            else {
                // 逗号后应该是新的类型声明
                if (static_cast<int>(token.id + 2) < static_cast<int>(RES_TOKEN.size())) {
                    Token typeToken = RES_TOKEN[token.id + 1];
                    Token varToken = RES_TOKEN[token.id + 2];

                    // 确保类型token是有效的类型标识符
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
            // 局部变量定义
            if (static_cast<int>(token.id + 1) < static_cast<int>(RES_TOKEN.size())) {
                return syn_table.addVariableToTable(RES_TOKEN[token.id + 1], token.val);
            }
        }

        // 新增：处理赋值操作的类型检查
        if (w == "=" && token.val == "=") {
            return checkAssignmentType(token);
        }

        if (x == "NormalStatement" || (x == "F" && w == "ID")) {
            // 变量使用检查
            if (token.val.find('.') == string::npos && token.val.find('[') == string::npos) {
                return syn_table.checkDoDefineInFunction(token);
            }
        }

        // 处理复合赋值操作符
        if (x == "CompoundAssignOp") {
            // 对于复合赋值操作符，也需要进行类型检查
            if (token.val == "+=" || token.val == "-=" || token.val == "*=" || token.val == "/=") {
                return checkCompoundAssignmentType(token);
            }
        }

        // 处理后缀操作符
        if (x == "PostfixOp") {
            // 对于后缀++/--，需要检查操作符前面的变量是否已定义
            if (token.id > 0) {
                Token varToken = RES_TOKEN[token.id - 1];
                if (varToken.type == "i") {
                    return syn_table.checkDoDefineInFunction(varToken);
                }
            }
        }

        // 处理前缀操作符
        if (x == "UnaryOp") {
            // 对于前缀++/--，需要检查操作符后面的变量是否已定义
            if (static_cast<int>(token.id + 1) < static_cast<int>(RES_TOKEN.size())) {
                Token varToken = RES_TOKEN[token.id + 1];
                if (varToken.type == "i") {
                    return syn_table.checkDoDefineInFunction(varToken);
                }
            }
        }

        if (x == "FuncCallFollow") {
            // 函数调用检查
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

    // 新增：检查赋值类型兼容性
    Message checkAssignmentType(const Token& assignOp) {
        cout << "[DEBUG] LL1::checkAssignmentType - Checking assignment at token " << assignOp.id << endl;

        if (assignOp.id == 0) {
            return Message("Invalid assignment", "", "Assignment operator at invalid position");
        }

        // 获取左侧变量
        Token& lhsToken = RES_TOKEN[assignOp.id - 1];
        if (lhsToken.type != "i") {
            return Message(); // 不是变量赋值，可能是其他语法结构
        }

        // 获取左侧变量类型
        string lhsType = syn_table.getVariableType(lhsToken);
        if (lhsType == "unknown") {
            // 变量未定义的错误已经在其他地方检查
            return Message();
        }

        // 获取右侧表达式范围
        pair<int, int> rhsRange = findAssignmentRHS(assignOp.id);

        // 推导右侧表达式类型
        string rhsType = inferExpressionType(rhsRange.first, rhsRange.second);

        cout << "[DEBUG] Assignment type check: " << lhsToken.val
            << " (" << lhsType << ") = expression (" << rhsType << ")" << endl;

        // 检查类型兼容性
        if (!syn_table.isTypeCompatible(lhsType, rhsType)) {
            stringstream ss;
            ss << "Location:line " << assignOp.cur_line;
            string location = ss.str();

            string errorMsg = "type mismatch in assignment: cannot assign '" + rhsType +
                "' to variable '" + lhsToken.val + "' of type '" + lhsType + "'";

            // 特别提示常见错误
            if (lhsType == "int" && rhsType == "float") {
                errorMsg += " (possible loss of precision)";
            }

            return Message("Type mismatch", location, errorMsg);
        }

        return Message();
    }

    // 新增：检查复合赋值类型兼容性
    Message checkCompoundAssignmentType(const Token& compoundOp) {
        cout << "[DEBUG] LL1::checkCompoundAssignmentType - Checking compound assignment" << endl;

        if (compoundOp.id == 0) {
            return Message("Invalid compound assignment", "", "Compound operator at invalid position");
        }

        // 获取左侧变量
        Token& lhsToken = RES_TOKEN[compoundOp.id - 1];
        if (lhsToken.type != "i") {
            return Message();
        }

        // 获取变量类型
        string lhsType = syn_table.getVariableType(lhsToken);
        if (lhsType == "unknown") {
            return Message();
        }

        // 获取右侧表达式
        pair<int, int> rhsRange = findAssignmentRHS(compoundOp.id);
        string rhsType = inferExpressionType(rhsRange.first, rhsRange.second);

        // 对于复合赋值，检查操作数类型是否兼容
        if (!syn_table.isTypeCompatible(lhsType, rhsType)) {
            stringstream ss;
            ss << "Location:line " << compoundOp.cur_line;
            string location = ss.str();

            string errorMsg = "type mismatch in compound assignment: cannot apply '" +
                compoundOp.val + "' between '" + lhsType + "' and '" + rhsType + "'";

            return Message("Type mismatch", location, errorMsg);
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
// Quaternion struct (四元式结构)
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
// QtGen class (完全重新设计的四元式生成器)
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

        // 如果没有提供文法路径，使用默认的翻译文法
        if (grammarPath.empty()) {
            loadEmbeddedTranslationGrammar();
        }

        // 构建分析表
        initList();
    }

    // 获取四元式结果
    vector<vector<Quaternion> > getQtRes() const {
        return qt_res;
    }

    // 生成四元式
    void genQt(const vector<Token>& funcBlock) {
        cout << "[DEBUG] QtGen::genQt - Generating quaternions for function block" << endl;

        vector<Quaternion> qtList;
        vector<string> SEM_STACK;     // 语义栈
        vector<string> SYMBOL_STACK;  // 语义符号栈
        vector<string> SYN;           // 语法栈

        // 新增：token栈，用于跟踪匹配的tokens
        vector<Token> MATCHED_TOKENS;

        SYN.push_back("#");
        SYN.push_back(Z);

        // 创建token列表副本
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

            // 处理语义动作
            if (!x.empty() && x[0] == '@') {
                catchAction(x, MATCHED_TOKENS, SYMBOL_STACK, SEM_STACK, qtList);
                continue;
            }

            if (x != w) {
                // x是非终结符，查找产生式
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
                                // 逆序压栈
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
                // 匹配成功
                cout << "[DEBUG] Matched: " << x << " with token: " << token.val << endl;

                // 将匹配的token添加到栈中
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

    // 打印四元式
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
    // 获取Token的值（用于语法分析）
    string getTokenVal(const Token& token) {
        // 检查是否是结构体名
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

        // 特殊处理CALL
        if (token.val == "CALL") {
            return "CALL";
        }

        return token.val;
    }

    // 重新设计的语义动作处理
    void catchAction(const string& x, vector<Token>& MATCHED_TOKENS,
        vector<string>& SYMBOL_STACK,
        vector<string>& SEM_STACK,
        vector<Quaternion>& qtList) {

        cout << "[DEBUG] Semantic action: " << x << endl;

        // 处理 @PUSH_VAL
        if (x == "@PUSH_VAL") {
            // 从匹配的token栈中获取最近匹配的token
            if (!MATCHED_TOKENS.empty()) {
                Token lastMatched = MATCHED_TOKENS.back();
                SEM_STACK.push_back(lastMatched.val);
                cout << "[DEBUG] Pushed to SEM_STACK: " << lastMatched.val << endl;
            }
            return;
        }

        // 处理 @PUSH_ID - 推入最近匹配的ID token
        if (x == "@PUSH_ID") {
            // 从后往前找最近的ID token
            for (int i = static_cast<int>(MATCHED_TOKENS.size()) - 1; i >= 0; --i) {
                if (MATCHED_TOKENS[i].type == "i") {
                    SEM_STACK.push_back(MATCHED_TOKENS[i].val);
                    cout << "[DEBUG] Pushed ID to SEM_STACK: " << MATCHED_TOKENS[i].val << endl;
                    break;
                }
            }
            return;
        }

        // 处理 @PUSH_NUM - 推入最近匹配的NUM token
        if (x == "@PUSH_NUM") {
            // 从后往前找最近的NUM token
            for (int i = static_cast<int>(MATCHED_TOKENS.size()) - 1; i >= 0; --i) {
                if (MATCHED_TOKENS[i].type == "con") {
                    SEM_STACK.push_back(MATCHED_TOKENS[i].val);
                    cout << "[DEBUG] Pushed NUM to SEM_STACK: " << MATCHED_TOKENS[i].val << endl;
                    break;
                }
            }
            return;
        }

        // 处理 @SAVE_X 格式
        if (x.find("@SAVE_") == 0) {
            string symbol = x.substr(6);  // 获取 @SAVE_ 后面的符号
            SYMBOL_STACK.push_back(symbol);
            cout << "[DEBUG] Pushed to SYMBOL_STACK: " << symbol << endl;
            return;
        }

        // 处理 @GEQ_X 格式
        if (x.find("@GEQ_") == 0) {
            string action = x.substr(5);  // 获取 @GEQ_ 后面的动作

            if (action == "G") {
                // 通用四元式生成
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
                        // 函数定义
                        if (!SEM_STACK.empty()) {
                            string funcName = SEM_STACK.back();
                            SEM_STACK.pop_back();
                            qtList.push_back(Quaternion("FUN", funcName, "_", "_"));
                        }
                    }
                    else if (op == "=") {
                        // 赋值操作
                        if (SEM_STACK.size() >= 2) {
                            string rvalue = SEM_STACK.back(); SEM_STACK.pop_back();
                            string lvalue = SEM_STACK.back(); SEM_STACK.pop_back();
                            qtList.push_back(Quaternion("=", rvalue, "_", lvalue));
                        }
                    }
                    else if (op == "+=" || op == "-=" || op == "*=" || op == "/=") {
                        // 复合赋值
                        if (SEM_STACK.size() >= 2) {
                            string rvalue = SEM_STACK.back(); SEM_STACK.pop_back();
                            string lvalue = SEM_STACK.back(); SEM_STACK.pop_back();

                            // 先计算
                            string binOp = op.substr(0, 1);
                            stringstream ss;
                            ss << "@t" << t_id++;
                            string temp = ss.str();
                            qtList.push_back(Quaternion(binOp, lvalue, rvalue, temp));

                            // 再赋值
                            qtList.push_back(Quaternion("=", temp, "_", lvalue));
                        }
                    }
                    else if (op == "++" || op == "--") {
                        // 前缀自增自减
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
                        // 后缀自增自减
                        if (!SEM_STACK.empty()) {
                            string var = SEM_STACK.back(); SEM_STACK.pop_back();

                            stringstream ss;
                            ss << "@t" << t_id++;
                            string temp = ss.str();

                            // 先保存原值
                            qtList.push_back(Quaternion("=", var, "_", temp));

                            // 再自增/自减
                            string binOp = (op == "p++") ? "+" : "-";
                            stringstream ss2;
                            ss2 << "@t" << t_id++;
                            string temp2 = ss2.str();

                            qtList.push_back(Quaternion(binOp, var, "1", temp2));
                            qtList.push_back(Quaternion("=", temp2, "_", var));

                            // 返回原值
                            SEM_STACK.push_back(temp);
                        }
                    }
                    else if (op == "+" || op == "-" || op == "*" || op == "/" ||
                        op == ">" || op == "<" || op == ">=" || op == "<=" ||
                        op == "==" || op == "!=") {
                        // 二元运算
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
                        // return语句
                        if (!SEM_STACK.empty()) {
                            string retVal = SEM_STACK.back(); SEM_STACK.pop_back();
                            qtList.push_back(Quaternion("return", retVal, "_", "_"));
                        }
                    }
                    else if (op == "if" || op == "elif") {
                        // 条件语句
                        if (!SEM_STACK.empty()) {
                            string condition = SEM_STACK.back(); SEM_STACK.pop_back();
                            qtList.push_back(Quaternion(op, condition, "_", "_"));
                        }
                    }
                    else if (op == "do") {
                        // while循环条件
                        if (!SEM_STACK.empty()) {
                            string condition = SEM_STACK.back(); SEM_STACK.pop_back();
                            qtList.push_back(Quaternion("do", condition, "_", "_"));
                        }
                    }
                    else if (op == "push") {
                        // 函数参数
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
                // 有返回值的函数调用
                if (SEM_STACK.size() >= 2) {
                    string funcName = SEM_STACK.back(); SEM_STACK.pop_back();
                    string retVar = SEM_STACK.back(); SEM_STACK.pop_back();
                    qtList.push_back(Quaternion("callr", funcName, "_", retVar));
                }
            }
            else if (action == "c2") {
                // 无返回值的函数调用
                if (!SEM_STACK.empty()) {
                    string funcName = SEM_STACK.back(); SEM_STACK.pop_back();
                    qtList.push_back(Quaternion("call", funcName, "_", "_"));
                }
            }
        }
    }

    // 重新设计的翻译文法
    void loadEmbeddedTranslationGrammar() {
        // 修正的翻译文法规则 - 使用更精确的语义动作
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

            // 添加非终结符
            VN.insert(vn);

            // 分割右部
            vector<string> tmp;
            stringstream ss(right);
            string token;
            while (ss >> token) {
                tmp.push_back(token);
                // 提取终结符
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
// DAGNode struct (DAG节点结构)
// ============================================================================
struct DAGNode {
    string op;                    // 操作符
    int ID;                      // 节点ID
    vector<string> signs;        // 标记列表（符号列表）
    int leftNodeID;             // 左子节点ID（-1表示无）
    int rightNodeID;            // 右子节点ID（-1表示无）

    DAGNode(const string& o, int id, int left = -1, int right = -1)
        : op(o), ID(id), leftNodeID(left), rightNodeID(right) {}
};

// ============================================================================
// Optimization class (DAG优化类)
// ============================================================================
class Optimization {
private:
    SYMBOL* symTable;                    // 符号表指针
    vector<DAGNode> nodes;               // DAG节点列表
    vector<Quaternion> new_qt;           // 优化后的四元式
    set<string> operation;               // 操作符集合

public:
    Optimization(SYMBOL* sym) : symTable(sym) {
        cout << "[DEBUG] Optimization constructor called" << endl;

        // 初始化操作符集合
        operation.insert("+");
        operation.insert("-");
        operation.insert("*");
        operation.insert("/");
        operation.insert("=");
        operation.insert(">");
        operation.insert("<");
        operation.insert("==");
        operation.insert(">=");
        operation.insert("<=");
        operation.insert("!=");
    }

    // 优化整个函数块
    vector<vector<Quaternion> > opt(const vector<Quaternion>& funcBlock) {
        cout << "[DEBUG] Optimization::opt - Starting optimization for function block" << endl;

        vector<Quaternion> fBlock = funcBlock;  // 复制函数块
        vector<vector<Quaternion> > res;        // 基本块列表
        vector<Quaternion> bloc;                // 当前基本块

        // 将函数块划分为基本块
        while (!fBlock.empty()) {
            Quaternion tmp = fBlock[0];
            fBlock.erase(fBlock.begin());

            if (tmp.op == "wh") {
                if (!bloc.empty()) {
                    res.push_back(bloc);
                }
                bloc.clear();
                bloc.push_back(tmp);
            }
            else if (tmp.op == "do" || tmp.op == "we" || tmp.op == "if" ||
                tmp.op == "el" || tmp.op == "elif" || tmp.op == "ie" ||
                tmp.op == "return" || tmp.op == "continue" || tmp.op == "break") {
                bloc.push_back(tmp);
                res.push_back(bloc);
                bloc.clear();
            }
            else if (tmp.op == "FUN") {
                vector<Quaternion> funBloc;
                funBloc.push_back(tmp);
                res.push_back(funBloc);
            }
            else {
                bloc.push_back(tmp);
            }
        }

        // 处理最后一个基本块
        if (!bloc.empty()) {
            res.push_back(bloc);
        }

        // 优化每个基本块
        vector<vector<Quaternion> > funcBlock_new;
        for (size_t i = 0; i < res.size(); ++i) {
            optTheBloc(res[i]);
            funcBlock_new.push_back(new_qt);
        }

        cout << "[DEBUG] Optimization completed. " << res.size()
            << " basic blocks processed" << endl;

        return funcBlock_new;
    }

private:
    // 优化单个基本块
    void optTheBloc(const vector<Quaternion>& bloc) {
        cout << "[DEBUG] Optimizing basic block with " << bloc.size()
            << " quaternions" << endl;

        nodes.clear();
        new_qt.clear();

        // 构建DAG
        for (size_t i = 0; i < bloc.size(); ++i) {
            const Quaternion& qt = bloc[i];

            if (operation.find(qt.op) != operation.end()) {
                if (qt.op == "=") {
                    // 一元运算符（赋值）
                    int idB = Get_NODE(qt.arg1);
                    delete_sym(qt.result);
                    add_to_node(idB, qt.result);
                }
                else {
                    // 二元运算符
                    // 检查是否可以进行常量折叠
                    if (isNumber(qt.arg1) && isNumber(qt.arg2)) {
                        // 常量折叠
                        double val1 = atof(qt.arg1.c_str());
                        double val2 = atof(qt.arg2.c_str());
                        double result = 0;

                        if (qt.op == "+") result = val1 + val2;
                        else if (qt.op == "-") result = val1 - val2;
                        else if (qt.op == "*") result = val1 * val2;
                        else if (qt.op == "/") result = val1 / val2;

                        stringstream ss;
                        ss << result;
                        int idP = Get_NODE(ss.str());
                        delete_sym(qt.result);
                        add_to_node(idP, qt.result);
                    }
                    else {
                        // 正常的二元运算
                        int idB = Get_NODE(qt.arg1);
                        int idC = Get_NODE(qt.arg2);
                        int idop = Get_NODE(qt.op, idB, idC);
                        delete_sym(qt.result);
                        add_to_node(idop, qt.result);
                    }
                }
            }
        }

        // 从DAG生成新的四元式
        generate_new_qt(nodes);

        // 添加非运算四元式
        int flag = 0;
        int insertPos = 0;

        for (size_t i = 0; i < bloc.size(); ++i) {
            const Quaternion& qt = bloc[i];

            if (operation.find(qt.op) == operation.end()) {
                // 非运算四元式
                if (flag == 1) {
                    new_qt.push_back(qt);
                }
                else {
                    new_qt.insert(new_qt.begin() + insertPos, qt);
                    insertPos++;
                }
            }
            else {
                flag = 1;
            }
        }
    }

    // 获取或创建DAG节点
    int Get_NODE(const string& sym, int leftNodeID = -1, int rightNodeID = -1) {
        // 如果是操作符
        if (operation.find(sym) != operation.end()) {
            // 倒序遍历，查找已存在的节点
            for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; --i) {
                const DAGNode& node = nodes[i];

                if (sym == node.op) {
                    // 对于交换律运算符
                    if (sym == "+" || sym == "*") {
                        if ((leftNodeID == node.leftNodeID && rightNodeID == node.rightNodeID) ||
                            (leftNodeID == node.rightNodeID && rightNodeID == node.leftNodeID)) {
                            return node.ID;
                        }
                    }
                    else {
                        // 非交换律运算符
                        if (leftNodeID == node.leftNodeID && rightNodeID == node.rightNodeID) {
                            return node.ID;
                        }
                    }
                }
            }

            // 创建新节点
            DAGNode newNode(sym, static_cast<int>(nodes.size()), leftNodeID, rightNodeID);
            nodes.push_back(newNode);
            return newNode.ID;
        }
        else {
            // 操作数
            // 倒序遍历，查找包含该符号的节点
            for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; --i) {
                const DAGNode& node = nodes[i];

                if (find(node.signs.begin(), node.signs.end(), sym) != node.signs.end()) {
                    return node.ID;
                }
            }

            // 创建新节点
            DAGNode newNode("", static_cast<int>(nodes.size()));
            newNode.signs.push_back(sym);
            nodes.push_back(newNode);
            return newNode.ID;
        }
    }

    // 从节点中删除符号（如果不是主标记）
    bool delete_sym(const string& A) {
        for (size_t i = 0; i < nodes.size(); ++i) {
            DAGNode& node = nodes[i];

            // 查找符号A
            vector<string>::iterator it = find(node.signs.begin(), node.signs.end(), A);

            // 如果找到且不是主标记（第一个元素）
            if (it != node.signs.end() && it != node.signs.begin()) {
                node.signs.erase(it);
                return true;
            }
        }
        return false;
    }

    // 将符号添加到节点
    void add_to_node(int nodeID, const string& sym) {
        if (nodeID < 0 || nodeID >= static_cast<int>(nodes.size())) {
            return;
        }

        DAGNode& node = nodes[nodeID];

        // 如果主标记是临时变量且新符号不是临时变量，则交换位置
        if (!node.signs.empty() &&
            node.signs[0].size() > 0 && node.signs[0][0] == '@' &&
            sym.size() > 0 && sym[0] != '@') {
            string tmp = node.signs[0];
            node.signs[0] = sym;
            node.signs.push_back(tmp);
        }
        else {
            node.signs.push_back(sym);
        }
    }

    // 从DAG生成新的四元式
    void generate_new_qt(const vector<DAGNode>& NODES) {
        for (size_t i = 0; i < NODES.size(); ++i) {
            const DAGNode& node = NODES[i];

            // 叶节点，多个标记
            if (node.leftNodeID == -1 && node.rightNodeID == -1 && node.signs.size() > 1) {
                for (size_t j = 1; j < node.signs.size(); ++j) {
                    if (node.signs[j].size() > 0 && node.signs[j][0] != '@') {
                        // Ai = B
                        Quaternion qt("=", node.signs[0], "_", node.signs[j]);
                        new_qt.push_back(qt);
                    }
                }
            }
            // 内部节点，多个标记
            else if (node.leftNodeID != -1 && node.rightNodeID != -1 && node.signs.size() > 1) {
                // A = B op C
                Quaternion qt(node.op,
                    nodes[node.leftNodeID].signs[0],
                    nodes[node.rightNodeID].signs[0],
                    node.signs[0]);
                new_qt.push_back(qt);

                // 处理其他标记
                for (size_t j = 1; j < node.signs.size(); ++j) {
                    if (node.signs[j].size() > 0 && node.signs[j][0] != '@') {
                        Quaternion qt2("=", node.signs[0], "_", node.signs[j]);
                        new_qt.push_back(qt2);
                    }
                }
            }
            // 内部节点，单个标记
            else if (node.leftNodeID != -1 && node.rightNodeID != -1) {
                Quaternion qt(node.op,
                    nodes[node.leftNodeID].signs[0],
                    nodes[node.rightNodeID].signs[0],
                    node.signs[0]);
                new_qt.push_back(qt);
            }
        }
    }

    // 判断字符串是否为数字
    bool isNumber(const string& str) {
        if (str.empty()) return false;

        size_t i = 0;
        if (str[0] == '-' || str[0] == '+') i = 1;

        bool hasDecimal = false;
        for (; i < str.length(); ++i) {
            if (str[i] == '.') {
                if (hasDecimal) return false;
                hasDecimal = true;
            }
            else if (!isdigit(str[i])) {
                return false;
            }
        }

        return true;
    }
};


// ============================================================================
// AsmCodeGen class - 8086 Assembly Code Generator
// 将此代码直接添加到您的原始cpp文件的末尾，在main函数之前
// ============================================================================

// 定义QtxInfo结构体，对应Python中的namedtuple
struct QtxInfo {
    string val;         // 变量名
    bool actInfo;       // 活跃信息
    int addr1;          // 一级地址相对于函数
    int addr2;          // 二级地址相对于结构体数组
    bool dosePointer;   // 是否是指针

    QtxInfo() : val(""), actInfo(false), addr1(0), addr2(0), dosePointer(false) {}

    QtxInfo(const string& v, bool act, int a1, int a2, bool ptr)
        : val(v), actInfo(act), addr1(a1), addr2(a2), dosePointer(ptr) {}
};
// Extended Quaternion structure with QtxInfo
// Extended Quaternion structure with QtxInfo
struct QuaternionExt {
    QtxInfo op;
    QtxInfo arg1;
    QtxInfo arg2;
    QtxInfo result;

    QuaternionExt() {}

    // From normal Quaternion conversion constructor
    QuaternionExt(const Quaternion& q) {
        op.val = q.op;
        arg1.val = q.arg1;
        arg2.val = q.arg2;
        result.val = q.result;
    }
};

class AsmCodeGen {
private:
    SYMBOL* symTable;
    vector<vector<vector<Quaternion> > > allCode;
    vector<string> allAsmCode;
    vector<vector<vector<string> > > funcsAsmCode;
    vector<vector<vector<string> > > mainAsmCode;

    int id;  // Label generator
    map<string, string> op2asm;

    // Helper function: Check if string is a number
    bool isNumber(const string& str) {
        if (str.empty()) return false;
        size_t start = 0;
        if (str[0] == '-' || str[0] == '+') start = 1;
        for (size_t i = start; i < str.length(); ++i) {
            if (!isdigit(str[i])) {
                return false;
            }
        }
        return true;
    }

    // Helper function: Convert integer to string
    string toString(int value) {
        stringstream ss;
        ss << value;
        return ss.str();
    }

    // Helper function: Parse array access
    void parseArrayAccess(const string& str, string& arrayName, string& index) {
        size_t leftBracket = str.find('[');
        size_t rightBracket = str.find(']');

        if (leftBracket != string::npos && rightBracket != string::npos) {
            arrayName = str.substr(0, leftBracket);
            index = str.substr(leftBracket + 1, rightBracket - leftBracket - 1);
        }
        else {
            arrayName = str;
            index = "";
        }
    }

    // Get variable info from symbol table
    Variable* getVariableInfo(const string& varName, Function* funcTable) {
        // First check in function's local variables
        if (funcTable) {
            map<string, Variable>::iterator it = funcTable->variableDict.find(varName);
            if (it != funcTable->variableDict.end()) {
                return &(it->second);
            }
        }

        // Check in main function for global variables
        Function* mainFunc = NULL;
        for (size_t i = 0; i < symTable->functionList.size(); ++i) {
            if (symTable->functionList[i]->functionName == "main") {
                mainFunc = symTable->functionList[i];
                break;
            }
        }

        if (mainFunc && mainFunc != funcTable) {
            map<string, Variable>::iterator it = mainFunc->variableDict.find(varName);
            if (it != mainFunc->variableDict.end()) {
                return &(it->second);
            }
        }

        return NULL;
    }

    // Fixed LD_Main to handle struct array members properly
    vector<string> LD_Main(const QtxInfo& x, Function* funcTable) {
        vector<string> res;

        // Handle struct parameters (pass address)
        Variable* var = getVariableInfo(x.val, funcTable);
        if (var) {
            if (find(symTable->structNameList.begin(), symTable->structNameList.end(), var->type)
                != symTable->structNameList.end()) {
                res.push_back("MOV AX,OFFSET " + x.val);
                return res;
            }
        }

        // Immediate number
        if (isNumber(x.val)) {
            res.push_back("MOV AX," + x.val);
        }
        // Temporary variable
        else if (x.val.length() > 0 && x.val[0] == '@') {
            res.push_back("MOV AX,SS:[BP-" + toString(x.addr1) + "]");
        }
        // Array element
        else if (x.val.find('[') != string::npos) {
            string arrayName, indexStr;
            parseArrayAccess(x.val, arrayName, indexStr);

            if (isNumber(indexStr)) {
                int index = atoi(indexStr.c_str());
                res.push_back("MOV BX,OFFSET " + arrayName);
                res.push_back("MOV AX,WORD PTR DS:[BX+" + toString(index * 2) + "]");
            }
            else {
                res.push_back("MOV BX,OFFSET " + arrayName);
                res.push_back("MOV SI," + indexStr);
                res.push_back("SHL SI,1");
                res.push_back("MOV AX,WORD PTR DS:[BX+SI]");
            }
        }
        // Struct member access
        else if (x.val.find('.') != string::npos) {
            string structVar, member;
            size_t dotPos = x.val.find('.');
            structVar = x.val.substr(0, dotPos);
            member = x.val.substr(dotPos + 1);

            Variable* varInfo = getVariableInfo(structVar, funcTable);
            if (varInfo) {
                // Find struct definition
                Struct* st = NULL;
                for (size_t i = 0; i < symTable->structList.size(); ++i) {
                    if (symTable->structList[i]->structName == varInfo->type) {
                        st = symTable->structList[i];
                        break;
                    }
                }

                if (st) {
                    Variable& memberVar = st->variableDict[member];

                    // If member is array, load first element by default
                    res.push_back("MOV BX,OFFSET " + structVar);
                    res.push_back("MOV AX,WORD PTR DS:[BX+" + toString(memberVar.addr) + "]");
                }
            }
        }
        // Regular variable
        else {
            res.push_back("MOV AX," + x.val);
        }

        return res;
    }

    // Fixed ST_Main to handle struct member assignment correctly
    vector<string> ST_Main(const QtxInfo& x) {
        vector<string> res;

        if (x.val.length() > 0 && x.val[0] == '@') {
            res.push_back("MOV SS:[BP-" + toString(x.addr1) + "],AX");
        }
        else if (x.val.find('[') != string::npos) {
            string arrayName, indexStr;
            parseArrayAccess(x.val, arrayName, indexStr);

            if (isNumber(indexStr)) {
                int index = atoi(indexStr.c_str());
                res.push_back("MOV BX,OFFSET " + arrayName);
                res.push_back("MOV DS:[BX+" + toString(index * 2) + "],AX");
            }
            else {
                res.push_back("MOV BX,OFFSET " + arrayName);
                res.push_back("MOV SI," + indexStr);
                res.push_back("SHL SI,1");
                res.push_back("MOV DS:[BX+SI],AX");
            }
        }
        else if (x.val.find('.') != string::npos) {
            string structVar, member;
            size_t dotPos = x.val.find('.');
            structVar = x.val.substr(0, dotPos);
            member = x.val.substr(dotPos + 1);

            // Simply use struct.member syntax for main function
            res.push_back("MOV " + x.val + ",AX");
        }
        else {
            res.push_back("MOV " + x.val + ",AX");
        }

        return res;
    }

    // Fixed DOP_Main to handle division properly
    vector<string> DOP_Main(const QtxInfo& op, const QtxInfo& operand) {
        vector<string> res;

        // Compare operations
        if (op.val == "<" || op.val == "<=" || op.val == ">" ||
            op.val == ">=" || op.val == "==") {

            if (isNumber(operand.val)) {
                res.push_back("CMP AX," + operand.val);
            }
            else if (operand.val.find('[') != string::npos) {
                string arrayName, indexStr;
                parseArrayAccess(operand.val, arrayName, indexStr);

                if (isNumber(indexStr)) {
                    int index = atoi(indexStr.c_str());
                    res.push_back("MOV BX,OFFSET " + arrayName);
                    res.push_back("CMP AX,WORD PTR DS:[BX+" + toString(index * 2) + "]");
                }
                else {
                    res.push_back("MOV BX,OFFSET " + arrayName);
                    res.push_back("MOV SI," + indexStr);
                    res.push_back("SHL SI,1");
                    res.push_back("CMP AX,WORD PTR DS:[BX+SI]");
                }
            }
            else {
                res.push_back("CMP AX," + operand.val);
            }
            res.push_back(op2asm[op.val] + " ");
        }
        // Add/subtract operations
        else if (op.val == "+" || op.val == "-") {
            if (isNumber(operand.val)) {
                res.push_back(op2asm[op.val] + " AX," + operand.val);
            }
            else if (operand.val.find('[') != string::npos) {
                string arrayName, indexStr;
                parseArrayAccess(operand.val, arrayName, indexStr);

                if (isNumber(indexStr)) {
                    int index = atoi(indexStr.c_str());
                    res.push_back("MOV BX,OFFSET " + arrayName);
                    res.push_back(op2asm[op.val] + " AX,WORD PTR DS:[BX+" + toString(index * 2) + "]");
                }
                else {
                    res.push_back("MOV BX,OFFSET " + arrayName);
                    res.push_back("MOV SI," + indexStr);
                    res.push_back("SHL SI,1");
                    res.push_back(op2asm[op.val] + " AX,WORD PTR DS:[BX+SI]");
                }
            }
            else {
                res.push_back(op2asm[op.val] + " AX," + operand.val);
            }
        }
        // Multiply/divide operations
        else if (op.val == "*" || op.val == "/") {
            // Division requires sign extension
            if (op.val == "/") {
                res.push_back("CWD");  // Sign extend AX to DX:AX
            }

            if (isNumber(operand.val)) {
                res.push_back("MOV BX," + operand.val);
                res.push_back(op2asm[op.val] + " BX");
            }
            else if (operand.val.find('[') != string::npos) {
                string arrayName, indexStr;
                parseArrayAccess(operand.val, arrayName, indexStr);

                if (isNumber(indexStr)) {
                    int index = atoi(indexStr.c_str());
                    res.push_back("MOV BX,OFFSET " + arrayName);
                    res.push_back(op2asm[op.val] + " WORD PTR DS:[BX+" + toString(index * 2) + "]");
                }
                else {
                    res.push_back("MOV BX,OFFSET " + arrayName);
                    res.push_back("MOV SI," + indexStr);
                    res.push_back("SHL SI,1");
                    res.push_back(op2asm[op.val] + " WORD PTR DS:[BX+SI]");
                }
            }
            else {
                res.push_back("MOV BX," + operand.val);
                res.push_back(op2asm[op.val] + " BX");
            }
        }

        return res;
    }

    // Function LD operation
    vector<string> LD_Func(const QtxInfo& x) {
        vector<string> res;

        if (isNumber(x.val)) {
            res.push_back("MOV AX," + x.val);
        }
        else if (x.dosePointer) {
            res.push_back("MOV BX,SS:[BP+" + toString(abs(x.addr1)) + "]");
            res.push_back("MOV AX,DS:[BX+" + toString(x.addr2) + "]");
        }
        else {
            if (x.addr1 < 0) {
                res.push_back("MOV AX,SS:[BP+" + toString(abs(x.addr1)) + "]");
            }
            else {
                res.push_back("MOV AX,SS:[BP-" + toString(x.addr1) + "]");
            }
        }

        return res;
    }

    // Function ST operation
    vector<string> ST_Func(const QtxInfo& x) {
        vector<string> res;

        if (x.dosePointer) {
            res.push_back("MOV BX,SS:[BP+" + toString(abs(x.addr1)) + "]");
            res.push_back("MOV DS:[BX+" + toString(x.addr2) + "],AX");
        }
        else {
            if (x.addr1 < 0) {
                res.push_back("MOV SS:[BP+" + toString(abs(x.addr1)) + "],AX");
            }
            else {
                res.push_back("MOV SS:[BP-" + toString(x.addr1) + "],AX");
            }
        }

        return res;
    }

    vector<string> DOP_Func(const QtxInfo& op, const QtxInfo& operand) {
        vector<string> res;

        // Compare operations
        if (op.val == "<" || op.val == "<=" || op.val == ">" ||
            op.val == ">=" || op.val == "==") {

            if (isNumber(operand.val)) {
                res.push_back("CMP AX," + operand.val);
            }
            else if (operand.dosePointer) {
                res.push_back("MOV BX,SS:[BP+" + toString(abs(operand.addr1)) + "]");
                res.push_back("CMP AX,DS:[BX+" + toString(operand.addr2) + "]");
            }
            else {
                if (operand.addr1 < 0) {
                    res.push_back("CMP AX,SS:[BP+" + toString(abs(operand.addr1)) + "]");
                }
                else {
                    res.push_back("CMP AX,SS:[BP-" + toString(operand.addr1) + "]");
                }
            }
            res.push_back(op2asm[op.val] + " ");
        }
        // Add/subtract operations
        else if (op.val == "+" || op.val == "-") {
            if (isNumber(operand.val)) {
                res.push_back(op2asm[op.val] + " AX," + operand.val);
            }
            else if (operand.dosePointer) {
                res.push_back("MOV BX,SS:[BP+" + toString(abs(operand.addr1)) + "]");
                res.push_back(op2asm[op.val] + " AX,DS:[BX+" + toString(operand.addr2) + "]");
            }
            else {
                if (operand.addr1 < 0) {
                    res.push_back(op2asm[op.val] + " AX,SS:[BP+" + toString(abs(operand.addr1)) + "]");
                }
                else {
                    res.push_back(op2asm[op.val] + " AX,SS:[BP-" + toString(operand.addr1) + "]");
                }
            }
        }
        // Multiply/divide operations
        else if (op.val == "*" || op.val == "/") {
            // For division, use CWD for sign extension
            if (op.val == "/") {
                res.push_back("CWD");  // Sign extend AX to DX:AX
            }

            if (isNumber(operand.val)) {
                res.push_back("MOV BX," + operand.val);
                res.push_back(op2asm[op.val] + " BX");
            }
            else if (operand.dosePointer) {
                res.push_back("MOV BX,SS:[BP+" + toString(abs(operand.addr1)) + "]");
                res.push_back(op2asm[op.val] + " WORD PTR DS:[BX+" + toString(operand.addr2) + "]");
            }
            else {
                if (operand.addr1 < 0) {
                    res.push_back(op2asm[op.val] + " WORD PTR SS:[BP+" + toString(abs(operand.addr1)) + "]");
                }
                else {
                    res.push_back(op2asm[op.val] + " WORD PTR SS:[BP-" + toString(operand.addr1) + "]");
                }
            }
        }

        return res;
    }

    // Fixed helper function with corrected address calculation
    QtxInfo helper(const QtxInfo& x, const string& funcName, Function* funcTable,
        const map<string, bool>& actTable, const map<string, int>& t_table) {
        if (x.val == "_" || isNumber(x.val) ||
            find(symTable->functionNameList.begin(), symTable->functionNameList.end(), x.val) != symTable->functionNameList.end()) {
            return QtxInfo(x.val, false, 0, 0, false);
        }

        QtxInfo res;
        res.val = x.val;

        // Get activity info
        map<string, bool>::const_iterator actIt = actTable.find(x.val);
        res.actInfo = (actIt != actTable.end()) ? actIt->second : false;

        // Temporary variable
        if (x.val.length() > 0 && x.val[0] == '@') {
            map<string, int>::const_iterator tIt = t_table.find(x.val);
            res.addr1 = (tIt != t_table.end()) ? tIt->second + 2 : 0;
            res.addr2 = 0;
            res.dosePointer = false;
        }
        // Main function variables
        else if (funcName == "main") {
            res.addr1 = 0;
            res.addr2 = 0;
            res.dosePointer = false;
        }
        // Function variables
        else {
            // Handle struct members
            if (x.val.find('.') != string::npos) {
                string structVar, member;
                size_t dotPos = x.val.find('.');
                structVar = x.val.substr(0, dotPos);
                member = x.val.substr(dotPos + 1);

                Variable& varInfo = funcTable->variableDict[structVar];
                int addr1 = varInfo.addr;

                // Find struct in symbol table
                Struct* st = NULL;
                for (size_t i = 0; i < symTable->structList.size(); ++i) {
                    if (symTable->structList[i]->structName == varInfo.type) {
                        st = symTable->structList[i];
                        break;
                    }
                }

                if (st) {
                    int addr2 = st->variableDict[member].addr;

                    if (addr1 < 0) {
                        res.addr1 = addr1 - 2;
                        res.addr2 = addr2;
                        res.dosePointer = true;
                    }
                    else {
                        res.addr1 = addr1 + 2;
                        res.addr2 = addr2;
                        res.dosePointer = false;
                    }
                }
            }
            // Normal variable
            else if (funcTable->variableDict.find(x.val) != funcTable->variableDict.end()) {
                Variable& varInfo = funcTable->variableDict[x.val];
                int addr1 = varInfo.addr;

                if (addr1 < 0) {
                    res.addr1 = addr1 - 2;
                }
                else {
                    res.addr1 = addr1 + 2;
                }
                res.addr2 = 0;
                res.dosePointer = false;
            }
        }

        return res;
    }
    // Improved liveness analysis that properly tracks variable usage
    void actFunInfoGen(vector<QuaternionExt>& bloc, const string& funcName) {
        map<string, bool> actTable;

        // Find function symbol table
        Function* funcTable = NULL;
        for (size_t i = 0; i < symTable->functionList.size(); ++i) {
            if (symTable->functionList[i]->functionName == funcName) {
                funcTable = symTable->functionList[i];
                break;
            }
        }

        if (!funcTable) return;

        int maxSizeOfFunc = funcTable->totalSize;

        if (funcName == "main") {
            maxSizeOfFunc = 0;
        }

        map<string, int> t_table; // Temporary variable stack address storage

        // First pass: collect all variables and initialize
        for (size_t i = 0; i < bloc.size(); ++i) {
            QuaternionExt& q = bloc[i];

            // Skip control flow operations
            if (q.op.val == "do" || q.op.val == "if" || q.op.val == "elif" ||
                q.op.val == "wh" || q.op.val == "we" || q.op.val == "el" ||
                q.op.val == "ie" || q.op.val == "continue") {
                continue;
            }

            // Process all variables in the quaternion
            string vars[3] = { q.arg1.val, q.arg2.val, q.result.val };

            for (int k = 0; k < 3; k++) {
                const string& var = vars[k];
                if (var != "_" && !isNumber(var) && actTable.find(var) == actTable.end()) {
                    if (var.length() > 0 && var[0] == '@') {
                        // Temporary variable
                        actTable[var] = false;  // Initially inactive
                        if (t_table.find(var) == t_table.end()) {
                            t_table[var] = maxSizeOfFunc;
                            maxSizeOfFunc += 2;
                        }
                    }
                    else if (find(symTable->functionNameList.begin(),
                        symTable->functionNameList.end(), var) == symTable->functionNameList.end()) {
                        // Non-temporary variable - initially inactive
                        actTable[var] = false;
                    }
                }
            }
        }

        // Backward liveness analysis
        set<string> liveVars;

        // At basic block exit, non-temporary variables are live
        for (map<string, bool>::iterator it = actTable.begin(); it != actTable.end(); ++it) {
            if (it->first[0] != '@') {
                liveVars.insert(it->first);
                it->second = true;
            }
        }

        // Analyze from bottom to top
        for (int i = bloc.size() - 1; i >= 0; --i) {
            QuaternionExt& q = bloc[i];

            if (q.op.val == "do" || q.op.val == "if" || q.op.val == "elif" ||
                q.op.val == "wh" || q.op.val == "we" || q.op.val == "el" ||
                q.op.val == "ie" || q.op.val == "continue") {
                continue;
            }

            // Handle result (definition)
            if (q.result.val != "_" && !isNumber(q.result.val)) {
                // Set activity based on whether it's in live set
                if (liveVars.find(q.result.val) != liveVars.end()) {
                    actTable[q.result.val] = true;
                    q.result.actInfo = true;
                }
                else {
                    q.result.actInfo = false;
                }

                // Arrays and struct members always active
                if (q.result.val.find('[') != string::npos ||
                    q.result.val.find('.') != string::npos) {
                    actTable[q.result.val] = true;
                    q.result.actInfo = true;
                }

                // Remove from live set (killed by definition)
                liveVars.erase(q.result.val);
            }

            // Handle uses (arg1 and arg2)
            if (q.arg1.val != "_" && !isNumber(q.arg1.val)) {
                liveVars.insert(q.arg1.val);
                actTable[q.arg1.val] = true;
            }

            if (q.arg2.val != "_" && !isNumber(q.arg2.val)) {
                liveVars.insert(q.arg2.val);
                actTable[q.arg2.val] = true;
            }
        }

        // Apply helper function
        for (size_t i = 0; i < bloc.size(); ++i) {
            QuaternionExt& q = bloc[i];
            q.result = helper(q.result, funcName, funcTable, actTable, t_table);
            q.arg2 = helper(q.arg2, funcName, funcTable, actTable, t_table);
            q.arg1 = helper(q.arg1, funcName, funcTable, actTable, t_table);
            q.op = QtxInfo(q.op.val, false, 0, 0, false);
        }
    }


    // Check if next instruction is a basic block exit
    bool isBasicBlockExit(const vector<QuaternionExt>& bloc, size_t index) {
        if (index >= bloc.size() - 1) return true;  // Last instruction in block
        
        const string& nextOp = bloc[index + 1].op.val;
        return (nextOp == "if" || nextOp == "elif" || nextOp == "el" || 
                nextOp == "ie" || nextOp == "wh" || nextOp == "we" || 
                nextOp == "do" || nextOp == "continue" || nextOp == "return");
    }

    // Store active variables at basic block exit
    void storeActiveVarsAtExit(QtxInfo* RDL, vector<string>& codes) {
        if (RDL && RDL->actInfo && RDL->val != "_" && RDL->val[0] != '@') {
            vector<string> stCodes = ST_Main(*RDL);
            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
        }
    }

    // Improved genMainAsm with better register and activity tracking
    vector<vector<string> > genMainAsm(const vector<vector<QuaternionExt> >& funcBlock, const string& funcName) {
        Function* funcTable = NULL;
        for (size_t i = 0; i < symTable->functionList.size(); ++i) {
            if (symTable->functionList[i]->functionName == funcName) {
                funcTable = symTable->functionList[i];
                break;
            }
        }

        vector<vector<string> > asmCode;

        // Control flow labels
        vector<string> startOfWhile;
        vector<pair<int, int> > judgeOfWhile;
        vector<pair<int, int> > judgeOfIf;
        vector<pair<int, int> > jmpToEnd;

        for (size_t i = 0; i < funcBlock.size(); ++i) {
            const vector<QuaternionExt>& bloc = funcBlock[i];
            QtxInfo* RDL = NULL;  // Register descriptor
            vector<string> codes;

            for (size_t j = 0; j < bloc.size(); ++j) {
                const QuaternionExt& item = bloc[j];

                // Operator processing
                if (op2asm.find(item.op.val) != op2asm.end()) {
                    // Load first operand if needed
                    if (RDL == NULL || RDL->val != item.arg1.val) {
                        // Store current register value if active
                        if (RDL && RDL->actInfo) {
                            vector<string> stCodes = ST_Main(*RDL);
                            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                        }

                        vector<string> ldCodes = LD_Main(item.arg1, funcTable);
                        codes.insert(codes.end(), ldCodes.begin(), ldCodes.end());
                        RDL = NULL;
                    }

                    // Perform operation
                    vector<string> dopCodes = DOP_Main(item.op, item.arg2);
                    codes.insert(codes.end(), dopCodes.begin(), dopCodes.end());

                    // For comparisons, handle jumps
                    if (item.op.val == "<" || item.op.val == "<=" || item.op.val == ">" ||
                        item.op.val == ">=" || item.op.val == "==") {
                        if (!codes.empty() && codes[0].find("while") == 0) {
                            judgeOfWhile.push_back(make_pair(asmCode.size(), codes.size() - 1));
                        }
                        else {
                            judgeOfIf.push_back(make_pair(asmCode.size(), codes.size() - 1));
                        }
                        RDL = NULL;  // Comparison doesn't leave value in register
                    }
                    else {
                        // Result is in AX
                        RDL = const_cast<QtxInfo*>(&item.result);

                        // Store immediately if active
                        if (RDL->actInfo) {
                            vector<string> stCodes = ST_Main(*RDL);
                            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                            RDL = NULL;  // No longer in register after store
                        }
                    }
                }
                // Assignment operation
                else if (item.op.val == "=") {
                    // Load source if needed
                    if (RDL == NULL || RDL->val != item.arg1.val) {
                        if (RDL && RDL->actInfo) {
                            vector<string> stCodes = ST_Main(*RDL);
                            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                        }
                        vector<string> ldCodes = LD_Main(item.arg1, funcTable);
                        codes.insert(codes.end(), ldCodes.begin(), ldCodes.end());
                    }

                    // Always store if active or if it's array/struct
                    if (item.result.actInfo ||
                        item.result.val.find('[') != string::npos ||
                        item.result.val.find('.') != string::npos) {
                        vector<string> stCodes = ST_Main(item.result);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                        RDL = NULL;
                    }
                    else {
                        RDL = const_cast<QtxInfo*>(&item.result);
                    }
                }
                // Control flow operations
                else if (item.op.val == "continue") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Main(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("JMP " + startOfWhile.back());
                    RDL = NULL;
                }
                else if (item.op.val == "el") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Main(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("JMP ");
                    jmpToEnd.push_back(make_pair(asmCode.size(), codes.size() - 1));
                    codes.push_back("next" + toString(id) + ":");
                    pair<int, int> p = judgeOfIf.back();
                    judgeOfIf.pop_back();
                    asmCode[p.first][p.second] += "next" + toString(id);
                    id++;
                    RDL = NULL;
                }
                else if (item.op.val == "elif") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Main(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    RDL = NULL;
                }
                else if (item.op.val == "ie") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Main(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("endif" + toString(id) + ":");
                    while (!jmpToEnd.empty()) {
                        pair<int, int> p = jmpToEnd.back();
                        jmpToEnd.pop_back();
                        asmCode[p.first][p.second] += "endif" + toString(id);
                    }
                    id++;
                    RDL = NULL;
                }
                else if (item.op.val == "wh") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Main(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("while" + toString(id) + ":");
                    startOfWhile.push_back("while" + toString(id));
                    id++;
                    RDL = NULL;
                }
                else if (item.op.val == "we") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Main(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    string tgt = startOfWhile.back();
                    startOfWhile.pop_back();
                    codes.push_back("JMP " + tgt);
                    codes.push_back("endWhile" + toString(id) + ":");
                    pair<int, int> p = judgeOfWhile.back();
                    judgeOfWhile.pop_back();
                    asmCode[p.first][p.second] += "endWhile" + toString(id);
                    id++;
                    RDL = NULL;
                }
                else if (item.op.val == "push") {
                    if (RDL == NULL || RDL->val != item.arg1.val) {
                        if (RDL && RDL->actInfo) {
                            vector<string> stCodes = ST_Main(*RDL);
                            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                        }
                        vector<string> ldCodes = LD_Main(item.arg1, funcTable);
                        codes.insert(codes.end(), ldCodes.begin(), ldCodes.end());
                    }
                    codes.push_back("PUSH AX");
                    RDL = NULL;
                }
                else if (item.op.val == "call") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Main(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("CALL " + item.arg1.val);
                    RDL = NULL;
                }
                else if (item.op.val == "callr") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Main(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("CALL " + item.arg1.val);
                    RDL = const_cast<QtxInfo*>(&item.result);
                }
                else if (item.op.val == "FUN") {
                    codes.push_back(item.arg1.val + ":");
                    codes.push_back("MOV BP,SP");
                    codes.push_back("SUB SP,20");
                    codes.push_back("MOV AX,DSEG");
                    codes.push_back("MOV DS,AX");
                }
                else if (item.op.val == "return") {
                    if (RDL && RDL->val != item.arg1.val) {
                        if (RDL && RDL->actInfo) {
                            vector<string> stCodes = ST_Main(*RDL);
                            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                        }
                        if (item.arg1.val != "0") {
                            vector<string> ldCodes = LD_Main(item.arg1, funcTable);
                            codes.insert(codes.end(), ldCodes.begin(), ldCodes.end());
                        }
                    }
                    codes.push_back("MOV SP,BP");
                    codes.push_back("MOV AX,4C00H");
                    codes.push_back("INT 21H");
                    RDL = NULL;
                }
            }

            // Store any active value at end of block
            if (RDL && RDL->actInfo) {
                vector<string> stCodes = ST_Main(*RDL);
                codes.insert(codes.end(), stCodes.begin(), stCodes.end());
            }

            asmCode.push_back(codes);
        }

        return asmCode;
    }

    // Fixed Function code generation with proper parameter addressing
    vector<vector<string> > genFunAsm(const vector<vector<QuaternionExt> >& funcBlock, const string& funcName) {
        vector<vector<string> > asmCode;

        // Control flow labels
        vector<string> startOfWhile;
        vector<pair<int, int> > judgeOfWhile;
        vector<pair<int, int> > judgeOfIf;
        vector<pair<int, int> > jmpToEnd;

        for (size_t i = 0; i < funcBlock.size(); ++i) {
            const vector<QuaternionExt>& bloc = funcBlock[i];
            QtxInfo* RDL = NULL;
            vector<string> codes;

            for (size_t j = 0; j < bloc.size(); ++j) {
                const QuaternionExt& item = bloc[j];

                // Operator processing
                if (op2asm.find(item.op.val) != op2asm.end()) {
                    // Load first operand if needed
                    if (RDL == NULL || RDL->val != item.arg1.val) {
                        if (RDL && RDL->actInfo) {
                            vector<string> stCodes = ST_Func(*RDL);
                            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                        }
                        vector<string> ldCodes = LD_Func(item.arg1);
                        codes.insert(codes.end(), ldCodes.begin(), ldCodes.end());
                        RDL = NULL;
                    }

                    // Perform operation
                    vector<string> dopCodes = DOP_Func(item.op, item.arg2);
                    codes.insert(codes.end(), dopCodes.begin(), dopCodes.end());

                    // Handle comparison or arithmetic result
                    if (item.op.val == "<" || item.op.val == "<=" || item.op.val == ">" ||
                        item.op.val == ">=" || item.op.val == "==") {
                        if (!codes.empty() && codes[0].find("while") == 0) {
                            judgeOfWhile.push_back(make_pair(asmCode.size(), codes.size() - 1));
                        }
                        else {
                            judgeOfIf.push_back(make_pair(asmCode.size(), codes.size() - 1));
                        }
                        RDL = NULL;
                    }
                    else {
                        RDL = const_cast<QtxInfo*>(&item.result);
                        // Store immediately if active
                        if (RDL->actInfo) {
                            vector<string> stCodes = ST_Func(*RDL);
                            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                            RDL = NULL;
                        }
                    }
                }
                // Assignment operation
                else if (item.op.val == "=") {
                    if (RDL == NULL || RDL->val != item.arg1.val) {
                        if (RDL && RDL->actInfo) {
                            vector<string> stCodes = ST_Func(*RDL);
                            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                        }
                        vector<string> ldCodes = LD_Func(item.arg1);
                        codes.insert(codes.end(), ldCodes.begin(), ldCodes.end());
                    }

                    // Always store if active
                    if (item.result.actInfo || item.result.dosePointer) {
                        vector<string> stCodes = ST_Func(item.result);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                        RDL = NULL;
                    }
                    else {
                        RDL = const_cast<QtxInfo*>(&item.result);
                    }
                }
                // Function start
                else if (item.op.val == "FUN") {
                    codes.push_back(item.arg1.val + " PROC NEAR");
                    codes.push_back("PUSH BP");
                    codes.push_back("MOV BP,SP");
                    codes.push_back("SUB SP,20");
                }
                // Return statement
                else if (item.op.val == "return") {
                    if (RDL && RDL->val != item.arg1.val) {
                        if (RDL && RDL->actInfo) {
                            vector<string> stCodes = ST_Func(*RDL);
                            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                        }
                        vector<string> ldCodes = LD_Func(item.arg1);
                        codes.insert(codes.end(), ldCodes.begin(), ldCodes.end());
                    }
                    else if (RDL == NULL) {
                        vector<string> ldCodes = LD_Func(item.arg1);
                        codes.insert(codes.end(), ldCodes.begin(), ldCodes.end());
                    }

                    codes.push_back("MOV SP,BP");
                    codes.push_back("POP BP");

                    // Get parameter count
                    Function* func = NULL;
                    for (size_t i = 0; i < symTable->functionList.size(); ++i) {
                        if (symTable->functionList[i]->functionName == funcName) {
                            func = symTable->functionList[i];
                            break;
                        }
                    }

                    int numOfPar = func ? func->numOfParameters : 0;
                    codes.push_back("RET " + toString(numOfPar * 2));
                    codes.push_back(funcName + " ENDP");
                }
                // Continue statement
                else if (item.op.val == "continue") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Func(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("JMP " + startOfWhile.back());
                    RDL = NULL;
                }
                // Control flow operations
                else if (item.op.val == "el") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Func(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("JMP ");
                    jmpToEnd.push_back(make_pair(asmCode.size(), codes.size() - 1));
                    codes.push_back("next" + toString(id) + ":");
                    pair<int, int> p = judgeOfIf.back();
                    judgeOfIf.pop_back();
                    asmCode[p.first][p.second] += "next" + toString(id);
                    id++;
                    RDL = NULL;
                }
                else if (item.op.val == "elif") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Func(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    RDL = NULL;
                }
                else if (item.op.val == "ie") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Func(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("endif" + toString(id) + ":");
                    while (!jmpToEnd.empty()) {
                        pair<int, int> p = jmpToEnd.back();
                        jmpToEnd.pop_back();
                        asmCode[p.first][p.second] += "endif" + toString(id);
                    }
                    id++;
                    RDL = NULL;
                }
                else if (item.op.val == "wh") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Func(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("while" + toString(id) + ":");
                    startOfWhile.push_back("while" + toString(id));
                    id++;
                    RDL = NULL;
                }
                else if (item.op.val == "we") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Func(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    string tgt = startOfWhile.back();
                    startOfWhile.pop_back();
                    codes.push_back("JMP " + tgt);
                    codes.push_back("endWhile" + toString(id) + ":");
                    pair<int, int> p = judgeOfWhile.back();
                    judgeOfWhile.pop_back();
                    asmCode[p.first][p.second] += "endWhile" + toString(id);
                    id++;
                    RDL = NULL;
                }
                else if (item.op.val == "push") {
                    if (RDL == NULL || RDL->val != item.arg1.val) {
                        if (RDL && RDL->actInfo) {
                            vector<string> stCodes = ST_Func(*RDL);
                            codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                        }
                        vector<string> ldCodes = LD_Func(item.arg1);
                        codes.insert(codes.end(), ldCodes.begin(), ldCodes.end());
                    }
                    codes.push_back("PUSH AX");
                    RDL = NULL;
                }
                else if (item.op.val == "call") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Func(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("CALL " + item.arg1.val);
                    RDL = NULL;
                }
                else if (item.op.val == "callr") {
                    if (RDL && RDL->actInfo) {
                        vector<string> stCodes = ST_Func(*RDL);
                        codes.insert(codes.end(), stCodes.begin(), stCodes.end());
                    }
                    codes.push_back("CALL " + item.arg1.val);
                    RDL = const_cast<QtxInfo*>(&item.result);
                }
            }

            // Store active value at end of block
            if (RDL && RDL->actInfo) {
                vector<string> stCodes = ST_Func(*RDL);
                codes.insert(codes.end(), stCodes.begin(), stCodes.end());
            }

            asmCode.push_back(codes);
        }

        return asmCode;
    }


    // Process function block
    void getAsm(const vector<vector<Quaternion> >& funcBlock) {
        if (funcBlock.empty() || funcBlock[0].empty()) return;

        // Convert to extended quaternion format
        vector<vector<QuaternionExt> > extFuncBlock;
        for (size_t i = 0; i < funcBlock.size(); ++i) {
            vector<QuaternionExt> extBloc;
            for (size_t j = 0; j < funcBlock[i].size(); ++j) {
                extBloc.push_back(QuaternionExt(funcBlock[i][j]));
            }
            extFuncBlock.push_back(extBloc);
        }

        string funcName = funcBlock[0][0].arg1;

        if (funcName != "main") {
            // Add activity info for each basic block
            for (size_t i = 0; i < extFuncBlock.size(); ++i) {
                actFunInfoGen(extFuncBlock[i], funcName);
            }
            vector<vector<string> > res = genFunAsm(extFuncBlock, funcName);
            funcsAsmCode.push_back(res);
        }
        else {
            // Main function processing
            for (size_t i = 0; i < extFuncBlock.size(); ++i) {
                actFunInfoGen(extFuncBlock[i], funcName);
            }
            vector<vector<string> > res = genMainAsm(extFuncBlock, funcName);
            mainAsmCode.push_back(res);
        }
    }

    // Generate complete assembly program - Fixed struct order
    void getAll() {
        // Generate struct definitions with correct member order
        for (size_t i = 0; i < symTable->structList.size(); ++i) {
            Struct* st = symTable->structList[i];
            allAsmCode.push_back(st->structName + " STRUCT");

            // Create a vector to sort members by address
            vector<pair<int, Variable*> > sortedMembers;
            for (map<string, Variable>::iterator it = st->variableDict.begin();
                it != st->variableDict.end(); ++it) {
                sortedMembers.push_back(make_pair(it->second.addr, &(it->second)));
            }

            // Sort by address to maintain declaration order
            sort(sortedMembers.begin(), sortedMembers.end());

            // Generate members in correct order
            for (size_t j = 0; j < sortedMembers.size(); ++j) {
                Variable* v = sortedMembers[j].second;

                if (v->isArray) {
                    string sizeSpec = "";
                    if (v->type == "int") sizeSpec = "dw";
                    else if (v->type == "float") sizeSpec = "dd";
                    else if (v->type == "char") sizeSpec = "db";
                    else sizeSpec = "dw";

                    allAsmCode.push_back(v->name + " " + sizeSpec + " " +
                        toString(v->arraySize) + " DUP (0)");
                }
                else {
                    if (v->type == "int") {
                        allAsmCode.push_back(v->name + " dw ?");
                    }
                    else if (v->type == "float") {
                        allAsmCode.push_back(v->name + " dd ?");
                    }
                    else if (v->type == "char") {
                        allAsmCode.push_back(v->name + " db ?");
                    }
                    else {
                        allAsmCode.push_back(v->name + " " + v->type + " <?>");
                    }
                }
            }

            allAsmCode.push_back(st->structName + " ENDS");
        }

        // Generate data segment
        allAsmCode.push_back("DSEG SEGMENT");

        // Find main function
        Function* mainFunc = NULL;
        for (size_t i = 0; i < symTable->functionList.size(); ++i) {
            if (symTable->functionList[i]->functionName == "main") {
                mainFunc = symTable->functionList[i];
                break;
            }
        }

        if (mainFunc) {
            // Sort variables by address to maintain declaration order
            vector<pair<int, Variable*> > sortedVars;
            for (map<string, Variable>::iterator it = mainFunc->variableDict.begin();
                it != mainFunc->variableDict.end(); ++it) {
                sortedVars.push_back(make_pair(it->second.addr, &(it->second)));
            }
            sort(sortedVars.begin(), sortedVars.end());

            for (size_t i = 0; i < sortedVars.size(); ++i) {
                Variable* v = sortedVars[i].second;

                if (v->type == "int" && !v->isArray) {
                    allAsmCode.push_back(v->name + " dw ?");
                }
                else if (v->type == "float" && !v->isArray) {
                    allAsmCode.push_back(v->name + " dd ?");
                }
                else if (v->type == "char" && !v->isArray) {
                    allAsmCode.push_back(v->name + " db ?");
                }
                else if (v->isArray) {
                    string sizeSpec = "";
                    if (v->type == "int") sizeSpec = "dw";
                    else if (v->type == "float") sizeSpec = "dd";
                    else if (v->type == "char") sizeSpec = "db";
                    else sizeSpec = "db";

                    allAsmCode.push_back(v->name + " " + sizeSpec + " " +
                        toString(v->arraySize) + " DUP (0)");
                }
                else {
                    // Struct variable
                    allAsmCode.push_back(v->name + " " + v->type + " <?>");
                }
            }
        }

        allAsmCode.push_back("DSEG ENDS");

        // Generate stack segment
        allAsmCode.push_back("SSEG SEGMENT STACK");
        allAsmCode.push_back("STK DB 40 DUP (0)");
        allAsmCode.push_back("SSEG ENDS");

        // Generate code segment
        allAsmCode.push_back("CSEG SEGMENT");
        allAsmCode.push_back("ASSUME CS:CSEG,DS:DSEG,SS:SSEG");

        // Add main function code first
        for (size_t i = 0; i < mainAsmCode.size(); ++i) {
            for (size_t j = 0; j < mainAsmCode[i].size(); ++j) {
                for (size_t k = 0; k < mainAsmCode[i][j].size(); ++k) {
                    allAsmCode.push_back(mainAsmCode[i][j][k]);
                }
            }
        }

        // Add other function code BEFORE closing the segment
        for (size_t i = 0; i < funcsAsmCode.size(); ++i) {
            for (size_t j = 0; j < funcsAsmCode[i].size(); ++j) {
                for (size_t k = 0; k < funcsAsmCode[i][j].size(); ++k) {
                    allAsmCode.push_back(funcsAsmCode[i][j][k]);
                }
            }
        }

        // Close code segment properly
        allAsmCode.push_back("CSEG ENDS");
        allAsmCode.push_back("END main");
    }

public:
    // Constructor
    AsmCodeGen(SYMBOL* sym, const vector<vector<vector<Quaternion> > >& code)
        : symTable(sym), allCode(code), id(0) {

        // Initialize operator mapping
        op2asm["+"] = "ADD";
        op2asm["-"] = "SUB";
        op2asm["*"] = "MUL";
        op2asm["/"] = "DIV";
        op2asm["<"] = "JAE";
        op2asm[">"] = "JBE";
        op2asm[">="] = "JB";
        op2asm["<="] = "JA";
        op2asm["=="] = "JNE";

        // Generate assembly code for all functions
        for (size_t i = 0; i < allCode.size(); ++i) {
            getAsm(allCode[i]);
        }

        // Generate complete assembly program
        getAll();
    }

    // Get generated assembly code
    vector<string> getGeneratedCode() const {
        return allAsmCode;
    }

    // Print generated assembly code
    void printAsmCode() const {
        cout << "\n=== Generated 8086 Assembly Code ===" << endl;
        for (size_t i = 0; i < allAsmCode.size(); ++i) {
            cout << allAsmCode[i] << endl;
        }
    }

    // Write assembly code to file
    void writeToFile(const string& filename) const {
        ofstream outFile(filename.c_str());
        if (outFile.is_open()) {
            for (size_t i = 0; i < allAsmCode.size(); ++i) {
                outFile << allAsmCode[i] << endl;
            }
            outFile.close();
            cout << "[DEBUG] Assembly code written to: " << filename << endl;
        }
        else {
            cout << "[ERROR] Could not create output file: " << filename << endl;
        }
    }
};

int main() {
    cout << "=== C++ Compiler Frontend with Quaternion Generation, DAG Optimization and Target Code Generation ===" << endl;

    // 读取输入文件
    string filename = "C://Users/汤/Desktop/c_input.txt";
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

   
        // 创建LL1分析器（用于语法分析）
        LL1 ll1("C://Users/汤/Desktop/Project2/c_like_grammar.txt", true);

        // 处理输入
        ll1.getInput(INPUT);

        // 进行语法和语义分析
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

        // 打印符号表
        cout << "\n=== Symbol Table ===" << endl;
        ll1.printSymbolTable();

        // 生成四元式
        cout << "\n[DEBUG] Starting quaternion generation..." << endl;

        // 创建QtGen，使用翻译文法
        QtGen qtGen(ll1.getSymbolTable(),
            "C://Users/汤/Desktop/Project2/transalation_grammar.txt");

        // 获取函数块列表
        vector<vector<Token> > funcBlocks = ll1.getFuncBlocks();

        // 为每个函数块生成四元式
        for (size_t i = 0; i < funcBlocks.size(); ++i) {
            cout << "\n[DEBUG] Generating quaternions for function " << i << endl;
            qtGen.genQt(funcBlocks[i]);
        }

        // 打印优化前的四元式
        cout << "\n=== Quaternions (Before Optimization) ===" << endl;
        qtGen.printQuaternions();

        // DAG优化
        cout << "\n[DEBUG] Starting DAG optimization..." << endl;

        // 创建优化器
        Optimization optimizer(ll1.getSymbolTable());
        cout << "\n=== Symbol Table ===" << endl;
        // 打印符号表
        cout << "\n=== Symbol Table ===" << endl;
        ll1.printSymbolTable();
        // 获取原始四元式
        vector<vector<Quaternion> > originalQt = qtGen.getQtRes();

        // 对每个函数的四元式进行优化
        vector<vector<vector<Quaternion> > > optimizedQt;
        for (size_t i = 0; i < originalQt.size(); ++i) {
            cout << "\n[DEBUG] Optimizing quaternions for function " << i << endl;
            // 优化当前函数的四元式
            vector<vector<Quaternion> > optimizedBlocks = optimizer.opt(originalQt[i]);
            optimizedQt.push_back(optimizedBlocks);
        }

        // 打印优化后的四元式
        cout << "\n=== Quaternions (After DAG Optimization) ===" << endl;
        for (size_t i = 0; i < optimizedQt.size(); ++i) {
            cout << "Function " << i << ":" << endl;
            int qtIndex = 0;
            for (size_t j = 0; j < optimizedQt[i].size(); ++j) {
                cout << "  Basic Block " << j << ":" << endl;
                for (size_t k = 0; k < optimizedQt[i][j].size(); ++k) {
                    const Quaternion& q = optimizedQt[i][j][k];
                    cout << "    [" << qtIndex++ << "] (" << q.op << ", "
                        << q.arg1 << ", " << q.arg2 << ", "
                        << q.result << ")" << endl;
                }
            }
        }

        // 统计优化效果
        cout << "\n=== Optimization Statistics ===" << endl;
        int originalCount = 0;
        int optimizedCount = 0;

        for (size_t i = 0; i < originalQt.size(); ++i) {
            originalCount += originalQt[i].size();
        }

        for (size_t i = 0; i < optimizedQt.size(); ++i) {
            for (size_t j = 0; j < optimizedQt[i].size(); ++j) {
                optimizedCount += optimizedQt[i][j].size();
            }
        }

        cout << "Original quaternion count: " << originalCount << endl;
        cout << "Optimized quaternion count: " << optimizedCount << endl;
        cout << "Reduction: " << (originalCount - optimizedCount)
            << " (" << (100.0 * (originalCount - optimizedCount) / originalCount)
            << "%)" << endl;
        cout << "\n[DEBUG] Starting target code generation..." << endl;

        // 创建目标代码生成器
        AsmCodeGen asmGen(ll1.getSymbolTable(), optimizedQt);

        // 打印生成的汇编代码
        asmGen.printAsmCode();

        // 将汇编代码写入文件
        string asmFilename = "C://Users/汤/Desktop/output.asm";
        asmGen.writeToFile(asmFilename);
        return 0;
 }
 