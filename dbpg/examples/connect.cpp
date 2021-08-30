//g++ -Wall -o x -lpqxx

#include <iostream>
#include <string>
#include <vector>

#include <pqxx/pqxx>

pqxx::result insert_query(std::string obj_name) {
    std::string connection_string("host=79.173.96.138 port=5432 dbname=test_db user=tmk password=tmkdb");
    pqxx::connection conn(connection_string.c_str());
    pqxx::work transaction(conn);

    pqxx::result r {
        transaction.exec("INSERT INTO objects VALUES (DEFAULT, (SELECT object_types.id FROM object_types WHERE object_types.name = 'mandrel'), '----'," + transaction.quote(obj_name) + ");")
    };

    transaction.commit();
    return r;
}

pqxx::result select_query() {
    std::string connection_string("host=79.173.96.138 port=5432 dbname=test_db user=tmk password=tmkdb");
    pqxx::connection conn(connection_string.c_str());
    pqxx::work transaction(conn);

    pqxx::result r {
        transaction.exec("SELECT * FROM objects;")
    };
    if (r.size() < 1) {
        std::cout << "No data" << std::endl;
    }

    std::cout << "id\tobj_number\t\tobj_generated_id" << std::endl;
    for (auto row: r) {
        std::cout << row[0].c_str() << "\t" << row[2].c_str() << "\t\t" << row[3].c_str() << std::endl;
    }

    transaction.commit();
    return r;
}

int main (int argc, char ** argv)
{
    //work correctly only for client hosts if it's in the /etc/postgresql/13/main/pg_hba.conf list on server 79.173.96.138
    //you can use '79.173.96.138' or '10.24.4.167' for start this code from jenkins
    //std::string connection_string("host=79.173.96.138 port=5432 dbname=test_db user=tmk password=tmkdb");
    //pqxx::connection conn(connection_string.c_str());

    try {
        std::string obj_generated_id = "object_2021-08-04_16-17-02.352521";
        pqxx::result r{insert_query(obj_generated_id)};

        for (auto row: r) {
            std::cout << "Row: ";
            for (auto field: row) std::cout << field.c_str() << " ";
            std::cout << std::endl;
        }
    }
    catch (pqxx::sql_error const &e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        return 2;
    }
    catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
