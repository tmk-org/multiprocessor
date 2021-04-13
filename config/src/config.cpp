#include "config/config.h"

#include <nlohmann/json.hpp>

//#include <iostream>
#include <glog/logging.h>

#include <fstream>
#include <string>
#include <string.h>

using json = nlohmann::json;

#define CONFIG_EXTRA_CHECK

#ifdef CONFIG_CHECK
#define CHECK_FAIL \
LOG(ERROR) << "Ill-formed json file " << std::endl; \
return -1;

int dfs_check(module_t **modules, bool *was, int size, int now) {
    if (now < -1 || now >= size) { // out of range
        CHECK_FAIL;
    }
    if (was[now] == 1) { // loop found
        CHECK_FAIL;
    }
    was[now] = 1;
    if ((*modules)[now].next == -1) {
        for (int i = 0; i < size; ++i) {
            if (was[i] == 0) { // we will never start this module
                CHECK_FAIL;
            }
        }
        // all good
        return 0;
    }
    return dfs_check(modules, was, size, (*modules)[now].next);
}
#endif

inline void throw_wrong_type_error(const std::string which, const std::string real, const std::string should) {
    throw std::logic_error("Wrong type of " + which + ", should be " + should + ", but is " + real);
}

int model_read_configuration(const char *fileName, module_t **target) {
    if (!fileName || fileName[0] == '\0') {
        LOG(ERROR) << "Empty string instead of fileName(.json)" << std::endl;
        return -1;
    }

    std::ifstream jsonFile(fileName);

    /* no file */
    if (!jsonFile.is_open() || !jsonFile.good()) {
        LOG(ERROR) << "Failed to open " << fileName << std::endl;
        return -1;
    }

    json jsonObj;
    try {
        jsonFile >> jsonObj;
    } catch (std::logic_error &e) {
        LOG(ERROR) << "Failed parse json file " << fileName << std::endl;
        jsonFile.close();
        return -1;
    }
    jsonFile.close();

    /* now create & fill */
    module_t *modules = static_cast<module_t *>(malloc(sizeof(module_t) * jsonObj.size()));
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
            modules[i].argv = static_cast<char **>(malloc(sizeof(char*) * (p_size + 1 + has_name)));
            if (has_name == 1) {
                modules[i].argv[0] = static_cast<char *>(malloc(sizeof(char) * (*it)["name"].get<std::string>().size()));
                strcpy(modules[i].argv[0], (*it)["name"].get<std::string>().c_str()); // name of module as first parameter
            }
            modules[i].argv[p_size + has_name] = NULL; // NULL as last parameter
            for (int j = has_name; j < p_size + has_name; ++j) {
                modules[i].argv[j] = static_cast<char *>(malloc(sizeof(char) * params[j - 1].size()));
                strcpy(modules[i].argv[j], params[j - 1].c_str());
            }
            if (!(*it)["next_id"].is_null()) {
                modules[i].next = (*it)["next_id"].get<int>();
                if ((*it)["type"] == "LAST") {
                    throw_wrong_type_error(*it, (*it)["type"], "MIDDLE");
                }
                modules[i].type = MT_MIDDLE;
            } else {
                modules[i].next = -1;
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
        } catch (std::logic_error &e) {
            LOG(ERROR) << "Invalid json file " << fileName << std::endl;
            LOG(ERROR) << "Every module should contain id, name, \
                          parameters, type, next_id" << std::endl;
            LOG(ERROR) << "Error was \"" << e.what() << "\"" << std::endl;
            return -1;
        }
    }
    if (first == -1) {
        LOG(ERROR) << "No FIRST type" << std::endl;
        return -1;
    }
#ifdef CONFIG_CHECK
    bool dfs_was[jsonObj.size()] = {}; // default 0
    if (dfs_check(&modules, dfs_was, jsonObj.size(), first) != 0) {
        return -1;
    }
#endif
    *target = &(modules[0]);
    return jsonObj.size();
}

