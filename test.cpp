#include "markdown.h"

int main() {
	markdownParser mp("test.md");
	mp.transform();
	mp.generateHtml();
	cout << "这是一个中文" << endl;
	getchar();
	return 0;
}