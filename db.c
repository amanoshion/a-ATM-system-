#include <stdio.h>
#include <sqlite3.h>

#define DB_PATH "./atm.db"
#define ACCOUNT_TABLE_NAME "account"
#define LOG_TABLE_NAME "log"
#define ERROR -1

#define S 24
#define M 64
#define L 256
typedef enum Opt_Type{
        Query,
        Deposit,
        Withdraw,
        Transfer,
        Quit
} opt_type;
typedef MSG {
        opt_type opt_type;
        char dst[S];
        unsigned long data;
} MSG;

sqlite3 *ppdb;
int ini_db() {          // TODO : need reference one time 
        int ret_open = sqlite3_open(DB_PATH, &ppdb);
        if (ret_open != SQLITE_OK) {
                printf("db open fail:%s\n", sqlite3_errmsg(db));
                return ERROR;
        }
        char sentence_create_account[L] = {0};
        snprintf(sentence_create_account ,"CREATE TABLE IF NOT EXIST %s                // TODO: 备忘词，事务处理，加密
                (public_key TEXT PRIMARY KEY NOT NULL, private_key TEXT NOT NULL UNIQUE);", ACCOUNT_TABLE_NAME);
        return 0;
}

void close_db() {
        sqlite3_close(DB_PATH, db);
}

int check_if_legal(MSG *msg) {
        
}
void opt_account_db(MSG *msg) {
        switch(msg->opt_type) {
                case Query:
                        break;
                case Deposit:
                        break;
                case Withdraw:
                        break;
                case Transfer:          // TODO
                        check_if_legal();
                        break;
                case Quit:
                        close_db();
                        break;
        }
}

void opt_log_db(MSG *msg) {

}