#include<iostream>
#include<fstream>
#include<cstring>
#include<string.h>
#include<string>
#include<vector>
#include<stack>
using namespace std;

//词法分析步骤：
//	1. 先做词语分割
//	2. 保留字在分割程序中判断
//这里，我改成标识符可以是“字母+数字”的形式

/*
	先看看这个文法需要注意的地方：首先赋值号是:=，是否相等是=
*/

struct var_and_type;//该结构用于语法分析时存放变量声明以后的各个变量
struct back_patch;//该结构用于回填，并且记录各个if、while、repeat语句的真假入口

//声明所有语法分析的函数，有一部分函数需要记录真假链，回填真假出口，所以要用到back_patch这个结构
void var_declare();
void var_declare_1();
void var_declare_2();
void var_declare_3();
void var_declare_4();
void program();
void judge_type();
int identifier();
string getnext();
void b_and_e();
void statement_list();
void statement();
void statement_list_1();
back_patch if_sentence();
back_patch if_sentence_1(back_patch &judge);
back_patch while_sentence();
back_patch repeat_sentence();
void assignment_statement();
//从下面开始，"<布尔量>—>"这一产生式各项的First集有交集
//没有办法，只能够通过遍历布尔量的各项来完成语法分析
//遍历的话我是通过回溯来完成的
back_patch bool_sentence();
back_patch bool_sentence_1(back_patch &judge);
back_patch bool_xiang(back_patch &judge);
back_patch bool_xiang_1(back_patch &judge);
back_patch bool_factor(back_patch &judge);
back_patch boolean(back_patch &judge);
back_patch arithmetic_sentence();
int arithmetic_sentence_1();
int arithmetic_quantity();
int xiang();
int xiang_1();
int factor();
int integer();
int relation_character(back_patch &temp);

struct bsf_zs_zfcs//用来记录识别出来的标识符、整数、字符常数的string数组（命名规则是拼音首字母(＾▽＾))
{
	string data;
	int type;//37为整数，3?为标识符，38为字符常数
};

string symbol[61] = { "","and","array","begin","bool","call","case",//这玩意是最后输出的时候才要用到的
	"char","constant","dim","do","else","end","false",
	"for","if","input","integer","not","of","or","output",
	"procedure","program","read","real","repeat","set",
	"stop","then","to","true","until","var","while","write","标识符","整数","字符常数",
	"(",")","*","*/","+",",","-",".","..","/","/*",":",":=",";","<",
	"<=","<>","=",">",">=","[","]" };

string word[10000];//用来记录识别出来的词的string数组
int num = 0;//用来记录词的个数，数组从1开始记录
bsf_zs_zfcs bsf_zs_zf[1000];//用来记录识别出来的标识符、整数、字符常数的string数组
int bsf_zs_zf_num = 0;//用来记录标识符，数组从1开始记录

string bsf[1000];//标识符表
int bsf_num = 1;

vector<int> line_first_pos;//从来记录每一行的第一个词在word中的位置，方便统计行数

bool error_judge(char a)
{
	if (a == '\'' || a == '(' || a == ')' || a == '*' || a == '+' || a == ',' || a == '-' || a == '.'
		|| a == '/' || a == ':' || a == ';' || a == '<' || a == '=' || a == '>' || a == '[' || a == ']') return true;
	else if ((a >= 'a'&&a <= 'z') || (a >= 'A' && a <= 'Z') || (a >= '0'&&a <= '9')) return true;
	else if (a == ' ' || a == '	') return true;//这里既有空格，又有tab
	else return false;
}

bool is_symbol(string a)//判断一个单词是不是保留字
{
	for (int i = 0; i <= 35; i++)
	{
		if (a == symbol[i]) return true;//说明是保留字
	}
	return false;
}

//单词分割函数，用来识别string，提取里面的串，line是识别出错时报错用的
//另外识别到的最后一个单词的时候，要把这个单词的num记录一下
void word_analysis(string a, int line)
{
	int i = 0;//a的指针，表示读到哪里
	int first_flag = 0;//用来标识是否已经记录的本行第一个词的位置

	//逐个字符读出，如果为空格，那么不会进入任何一个if，所以继续while循环
	while (i < a.size())
	{
		if (error_judge(a[i]) == false) {
			cout << "检测到非法字符，错误行数为：" << line
				<< "  具体的错误字符为：" << a[i] << endl << endl; abort();
		}
		if (a[i] == ' ' || a[i] == '	') { i++; continue; }//识别到空格或tab就跳出
		if (a[i] == '(' || a[i] == ')' || a[i] == '+' || a[i] == ',' || a[i] == '*'//判断是不是单界符，如果是单界符，那么直接记录为一个词
			|| a[i] == '-' || a[i] == ';' || a[i] == '[' || a[i] == ']' || a[i] == '=')
		{
			num++; word[num] = a[i]; i++;
			if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
		}

		else if (a[i] >= '0' && a[i] <= '9')//判断是不是整数
		{
			string t;//没读出完整一个词就赋值给t，等t完整以后，再把t放到记录词的string数组里面
			while (i < a.size() && a[i] >= '0' && a[i] <= '9')//判断是不是数字串
			{
				t += a[i];
				i++;
				//如果接下来的一个字符不为0~9，说明这个整数读完了
				if (i < a.size())//防止越界
				{
					if (a[i] < '0' || a[i] > '9')
					{
						//这里要检查一下这个标识符以前是否出现过，如果出现过，那么要计数
						int check = 0;
						for (int j = 1; j <= bsf_zs_zf_num; j++)
						{
							if (t == bsf_zs_zf[j].data)
							{
								check = 1;
								break;
							}
						}
						if (check == 1)
						{
							num++; word[num] = t;
							if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
						}
						else
						{
							bsf_zs_zf_num++; bsf_zs_zf[bsf_zs_zf_num].data = t; bsf_zs_zf[bsf_zs_zf_num].type = 37;
							num++; word[num] = t;
							if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
						}
					}
				}
				else if (i == a.size())
				{
					int check = 0;
					for (int j = 1; j <= bsf_zs_zf_num; j++)
					{
						if (t == bsf_zs_zf[j].data)
						{
							check = 1;
							break;
						}
					}
					if (check == 1)
					{
						num++; word[num] = t;
						if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
					}
					else
					{
						bsf_zs_zf_num++; bsf_zs_zf[bsf_zs_zf_num].data = t; bsf_zs_zf[bsf_zs_zf_num].type = 37;
						num++; word[num] = t;
						if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
					}
				}
			}
		}

		else if ((a[i] >= 'a'&&a[i] <= 'z') || (a[i] >= 'A' && a[i] <= 'Z'))//判断是不是字母和数字形成标识符
		{
			string t;
			while (i < a.size() && ((a[i] >= 'a'&&a[i] <= 'z') || (a[i] >= 'A' && a[i] <= 'Z') || (a[i] >= '0'&&a[i] <= '9')))
			{
				t += a[i];
				i++;
				//如果接下来的一个字符既不是字母也不是数字，说明这个标识符已经读取完了
				if (i < a.size())//防止越界
				{
					if ((a[i] < 'a' || a[i] > 'z') && (a[i] < 'A' || a[i] > 'Z') && (a[i] < '0' || a[i] > '9'))
					{
						//这里先要检查这个单词是不是保留字，如果是，则不加入到bsf中，直接加到word中即可
						if (is_symbol(t))
						{
							num++; word[num] = t;
							if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
						}
						else //然后要检查一下这个标识符以前是否出现过，如果出现过，那么要计数
						{
							int check = 0;
							for (int j = 1; j <= bsf_zs_zf_num; j++)
							{
								if (t == bsf_zs_zf[j].data)
								{
									check = 1;
									break;
								}
							}
							if (check == 1)
							{
								num++; word[num] = t;
								if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
							}
							else
							{
								bsf_zs_zf_num++; bsf_zs_zf[bsf_zs_zf_num].data = t; bsf_zs_zf[bsf_zs_zf_num].type = 36;
								num++; word[num] = t;
								if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
							}
						}
					}
				}
				else if (i == a.size())//如果刚好到string的最后一个字符
				{
					//这里先要检查这个单词是不是保留字，如果是，则不加入到bsf中，直接加到word中即可
					if (is_symbol(t))
					{
						num++; word[num] = t;
						if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
					}
					else //然后要检查一下这个标识符以前是否出现过，如果出现过，那么要计数
					{
						int check = 0;
						for (int j = 1; j <= bsf_zs_zf_num; j++)
						{
							if (t == bsf_zs_zf[j].data)
							{
								check = 1;
								break;
							}
						}
						if (check == 1)
						{
							num++; word[num] = t;
							if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
						}
						else
						{
							bsf_zs_zf_num++; bsf_zs_zf[bsf_zs_zf_num].data = t; bsf_zs_zf[bsf_zs_zf_num].type = 36;
							num++; word[num] = t;
							if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
						}
					}
				}
			}
		}
		else if (a[i] == '\'') //判断是不是',如果是，则接下来的都是字符常数
		{
			string t; t += a[i];
			i++;
			int check_end = 0;//用来表示能否在同一行检查出第二个‘
			while (i < a.size() && error_judge(a[i]) == true && a[i] != '\'')//如果字符在字符集中，并且不是第二个‘，那么继续读取下一个字符
			{
				t += a[i];
				i++;
				if (i < a.size())
				{
					if (a[i] == '\'')//判断下一个字符是否为'，如果是那就要进行以下处理
					{
						check_end = 1;
						t += a[i];//把’加到t里面，然后t就完整了
						i++;
						//这里要检查一下这个字符常数以前是否出现过，如果出现过，那么要计数
						int check = 0;
						for (int j = 1; j <= bsf_zs_zf_num; j++)
						{
							if (t == bsf_zs_zf[j].data)
							{
								check = 1;
								break;
							}
						}
						if (check == 1)
						{
							num++; word[num] = t;
							if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
							break;
						}
						else
						{
							bsf_zs_zf_num++; bsf_zs_zf[bsf_zs_zf_num].data = t; bsf_zs_zf[bsf_zs_zf_num].type = 38;
							num++; word[num] = t;
							if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
							break;
						}
					}
				}
				else if (i == a.size())
				{
					int check = 0;
					for (int j = 1; j <= bsf_zs_zf_num; j++)
					{
						if (t == bsf_zs_zf[j].data)
						{
							check = 1;
							break;
						}
					}
					if (check == 1)
					{
						num++; word[num] = t;
						if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
						break;
					}
					else
					{
						bsf_zs_zf_num++; bsf_zs_zf[bsf_zs_zf_num].data = t; bsf_zs_zf[bsf_zs_zf_num].type = 38;
						num++; word[num] = t;
						if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
						break;
					}
				}

			}
			if (i == a.size() && check_end == 0) { cout << "检测到\'没有得到匹配，错误行数为：" << line << endl << endl; abort(); }
		}
		else if (a[i] == '/')//如果遇到这个字符，那么就要判断一下下一个字符是否为*，如果不为*则/表示除号
		{
			string t; t += a[i];
			i++;
			int check_end = 0;
			if (a[i] == '*')//如果a[i]为乘号，那么组成了/*，注释里面的一大段都要省略
			{
				i++;
				while (i < a.size() && error_judge(a[i]) == true)
				{
					if (a[i] == '*')//如果检测到*号，则有可能会出现*/，则要进行下面的判断
					{
						i++;
						if (a[i] == '/')//如果a[i]为/，则组成*/，整个注释结束
						{
							check_end = 1;
							i++;
							break;
						}
					}
					else i++;//如果不是*号，那就继续读注释出来
				}
				if (i == a.size() && check_end == 0) { cout << "检测到/*没有得到匹配，错误行数为：" << line << endl << endl; abort(); }
			}
			else//如果是普通的/号，则直接作为单界符记录就OK
			{
				num++; word[num] = t;
				if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
			}
		}
		else if (a[i] == '.')//判断是.还是..
		{
			string t; t += a[i];
			i++;
			if (i < a.size() && a[i] == '.')//如果a[i]是.，那么就把它加到t里面，t变成了..，然后再赋值给word
			{
				t += a[i]; num++; word[num] = t; i++;
				if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
			}
			else//如果a[i]不是.，那么直接当作单界符记录
			{
				num++; word[num] = t;
				if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
			}
		}
		else if (a[i] == ':')//判断是:还是:=
		{
			string t; t += a[i];
			i++;
			if (i < a.size() && a[i] == '=')//如果a[i]是=，那么就把它加到t里面，t变成了:=，然后再赋值给word
			{
				t += a[i]; num++; word[num] = t; i++;
				if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
			}
			else//如果a[i]不是=，那么直接当作单界符记录
			{
				num++; word[num] = t;
				if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
			}
		}
		else if (a[i] == '<')//判断是< 还是 <= 还是 <>
		{
			string t; t += a[i];
			i++;
			if (i < a.size() && (a[i] == '=' || a[i] == '>'))//如果a[i]是=或者>，那么就把它加到t里面，t变成了<=或<>，然后再赋值给word
			{
				t += a[i]; num++; word[num] = t; i++;
				if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
			}
			else//如果a[i]不是=，那么直接当作单界符记录
			{
				num++; word[num] = t;
				if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
			}
		}
		else if (a[i] == '>')//判断是 > 还是 >=
		{
			string t; t += a[i];
			i++;
			if (i < a.size() && a[i] == '=')//如果a[i]是=，那么就把它加到t里面，t变成了>=，然后再赋值给word
			{
				t += a[i]; num++; word[num] = t; i++;
				if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
			}
			else//如果a[i]不是=，那么直接当作单界符记录
			{
				num++; word[num] = t;
				if (first_flag == 0) { line_first_pos.push_back(num); first_flag = 1; }
			}
		}
	}
}

/*****************************************************华丽的分割线1*****************************************************/

//语法分析函数（中间生成四元式）
//语法分析最基本需要的变量如下：
int p = 0;//用来记录读到第几个TOKEN字
string TOKEN;//用来存放下一个要读入的字

//记录程序名这个标识符是否已经被读取过在identifier函数中使用，判断程序名是否被定义为其他变量的标识符，如果是就报错
int if_program_name;

//变量声明函数要用到的数据类型
int var_flag;//标志是否正在进行变量声明，如果正在进行变量声明，则需要在<标识符>identifier函数里面进行var_list的记录操作
struct var_and_type//用来回填变量声明的变量类型
{
	string data;//存放已经读出的标识符的串数据
	double value;//用来存放这个变量的大小（我们这个编译器只做int类型的赋值等运算）
	int type;//1为integer，2为char，3为bool

	//之所以要把默认类型置为-1，是因为变量声明类型回填的时候要检查从-1的元素开始进行回填
	var_and_type()
	{
		type = -1;//默认类型为-1
	}

	var_and_type(string d, int t)
	{
		data = d;
		type = t;
	}
};
//用来记录变量声明时读到的所有变量（不包含程序名，因为那时候还没开始变量声明）（在变量声明函数var_declare函数中被使用）
vector<var_and_type> var_list;

int var_end = 0;//用来记录变量声明完成没有（用于identifier里面）

//记录回溯的位置，在可能出现First集交集的地方要记录一下当前保留字的位置，如果一种句型匹配失败，则让p回溯到该保留字
int backtrack_pos = 0;

//用来记录语法分析到第几行，方便报错
int error_line = 0;

//下面是一些生成四元式要用到的变量
vector<vector<string>> quaternary_list;//用来记录所有生成的四元式
stack<var_and_type> cal_stack;//用来进行算术运算的变量记录在这个栈里面
int temp_var_num;//记录临时变量现在有多少个，记录当前临时变量的数量

//该结构用于回填，并且记录各个if、while、repeat语句的真假入口
struct back_patch
{
	vector<int> true_chain;//待回填的真链
	vector<int> false_chain;//待回填的假链
	vector<int> else_if_chain;//有else的句子，if then之后要直接跳出到整个if else
	//如果有多个and后面跟着一个or，那么这个and布尔项的假出口不能加到假链里面，而是加到这个链上面
	//每次只要再次遇到一个or，那就进行回填，如果一直到布尔表达式结束都没有or，那就直接把and布尔项的假链都加到真正的假链里面
	vector<int> and_or_false_chain;

	int codebegin;//第一个四元式的序号
	int accept;//用来标识式子是不是算术表达式
	string op;//在<算术表达式><关系符><算术表达式>中生成四元式要用到
	bool is_swap;//用来记录语句中是否有not，又不要交换真假链
	back_patch() { accept = 0; is_swap = 0; }
	//如果bool表达式里面有or把多个布尔项连接起来呢，那么无论满足了那个bool项都能到达真假出口，因此他们的链需要连起来，最后一起回填
	void addTrueChain(back_patch &st)
	{
		if (st.is_swap == 0)	true_chain.insert(true_chain.end(), st.true_chain.begin(), st.true_chain.end());
		else true_chain.insert(true_chain.end(), st.false_chain.begin(), st.false_chain.end());
	}
	void addFalseChain(back_patch &st)//把这个位置的出口加到假链上
	{
		if (st.is_swap == 0)	false_chain.insert(false_chain.end(), st.false_chain.begin(), st.false_chain.end());
		else false_chain.insert(false_chain.end(), st.true_chain.begin(), st.true_chain.end());
	}
	void addAndOrFalseChain(back_patch &st)
	{
		and_or_false_chain.insert(and_or_false_chain.end(), st.and_or_false_chain.begin(), st.and_or_false_chain.end());
	}
};

//语法分析函数
void grammar_analysis()
{
	TOKEN = getnext();
	program();
	//到这里整个程序都结束了，生成四元式(sys,-,-,-)
	vector<string> temp;
	temp.push_back("sys");
	temp.push_back("-");
	temp.push_back("-");
	temp.push_back("-");
	quaternary_list.push_back(temp);
	return;
}

//用于获取下一个字到TOKEN串里面
//每次读词的时候都检查一下p是否到了要换行的时候
string getnext()
{
	p++;
	//每次读一个词都检查一下是否要换行，而且每次检查都从正在读取的行数开始
	int i = error_line;
	if (p == line_first_pos[i]) { /*cout << word[p]<<" " <<error_line<< endl;*/ error_line++; }//检查一下这个词是不是下一行的第一个词

	return word[p];
}

void print_error_line()
{
	cout << "具体错误行数：" << error_line << "	编译结束" << endl << endl;
}

//语法分析的错误报告
void error(int i)
{
	if (i == 1) { cout << "语法错误：程序第一个词不是program" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 2) { cout << "语法错误：变量声明处，var	后面缺少标识符" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 3) { cout << "语法错误：程序声明处，program <标识符>	后面缺少分号‘;’" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 4) { cout << "语法错误：缺少程序结束符‘.’" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 5) { cout << "语法错误：程序名不能被用作其他变量名" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 6) { cout << "语法错误：变量声明处，缺少冒号‘:’，或者缺少关键字begin，无法结束变量声明" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 7) { cout << "语法错误：变量声明处，缺少分号‘;’" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 8) { cout << "语法错误：变量声明处，没有找到相应的标识符" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 9) { cout << "语法错误：变量声明处，不存在的变量类型（不是integer/char/bool）" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 10) { cout << "语法错误：缺少保留字begin" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 11) { cout << "语法错误：缺少保留字end，或语句段中途缺少分号‘;’，或语句中有不存在的标识符" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 12) { cout << "语法错误：if语句缺少if保留字" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 13) { cout << "语法错误：if语句的<布尔表达式>有误" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 14) { cout << "语法错误：if语句缺少then保留字,或者语句中的布尔表达式有误" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 15) { cout << "语法错误：while语句缺少保留字while" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 16) { cout << "语法错误：while语句缺少do关键字" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 17) { cout << "语法错误：repeat语句缺少保留字repeat" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 18) { cout << "语法错误：repeat语句缺少保留字until" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 19) { cout << "语法错误：repeat语句，until后的<布尔表达式>有误" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 20) { cout << "语法错误：赋值句没有找到标识符,标识符未声明；或者该语句句型不合法" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 21) { cout << "语法错误：赋值句缺少:= 或者 没有找到相应的标识符" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 22) { cout << "语法错误：赋值句的算术表达式有误" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 23) { cout << "语法错误：算术量的\"(<算术表达式>)\"缺少右括号" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 24) { cout << "语法错误：布尔量中的布尔表达式缺少右括号" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 25) { cout << "语法错误：\"<算术表达式><关系符><算术表达式>\"的<关系符>没识别出来" << endl; print_error_line(); system("pause"); exit(0); }
	if (i == 26) { cout << "语法错误：布尔量有误（无法识别的布尔量），请检查该行条件判断语句是否正确" << endl; print_error_line(); system("pause"); exit(0); }
}

//<程序> → program <标识符> ；<变量说明> <复合语句> .
void program()
{
	string tmp;

	tmp = "program";
	if (TOKEN.compare(tmp) != 0) error(1);//错误1：程序第一个词不是program
	TOKEN = getnext();

	if (identifier() == -1) error(2);//转到<标识符>，错误2：没有找到标识符
	//这里要生成一下第一条四元式（program,程序名,-,-)
	vector<string> temp;
	temp.push_back("program"); temp.push_back(word[p - 1]); temp.push_back("-"); temp.push_back("-");
	quaternary_list.push_back(temp);

	tmp = ";";
	if (TOKEN.compare(tmp) != 0) error(3);//错误3：缺少分号
	TOKEN = getnext();


	var_declare();//转到<变量说明>
	b_and_e();//转到<复合语句>

	tmp = ".";
	if (TOKEN.compare(tmp) != 0) error(4);//错误4：没有程序结束符
	return;
}

//标识符识别，检查TOKEN是否标识符，并看看是否是变量声明操作中出现的标识符
//如果是，那么要把标识符加入到var_list里面
//返回这个变量的下标，如果没找到或是其他情况，则返回-1
int identifier()
{
	//用来记录找到的标识符是整个程序中标识符的第几个
	int t = -1;
	//先找到现在的TOKEN是第几个标识符
	for (int i = 1; i <= bsf_num; i++)
	{
		if (TOKEN.compare(bsf[i]) == 0) { t = i; TOKEN = getnext(); break; }
	}

	//在变量声明完以后，下一个TOKEN应该是begin，由于var_declare4需要重新调用var_declare1
	//所以要看是不是begin,如果是，那么退出，并把完结标志位var_end置1
	string tmp = "begin";
	if (t == -1 && TOKEN.compare(tmp) == 0) { var_end = 1; return 0; }


	//因为判断句型的时候，如果不是if、while、repeat、复合句、那么就会自动归类到赋值句
	//而赋值句里面第一个要取的就是end，所以现在直接跳出，返回一个特殊的值给赋值句
	tmp = "end";
	if (t == -1 && TOKEN.compare(tmp) == 0 && word[p + 1].compare(".") == 0) { return -2; }

	//如果没有在标识符表里面找到该标识符，则直接返回
	if (t == -1) return -1;

	//如果某个标识符是第一个标识符(程序名)，若是第一次读，则if_program==0，语法正确
	//一旦第二次还出现这个程序名作为其他变量的标识符，那么就算是出错
	if (t == 1)
	{
		if (if_program_name == 1) error(5);//错误5：程序名被用作其他变量名
		if_program_name = 1;//第一次读完这个程序名，就要把标志位置1
		return 0;
	}

	//当识别出标识符的时候有两种情况：1. 变量声明中的变量	2. 程序中各种运算和赋值语句等的变量操作					

	if (var_flag == 1)//如果是1. 变量声明中出现的变量，那么自然是要存到var_list里面的
	{
		var_and_type tmp;
		tmp.data = word[p - 1];
		var_list.push_back(tmp);
		return 0;
	}
	else //如果是2. 那么所有的标识符应该都在var_list里面了，程序中各种运算和赋值语句等的变量操作，那么也要进行相应的变量记录操作
	{
		//先找到var_list里面对应这个string的标识符，如果找到，就返回这个标识符在var_list的位置
		for (int i = 0; i < var_list.size(); i++)
		{
			if (word[p - 1].compare(var_list[i].data) == 0) return i;
		}
	}
}

//<变量说明> → var <变量定义>│ε
void var_declare()
{
	//先看看该程序片段有没有变量声明
	string tmp;
	tmp = "var";
	//如果检测到var说明有变量声明
	if (TOKEN.compare(tmp) == 0)
	{
		TOKEN = getnext();
		var_flag = 1;//准备把声明的变量都存到var_list里面
		var_declare_1();//变量定义
		var_flag = 0;//变量声明结束，后面程序中再检测到变量也不用加入到var_list里面
	}
	//如果没有任何字符声明那么就结束，直接进入复合句，退出该函数
	return;
}

//<变量定义> → <标识符表> ：<类型> ；var_declare_4（消除左公因式）
void var_declare_1()
{
	var_declare_2();//转到标识符表

	if (var_end == 1) return;//标识符后面要看看是不是begin，如果是那么结束var_declare所有过程

	string tmp = ":";//冒号
	if (TOKEN.compare(tmp) != 0) error(6);//错误6：变量定义处缺少冒号
	TOKEN = getnext();

	judge_type();

	tmp = ";";//分号
	if (TOKEN.compare(tmp) != 0) error(7);//错误7：变量定义处缺少分号
	TOKEN = getnext();

	var_declare_4();
	return;
}

// <标识符表> → <标识符> var_declare_3（消除左公因式）
void var_declare_2()
{
	if (identifier() == -1) error(8);//判断TOKEN是否为标识符，如果为-1，则 错误8：没有找到相应标识符
	if (var_end == 1) return;//标识符后面要看看是不是begin，如果是那么结束var_declare所有过程
	var_declare_3();//转到<var_declare_3>
	return;
}

//<类型> → integer│bool│char		这是一个类型回填函数
//!!!!!此处检测出变量类型，应该对变量声明的变量进行回填操作!!!!!
void judge_type()
{
	string tmp1 = "integer", tmp2 = "char", tmp3 = "bool";
	if (TOKEN.compare(tmp1) == 0)
	{
		TOKEN = getnext();
		//先遍历整个var_list，当检测到变量的type为-1的时候，说明这些变量是变量声明加入进去的，那么就进行类型回填
		for (vector<var_and_type>::iterator it = var_list.begin(); it != var_list.end(); it++)
		{
			if (it->type == -1) it->type = 1;
		}
		return;
	}
	else if (TOKEN.compare(tmp2) == 0)
	{
		TOKEN = getnext();
		//先遍历整个var_list，当检测到变量的type为-1的时候，说明这些变量是变量声明加入进去的，那么就进行类型回填
		for (vector<var_and_type>::iterator it = var_list.begin(); it != var_list.end(); it++)
		{
			if (it->type == -1) it->type = 2;
		}
		return;
	}
	else if (TOKEN.compare(tmp3) == 0)
	{
		TOKEN = getnext();
		//先遍历整个var_list，当检测到变量的type为-1的时候，说明这些变量是变量声明加入进去的，那么就进行类型回填
		for (vector<var_and_type>::iterator it = var_list.begin(); it != var_list.end(); it++)
		{
			if (it->type == -1) it->type = 3;
		}
		return;
	}
	else error(9);//错误9：不存在的变量类型（不是integer/char/bool）
}

//var_declare_4 → <变量定义>│ε
void var_declare_4()
{
	var_declare_1();//调用变量定义
}


//var_declare_3 →，<标识符表>│ε
void var_declare_3()
{
	string tmp = ",";
	if (TOKEN.compare(tmp) == 0) { TOKEN = getnext(); var_declare_2(); }//如果能够匹配上逗号，那么继续<标识符表>
	return;//如果没有逗号，那么直接返回
}

//<复合句> → begin <语句表> end
void b_and_e()
{
	string tmp = "begin";
	if (TOKEN.compare(tmp) != 0) error(10);//错误10：缺少保留字begin
	TOKEN = getnext();

	statement_list();

	tmp = "end";
	if (TOKEN.compare(tmp) != 0) error(11);//错误11：缺少保留字end，或者缺少分号‘;’导致程序结束
	TOKEN = getnext();

	return;
}

//<语句表> → <语句> statement_list_1（消除左公因式）
void statement_list()
{
	statement();
	statement_list_1();
	return;
}

//statement_list_1 ->；<语句表>│ε
void statement_list_1()
{
	string tmp = ";";
	if (TOKEN.compare(tmp) == 0) { TOKEN = getnext(); statement_list(); }//如果后面接分号，那么转入<语句表>，否则不做任何事情
	return;
}


/*****************************************************华丽的分割线2********************************************************/
//进入到语句的识别，问题就来了，很多first集会有交集，我的解决办法是回溯（其实就是逐一句型去尝试），如果任何一个句型都匹配不上就报错

//然后句型里面的报错，由这个句型的函数来管，<语句> 函数只负责检查要进哪种句型

//注意，声明变量以后，所有错误都只会在各种语句的函数里面发生（有一些中途过程有识别到终结符的也可能发生）

//<语句> → <赋值句>│<if句>│<while句>│<repeat句>│<复合句>
void statement()
{
	back_patch temp;
	//这里我们要判断下面一个句子是什么类型的
	//首先看看第一个词是不是if,while,repeat,begin中的一个，如果不是，那么就是赋值句
	string tmp1 = "if", tmp2 = "while", tmp3 = "repeat", tmp4 = "begin";
	if (TOKEN.compare(tmp1) == 0) //要么是if语句
	{
		temp = if_sentence();
		//这里要回填一下judge的else_if链
		for (int i = 0; i < temp.else_if_chain.size(); i++)
		{
			quaternary_list[temp.else_if_chain[i]][3] = to_string(quaternary_list.size());
		}
	}
	else if (TOKEN.compare(tmp2) == 0) { temp = while_sentence(); } //要么是while语句
	else if (TOKEN.compare(tmp3) == 0) { temp = repeat_sentence(); }//要么是repeat语句
	else if (TOKEN.compare(tmp4) == 0) { b_and_e(); }//要么是复合句
	else { assignment_statement(); }//全都不是就只能是赋值语句了
	return;
}

//<if句>→if <布尔表达式> then <语句>  if_sentence_1（消除左公因式）
back_patch if_sentence()
{
	back_patch judge;
	string tmp = "if";
	if (TOKEN.compare(tmp) != 0) error(12);//错误12：if语句缺少if保留字（不可能的错误）
	TOKEN = getnext();

	int codebegin = quaternary_list.size();//先把if语句的位置记录下来再说

	judge = bool_sentence();//转到<布尔表达式>
	if (judge.accept == -1) error(13);//错误13：if语句的<布尔表达式>有误

	tmp = "then";
	if (TOKEN.compare(tmp) != 0) error(14);//错误1?：if语句缺少then保留字
	TOKEN = getnext();
	//执行完bool_sentence()，每个布尔项的真出口、假出口及其<布尔表达式>的四元式已经生成完了，现在紧跟着要回填真链

	//这里开始要对布尔表达式返回来的真假出口进行回填
	//bool项生成有，可以对真链的出口进行回填了
	for (int i = 0; i < judge.true_chain.size(); i++)
	{
		quaternary_list[judge.true_chain[i]][3] = to_string(quaternary_list.size());
	}

	statement();//转到<语句>

	//这里要判断一下后面有没有else如果有，那么执行完then马上跳出
	if (TOKEN.compare("else") == 0)
	{
		//这个布尔项的假出口也一并生成了，并且要把假出口填到里面去
		vector<string> tempp; tempp.push_back("j"); tempp.push_back("-"); tempp.push_back("-"); tempp.push_back("待回填的假出口");
		quaternary_list.push_back(tempp);
		judge.else_if_chain.push_back(quaternary_list.size() - 1);//专门拿一条链来存这个有else的then后语句执行后跳出的位置
	}

	judge = if_sentence_1(judge);//转到if_sentence_1

	//连可能的else都执行完，就可以回填假链的出口了


	//没else时直接填<语句>执行后返回的codebegin（记录该语句最初开始生成的四元式）
	if (judge.accept == -2)
	{
		for (int i = 0; i < judge.false_chain.size(); i++)
		{
			quaternary_list[judge.false_chain[i]][3] = to_string(quaternary_list.size());
		}
		//对于and句型如果一直都没有遇到or，说明不是and_or句型，那么直接对and布尔项的假链进行回填
		for (int i = 0; i < judge.and_or_false_chain.size(); i++)
		{
			quaternary_list[judge.and_or_false_chain[i]][3] = to_string(quaternary_list.size());
		}
	}
	//还有一个情况就是else if 语句，else if语句需要判断之后还有没有else语句
	//如果有，那么假出口就是下一个else后接着的语句
	//如果没有，那么直接跳出
	else //有else是肯定是填else的<语句>返回的codebegin了(记录else的<语句>最开始生成的四元式）
	{
		for (int i = 0; i < judge.false_chain.size(); i++)
		{
			quaternary_list[judge.false_chain[i]][3] = to_string(judge.codebegin);
		}
		//对于and句型如果一直都没有遇到or，说明不是and_or句型，那么直接对and布尔项的假链进行回填
		for (int i = 0; i < judge.and_or_false_chain.size(); i++)
		{
			quaternary_list[judge.and_or_false_chain[i]][3] = to_string(judge.codebegin);
		}
	}

	return judge;
}

//if_sentence_1 ->(else<语句>| ε )
back_patch if_sentence_1(back_patch &judge)
{
	string tmp = "else";
	if (TOKEN.compare(tmp) == 0)//转到<语句>，并且语句执行以后要获取一下现在的四元式生成到哪里了
	{
		judge.codebegin = quaternary_list.size();
		TOKEN = getnext();
		statement();
	}
	else judge.accept = -2;//用来表示有没有执行过else
	return judge;
}

//<while句> → while <布尔表达式> do <语句>
back_patch while_sentence()
{
	back_patch judge;
	string tmp = "while";
	if (TOKEN.compare(tmp) != 0) error(15);//错误15：while语句缺少保留字while（不可能的错误）
	TOKEN = getnext();

	judge = bool_sentence();//转到<布尔表达式>

	tmp = "do";
	if (TOKEN.compare(tmp) != 0) error(16);//错误16：while语句缺少do关键字
	TOKEN = getnext();

	//这里我们要回填真链
	for (int i = 0; i < judge.true_chain.size(); i++)
	{
		quaternary_list[judge.true_chain[i]][3] = to_string(quaternary_list.size());
	}

	statement();//转到<语句>
	//所有句子执行完一遍，要再次回到句子开头去判断,除此之外还要把假链给填了
	vector<string> temp; temp.push_back("j"); temp.push_back("-"); temp.push_back("-"); temp.push_back(to_string(judge.codebegin));
	quaternary_list.push_back(temp);

	//回填假链
	for (int i = 0; i < judge.false_chain.size(); i++)
	{
		quaternary_list[judge.false_chain[i]][3] = to_string(quaternary_list.size());
	}

	//对于and句型如果一直都没有遇到or，说明不是and_or句型，那么直接对and布尔项的假链进行回填
	for (int i = 0; i < judge.and_or_false_chain.size(); i++)
	{
		quaternary_list[judge.and_or_false_chain[i]][3] = to_string(quaternary_list.size());
	}

	return judge;
}

//<repeat句> → repeat <语句> until <布尔表达式>
back_patch repeat_sentence()
{
	string tmp = "repeat";
	if (TOKEN.compare(tmp) != 0) error(17);//错误17：repeat语句缺少保留字repeat（不可能的错误）
	TOKEN = getnext();

	int temp_codebegin = quaternary_list.size();//先把repeat的语句的开头记录下来

	statement();//转到<语句>

	tmp = "until";
	if (TOKEN.compare(tmp) != 0) error(18);//错误18：repeat语句缺少保留字until
	TOKEN = getnext();

	back_patch judge;

	judge = bool_sentence();//转到<布尔表达式>

	if (judge.accept == -1) error(19);//错误19：repeat语句，until后的<布尔表达式>有误

	//整个repeat until结构结束，开始回填真链和假链
	for (int i = 0; i < judge.true_chain.size(); i++)
	{
		quaternary_list[judge.true_chain[i]][3] = to_string(quaternary_list.size());
	}

	for (int i = 0; i < judge.false_chain.size(); i++)
	{
		quaternary_list[judge.false_chain[i]][3] = to_string(temp_codebegin);
	}

	//对于and句型如果一直都没有遇到or，说明不是and_or句型，那么直接对and布尔项的假链进行回填
	for (int i = 0; i < judge.and_or_false_chain.size(); i++)
	{
		quaternary_list[judge.and_or_false_chain[i]][3] = to_string(temp_codebegin);
	}

	return judge;
}

//<赋值句> → <标识符> := <算术表达式>
void assignment_statement()
{
	//赋值句是一定会产生四元式的，而且有可能产生多条四元式，因此会需要到回填，所以我们要记录一下开始位置
	int codebegin = quaternary_list.size();

	int judge = identifier();
	if (judge == -1) error(20);//错误20：赋值句没有找到标识符,标识符没有声明；或者什么句型都没识别到
	else if (judge == -2) return;
	//如果这是一个标识符，那么要记下这个被赋值的标识符的下标
	int assigned_bsf = judge;

	string tmp = ":=";
	if (TOKEN.compare(tmp) != 0) error(21);//错误21：赋值句缺少:=
	TOKEN = getnext();

	int now_pos = p;//记录当前的位置，等一下生成四元式要用到

	judge = arithmetic_sentence().accept;//转到<算术表达式>
	if (judge == -1) error(22);//错误22：赋值句的算术表达式有误；

	var_and_type x = cal_stack.top(); cal_stack.pop();//一个赋值语句结束，栈的顶端存放的应该是赋值句整个算术表达式的var_and_type
	vector<string> temp;
	temp.push_back(":=");
	temp.push_back(x.data);
	temp.push_back("-");//占位符
	temp.push_back(var_list[assigned_bsf].data);
	quaternary_list.push_back(temp);
	cal_stack.push(var_list[assigned_bsf]);

	return;
}

//<布尔表达式> → <布尔项> bool_sentence_1
back_patch bool_sentence()
{
	back_patch judge;
	judge.codebegin = quaternary_list.size();//获取即将生成的四元式的位置，不知道后面要不要回填
	judge = bool_xiang(judge);//转到<布尔项>
	if (judge.accept != -1) bool_sentence_1(judge);//转到bool_sentence_1

	return judge;//这里这个judge还用来返回回填四元式的真假链出口
}

//bool_sentence_1 → or <布尔项> bool_sentence_1 | ε
back_patch bool_sentence_1(back_patch &judge)
{
	string tmp = "or";
	if (TOKEN.compare(tmp) == 0)
	{
		//这里要对and_or_false_chain进行回填，然后清空and_or_false_chain
		for (int i = 0; i < judge.and_or_false_chain.size(); i++)
		{
			quaternary_list[judge.and_or_false_chain[i]][3] = to_string(quaternary_list.size());
		}
		judge.and_or_false_chain.clear();

		//这里要注意一下，or的假出口要填到下一个布尔项的codebegin位置，直到最后一个or出现，布尔项的假出口才是真正的假出口
		back_patch judge2;//用来临时存储每个布尔项回传的真假链
		TOKEN = getnext();
		judge2 = bool_xiang(judge2);//转道<布尔项>

		judge.addTrueChain(judge2);//保存or以后的布尔项的真链，等待回填
		judge.addFalseChain(judge2);//将当前的布尔表达式中的最后那个需要回填的假链保存在judge中
		judge.addAndOrFalseChain(judge2);//连接and_or_false_chain

		if (judge.accept == -1) return judge;//如果bool_xiang都识别失败，就直接返回了
		judge = bool_sentence_1(judge);//转到......
	}
	return judge;
}

//<布尔项> → <布因子> bool_xiang_1
back_patch bool_xiang(back_patch &judge)
{
	judge = bool_factor(judge);
	if (judge.accept == -1) return judge;//说明<布因子>识别失败，直接返回

	judge = bool_xiang_1(judge);

	return judge;
}

//bool_xiang_1 → and <布因子> bool_xiang_1 | ε
back_patch bool_xiang_1(back_patch &judge)
{
	string tmp = "and";
	if (TOKEN.compare(tmp) == 0)
	{
		back_patch judge3;
		TOKEN = getnext();
		judge3 = bool_factor(judge3);


		//这里我们开始串链

		judge.addTrueChain(judge3);//保存or以后的布尔项的真链，等待回填
		judge.addFalseChain(judge3);//将当前的布尔表达式中的最后那个需要回填的假链保存在judge中
		judge.addAndOrFalseChain(judge3);

		if (judge.accept == -1) return judge;//如果judge为-1，说明<布因子>识别失败，直接返回
		judge = bool_xiang_1(judge);
	}
	return judge;
}

//<布因子> → <布尔量>│not <布因子>
back_patch bool_factor(back_patch &judge)
{
	string tmp = "not";
	if (TOKEN.compare(tmp) == 0)
	{
		TOKEN = getnext();
		//这里真链假链要换过来
		judge.is_swap = 1;
		judge = bool_factor(judge);
		//这里是考虑要不要进行swapchain（如果是只有一个布尔项的布尔表达式，则不会经历链连接的步骤，所以我在这里单独判断）
		if (judge.true_chain.size() == 1 && judge.false_chain.size() == 1)
		{
			vector<int> temp = judge.true_chain;
			judge.true_chain = judge.false_chain;
			judge.false_chain = temp;
		}
	}
	else
	{
		judge = boolean(judge);
		//这里要生成四元式
	}
	return judge;
}

//<布尔量> → <布尔常量>│( <布尔表达式> )│<算术表达式> <关系符> <算术表达式> | 标识符
back_patch boolean(back_patch &judge)
{
	string tmp1 = "true", tmp2 = "false", tmp3 = "(", tmp4 = "-", tmp5 = ")";
	if (TOKEN.compare(tmp1) == 0) //true
	{
		TOKEN = getnext();
		//如果if、while、repeat语句的判断句子存放的是true，那么我们无论如何先生成一个(jump,-,-,-)，然后后面再根据完整的语句进行回填
		//我不打算让用户写if true and false这种语句，因为没有意义，只允许if/while (true)这样的
		vector<string> tp;
		tp.push_back("j"); tp.push_back("-"); tp.push_back("-"); tp.push_back("真出口待回填");
		quaternary_list.push_back(tp);

		//这个四元式的位置要记录一下（等一下还要回填）
		judge.true_chain.push_back(quaternary_list.size() - 1);
		var_and_type temp; temp.data = "true";
		//生成四元式之后，因为布尔项或许还需要进行AND运算，所以要入栈
		cal_stack.push(temp);

		//这个布尔项的假出口也一并生成了，并且要把假出口填到里面去
		vector<string> tempp; tempp.push_back("j"); tempp.push_back("-"); tempp.push_back("-"); tempp.push_back("待回填的假出口");
		quaternary_list.push_back(tempp);
		judge.false_chain.push_back(quaternary_list.size() - 1);

		judge.codebegin = quaternary_list.size() + 1;//因为只允许if/while (true)这样的，所以真出口必然是在下下跳四元式产生的地方
		return judge;
	}
	else if (TOKEN.compare(tmp2) == 0) //false
	{
		TOKEN = getnext();
		//如果if、while、repeat语句的判断句子存放的是true，那么我们无论如何先生成一个(jump,-,-,-)，然后后面再根据完整的语句进行回填
		//我不打算让用户写if true and false这种语句，因为没有意义
		vector<string> tp;
		tp.push_back("j"); tp.push_back("-"); tp.push_back("-"); tp.push_back("假出口待回填");
		quaternary_list.push_back(tp);
		//这个四元式的位置要记录一下（等一下还要回填）
		judge.false_chain.push_back(quaternary_list.size());
		var_and_type temp; temp.data = "false";
		//生成四元式之后，因为布尔项或许还需要进行AND运算，所以要入栈
		cal_stack.push(temp);
		return judge;
	}

	backtrack_pos = p;//记录回溯点
	//后面三项都是有交集的，只能通过遍历进行尝试，不行就回退

	//之所以用while是因为我要根据judge的结果进行break！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！

	while (TOKEN.compare(tmp3) == 0)//(<布尔表达式>)
	{
		TOKEN = getnext();
		judge = bool_sentence();
		if (judge.accept == -1) break;//如果<布尔表达式>匹配不上，那么跳出
		if (TOKEN.compare(tmp5) != 0) error(24);//错误24：布尔量中的布尔表达式缺少右括号
		TOKEN = getnext();
		return judge;
	}//这里应该要回填一些东西

	p = backtrack_pos;//把p位置重置到回溯点
	while (1) //<算术表达式> <关系符> <算术表达式>
	{
		back_patch temp_judge;
		temp_judge = arithmetic_sentence();
		if (temp_judge.accept == -1) break;//如果算术表达式匹配不上，那么跳出
		judge.accept = relation_character(judge);
		if (judge.accept == -1) break;//如果关系符匹配不上，那么跳出
		temp_judge = arithmetic_sentence();
		if (temp_judge.accept == -1) break;//如果算术表达式匹配不上，那么跳出
		//现在的cal_stack里面有两个数，分别是<关系符>左边和右边的<算术表达式>
		//所以我们可以开始生成跳转判断的四元式了：(j<关系符>,算术表达式1，算术表达式2，待回填的真出口)
		//因为这个真出口是待回填的，所以我们要把它加入到judge的真链里面
		var_and_type x = cal_stack.top(); cal_stack.pop();
		var_and_type y = cal_stack.top(); cal_stack.pop();
		vector<string> temp; string tmpp = "j"; tmpp += judge.op;
		temp.push_back(tmpp); temp.push_back(y.data); temp.push_back(x.data); temp.push_back("待回填的真出口");
		quaternary_list.push_back(temp);

		//下面是把该布尔项生成的四元式加到真链里面，但是！！如果这个布尔项后面跟着是and
		//那么不能加到真链里面，因为我们还要判断一下and后面的布尔项是不是为真，所以出口应该是and后面的布尔项
		if (TOKEN.compare("and") != 0) //回填待填写的真出口的在四元式表中的位置
		{
			if (TOKEN.compare("or") == 0)//真出口本来应该加入到true_chain里面，但是如果有or的话
										 //就不能这么干了，直接跳转到下一个布尔项的起始位置
			{
				if (judge.is_swap == 0) judge.true_chain.push_back(quaternary_list.size() - 1);
				//not的比较特别，因为有not的布尔项的真出口相当于假出口
				//布尔项的假出口在下一个词为or的情况下就直接跳转到下一个布尔项codebegin的位置
				else quaternary_list[quaternary_list.size() - 1][3] = to_string(quaternary_list.size() + 1);
			}
			else judge.true_chain.push_back(quaternary_list.size() - 1);
		}
		else//如果是and，自然是直接跳转到下一个布尔项的开始位置
		{
			//但是我们还要考虑一下not情况如何
			if (judge.is_swap == 0) quaternary_list[quaternary_list.size() - 1][3] = to_string(quaternary_list.size() + 1);
			else judge.true_chain.push_back(quaternary_list.size() - 1);//not情况下，布尔项的真假出口颠倒，and的真出口自然是直接加到链里面
		}

		//这个布尔项的假出口也一并生成了，并且要把假出口填到里面去，但是！！如果这个布尔项后面跟着的是or
		//那么不能加到假链里面，因为我们还要判断一下or后面的布尔项是不是为假，所以假出口应该是or后面的布尔项的codebegin
		vector<string> tempp; tempp.push_back("j"); tempp.push_back("-"); tempp.push_back("-"); tempp.push_back("待回填的假出口");
		quaternary_list.push_back(tempp);

		if (TOKEN.compare("or") != 0)//如果不是or
		{
			//不过我们还要判断一下是否有and
			if (TOKEN.compare("and") == 0)
			{
				//这里有一个麻烦，如果多个and后面还跟着一个or，那么就应该跳转到这个or的codebegin的位置
				//所以我们干脆把所有and布尔项的假出口全部加到一条链子里面，根据后面语法分析是否有or
				//来判断是否需要将这些and布尔项的假出口设置为下一个or的codebegin
				if (judge.is_swap == 0) judge.and_or_false_chain.push_back(quaternary_list.size() - 1);
				else quaternary_list[quaternary_list.size() - 1][3] = to_string(quaternary_list.size());
			}
			else judge.false_chain.push_back(quaternary_list.size() - 1);
		}
		else//这里就比较特殊了，本来后面如果是or就应该取nextstat（即quaternary.size())作为假出口的
			//但是如果有个这个布尔项有个not在，该布尔项无条件跳转语句就变成了跳到真出口的位置（这里直接加入到假链，后面会进行链交换）
		{
			if (judge.is_swap == 0) quaternary_list[quaternary_list.size() - 1][3] = to_string(quaternary_list.size());
			else judge.false_chain.push_back(quaternary_list.size() - 1);
		}

		judge.codebegin = quaternary_list.size() - 2;

		return judge;
	}

	p = backtrack_pos;//把p位置重置到回溯点
	while (1)
	{
		judge.accept = identifier();
		//如果没有找到标识符又或者这个标识符不是bool类型的变量
		if (judge.accept == -1 || var_list[judge.accept].type != 3) error(26);//布尔量有误（无法识别的布尔量）
		break;
	}//这里应该要回填一些东西
	return judge;
}

//<算术表达式> → <项> arithmetic_sentence_1
back_patch arithmetic_sentence()
{

	back_patch judge;

	judge.accept = xiang();//转到<项>
	if (judge.accept == -1) return judge;//如果judge为-1，说明<项>识别失败，返回-1

	judge.accept = arithmetic_sentence_1();//转到arithmetic_sentence_1
	return judge;
}

//arithmetic_sentence_1 → + <项>arithmetic_se^_1│ - <项>arithmetic_se^_1 | ε
int arithmetic_sentence_1()
{
	int judge = 0;
	string tmp1 = "+", tmp2 = "-";
	if (TOKEN.compare(tmp1) == 0 || TOKEN.compare(tmp2) == 0)
	{
		//如果算术表达式识别到的是+ -号，那么肯定就要进行运算了,而最后结果肯定是要存到tempp中的
		string op = TOKEN;//把运算符号记下来
		TOKEN = getnext();
		judge = xiang();
		if (judge == -1) return judge;//如果judge为-1，说明<项>识别失败，返回-1

		//记录两个项计算出来的值，所以申请两个var_and_type结构
		string temp_var_name = "T";//先构造出临时变量名
		temp_var_name += to_string(temp_var_num + 1); temp_var_num++;
		var_and_type tempp(temp_var_name, 1);

		//OK，执行完+ -号隔壁的第二个项以后，栈中一定会有两个元素，现在把它取出来，生成四元式
		//注意，这里我不打算直接计算获得结果，运算结果是四元式函数要干的事，这里只负责生成四元式
		var_and_type x = cal_stack.top(); cal_stack.pop();
		var_and_type y = cal_stack.top(); cal_stack.pop();
		vector<string> temp;
		temp.push_back(op);
		temp.push_back(y.data);
		temp.push_back(x.data);
		temp.push_back(tempp.data);
		quaternary_list.push_back(temp);
		cal_stack.push(tempp);//有可能是：算术表达式+ - 算术表达式 ，所以这个算术表达式也要存起来

		judge = arithmetic_sentence_1();
	}
	//如果不是加减法，那说明是一个因子或"-因子"组成的四元式，栈里面的东西肯定是可以直接用来赋值的，所以不用管
	return judge;
}

//多项式的一项
// <项> → <因子> xiang_1
int xiang()
{
	int judge = 0;
	judge = factor();
	if (judge == -1) return judge;//如果judge为-1，说明<因子>识别失败，返回-1

	judge = xiang_1();
	return judge;
}

//xiang_1 → * <因子> xiang_1│/ <因子> xiang_1| ε
int xiang_1()
{
	int judge = 0;
	string tmp1 = "*", tmp2 = "/";
	if (TOKEN.compare(tmp1) == 0 || TOKEN.compare(tmp2) == 0)
	{
		//如果一个项是由多个因子相乘除得到的，那么肯定就要进行运算了,而最后结果肯定是要存到tempp中的
		//记录这个项计算出来的值，这里还是要申请两个var_and_type结构，因为项是由多个因子乘除得到的
		string temp_var_name = "T";//先构造出临时变量名
		temp_var_name += to_string(temp_var_num + 1); temp_var_num++;
		var_and_type tempp(temp_var_name, 1);

		string op = TOKEN;//把运算符号记下来
		TOKEN = getnext();
		judge = factor();
		if (judge == -1) return judge;//如果judge为-1，说明<因子>识别失败，返回-1

		//OK，执行完* /号隔壁的第二个因子以后，栈中一定会有两个元素，现在把它取出来，生成四元式
		//注意，这里我不打算直接计算获得结果，运算结果不是生成四元式要干的事，而这里只负责生成四元式
		var_and_type x = cal_stack.top(); cal_stack.pop();//这里读取的是一个项的在栈中的两个操作数（无论因子怎么样，最后递归出来都会有两个操作数）
		var_and_type y = cal_stack.top(); cal_stack.pop();
		vector<string> temp;
		temp.push_back(op);
		temp.push_back(y.data);
		temp.push_back(x.data);
		temp.push_back(tempp.data);
		quaternary_list.push_back(temp);
		cal_stack.push(tempp);//有可能是：算术表达式+ - 算术表达式 ，所以这个算术表达式也要存起来

		judge = xiang_1();
	}
	return judge;
}

//<因子> → <算术量>│- <因子>
int factor()
{
	//因子也有可能是一个算术量，但是如果是算术量，它就会开始递归，因子已经接触到终结符层面
	//无需生成新的临时变量，用算术表达式生成出来的就可以了，但是如果要将一个项取负数，那么就必须生成一个临时变量了


	int judge = 0;
	string tmp = "-";//这个是负号
	if (TOKEN.compare(tmp) == 0)
	{
		//如果有负号必须临时生成一个变量
		string temp_var_name = "T";
		temp_var_name += to_string(temp_var_num + 1); temp_var_num++;
		var_and_type tempp(temp_var_name, 1);

		string op = "-";
		//如果取的是一个负数，那么也要用生成一条四元式
		judge = factor();

		//如果是负号，那么只需要将本来在栈顶的结果(一个因子)取反就可以了
		var_and_type x = cal_stack.top(); cal_stack.pop();
		vector<string> temp;
		temp.push_back("-");
		temp.push_back(x.data);
		temp.push_back("-");//占位符
		temp.push_back(tempp.data);
		quaternary_list.push_back(temp);
		cal_stack.push(tempp);//压栈
	}
	else//如果不是负号，那么就进入算术量的分支，算术量分支没有必要生成四元式，交给算术量压栈处理就是了
	{
		judge = arithmetic_quantity();
		//这里是最重要的，因为栈的源头是终结符，终结符先进栈才有后面的算术运算之类的
	}
	return judge;
}

//<算术量> → <整数>│<标识符>│（ <算术表达式> ）
int arithmetic_quantity()
{
	int i, t = -1, judge = 0;

	t = integer();//看看是不是整数
	if (t != -1) //这里就要进行一下压栈操作
	{
		//终于到达终结符了，获得了一个正确的整数，所以我们现在开始入栈
		var_and_type tempp(bsf_zs_zf[t].data, 1);
		cal_stack.push(tempp);//压栈
		return judge;
	}

	judge = identifier();//看是不是整型标识符
	if (judge != -1)
	{
		//识别到标识符就要入栈
		var_and_type tempp(var_list[judge].data, 1);
		cal_stack.push(tempp);//压栈
		return judge;
	}

	string tmp = "(";//看是不是(<算术表达式>)
	if (TOKEN.compare(tmp) == 0)
	{
		TOKEN = getnext();
		arithmetic_sentence();
		tmp = ")";
		if (TOKEN.compare(tmp) != 0) error(23);//错误23：算术量的"(<算术表达式>)"缺少了右括号
	}
	else judge = -1;

	return judge;
}

//<整数>
int integer()
{
	int i;
	for (i = 1; i <= bsf_zs_zf_num; i++)//找找看是不是整数
	{
		if (bsf_zs_zf[i].type == 37)
		{
			if (TOKEN.compare(bsf_zs_zf[i].data) == 0) { TOKEN = getnext(); return i; }//这里应该要做一些记录数据的操作
		}
	}
	return -1;
}

int relation_character(back_patch &temp)
{
	string tmp1 = "<", tmp2 = "<>", tmp3 = "<=", tmp4 = ">=", tmp5 = ">", tmp6 = "=";
	if (TOKEN.compare(tmp1) == 0) { temp.op = TOKEN; TOKEN = getnext(); return 0; }//这里要记录一下关系符
	if (TOKEN.compare(tmp2) == 0) { temp.op = TOKEN; TOKEN = getnext(); return 0; }//这里要记录一下关系符
	if (TOKEN.compare(tmp3) == 0) { temp.op = TOKEN; TOKEN = getnext(); return 0; }//这里要记录一下关系符
	if (TOKEN.compare(tmp4) == 0) { temp.op = TOKEN; TOKEN = getnext(); return 0; }//这里要记录一下关系符
	if (TOKEN.compare(tmp5) == 0) { temp.op = TOKEN; TOKEN = getnext(); return 0; }//这里要记录一下关系符
	if (TOKEN.compare(tmp6) == 0) { temp.op = TOKEN; TOKEN = getnext(); return 0; }//这里要记录一下关系符

	error(25);//错误25："<算术表达式><关系符><算术表达式>"的关系符没识别出来
}

//这是专门拿来压缩多次无条件跳跃的函数
int compress(int i)
{
	if (quaternary_list[i][0].compare("j") == 0 && quaternary_list[i][1].compare("-") == 0 &&
		quaternary_list[i][2].compare("-") == 0)
	{
		quaternary_list[i][3] = to_string(compress(stoi(quaternary_list[i][3])));
		return stoi(quaternary_list[i][3]);
	}
	else return i;
}

//打印四元式的函数
void print_quaternary()
{
	for (int i = 0; i < quaternary_list.size(); i++)
	{
		compress(i);
		cout << i << " (";
		for (int j = 0; j < quaternary_list[i].size(); j++)
		{
			cout << quaternary_list[i][j];
			if (j < 3) cout << ',';
		}
		cout << ")" << endl;
	}
}

/*测试词法分析程序
int main()
{
	string a = "var  A,B,C:integer;";
	word_analysis(a, 1);
	for (int i = 1; i < num; i++)
	{
		cout << word[i] << " ";
	}
	cout << endl << endl;
	for (int i = 1; i <= bsf_zs_zf_num; i++)
	{
		cout << bsf_zs_zf[i].data << " ";
	}
	cout << endl << endl;

	system("pause");
}
*/
int main()
{
	//char path[100] = "E:\\Desktop\\test.txt";
	char path[100];
	int type = 0;//记录读出的上一个字符是什么类型
	int line = 1;//从来记录读到的行数
	char l[1000];//用来读出txt的一行

	cout << "姓名：李俊祺	班级：计科1班	学号：201635598966\n\n";
	cout << "请输入您要进行词义分析的文件名:";
	cin >> path; cout << endl;
	cout << endl;

	ifstream a(path, ios::in);//打开文件
	if (!a)
	{
		cout << "打开文件失败" << endl << endl;
		system("pause");
	}

	cout << "您输入的程序如下:" << endl << endl;

	//词法分析
	while (a.getline(l, 1000))//用temp读入一行
	{
		string temp = l;//把读出的txt的一行转为string
		cout << temp << endl;
		word_analysis(temp, line);
		line++;
	}
	a.close();

	//标识符表生成
	for (int i = 0; i <= bsf_zs_zf_num; i++)
	{
		if (bsf_zs_zf[i].type == 36)
		{
			bsf[bsf_num] = bsf_zs_zf[i].data;
			bsf_num++;
		}
	}
	line_first_pos.push_back(-1);//为了避免后面vector越界，设置一个多余的元素

	/*cout << "这是检测到的所有单词：" << endl;
	for (int i = 0; i < num; i++)
	{
		cout << word[i] << " ";
	}
	cout << "\n\n这是检测到的标识符、整数、字符常数：" << endl;
	for (int i = 0; i <= bsf_zs_zf_num; i++)
	{
		cout << bsf_zs_zf[i] << " ";
	}
	cout << endl << endl;*/

	//词法分析输出代码
	cout << endl << "下面开始进行词法分析：" << endl << endl;
	for (int i = 1; i <= num; i++)
	{
		//先检查一下是不是标识符、整数、字符常数
		for (int j = 1; j <= bsf_zs_zf_num; j++)
		{
			if (word[i] == bsf_zs_zf[j].data)
			{
				cout << "(" << bsf_zs_zf[j].type << ',' << j << ")" << " ";
				break;
			}
		}

		for (int k = 1; k < 61; k++)
		{
			if (word[i] == symbol[k]) cout << "(" << k << ',' << "-)" << " ";
		}
		if (i % 5 == 0) cout << endl;
	}
	cout << endl << endl << "下面进行语法分析并生成中间代码（四元式）：" << endl << endl;


	//语法分析代码
	grammar_analysis();
	//输出四元式
	print_quaternary();

	cout << endl;
	system("pause");

}

