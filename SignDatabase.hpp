#ifndef SIGN_DATABASE_HPP
#define SIGN_DATABASE_HPP

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <filesystem> // C++17 标准库，用于扫描文件夹

namespace fs = std::filesystem;

class SignDatabase {
public:
    // 加载指定文件夹下的所有 .bin 文件
    bool loadFromFolder(const std::string& folderPath) {
        bool success = false;

        // 检查路径是否存在
        if (!fs::exists(folderPath)) {
            std::cerr << "错误: 找不到文件夹 " << folderPath << std::endl;
            return false;
        }

        // 遍历文件夹
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.path().extension() == ".bin") {
                if (loadSingleFile(entry.path().string())) {
                    success = true;
                }
            }
        }
        return success;
    }

    const std::map<std::string, std::vector<float>>& getTemplates() const {
        return templates;
    }

private:
    std::map<std::string, std::vector<float>> templates;

    // 解析单个文件的核心逻辑（对应你的 Python 写入逻辑）
    bool loadSingleFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) return false;

        // 1. 读取名字长度 (int) - 对应 Python: struct.pack("<i", len(name_bytes))
        int nameLen = 0;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(int));

        // 2. 读取名字字符串 - 对应 Python: f.write(name_bytes)
        std::string gestureName(nameLen, '\0');
        file.read(&gestureName[0], nameLen);

        // 3. 读取维度 (3个 int) - 对应 Python: struct.pack("<iii", 30, 21, 3)
        int frames, points, coords;
        file.read(reinterpret_cast<char*>(&frames), sizeof(int));
        file.read(reinterpret_cast<char*>(&points), sizeof(int));
        file.read(reinterpret_cast<char*>(&coords), sizeof(int));

        // 4. 读取关键点数据 (float 数组) - 对应 Python: landmarks.flatten().tofile(f)
        size_t totalFloats = frames * points * coords;
        std::vector<float> data(totalFloats);
        file.read(reinterpret_cast<char*>(data.data()), totalFloats * sizeof(float));

        if (file) {
            templates[gestureName] = data;
            std::cout << "✅ 已加载手语模板: " << gestureName << " (数据大小: " << totalFloats << ")" << std::endl;
            return true;
        } else {
            std::cerr << "❌ 读取失败: " << filepath << std::endl;
            return false;
        }
    }
};

#endif