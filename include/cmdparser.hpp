/*
 This file is part of the C++ CmdParser utility.
 Copyright (c) 2015 - 2016 Florian Rappl
 */

#pragma once
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <functional>

namespace cli {
    struct CallbackArgs {
        const std::vector<std::string>& arguments;
        std::ostream& output;
        std::ostream& error;
    };
    class Parser {
    private:
        class CmdBase {
        public:
            explicit CmdBase(const std::string& name, const std::string& alternative, const std::string& description, bool required) :
            name(name),
            command(name.size() > 0 ? "-" + name : ""),
            alternative(alternative.size() > 0 ? "--" + alternative : ""),
            description(description),
            required(required),
            handled(false),
            arguments({}) {
            }
            
            virtual ~CmdBase() {
            }
            
            std::string name;
            std::string command;
            std::string alternative;
            std::string description;
            bool required;
            bool handled;
            std::vector<std::string> arguments;
            
            virtual std::string print_value() const = 0;
            virtual bool parse(std::ostream& output, std::ostream& error) = 0;
            
            bool is(const std::string& given) const {
                return given == command || given == alternative;
            }
        };
        
        template<typename T>
        class CmdFunction final : public CmdBase {
        public:
            explicit CmdFunction(const std::string& name, const std::string& alternative, const std::string& description, bool required) :
            CmdBase(name, alternative, description, required) {
            }
            
            virtual bool parse(std::ostream& output, std::ostream& error) {
                try {
                    CallbackArgs args { arguments, output, error };
                    value = callback(args);
                    return true;
                } catch (...) {
                    return false;
                }
            }
            
            virtual std::string print_value() const {
                return "";
            }
            
            std::function<T(CallbackArgs&)> callback;
            T value;
        };
        
        template<typename T>
        class CmdArgument final : public CmdBase {
        public:
            explicit CmdArgument(const std::string& name, const std::string& alternative, const std::string& description, bool required) :
            CmdBase(name, alternative, description, required) {
            }
            
            virtual bool parse(std::ostream& output, std::ostream& error) {
                try {
                    value = Parser::parse(arguments, value);
                    return true;
                } catch (...) {
                    return false;
                }
            }
            
            virtual std::string print_value() const {
                return stringify(value);
            }
            
            T value;
        };
        
        static int parse(const std::vector<std::string>& elements, const int&) {
            if (elements.size() != 1)
                throw std::bad_cast();
            
            return std::stoi(elements[0]);
        }
        
        static bool parse(const std::vector<std::string>& elements, const bool& defval) {
            if (elements.size() != 0)
                throw std::runtime_error("A boolean command line parameter cannot have any arguments.");
            
            return !defval;
        }
        
        static double parse(const std::vector<std::string>& elements, const double&) {
            if (elements.size() != 1)
                throw std::bad_cast();
            
            return std::stod(elements[0]);
        }
        
        static float parse(const std::vector<std::string>& elements, const float&) {
            if (elements.size() != 1)
                throw std::bad_cast();
            
            return std::stof(elements[0]);
        }
        
        static long double parse(const std::vector<std::string>& elements, const long double&) {
            if (elements.size() != 1)
                throw std::bad_cast();
            
            return std::stold(elements[0]);
        }
        
        static unsigned int parse(const std::vector<std::string>& elements, const unsigned int&) {
            if (elements.size() != 1)
                throw std::bad_cast();
            
            return static_cast<unsigned int>(std::stoul(elements[0]));
        }
        
        static unsigned long parse(const std::vector<std::string>& elements, const unsigned long&) {
            if (elements.size() != 1)
                throw std::bad_cast();
            
            return std::stoul(elements[0]);
        }
        
        static long parse(const std::vector<std::string>& elements, const long&) {
            if (elements.size() != 1)
                throw std::bad_cast();
            
            return std::stol(elements[0]);
        }
        
        static std::string parse(const std::vector<std::string>& elements, const std::string&) {
            if (elements.size() != 1)
                throw std::bad_cast();
            
            return elements[0];
        }
        
        template<class T>
        static std::vector<T> parse(const std::vector<std::string>& elements, const std::vector<T>&) {
            const T defval = 0;
            std::vector<T> values { };
            std::vector<std::string> buffer(1);
            
            for (const auto& element : elements) {
                buffer[0] = element;
                values.push_back(parse(buffer, defval));
            }
            
            return values;
        }
        
        template<class T>
        static std::string stringify(const T& value) {
            return std::to_string(value);
        }
        
        template<class T>
        static std::string stringify(const std::vector<T>& values) {
            std::stringstream ss { };
            ss << "[ ";
            
            for (const auto& value : values) {
                ss << stringify(value) << " ";
            }
            
            ss << "]";
            return ss.str();
        }
        
        static std::string stringify(const std::string& str) {
            return str;
        }
        
    public:
        explicit Parser(int argc, const char** argv) :
        _appname(argv[0]) {
            for (int i = 1; i < argc; ++i) {
                _arguments.push_back(argv[i]);
            }
            enable_help();
        }
        
        explicit Parser(int argc, char** argv) :
        _appname(argv[0]) {
            for (int i = 1; i < argc; ++i) {
                _arguments.push_back(argv[i]);
            }
            enable_help();
        }
        
        ~Parser() {
            for (int i = 0, n = _commands.size(); i < n; ++i) {
                delete _commands[i];
            }
        }
        
        bool has_help() const {
            for (const auto command : _commands) {
                if (command->name == "h" && command->alternative == "--help") {
                    return true;
                }
            }
            
            return false;
        }
        
        void enable_help() {
            set_callback("h", "help", std::function<bool(CallbackArgs&)>([this](CallbackArgs& args){
                args.output << this->usage();
                exit(0);
                return false;
            }));
        }
        
        void disable_help() {
            for (auto command = _commands.begin(); command != _commands.end(); ++command) {
                if ((*command)->name == "h" && (*command)->alternative == "--help") {
                    _commands.erase(command);
                    break;
                }
            }
        }
        
        template<typename T>
        void set_default(bool is_required, const std::string& description = "") {
            auto command = new CmdArgument<T> { "", "", description, is_required };
            _commands.push_back(command);
        }
        
        template<typename T>
        void set_required(const std::string& name, const std::string& alternative, const std::string& description = "") {
            auto command = new CmdArgument<T> { name, alternative, description, true };
            _commands.push_back(command);
        }
        
        template<typename T>
        void set_optional(const std::string& name, const std::string& alternative, T defaultValue, const std::string& description = "") {
            auto command = new CmdArgument<T> { name, alternative, description, false };
            command->value = defaultValue;
            _commands.push_back(command);
        }
        
        template<typename T>
        void set_callback(const std::string& name, const std::string& alternative, std::function<T(CallbackArgs&)> callback, const std::string& description = "") {
            auto command = new CmdFunction<T> { name, alternative, description, false };
            command->callback = callback;
            _commands.push_back(command);
        }
        
        inline void run_and_exit_if_error() {
            if (run() == false) {
                exit(1);
            }
        }
        
        inline bool run() {
            return run(std::cout, std::cerr);
        }
        
        inline bool run(std::ostream& output) {
            return run(output, std::cerr);
        }
        
        bool run(std::ostream& output, std::ostream& error) {
            if (_arguments.size() > 0) {
                auto current = find_default();
                
                for (int i = 0, n = _arguments.size(); i < n; ++i) {
                    auto isarg = _arguments[i].size() > 0 && _arguments[i][0] == '-';
                    auto associated = isarg ? find(_arguments[i]) : nullptr;
                    
                    if (associated != nullptr) {
                        current = associated;
                        associated->handled = true;
                    } else if (current == nullptr) {
                        error << no_default();
                        return false;
                    } else {
                        current->arguments.push_back(_arguments[i]);
                    }
                }
            }
            
            for (auto command : _commands) {
                if (command->required && !command->handled) {
                    error << howto_required(command);
                    return false;
                }
            }
            
            for (auto command : _commands) {
                if (command->handled && !command->parse(output, error)) {
                    error << howto_use(command);
                    return false;
                }
            }
            
            return true;
        }
        
        template<typename T>
        T get(const std::string& name) const {
            for (const auto& command : _commands) {
                if (command->name == name) {
                    auto cmd = dynamic_cast<CmdArgument<T>*>(command);
                    
                    if (cmd == nullptr) {
                        throw std::runtime_error("Invalid usage of the parameter " + name + " detected.");
                    }
                    
                    return cmd->value;
                }
            }
            
            throw std::runtime_error("The parameter " + name + " could not be found.");
        }
        
        template<typename T>
        T get_if(const std::string& name, std::function<T(T)> callback) const {
            auto value = get<T>(name);
            return callback(value);
        }
        
        int requirements() const {
            int count = 0;
            
            for (const auto& command : _commands) {
                if (command->required) {
                    ++count;
                }
            }
            
            return count;
        }
        
        int commands() const {
            return static_cast<int>(_commands.size());
        }
        
        inline const std::string& app_name() const {
            return _appname;
        }
        
    protected:
        CmdBase* find(const std::string& name) {
            for (auto command : _commands) {
                if (command->is(name)) {
                    return command;
                }
            }
            
            return nullptr;
        }
        
        CmdBase* find_default() {
            for (auto command : _commands) {
                if (command->name == "") {
                    return command;
                }
            }
            
            return nullptr;
        }
        
        std::string usage() const {
            std::stringstream ss { };
            ss << "Available parameters:\n\n";
            
            for (const auto& command : _commands) {
                ss << "  " << command->command << "\t" << command->alternative; 
                
                if (command->required == true) {
                    ss << "\t(required)";
                }
                
                ss << "\n   " << command->description;
                
                if (command->required == false) {
                    ss << "\n   " << "This parameter is optional. The default value is '" + command->print_value() << "'.";
                }
                
                ss << "\n\n";
            }
            
            return ss.str();
        }
        
        void print_help(std::stringstream& ss) const {
            if (has_help())  {
                ss << "For more help use --help or -h.\n";
            }
        }
        
        std::string howto_required(CmdBase* command) const {
            std::stringstream ss { };
            ss << "The parameter " << command->name << " is required.\n";
            ss << command->description << '\n';
            print_help(ss);
            return ss.str();
        }
        
        std::string howto_use(CmdBase* command) const {
            std::stringstream ss { };
            ss << "The parameter " << command->name << " has invalid arguments.\n";
            ss << command->description << '\n';
            print_help(ss);
            return ss.str();
        }
        
        std::string no_default() const {
            std::stringstream ss { };
            ss << "No default parameter has been specified.\n";
            ss << "The given argument must be used with a parameter.\n";
            print_help(ss);
            return ss.str();
        }
        
    private:
        const std::string _appname;
        std::vector<std::string> _arguments;
        std::vector<CmdBase*> _commands;
    };
}