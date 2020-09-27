#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#include "markdown.h"

using namespace std;

markdownParser::markdownParser(const string& filename)
	: _root(new Node(nul))
	, _filename(filename)
{}

void markdownParser::transform() {
	// 打开文件
	ifstream fin(_filename);
	if (!fin.is_open()) {
		cout << "file: " << _filename << "open failed" << endl;
		return;
	}

	bool inblock = false;
	// 读取内容(按行读取)
	string rowStr;
	while (!fin.eof()) {
		getline(fin, rowStr);
		// 预处理(处理行首空格)
		const char* start = processStr(rowStr.c_str());

		// 判断是否为空内容
		if (!inblock && start == nullptr)
			continue;

		// 判断是否为水平分割线
		if (!inblock && isCutLine(start)) {
			_root->_child.push_back(new Node(hr));
			continue;
		}

		// 解析行首语法
		pair<int, const char*> typeRet = parseType(start);

		// 创建语法节点
		// 代码块节点
		if (typeRet.first == blockcode) {
			// 判断是代码块的开始位置还是结束位置
			if (!inblock) {
				// 代码块的起始位置， 创建代码块节点
				_root->_child.push_back(new Node(blockcode));
			}
			//代码块结束位置， 不需要创建新的节点
			inblock = !inblock;
			continue;
		}

		// 判断是否为代码块内的代码
		if (inblock) {
			_root->_child.back()->elem[0] += rowStr;
			_root->_child.back()->elem[0] += '\n';
			continue;
		}

		// 段落节点
		if (typeRet.first == paragraph) {
			// 创建一个段落节点
			_root->_child.push_back(new Node(paragraph));
			//挨个字符插入
			insert(_root->_child.back(), typeRet.second);
			continue;
		}

		// 标题节点
		if (typeRet.first >= h1 && typeRet.first <= h6) {
			_root->_child.push_back(new Node(typeRet.first));		// 创建标题节点
			_root->_child.back()->elem[0] = typeRet.second;			// 插入标题内容
		}

		// 无序列表节点
		if (typeRet.first == ul) {
			// 判断是否为无序列表的第一项(文档开始，或者语法树中最后一个节点不是无序列表节点)
			if (_root->_child.empty() || _root->_child.back()->_type != ul) {
				_root->_child.push_back(new Node(ul));				// 创建无序列表节点
			}
			Node* uNode = _root->_child.back();
			uNode->_child.push_back(new Node(li));
			insert(uNode->_child.back(), typeRet.second);			// 给列表子节点插入内容
			continue;
		}

		// 有序列表节点
		if (typeRet.first == ol) {
			if (_root->_child.empty() || _root->_child.back()->_type != ol) {
				_root->_child.push_back(new Node(ol));
			}
			Node* oNode = _root->_child.back();
			oNode->_child.push_back(new Node(li));
			insert(oNode->_child.back(), typeRet.second);
			continue;
		}

		// 引用节点
		if (typeRet.first == quote) {
			_root->_child.push_back(new Node(quote));
			insert(_root->_child.back(), typeRet.second);
		}

		cout << "transform" << endl;
	}


	// 展开语法树，生成html文档
	dfs(_root);
}

// 把语法树转换成html文档(深度优先)
void markdownParser::dfs(Node* _root) {
	// 插入前置标签
	_content += frontTag[_root->_type];

	// 插入节点内容
	// 网址：<a href="网址">链接名称</a>
	if (_root->_type == href) {
		_content += "<a href=\"";
		_content += _root->elem[1];
		_content += "\">";
		_content += _root->elem[0];
		_content += "</a>";
	}
	// 图片：<img alt="图片名称" src="地址" />
	else if (_root->_type == image) {
		_content += "<img alt=\"";
		_content += _root->elem[0];
		_content += "\" src=\"";
		_content += _root->elem[1];
		_content += "\" />";
	}
	else
		_content += _root->elem[0];

	// 处理孩子节点
	for (Node* ch : _root->_child)
		dfs(ch);

	// 插入后置标签
	_content += backTag[_root->_type];
}

void markdownParser::insert(Node* curNode, const char* str) {
	bool incode = false;							// 判断是否为行内代码
	bool instrong = false;							// 判断是否为粗体
	bool inem = false;								// 判断是否为斜体

	// 先创建一个文本节点，如果第一个字符为纯文本，可以存入纯文本节点中
	curNode->_child.push_back(new Node(nul));
	int len = strlen(str);
	for (int i = 0; i < len; i++) {
		// 行内代码:	`行内代码`
		if (str[i] == '`') {
			if (incode)
				// 后一个：行内代码结束，创建一个新的节点，存放后续的内容
				curNode->_child.push_back(new Node(nul));
			else
				// 前一个：创建一个行内代码节点
				curNode->_child.push_back(new Node(code));
			incode = !incode;
			continue;
		}

		// 粗体：**		**(不出现在行内代码内)
		if (str[i] == '*' && i + 1 < len && str[i + 1] == '*' && !incode) {
			if (instrong)
				curNode->_child.push_back(new Node(nul));		// 后一个
			else
				curNode->_child.push_back(new Node(strong));	// 前一个：创建一个粗体节点
			instrong = !instrong;
			i++;
			continue;
		}

		// 斜体：_	_(不出现在行内代码内)
		if (str[i] == '_' && !incode) {
			if (inem)
				curNode->_child.push_back(new Node(nul));
			else
				curNode->_child.push_back(new Node(em));
			inem = !inem;
			continue;
		}

		// 图片：![图片名称](地址)
		if (str[i] == '!' && i + 1 < len && str[i + 1] == '[') {
			// 创建图片节点
			curNode->_child.push_back(new Node(image));
			Node* iNode = curNode->_child.back();
			i += 2;												// 跳过![
			for (; i < len && str[i] != ']'; i++) {
				iNode->elem[0] += str[i];
			}
			i += 2;												// 跳过](
			for (; i < len && str[i] != ')'; i++) {
				iNode->elem[1] += str[i];
			}
			// 创建一个新的节点，存放后续的内容
			curNode->_child.push_back(new Node(nul));
			continue;
		}

		// 链接：[链接名称](网址), 不出现在行内代码内
		if (str[i] == '[' && !incode) {
			// 创建一个链接节点
			curNode->_child.push_back(new Node(href));
			Node* hNode = curNode->_child.back();
			i++;
			// 存放链接名称和网址
			for (; i < len && str[i] != ']'; i++) {
				hNode->elem[0] += str[i];
			}
			i += 2;
			for (; i < len && str[i] != ')'; i++) {
				hNode->elem[1] += str[i];
			}
			// 创建新节点存放后续内容
			curNode->_child.push_back(new Node(nul));
			continue;
		}

		// 纯文本
		curNode->_child.back()->elem[0] += str[i];
	}
}

// 解析行首语法
// 返回值：语法类型 + 语法对应的内容起始位置
pair<int, const char*> markdownParser::parseType(const char* str) {
	//1. 解析标题# + 空格
	const char* ptr = str;
	int titleNum = 0;
	while (*ptr && *ptr == '#') {
		ptr++;
		titleNum++;
	}
	if (*ptr == ' ' && (titleNum > 0 && titleNum <= 6))
		return make_pair(h1 + titleNum - 1, ptr + 1);

	// 如果不符合标题的语法，需要重新解析
	ptr = str;

	// 2.解析代码块 ： ```代码内容```
	if (strncmp(ptr, "```", 3) == 0)
		return make_pair(blockcode, ptr + 3);

	// 3.无序列表：‘-’+ 空格
	if (strncmp(ptr, "- ", 2) == 0)
		return make_pair(ul, ptr + 2);

	//4. 有序列表：数字字符 + ‘.’+ 空格
	if (*ptr >= '0' && *ptr <= '9') {
		//遍历完数字
		while (*ptr && (*ptr >= '0' && *ptr <= '9'))
			ptr++;
		if (*ptr && *ptr == '.') {
			ptr++;
			if (*ptr && *ptr == ' ')
				return make_pair(ol, ptr + 1);
		}
	}

	ptr = str;
	// 5.引用：‘>’+ 空格
	if (strncmp(ptr, "> ", 2) == 0)
		return make_pair(quote, ptr + 2);

	// 其他语法：统一解析为段落
	return make_pair(paragraph, str);
}

// 判断是否是水平分割线
bool markdownParser::isCutLine(const char* str) {
	int cnt = 0;
	while (*str && *str == '-') {
		str++;
		cnt++;
	}
	return cnt >= 3;
}

// 去除行首的空格
const char* markdownParser::processStr(const char* str) {
	while (*str) {
		if (*str == ' ' || *str == '\t')
			str++;
		else
			break;
	}
	if (*str == '\0')
		return nullptr;
	return str;
}

string markdownParser::getContent() const {
	return _content;
}

// 生成html文件
void markdownParser::generateHtml() {
	std::string head = "<!DOCTYPE html><html><head>\
        <meta charset=\"utf-8\">\
        <title>Markdown</title>\
        <link rel=\"stylesheet\" href=\"github-markdown.css\">\
        </head><body><article class=\"markdown-body\">";
	std::string end = "</article></body></html>";

	ofstream fout("markdown.html");
	fout << head << _content << end;
}

void markdownParser::destory(Node* root) {
	if (root) {
		for (Node* ch : root->_child)
			destory(ch);
		destory(root);
	}
}

markdownParser::~markdownParser() {
	if (_root) {
		destory(_root);
	}
}
