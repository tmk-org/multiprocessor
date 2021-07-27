//g++ -Wall -o x -lpqxx

#include <iostream>
#include <string>

#include <pqxx/pqxx>

int main (int argc, char ** argv)
{
    //work correctly only for client hosts if it's in the /etc/postgresql/13/main/pg_hba.conf list on server 79.173.96.138
    //you can use '79.173.96.138' or '10.24.4.167' for start this code from jenkins
    std::string connection_string ("host=79.173.96.138 port=5432 dbname=test_db user=tmk password=tmkdb");

    pqxx::connection con(connection_string.c_str());
    pqxx::work wrk(con);

    pqxx::result res = wrk.exec("SELECT * FROM mandrels;");

    if (res.size()<1) {
        std::cout << "Table is empty" << std::endl;
    }

    std::cout << "id: " << std::endl;
    for (int i = 0; i < res.size(); ++i) {
        std::cout << res[i][0] << std::endl;
    }

    return 0;
}
