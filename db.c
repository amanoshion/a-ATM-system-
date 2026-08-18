#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

#define DB_PATH "./atm.db"
#define PASSWD_TABLE_NAME "passwd"
#define ACCOUNT_TABLE_NAME "account"
#define LOG_TABLE_NAME "log"

#define OK 0
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

typedef enum Result_Type {
        Fail = -1,
        Success = 0,
        Unknown = 1
} Result_Type;

typedef MSG {
        opt_type opt_type;
        char dst[S];
        unsigned long data;
} MSG;

typedef enum account_status {
        Normal,         // 0 idle account
        Locked,         // 1 login in account
        Frozen,         // 2 block by root
}

unsigned long char_to_int(char *str) {
        return strtol(str, NULL, 10);
}

void int_to_char(unsigned long int num, char *buf) {
        snprintf(buf, 32, "%ld", num);
}

// TODO: 事务处理，加密
sqlite3 *ppdb;
char *errmsg;
int ini_db() {          // TODO : add this into reference 
        int ret_open = sqlite3_open(DB_PATH, &ppdb);
        if (ret_open != SQLITE_OK) {
                printf("db open fail:%s\n", sqlite3_errmsg(db));
                return ERROR;
        }
        char sentence_create_passwd[L] = {0};                // create passwd table  
        snprintf(sentence_create_passwd ,"CREATE TABLE IF NOT EXISTS %s                
                (public_key TEXT PRIMARY KEY NOT NULL, 
                private_key TEXT NOT NULL UNIQUE,
                passwd TEXT NOT NULL UNIQUE,
                key_word NOT NULL);", PASSWD_TABLE_NAME
        );
        
        char sentence_create_account[L] = {0};
        snprintf(sentence_create_account, "CREATE TABLE IF NOT EXISTS %s
                (public_key TEXT PRIMARY KEY NOT NULL,
                account_status, INTEGER NOT NULL,
                blance INTEGER NOT NULL);", AMOUNT_TABLE_NAME
        );
        
        char sentence_create_log[L] = {0};
        snprintf(sentence_create_log, "CREATE TABLE IF NOT EXISTS %s
                (public_key TEXT PRIMARY KEY NOT NULL,
                operation_type TEXT NOT NULL,
                data INTEGER NOT NULL,
                detination TEXT NOT NULL,
                time TEXT NOT NULL);", LOG_TABLE_NAME
        );
        
        int ret_exec = sqlite3_exec(ppdb, sentence_create_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite_exec fail: %s\n", errmsg);
                return ERROR;
        }
        return OK;
}

void close_db() {
        sqlite3_close(DB_PATH, db);
}


char **resultp;
int nrow;
int ncolumn;
int select_func(char *select_sentence) {
        int ret = sqlite3_get_table(ppdb, select_sentence, &resultp, &nrow, &ncolumn, &errmsg);
        if (ret != 0) {
                return ERROR;
        }
        return OK;
}

void check_in_check(char *public_key,Result_Type *result_type, char *explain_msg, char *selecting_value) {
        snprintf(sentence_check_status, "SELECT %s FROM %s WHERE public_key = %s", selecting_value, ACCOUNT_TABLE_NAME, public_key);
        int ret = select_func(sentence_check_status);
        if (ret == ERROR) {
                memset(explain_msg, 0, M);
                printf("sqlite3_get_table fail : %s\n", errmsg);
                strncpy(explain_msg, "system error");
                *result_type = Fail;
                return;
        }
        if (nrow != 1) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "public key wrong");
                *result_type = Fail;
                return;
        }
        return;
}
void check_status(char *public_key, Result_Type *result_type, char *explain_msg) {
        check_in_check(public_key, result_type, explain_msg, "account_status");
        if ((*result_type) == -1) return;

        if (((strncmp(resultp[4]), "Normal", 5) == 0) && (strlen(resultp[5]) == 5 )) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "status ok , check over");        
                *result_type = Success;
        } else {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "status abnormal");        
                *result_type == Fail;
        }
        return;
}

int check_money_enough(char *public_key, Result_Type *result_type, char *explain_msg, int money_require) {
        check_in_check(public_key, result_type, explain_msg, "blance");
        if ((*result_type) == -1) return;

        unsigned long int num = char_to_int(resultp[5]);
        if (num >= money_require) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "banlance ok , check over");        
                *result_type = Success;
                return;
        } 
        memset(explain_msg, 0, M);
        strncpy(explain_msg, "money not enough");       
        *result_type = Error;
        return;
}

void check_illegal(MSG *msg, Result_Type *result_type, char *current_public_key, char *explain_msg) {
        if ((*result_type) == Fail)return;
        char selecting_value[M] = {0};
        check_status(current_public_key, result_type, explain_msg);
        if ((*result_type) == Fail)return;
        switch(msg->opt_type) {
                case Query:
                        break;
                case Deposit:
                        break;
                case Withdraw:        
                        check_money_enough(public_key, explain_msg, msg->data);
                        if ((*result_type) == Fail) return;
                        break;
                case Transfer:
                        while()
                        check_status(msg->dst, explain_msg);
                        if ((*result_type) == Fail) return;
                        check_money_enough(public_key, explain_msg, msg->data);
                        if ((*result_type) == Fail) return;
                        break;
                case Quit:
                        break;
                default:
                        memset(explain_msg, 0, M);
                        strncpy(explain_msg, "operation type incorrect");
                        *result_type == Fail;
        }
        *result_type = Success;
        return;
}

void opt_deposit(MSG *msg, Result_Type *result_type, char *public_key) {
        if ((*result_type) == Fail) return;

        char sentence_deposit[M] = {0};
        snprintf(sentence_deposit, "UPDATE %s SET balance = balance + %lu WHERE public_key = %s", ACCOUNT_TABLE_NAME, msg->data, public_key);
        
        int ret_exec = sqlite3_exec(ppdb, sentence_create_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite_exec fail: %s\n", errmsg);
                *result_type = Fail;
                return;
        }
        *result_type = Success;
        return;
}

void opt_withdraw(MSG *msg, Result_Type *result_type, char *public_key) {
        if ((*result_type) == Fail) return;

        char sentence_deposit[M] = {0};
        snprintf(sentence_deposit, "UPDATE %s SET balance = balance - %lu WHERE public_key = %s", ACCOUNT_TABLE_NAME, msg->data, public_key);
        
        int ret_exec = sqlite3_exec(ppdb, sentence_create_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite_exec fail: %s\n", errmsg);
                *result_type = Fail;
                return;
        }
        *result_type = Success;
        return;
}

char sentence_begin[S] = "START TRANSACTION;";
char sentence_commit[S] = "COMMIT;";
char sentence_rollback[S] = "ROLLBACK";

void opt_account_db(MSG *msg, Result_Type *result_type, char *public_key) {
        if ((*result_type) == Fail) return;

        switch(msg->opt_type) {
                case Query:
                        break;
                case Deposit:
                        opt_deposit(msg, result_type, public_key);
                        break;
                case Withdraw:
                        opt_withdraw(msg, result_type, public_key);
                        break;
                case Transfer:          // TODO
                        int ret_exec = sqlite3_exec(ppdb, sentence_begin, NULL, NULL, &errmsg);
                        if (ret_exec == SQLITE_ERROR) {
                                printf("sqlite_exec fail: %s\n", errmsg);
                                *result_type = Fail;
                                return;
                        }
                        opt_withdraw(msg, result_type, public_key);
                        if ((*result_type) == Fail) {
                                int ret_exec = sqlite3_exec(ppdb, sentence_rollback, NULL, NULL, &errmsg);
                                if (ret_exec == SQLITE_ERROR) {
                                        printf("sqlite_exec fail: %s\n", errmsg);
                                        *result_type = Fail;
                                        return;
                                }
                        }
                        opt_deposit(msg, result_type, msg->dst);
                        if ((*result_type) == Fail) {
                                int ret_exec = sqlite3_exec(ppdb, sentence_rollback, NULL, NULL, &errmsg);
                                if (ret_exec == SQLITE_ERROR) {
                                        printf("sqlite_exec fail: %s\n", errmsg);
                                        *result_type = Fail;
                                        return;
                                }
                        }
                        int ret_exec = sqlite3_exec(ppdb, sentence_commit, NULL, NULL, &errmsg);
                        if (ret_exec == SQLITE_ERROR) {
                                printf("sqlite_exec fail: %s\n", errmsg);
                                *result_type = Fail;
                                return;
                        }
                        break;
                case Quit:
                        close_db();
                        break;
        }
}

void opt_log_db(MSG *msg, Result_Type *result_type) {

}