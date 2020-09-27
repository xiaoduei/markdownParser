/* 
	简单的markdown解析器
	将输入的markdown文档转换为html文档
	常用语法：段落，标题，图片，链接，代码块，行内代码，引用，列表(有序，无序)，粗体，斜体
*/

#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

// nul:表示纯文本节点
enum Token {
	nul = 0, paragraph = 1, href = 2, ul = 3,
	ol = 4, li = 5, em = 6, strong = 7,
	hr = 8, image = 9, quote = 10, h1 = 11,
	h2 = 12, h3 = 13, h4 = 14, h5 = 15,
	h6 = 16, blockcode = 17, code = 18,
};

// HTML前置标签 
const std::string frontTag[] = {"", "<p>", "", "<ul>", "<ol>", "<li>",
								"<em>", "<strong>", "<hr color=#CCCCCC size=1 />",
								"", "<blockquote>", "<h1>", "<h2>", "<h3>", "<h4>",
								"<h5>", "<h6>","<pre><code>", "<code>" };
// HTML 后置标签 
const std::string backTag[] = { "", "</p>", "", "</ul>", "</ol>", "</li>",
								"</em>", "</strong>", "", "", "</blockquote>",
								"</h1>", "</h2>", "</h3>", "</h4>", "</h5>",
								"</h6>", "</code></pre>", "</code>" };

class Node {
public:
	int _type;					//语法类型
	vector<Node*> _child;		//孩子节点
	string elem[2];				//elem[1]:保存需要显示的内容， elem[2]:保存网址，路径

	Node(int type)
		:_type(type)
	{}
};

class markdownParser {
private:
	Node* _root;				//语法树根节点
	string _filename;			//文件名
	string _content;			//存放html文档内容

public:
	markdownParser(const string& filename);
	void transform();
	// 把语法树转换成html文档(深度优先)
	void dfs(Node* _root);
	void insert(Node* curNode, const char* str);
	// 解析行首语法
	// 返回值：语法类型 + 语法对应的内容起始位置
	pair<int, const char*> parseType(const char* str);
	// 判断是否是水平分割线
	bool isCutLine(const char* str);
	// 去除行首的空格
	const char* processStr(const char* str);

	string getContent() const;
	// 生成html文件
	void generateHtml();
	void destory(Node* root);
	~markdownParser();
};
