
#include <iostream>
#include <fstream>
#include <string>

// 检查时间是否为合法的 HH:MM:SS
bool isValidTime(const std::string& time) {
    if (time.size() != 8 ||
        time[2] != ':' ||
        time[5] != ':') {
        return false;
    }

    // 检查数字位置
    for (int i = 0; i < 8; i++) {
        if (i == 2 || i == 5) {
            continue;
        }

        if (time[i] < '0' || time[i] > '9') {
            return false;
        }
    }

    int hour = std::stoi(time.substr(0, 2));
    int minute = std::stoi(time.substr(3, 2));
    int second = std::stoi(time.substr(6, 2));

    return hour >= 0 && hour <= 23 &&
           minute >= 0 && minute <= 59 &&
           second >= 0 && second <= 59;
}

struct LogStats {
    int total_lines = 0;
    int info_count = 0;
    int warning_count = 0;
    int error_count = 0;
    int unknown_count = 0;
    int empty_count = 0;
};

bool isBlankLine(const std::string& line) {
    return line.find_first_not_of(" \t\r\n") == std::string::npos;
}

// 判断日志是否符合关键词条件
bool matchesKeyword(const std::string& line,
                    const std::string& keyword) {
    if (keyword.empty() || keyword == "-") {
        return true;
    }

    return line.find(keyword) != std::string::npos;
}

// 判断日志是否符合时间范围
bool matchesTimeRange(const std::string& line,
                      const std::string& startTime,
                      const std::string& endTime) {
    // 时间过滤关闭时，所有日志都符合时间条件
    if (startTime.empty() || endTime.empty()) {
        return true;
    }

    // 固定格式：YYYY-MM-DD HH:MM:SS
    if (line.size() < 19 ||
        line[10] != ' ' ||
        line[13] != ':' ||
        line[16] != ':') {
        return false;
    }

    std::string logTime = line.substr(11, 8);

    return logTime >= startTime && logTime <= endTime;
}

// 分析一行日志
void parseLine(const std::string& line, LogStats& stats) {
    if (isBlankLine(line)) {
        stats.empty_count++;
    } else if (line.find(" INFO ") != std::string::npos) {
        stats.info_count++;
    } else if (line.find(" WARNING ") != std::string::npos) {
        stats.warning_count++;
    } else if (line.find(" ERROR ") != std::string::npos) {
        stats.error_count++;
    } else {
        stats.unknown_count++;
    }
}

// 读取、筛选并分析日志
bool analyzeLogFile(const std::string& filePath,
                    LogStats& stats,
                    const std::string& keyword,
                    const std::string& startTime,
                    const std::string& endTime) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Error: Cannot open log file: "
                  << filePath << std::endl;
        return false;
    }

    std::string line;

    while (std::getline(file, line)) {
        // 关键词不匹配，跳过当前行
        if (!matchesKeyword(line, keyword)) {
            continue;
        }

        // 时间不匹配，跳过当前行
        if (!matchesTimeRange(line, startTime, endTime)) {
            continue;
        }

        // 只有通过筛选的行才参与统计
        stats.total_lines++;
        parseLine(line, stats);
    }
    if (stats.total_lines == 0) {
    std::cout << "Warning: Log file is empty." << std::endl;
}

    return true;
}

#include <ostream>

// 将报告内容写入任意输出流
void writeReport(std::ostream& out, const LogStats& stats) {
    out << "========== Log Analysis ==========" << std::endl;
    out << "Matched lines: " << stats.total_lines << std::endl;
    out << "INFO         : " << stats.info_count << std::endl;
    out << "WARNING      : " << stats.warning_count << std::endl;
    out << "ERROR        : " << stats.error_count << std::endl;
    out << "Unknown      : " << stats.unknown_count << std::endl;
    out << "Empty lines  : " << stats.empty_count << std::endl;
    out << "==================================" << std::endl;
}

// 在终端显示报告
void printReport(const LogStats& stats) {
    writeReport(std::cout, stats);
}

// 将报告保存到文件
bool saveReport(const std::string& reportPath,
                const LogStats& stats) {
    std::ofstream reportFile(reportPath);

    if (!reportFile.is_open()) {
        std::cerr << "Error: Cannot create report file: "
                  << reportPath << std::endl;
        return false;
    }

    writeReport(reportFile, stats);

    if (!reportFile) {
        std::cerr << "Error: Failed while writing report: "
                  << reportPath << std::endl;
        return false;
    }

    std::cout << "Report saved to: "
              << reportPath << std::endl;

    return true;
}


int main(int argc, char* argv[]) {
    // 支持：
    // ./log_analyzer <file>
    // ./log_analyzer <file> <keyword>
    // ./log_analyzer <file> <keyword-or-> <start-time> <end-time>

    if (argc != 2 && argc != 3 && argc != 5) {
        std::cerr << "Usage:" << std::endl;
        std::cerr << "  " << argv[0] << " <log_file>" << std::endl;
        std::cerr << "  " << argv[0] << " <log_file> <keyword>" << std::endl;
        std::cerr << "  " << argv[0]
                  << " <log_file> <keyword-or-> <start-time> <end-time>"
                  << std::endl;
        return 1;
    }

    std::string filePath = argv[1];
    std::string keyword;
    std::string startTime;
    std::string endTime;

    if (argc == 3) {
        keyword = argv[2];
    }

    if (argc == 5) {
        keyword = argv[2];
        startTime = argv[3];
        endTime = argv[4];
    }

    // 简单检查时间字符串格式
    if (!startTime.empty() &&
    (!isValidTime(startTime) || !isValidTime(endTime))) {
    std::cerr << "Error: Invalid time. "
              << "Use HH:MM:SS with valid values."
              << std::endl;
    return 1;
}

    if (!startTime.empty() && startTime > endTime) {
        std::cerr << "Error: Start time must not be later than end time."
                  << std::endl;
        return 1;
    }

    LogStats stats;

    if (!analyzeLogFile(filePath, stats, keyword,
                        startTime, endTime)) {
        return 1;
    }

    printReport(stats);

if (!saveReport("reports/analysis_report.txt", stats)) {
    return 1;
}

return 0;
}
