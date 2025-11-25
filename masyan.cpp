#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

std::unordered_map<std::string, std::string> variables;

std::string eval_expr(const std::string& expr) {
    std::vector<std::string> parts;
    std::string current;
    bool in_quotes = false;

    for (char c : expr) {
        if (c == '"') {
            in_quotes = !in_quotes;
            current += c;
        } else if (c == '+' && !in_quotes) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(current);

    std::string result;
    for (auto& part : parts) {
        std::string p = part;
        p.erase(0, p.find_first_not_of(" \t"));
        p.erase(p.find_last_not_of(" \t") + 1);
        if (p.size() >= 2 && p.front() == '"' && p.back() == '"') {
            result += p.substr(1, p.size() - 2);
        } else if (variables.count(p)) {
            result += variables[p];
        } else {
            result += p;
        }
    }
    return result;
}

// ======== Проверка условий ========
bool check_condition(const std::string& cond) {
    std::vector<std::string> ops = {"==","!=","<=",">=","<",">"};
    for (auto& op : ops) {
        auto pos = cond.find(op);
        if (pos != std::string::npos) {
            std::string left = cond.substr(0,pos);
            std::string right = cond.substr(pos+op.size());
            std::string left_val = eval_expr(left);
            std::string right_val = eval_expr(right);
            double l=0,r=0;
            try { l = std::stod(left_val); r = std::stod(right_val); } catch(...) {}
            if (op=="==") return left_val==right_val;
            if (op=="!=") return left_val!=right_val;
            if (op=="<=") return l<=r;
            if (op==">=") return l>=r;
            if (op=="<") return l<r;
            if (op==">") return l>r;
        }
    }
    return false;
}

// ======== Интерпретатор строки ========
struct IfState {
    bool active = false;
    bool done = false;
};
IfState* current_if = nullptr;

void run_line(const std::string& line) {
    if(line.substr(0,3) == "if ") {
        std::string cond = line.substr(3);
        if(cond.back() == ':') cond.pop_back();
        bool res = check_condition(cond);
        current_if = new IfState();
        current_if->active = res;
        current_if->done = res;
        return;
    }

    if(line.substr(0,5) == "elif ") {
        if(!current_if) {
            std::cout << "Error: 'elif' without 'if'\n";
            return;
        }
        if(current_if->done) {
            current_if->active = false;
        } else {
            std::string cond = line.substr(5);
            if(cond.back() == ':') cond.pop_back();
            bool res = check_condition(cond);
            current_if->active = res;
            if(res) current_if->done = true;
        }
        return;
    }

    if(line.substr(0,5) == "else:") {
        if(!current_if) {
            std::cout << "Error: 'else' without 'if'\n";
            return;
        }
        current_if->active = !current_if->done;
        current_if->done = true;
        return;
    }

    // если блок if не активен — пропускаем
    if(current_if && !current_if->active) return;

    // print
    if(line.substr(0,6)=="print ") {
        std::string expr = line.substr(6);
        std::cout << eval_expr(expr) << std::endl;
        return;
    }

    // add
    if(line.substr(0,4)=="add ") {
        std::istringstream iss(line.substr(4));
        std::string a_str,b_str,varname;
        iss >> a_str >> b_str >> varname;
        int a = std::stoi(eval_expr(a_str));
        int b = std::stoi(eval_expr(b_str));
        int res = a+b;
        if(!varname.empty()) variables[varname] = std::to_string(res);
        else std::cout << res << std::endl;
        return;
    }

    // mul
    if(line.substr(0,4)=="mul ") {
        std::istringstream iss(line.substr(4));
        std::string a_str,b_str,varname;
        iss >> a_str >> b_str >> varname;
        int a = std::stoi(eval_expr(a_str));
        int b = std::stoi(eval_expr(b_str));
        int res = a*b;
        if(!varname.empty()) variables[varname] = std::to_string(res);
        else std::cout << res << std::endl;
        return;
    }

    // div
    if(line.substr(0,4)=="div ") {
        std::istringstream iss(line.substr(4));
        std::vector<std::string> parts;
        std::string token;

        while(iss >> token) parts.push_back(token);

        std::string dtype = "int";
        std::string varname = "";
        double a=0, b=0;

        // определяем тип и переменную
        for(auto it = parts.begin(); it != parts.end(); ) {
            if(*it == "int" || *it == "float") {
                dtype = *it;
                it = parts.erase(it); // удаляем из списка
            } else {
                ++it;
            }
        }

        if(parts.size() < 2) {
            std::cout << "Error: div requires at least 2 numbers\n";
            return;
        }

        a = std::stod(eval_expr(parts[0]));
        b = std::stod(eval_expr(parts[1]));

        if(parts.size() >= 3) varname = parts[2]; // третье — переменная

        double res = 0;
        if(dtype=="int") {
            if(int(b)==0) { std::cout << "Error: division by zero\n"; return; }
            res = int(a)/int(b);
        } else {
            if(b==0) { std::cout << "Error: division by zero\n"; return; }
            res = a/b;
        }

        // сохранить или вывести
        if(!varname.empty()) {
            if(dtype=="int") variables[varname] = std::to_string(int(res));
            else variables[varname] = std::to_string(res);
        } else {
            if(dtype=="int") std::cout << int(res) << std::endl;
            else std::cout << res << std::endl;
        }

        return;
    }


    // input
    if(line.substr(0,6)=="input ") {
        std::istringstream iss(line.substr(6));
        std::string varname,prompt;
        iss >> varname;
        std::getline(iss,prompt);
        prompt = eval_expr(prompt);
        std::cout << prompt;
        std::string val; std::getline(std::cin,val);
        variables[varname]=val;
        return;
    }

    // присвоение
    auto eq_pos = line.find('=');
    if(eq_pos!=std::string::npos) {
        std::string var = line.substr(0,eq_pos);
        std::string expr = line.substr(eq_pos+1);
        var.erase(std::remove_if(var.begin(),var.end(),isspace), var.end());
        variables[var] = eval_expr(expr);
        return;
    }

    std::cout << "Unknown command: " << line << std::endl;
}

// ======== Чтение файла ========
void run_file(const std::string& filename) {
    std::ifstream f(filename);
    if(!f) { std::cerr<<"Error: cannot open file\n"; return; }
    std::string line;
    while(std::getline(f,line)) run_line(line);
}

// ======== main ========
int main(int argc,char** argv) {
    if(argc<2) { std::cerr<<"Error: missing arguments!\n"; return 1; }
    run_file(argv[1]);
    return 0;
}
