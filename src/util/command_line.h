#ifndef CLARG_H_
#define CLARG_H_

#include <charm++.h>
#include <map>
#include <string>
#include <vector>

// Should parse return a bool for success/failure? Errors maybe should abort
// but just print the error and return false?
class Parser {
public:
  virtual bool parse(std::string argument) const = 0;
  virtual void print() const = 0;
};

class AggregateParser : public Parser {
protected:
  std::vector<Parser*> parsers;
public:
  AggregateParser() {}

  void add_parser(Parser* p) {
    parsers.push_back(p);
  }

  bool parse(std::string argument) const {
    for (int i = 0; i < parsers.size(); i++) {
      if (parsers[i]->parse(argument)) {
        return true;
      }
    }
    return false;
  }

  void print() const {
    for (int i = 0; i < parsers.size(); i++) {
      parsers[i]->print();
    }
    CkPrintf("\n");
  }
};

template <typename ArgType>
class Argument : public Parser {
private:
  std::string name, description;
  ArgType& location;

public:
  Argument(std::string n, std::string d, ArgType& l)
      : name(n), description(d), location(l) {}

  bool parse(std::string argument) const {
    std::stringstream stream(argument);
    std::string lhs, rhs;
    std::getline(stream, lhs, '=');
    std::getline(stream, rhs, '=');

    // TODO: Error checking on >>
    if (lhs != name) {
      return false;
    } else if (rhs.length() == 0) {
      return false;
    } else {
      std::stringstream rhs_stream(rhs);
      rhs_stream >> location;
      return true;
    }
  }

  void print() const {
    CkPrintf("%24s: %s\n", name.c_str(), description.c_str());
  }
};

// Treating this as a parser might be weird or might be fine
// To merge multiple sets, we could just add this as a parser to a superset, but
// adding one entry for each argument in it's map
// Or we can just make a more proper composite set
// We want to preserve set names and order for printing
class ArgumentSet : public AggregateParser {
private:
  std::string name;
  std::map<std::string, uint8_t> indices;

public:
  ArgumentSet(std::string n) : name(n) {}

  template <typename ArgType>
  void register_argument(std::string name, std::string desc, ArgType& loc) {
    if (indices.find(name) != indices.end()) {
      CkPrintf("Duplicate argument registration: %s\n", name.c_str());
      CkAbort("Argument parse error\n");
    }
    indices[name] = parsers.size();
    add_parser(new Argument<ArgType>(name, desc, loc));
  }

  void print() const {
    CkPrintf("\n%s %s\n", std::string(25,'=').c_str(), name.c_str());
    AggregateParser::print();
  }
};

void tw_add_arguments(ArgumentSet* args);
void parse_command_line(int argc, char** argv);

#endif
