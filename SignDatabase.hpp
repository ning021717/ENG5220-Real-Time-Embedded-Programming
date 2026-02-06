#ifndef SIGN_DATABASE_HPP
#define SIGN_DATABASE_HPP

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <stdexcept>

class SignDatabase {
public:
    // 定义手语模板的大小：30帧 * 21个关键点 * 3坐标(x,y,z)
    const size_t TEMPLATE_SIZE = 30 * 21 * 3;

    // 载入二进制模板文件
    bool loadTemplate(const std::string& actionName, const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;

        // 检查文件大小是否匹配 30*21*3*sizeof(float)
        std::streamsize size = file.tellg();
        if (size != (std::streamsize)(TEMPLATE_SIZE * sizeof(float))) {
            return false;
        }

        file.seekg(0, std::ios::beg);
        std::vector<float> buffer(TEMPLATE_SIZE);
        if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            templates[actionName] = buffer;
            return true;
        }
        return false;
    }

    const std::vector<float>& getTemplate(const std::string& name) {
        return templates.at(name); // 若不存在会抛出异常，符合 failsafe 要求
    }

private:
    std::map<std::string, std::vector<float>> templates;
};
#endif