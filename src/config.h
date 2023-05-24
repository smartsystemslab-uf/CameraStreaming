#include <iostream>

#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>

// for convenience
using json = nlohmann::json;

// define default capacity of the queue
#define INPUT_FILE "/home/erman/immersion/IpDisco-Prj/ip_disco_server/config/config_file2.json"
#define OUTPUT_FILE "/home/erman/immersion/IpDisco-Prj/ip_disco_server/config/pretty_file.json"

class jsonconfig
{

private:
    std::string filename;

public:
    jsonconfig(std::string filename = INPUT_FILE){
        this->filename = filename;
    }
    ~jsonconfig(){}

    void load_json(){
        // read a JSON file
        std::ifstream myfile (filename);
        std::cout << filename << std::endl;
        if (myfile.is_open())
        {
            myfile >> js;
            myfile.close();
        }
    }

    void save_json_file(){
        // write prettified JSON to another file
        if(js.empty()){
            return;
        }

        std::ofstream myfile (OUTPUT_FILE);
        if (myfile.is_open())
        {
            myfile << std::setw(4) << js << std::endl;
            myfile.close();
        }
    }

    json js;
};

