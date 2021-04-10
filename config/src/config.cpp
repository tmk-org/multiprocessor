#include "config/config.h"

#include <nlohmann/json.hpp>

#include <iostream> // for debug info, sorry
#include <fstream>
#include <string>
#include <string.h>

using json = nlohmann::json;

inline void throw_wrong_type_error(const std::string which, const std::string real, const std::string should) {
    throw std::logic_error("Wrong type of " + which + ", should be " + should + ", but is " + real);
}

//int model_read_configuration(const char *fileName, module_t **target);
int create_config(const char *fileName, module_t **target) {
    if (!fileName || fileName[0] == '\0') {
        std::cout << "Empty string instead of fileName(.json)" << std::endl;
        return -1;
    }

    std::ifstream jsonFile(fileName);

    /* no file */
    if (!jsonFile.is_open() || !jsonFile.good()) {
        std::cout << "Failed to open " << fileName << std::endl;
        return -1;
    }

    json jsonObj;
    try {
        jsonFile >> jsonObj;
    } catch (std::logic_error e) {
        std::cout << "Failed parse json file " << fileName << std::endl;
        jsonFile.close();
        return -1;
    }
    jsonFile.close();

    /* now create & fill */
    module_t *modules = new module_t[jsonObj.size()]; // TODO add operator delete somewhere
    json::iterator it = jsonObj.begin(); // iterator
    int first = -1; // for checking
    for (int i = 0; it != jsonObj.end(); ++it, ++i) {
        /* parse (*it) to modules[i] */
        try {
            modules[i].id = (*it)["id"];
            std::vector<std::string> params = 
                 (*it)["parameters"].get<std::vector<std::string>>();
            const int has_name = ((*it)["name"].is_null()? 0 : 1);
            const int p_size = params.size(); // shorter naming
            modules[i].argv = new char*[p_size + 1 + has_name]; // TODO add operator delete somewhere
            if (has_name == 1) {
                modules[i].argv[0] = new char[(*it)["name"].get<std::string>().size()];
                strcpy(modules[i].argv[0], (*it)["name"].get<std::string>().c_str()); // name of module as first parameter
            }
            modules[i].argv[p_size + has_name] = NULL; // NULL as last parameter
            for (int j = has_name; j < p_size + has_name; ++j) {
                modules[i].argv[j] = new char[params[j - 1].size()]; // TODO add operator delete somewhere
                strcpy(modules[i].argv[j], params[j - 1].c_str());
            }
            if (!(*it)["next_id"].is_null()) {
#if 0
                modules[i].next = &modules[(*it)["next_id"].get<int>()];
#else
                modules[i].next = (*it)["next_id"].get<int>();
#endif
                if ((*it)["type"] == "LAST") {
                    throw_wrong_type_error(*it, (*it)["type"], "MIDDLE");
                }
                modules[i].type = MT_MIDDLE;
            } else {
#if 0
                modules[i].next = NULL;
#else
                modules[i].next = -1;
#endif
                if ((*it)["type"] != "LAST") {
                    throw_wrong_type_error(*it, (*it)["type"], "LAST");
                }
                modules[i].type = MT_LAST;
            }
            if ((*it)["type"] == "FIRST") {
                if (first != -1) {
                    throw_wrong_type_error(*it, (*it)["type"], "MIDDLE");
                }
                first = i;
                modules[i].type = MT_FIRST;
            }
        } catch (std::logic_error e) {
            std::cout << "Invalid json file " << fileName << std::endl;
            std::cout << "Every module should contain id, name, \
                          parameters, type, next_id and json_path" << std::endl; // TODO json_path?
            std::cout << "Error was \"" << e.what() << "\"" << std::endl;
            return -1;
        }
    }
    if (first == -1) {
        std::cout << "No FIRST type" << std::endl;
        return -1;
    }
    // here should be check for loop (dfs)?
    *target = &(modules[0]);
    return jsonObj.size();
}

