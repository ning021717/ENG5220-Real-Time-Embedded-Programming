#ifndef TRANSLATOR_HPP
#define TRANSLATOR_HPP

#include <string>
#include <iostream>
#include <map>

class Translator {
public:
    void translate(std::string action) {
        // 映射库：将内部 ID 转换为人类语言
        static std::map<std::string, std::string> dict = {
            {"Action_A", "你好 (Hello)"},
            {"Action_B", "谢谢 (Thank You)"}
        };

        if (action != lastOutput) { // 简单防抖：避免重复输出
            std::cout << "\r[翻译]: " << (dict.count(action) ? dict[action] : "识别中...") << std::flush;
            lastOutput = action;
        }
    }
private:
    std::string lastOutput;
};
#endif
// Created by 金扫帚奖最佳演员 on 2026/2/6.
//

#ifndef OPENCV_TEST_TRANSLATOR_H
#define OPENCV_TEST_TRANSLATOR_H

#endif //OPENCV_TEST_TRANSLATOR_H