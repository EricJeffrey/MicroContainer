#if !defined(APP_H)
#define APP_H

#include <unordered_map>
#include <functional>
#include <iostream>
#include <string>

using std::function;
using std::string;
using std::unordered_map;

class SubCommand {
public:
    int argCount;
    function<void(const char *argv[])> run;
    string argDesc;
    string usageDesc;

    SubCommand(int argc, function<void(const char *argv[])> run, const string &args,
               const string &desc)
        : argCount(argc), run(run), argDesc(args), usageDesc(desc) {}
    ~SubCommand() {}

    string str() { return argDesc + "\t" + usageDesc; }
};

class App {
private:
    string appName;
    string desc;
    unordered_map<string, SubCommand> commands;

public:
    App(const string &name, const string &desc, const unordered_map<string, SubCommand> &cmds)
        : appName(name), desc(desc), commands(cmds) {}
    ~App() {}

    void start(int argc, const char *argv[]) {
        if (argc > 1) {
            auto it = commands.find(argv[1]);
            if (it != commands.end() && it->second.argCount == argc - 2) {
                it->second.run(argv);
                return;
            }
        }
        // print usage
        using std::cerr;
        using std::endl;
        cerr << appName << " - " << desc << endl << endl << "Usage:" << endl;
        for (auto &&cmdPair : commands)
            cerr << "\t" << appName << " " << cmdPair.first << " " << cmdPair.second.str() << endl;
    }
};

#endif // APP_H
