#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>

using namespace std;

typedef struct matrix{
    map<string, map<string, double>> confusion_matrix;
}Matrix;

typedef struct res{
    string name;                    //属性名
    double value = 0;
    double split_point = 0;         //连续数据分割点
    map<string,double> split_atrr;  //离散数据分割点

}Res;

typedef enum atrrkind{
    DISCRETE,       //离散值
    CONTINUOUS      //连续值
}Kind;

typedef struct node{//决策树节点
    string name;    //分割属性名称
    double split_point;//离散值分割点值
    map<string, struct node*> next;//分割集（被分割成的不同部分）
}Node;

map<string, Kind> atrr_type;//每个属性的值的类型（离散、连续）
map<string, set<string>> atrr_value;//每个属性的取值（离散），连续值只有smaller和bigger两种
string class_name;      //类名

void replaceCommaWithBlank(string &line){
    for (string::size_type i = 0; i < line.size(); ++i) {
        if(line.at(i) == ','){
            line[i] = ' ';
        }
    }
}

bool isNum(string str){
    double b;
    char c;
    stringstream sin(str);
    if(!(sin >> b))
        return false;
    if(sin >> c)
        return false;
    else
        return true;
}

void initAtrributeType(vector<map<string,string>> &sheet){
    auto sheet_b = sheet.begin();
    for(auto &atrr : (*sheet_b)){
        if(isNum(atrr.second))
            atrr_type[atrr.first] = CONTINUOUS;
        else
            atrr_type[atrr.first] = DISCRETE;
    }
}

void initAtrributeValue(vector<map<string, string>> &sheet){
    for(auto &row : sheet){
        for(auto &col : row){
            if(atrr_type[col.first] == DISCRETE){
                atrr_value[col.first].insert(col.second);
            }
        }
    }
}

/*
 * Function: getInfoEntropy
 * Description: 计算信息熵
 * Calls:
 * Called By:
 * Input: vector<map<string, string>>数据集， 类名(全局变量)
 * Returns: double 信息熵
 */
double getInfoEntropy(vector<map<string,string>> &sheet){
    //问题：sheet加上const不能对map用下标寻址(Subscripted value is not an array)
    map<string, double> type;
    double sum = 0;
    for(auto &row : sheet){
        ++type[row[class_name]];
        ++sum;
    }

    double info_entropy = 0;
    for(auto &num : type){
        double p = num.second / sum;
        info_entropy += -p * (log2(p));
    }
    return info_entropy;
}

/*
 * Function: getDiscreteConEntropy
 * Description: 对离散属性计算条件熵
 * Calls:
 * Called By:
 * Input: vector<map<string, string>>数据集， 离散属性名， 类名(全局变量)
 * Returns: Res {value:条件熵 split_atrr:分割属性}
 */
Res getDiscreteConEntropy(vector<map<string,string>> &sheet, string atrr_name){
    map<string, double> type_times;
    map<string, double> type_entropy;
    vector<map<string,string>> each_atrr;
    map<string, string> temp;
    double sum = 0;
    double conditional_entropy = 0;
    Res result;

    for(auto &row : sheet){
        ++type_times[row[atrr_name]];
        ++sum;
    }

    //对每个分割属性，存储对应的类，并计算对应信息熵
    for(auto &type_name : type_times){
        string name = type_name.first;
        for(auto &row : sheet){
            if(row[atrr_name] == name){
                temp[class_name] = row[class_name];
                each_atrr.push_back(temp);
            }
        }

        double info_entropy = getInfoEntropy(each_atrr);
        type_entropy[name] = info_entropy;

        each_atrr.clear();
    }

    for(auto &num : type_times){
        double p = num.second / sum;
        conditional_entropy += p * type_entropy[num.first];
    }

    result.split_atrr = type_times;
    result.value = conditional_entropy;

    return result;
}


/*
 * Function: getContinConEntropy
 * Description: 对连续属性计算条件熵
 * Calls:
 * Called By:
 * Input: vector<map<string, string>>数据集， 连续属性名， 数据范围百分比(0.1~0.9)， 类名(全局变量)
 * Returns: Res {value:条件熵 split_point:分割点}
 */
Res getContinConEntropy(vector<map<string,string>> &sheet, string atrr_name, double percent){
    vector<double> d_data;
    double smaller_num = 0, bigger_num = 0;
    double smaller_entropy = 0, bigger_entropy = 0;
    vector<map<string,string>> each_type;
    map<string, string> temp;
    double sum;
    Res result;

    for(auto &row : sheet){
        string v = row[atrr_name];
        double d = stod(v);
        d_data.push_back(d);
    }

    sum = d_data.size();

    auto max = max_element(d_data.begin(),d_data.end());
    auto min = min_element(d_data.begin(),d_data.end());

    double split_point = (*max - *min) * percent + *min;
    result.split_point = split_point;


    //建立小于分割点的sheet
    for(auto &row : sheet){
        string str = row[atrr_name];
        double dou = stod(str);
        if(dou <= split_point){
            ++smaller_num;
            temp[class_name] = row[class_name];
            each_type.push_back(temp);
        }
    }

    smaller_entropy = getInfoEntropy(each_type);
    result.split_atrr["smaller"] = smaller_num;
    result.value = (smaller_num / sum) * smaller_entropy;
    if(smaller_num >= sum){//如果出现特征的某一列的所有实例值是相同的情况 见428行
        return result;
    }

    each_type.clear();

    //建立大于分割点的sheet
    for(auto &row : sheet){
        string str = row[atrr_name];
        double dou = stod(str);
        if(dou > split_point){
            ++bigger_num;
            temp[class_name] = row[class_name];
            each_type.push_back(temp);
        }
    }

    bigger_entropy = getInfoEntropy(each_type);

    result.value += (bigger_num / sum) * bigger_entropy;
    result.split_atrr["bigger"] = bigger_num;


//    cout << sum << endl;
//    cout << smaller_num << " " << smaller_entropy << endl;
//    cout << bigger_num << " " << bigger_entropy << endl;

    return result;
}

/*
 * Function: calIntrinsicValue
 * Description: 计算Intrinsic Value(固有值)
 * Calls:
 * Called By: calGainRatio
 * Input: split_arr:被分割点分成的不同部分及对应行数
 * Returns: Intrinsic Value(固有值)
 */
double calIntrinsicValue(map<string, double> &split_atrr){
    double sum = 0;
    double intr_value = 0;
    for(auto &a : split_atrr){
        sum += a.second;
    }

    for(auto &a : split_atrr){
        double p = a.second / sum;

        intr_value += -p * (log2(p));
    }
    if(intr_value == 0.0){
        intr_value = 1.0;
    }
    return intr_value;
}

/*
 * Function: calGainRatio
 * Description: 计算信息增益比
 * Calls: calIntrinsicValue
 * Called By: getMaxGainRatio
 * Input: 信息熵 条件熵 split_arr:被分割点分成的不同部分及对应行数
 * Returns: 信息增益比
 */
double calGainRatio(double info_entropy, double conditional_entropy, map<string, double> &split_atrr){
    double info_gain = info_entropy - conditional_entropy;
    double intr_value = calIntrinsicValue(split_atrr);

//    cout << "信息增益：" << info_gain << endl;
//    cout << "固有值：" << intr_value << endl;

    return info_gain / intr_value;
}

/*
 * Function: getMaxGainRatio
 * Description: 寻找最大增益比
 * Calls: calGainRatio
 * Called By: main
 * Input: vector<map<string, string>>数据集， 类名(全局变量)
 * Returns: Res {name:属性名，value:信息增益比, split_point:分割点, split_arr:被分割点分成的不同部分及对应行数}
 */
Res getMaxGainRatio(vector<map<string, string>> &sheet, double &min_gain_ratio){
    Res result, temp;
    double info_entropy = getInfoEntropy(sheet);
    double gain_ratio = 0;
    auto sheet_b = sheet.begin();

    for(auto &row : (*sheet_b)){//对于给定数据集中的每一个属性
        string atrr_name = row.first;

        if(atrr_name == class_name)//排除类那一列
            continue;

        if(atrr_type[atrr_name] == DISCRETE){//离散类型的属性
            temp = getDiscreteConEntropy(sheet,atrr_name);//条件熵
            gain_ratio = calGainRatio(info_entropy, temp.value, temp.split_atrr);//信息增益比
            if(gain_ratio >= result.value){
                result.value = gain_ratio;
                result.name = atrr_name;
                result.split_atrr = temp.split_atrr;
            }
            if(gain_ratio <= min_gain_ratio){
                min_gain_ratio = gain_ratio;
            }
        }
        else{//连续类型的属性
            for (int i = 1; i < 10; ++i) {
                double percent = 0.1 * i;
                temp = getContinConEntropy(sheet,atrr_name,percent);
                gain_ratio = calGainRatio(info_entropy, temp.value, temp.split_atrr);
                if(gain_ratio >= result.value){
                    result.value = gain_ratio;
                    result.name = atrr_name;
                    result.split_point = temp.split_point;
                    result.split_atrr = temp.split_atrr;
                }
                if(gain_ratio <= min_gain_ratio){
                    min_gain_ratio = gain_ratio;
                }

//                cout << "属性名：" << atrr_name << endl;
//                cout << "百分比：" << percent << endl;
//                cout << "信息熵：" << info_entropy << endl;
//                cout << "条件熵：" << temp.value << endl;
//                cout << "信息增益比：" << gain_ratio << endl;
//                cout << "分割点：" << temp.split_point << endl;
//                for(auto &a : temp.split_atrr){
//                    cout << a.first << " " << a.second << endl;
//                }
//                cout << endl;
            }
        }


    }

    return result;
}

/*
 * Function: isSameClass
 * Description: 判断类别中是否只有一个
 * Calls:
 * Called By: createDecisionTree
 * Input: vector<map<string, string>>数据集
 * Returns: bool
 */
bool isSameClass(vector<map<string, string>> &sheet){
    auto temp_m = sheet.begin();
    string name = (*temp_m)[class_name];
    for(auto &row : sheet){
        if(row[class_name] != name)
            return false;
    }
    return true;
}

/*
 * Function: maxNumClass
 * Description: 找出数量最多的类
 * Calls:
 * Called By: createDecisionTree
 * Input: vector<map<string, string>>数据集
 * Returns: string 最大分类名
 */
string maxNumClass(vector<map<string, string>> &sheet){
    map<string, double> type_times;
    string max_class;
    double max_num = 0;

    for(auto &row : sheet){
        ++type_times[row[class_name]];
    }

    for(auto &a : type_times){
        if(a.second > max_num){
            max_class = a.first;
            max_num = a.second;
        }
    }

    return max_class;
}

/*
 * Function: createDecisionTree
 * Description: 建立决策树
 * Calls: isSameClass, maxNumClass, getMaxGainRatio
 * Called By: main()
 * Input: Node* 头节点指针, vector<map<string, string>>数据集
 * Returns: void
 */
void createDecisionTree(Node *head, vector<map<string, string>> &sheet){
    auto sheet_b = sheet.begin();
    string max_class = maxNumClass(sheet);
    //递归终止条件(生成叶子节点)
    //1、所有实例属于同一类
    //cout << sheet.size() << " " << (*sheet_b).size() << endl;
    if(isSameClass(sheet)){
        head->name = (*sheet_b)[class_name];
        //cout << "同一类：" << head->name << endl;
        return;
    }

    //2、没有实例可分，只有类别，选择实例数最大的类
    if((*sheet_b).size() == 1){
        head->name = max_class;
        //cout << "最大类：" << head->name << endl;
        return;
    }

    double min_gain_ratio = MAXFLOAT;
    Res result = getMaxGainRatio(sheet,min_gain_ratio);    //最大增益比结果

    //3、样本在特征值上的取值都相同，即最大增益比和最小增益比相同,除去只有一列特征的情况
    if((*sheet_b).size() > 2 && result.value == min_gain_ratio){
        head->name = max_class;
        //cout << "最大类：" << head->name << endl;
        return;
    }

    //不满足终止条件，继续分裂递归
    string atrr_name = result.name;         //特征名
    vector<map<string, string>> temp_sheet; //下次递归的数据集
    map<string,string> temp_map;            //临时存储的每一行实例

//    if(atrr_name.empty()){
//        cout << "属性名为空" << endl;
//        for(auto &row : sheet){
//            for(auto &a : row){
//                cout << a.first << ":" << a.second << endl;
//            }
//            cout << endl;
//        }
//        exit(1);
//    }
//
//    cout << atrr_name << "开始" << endl;


    head->name = atrr_name;
    if(atrr_type[atrr_name] == DISCRETE){//离散属性进行分支

        for(auto &a : atrr_value[atrr_name]){//对离散属性的每个取值

            //cout << "等于" << a << " : " << result.split_atrr[a] << endl;

            Node* seed = new Node;//创建一个新的分支
            if(result.split_atrr[a] <= 0){//取值为a的样本子集为空，则选最大类
                //cout << "最大类：" << max_class << endl;
                seed->name = max_class;
                head->next[a] = seed;
            }
            else{
                for(auto &row : sheet){//生成一个新的sheet
                    if(row[atrr_name] == a){
                        temp_map = row;
                        temp_map.erase(atrr_name);
                        temp_sheet.push_back(temp_map);
                    }
                }

                head->next[a] = seed;

                createDecisionTree(seed, temp_sheet);//对每个分支递归生成子树

                temp_sheet.clear();
            }
        }
    }

    else{//连续属性（分出两个儿子）
        head->split_point = result.split_point;

        Node* seed = new Node;
        //小于split_point的部分
        //cout << "小于" << head->split_point << endl;
        for(auto &row : sheet){
            string str = row[atrr_name];
            double dou = stod(str);
            if(dou <= result.split_point){
                temp_map = row;
                temp_map.erase(atrr_name);
                temp_sheet.push_back(temp_map);
            }
        }

        head->next["smaller"] = seed;
        createDecisionTree(seed, temp_sheet);

        temp_sheet.clear();
        if(temp_sheet.size() >= sheet.size()){
            //如果出现某特征的取值的样本子集为空，此处为bigger集为空 另见184行
            //选择sheet中样本最多的类
            //cout << "大于" << head->split_point << endl;
            //cout << "最大类：" << max_class << endl;

            seed = new Node;
            seed->name = max_class;
            head->next["bigger"] = seed;

            //cout << atrr_name << "结束" << endl;
            return;
        }

        //大于split_point的部分
        //cout << "大于" << head->split_point << endl;
        seed = new Node;
        for(auto &row : sheet){
            string str = row[atrr_name];
            double dou = stod(str);
            if(dou > result.split_point){
                temp_map = row;
                temp_map.erase(atrr_name);
                temp_sheet.push_back(temp_map);
            }
        }

        head->next["bigger"] = seed;
        createDecisionTree(seed, temp_sheet);
    }
    //cout << atrr_name << "结束" << endl;
}

/*
 * Function: readFile
 * Description: 读取训练集并格式化数据
 * Calls:
 * Called By: main()
 * Input: vector<map<string, string>>数据集， vector<string> 属性集
 * Returns: void
 */
void readFile(vector<map<string, string>> &sheet, vector<string> &names){
    string line;                            //per line
    ifstream in("adult.data");
    if(in.bad()){
        cout << "can not open file" << endl;
    }
    //读入第一行属性名
    getline(in, line);
    replaceCommaWithBlank(line);            //空格替换逗号
    string value;                           //every value
    istringstream name(line);
    while(name >> value){
        names.push_back(value);
    }

    while(getline(in, line)){
        vector<string>::size_type index = 0;
        map<string,string> temp;
        replaceCommaWithBlank(line);
        istringstream record(line);
        while(record >> value){
            temp[names[index++]] = value;  //names[index]属性列的值
        }
        sheet.push_back(temp);
    }
}

string traverseAndJudge(map<string, string> &row, Node* head){
    auto test = head;

    while(true){
        if(test == NULL)
            exit(1);

        string atrr_name = test->name;
//        cout << atrr_name << endl;
//
//        for(auto &s : test->next){
//            cout << s.first << " : " << s.second << endl;
//        }
//        cout << endl;

        if(test->next.empty()){//如果是叶子节点
            return atrr_name;
        }
        else{
            string atrr_to_row = row[atrr_name];

//            cout << "拥有属性：" << atrr_to_row << endl;
//            cout << endl;

            if(atrr_type[atrr_name] == DISCRETE){//如果是离散值
                test = test->next[atrr_to_row];
            }
            else{//如果是连续值
                if(atrr_to_row.empty())
                    exit(1);
                string str = atrr_to_row;
                double dou = stod(str);
                if(dou <= test->split_point){
                    test = test->next["smaller"];
                }
                else{
                    test = test->next["bigger"];
                }
            }
        }
    }
}

Matrix calResult(vector<map<string, string>> &test_data, Node* head){
    Matrix result;
    for(auto &row : test_data){
        string classify_res = traverseAndJudge(row, head);
        string classify_rel = row[class_name];
        ++result.confusion_matrix[classify_rel][classify_res];
    }
    return result;
}

/*
 * Function: readTestData
 * Description: 读取测试集并格式化数据
 * Calls:
 * Called By: main()
 * Input: vector<map<string, string>>数据集， vector<string> 属性集
 * Returns: void
 */
void readTestData(vector<map<string, string>> &test_data, vector<string> &names){
    string line;                            //per line
    string value;
    ifstream in("adult.test");
    if(in.bad()){
        cout << "can not open file" << endl;
    }

    while(getline(in, line)){
        vector<string>::size_type index = 0;
        map<string,string> temp;
        replaceCommaWithBlank(line);
        istringstream record(line);
        while(record >> value){
            temp[names[index++]] = value;  //names[index]属性列的值
        }
        test_data.push_back(temp);
    }
}

void displayConfusionMatrix(Matrix &result, map<string,double> &p_pre, map<string,double> &p){
    cout << "混淆矩阵:" << endl;

    cout << "Matrix" << '\t';
    for(auto &a : atrr_value[class_name]){
        cout << a << '\t';
    }
    cout << endl;
    for(auto &row : result.confusion_matrix){
        cout << row.first << '\t';
        for(auto &col : row.second){
            p_pre[col.first] += col.second;
            p[row.first] += col.second;
            cout << col.second << '\t';
        }
        cout << endl;
    }
}

void displayAccuracy(Matrix &result, double sum){
    double accuracy = 0;
    for(auto &row : result.confusion_matrix){
        accuracy += row.second[row.first];
    }
    accuracy /= sum;

    cout << "准确度: " << accuracy << endl;
}

void displayPrecision(Matrix &result, map<string,double> &p_pre){
    for(auto &row : result.confusion_matrix){
        p_pre[row.first] = row.second[row.first] / p_pre[row.first];
        cout << row.first << "的精度: " << p_pre[row.first] << endl;
    }
}

void displayRecall(Matrix &result, map<string,double> &p){
    for(auto &row : result.confusion_matrix){
        p[row.first] = row.second[row.first] / p[row.first];
        cout << row.first << "的召回率: " << p[row.first] << endl;
    }
}

void displayFScore(map<string,double> p_pre, map<string,double> p){
    for(auto &a : p_pre){
        cout << a.first << "的f-1值: " << (2 * a.second * p[a.first]) / (a.second + p[a.first]) << endl;
    }
}

int main() {
    vector<map<string, string>> sheet, test_data;
    vector<string> names;                   //atrribute name
    Node head;
    readFile(sheet, names);
    class_name = names.back();
    initAtrributeType(sheet);
    initAtrributeValue(sheet);

    auto start = clock();
    createDecisionTree(&head, sheet);
    auto finish = clock();

    cout << "建树结束..." << endl;

    readTestData(test_data, names);

    auto result = calResult(test_data, &head);

    map<string,double> p_pre;
    map<string,double> p;

    displayConfusionMatrix(result,p_pre,p);

    displayAccuracy(result,test_data.size());

    displayPrecision(result,p_pre);

    displayRecall(result, p);

    displayFScore(p_pre,p);


//    auto test = &head;
//    auto left = test->next["smaller"];
//    auto right = test->next["bigger"];

//    test = right;
//    left = test->next["smaller"];
//    right = test->next["bigger"];
//
//    test = right;
//    left = test->next["smaller"];
//    right = test->next["bigger"];
//
//    test = left;
//    left = test->next["smaller"];
//    right = test->next["bigger"];

//    cout << test->name << endl;
//    cout << right->name << right->name.empty() << endl;
//    cout << left->name << left->name.empty() << endl;

//    测试属性值
//    for(auto &v : atrr_value){
//        cout << v.first << endl;
//        for(auto &s : v.second){
//            cout << s << endl;
//        }
//        cout << endl;
//    }
//    测试判断
//    map<string, string> test = {
//            {"\"age\"","56"}, {"\"job\"","\"technician\""},{"\"marital\"", "\"divorced\""}, {"\"education\"","\"primary\""},
//            {"\"default\"", "\"no\""}, {"\"balance\"","13"}, {"\"housing\"","\"yes\""}, {"\"loan\"","\"no\""},
//            {"\"contact\"","\"unknown\""}, {"\"day\"","5"}, {"\"month\"","\"may\""}, {"\"duration\"","357"},
//            {"\"campaign\"","2"}, {"\"pdays\"","-1"}, {"\"previous\"","0"}, {"\"poutcome\"","\"unknown\""}, {"\"y\"","\"no\""}
//
//    };
//
//    cout << traverseAndJudge(test, &head) << endl;
//    map<string, string> a = {{"\"marital\"","\"single\""}, {"\"y\"","\"yes\""}};
//    map<string, string> b = {{"\"marital\"","\"single\""}, {"\"y\"","\"no\""}};
//    map<string, string> a = {{"\"day\"","11"}, {"\"y\"","\"yes\""}};
//    map<string, string> b = {{"\"day\"","11"}, {"\"y\"","\"no\""}};
//    vector<map<string, string>> s;
//    s.push_back(a);
//    s.push_back(b);

//    测试信息增益比
//    Res result = getMaxGainRatio(s);
//    cout << "结果：" << endl;
//    cout << result.name << endl;
//    cout << result.value << endl;
//    cout << result.split_point << endl;
//    for(auto &a : result.split_atrr){
//        cout << a.first << " " << a.second << endl;
//    }
//    测试连续条件熵
//    Res result = getContinConEntropy(sheet,names[0],0.5);
//    cout << result.value << endl;
//    cout << result.split_point << endl;
//    测试离散条件熵
//    Res result = getDiscreteConEntropy(sheet,names[1]);
//    cout << result.value << endl;
//    for(auto &a : result.split_atrr){
//        cout << a.first << endl;
//    }
//    测试信息熵
//    getInfoEntropy(sheet);
//    for(auto &a : sheet) {
//        for(auto &b : a) {
//            cout << b.second << " ";
//        }
//        cout << endl;
//    }
//    测试输出
//    ofstream ou("output.data");
//    for(auto &a : test_data){
//        for(auto &b : names){
//            ou << a[b] << " ";
//        }
//        ou << endl;
//    }
    cout << "用时：" << double(finish - start) / CLOCKS_PER_SEC << "s" << endl;
    return 0;
}