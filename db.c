#include "db.h"

char names_opt[] = {"Register", "Login", "Delete_account", "Query", "Deposit", "Withdraw", "Transfer", "Quit"};


char *names_result[3] = {"Success", "Fail", "Tips"};


unsigned long char_to_int(char *str) {
        return strtol(str, NULL, 10);
}

void int_to_char(unsigned long int num, char *buf) {
        snprintf(buf, 32, "%ld", num);
}

// TODO: 事务处理，加密
sqlite3 *ppdb;
char *errmsg;
int ini_db() {          
        int ret_open = sqlite3_open(DB_PATH, &ppdb);
        if (ret_open != SQLITE_OK) {
                printf("db open fail:%s\n", sqlite3_errmsg(db));
                return ERROR;
        }
        char sentence_create_passwd[L] = {0};                // create passwd table  
        snprintf(sentence_create_passwd ,"CREATE TABLE IF NOT EXISTS %s    
                (passwd_id INTEGER PRIMARY KEY AUTOINCREMENT,
                public_key TEXT UNIQUE NOT NULL,
                passwd INTEGER NOT NULL,
                );", PASSWD_TABLE_NAME
        );
        
        char sentence_create_account[L] = {0};
        snprintf(sentence_create_account, "CREATE TABLE IF NOT EXISTS %s
                (public_key TEXT PRIMARY KEY NOT NULL,
                account_status TEXT NOT NULL,
                blance INTEGER NOT NULL);", AMOUNT_TABLE_NAME
        );
        
        char sentence_create_log[L] = {0};
        snprintf(sentence_create_log, "CREATE TABLE IF NOT EXISTS %s
                (table_id INTEGER PRIMARY KEY AUTOINCREMENT,
                public_key TEXT NOT NULL,
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

void check_status(char *public_key, Result_Type *result_type, char *explain_msg) {
        char sentence_check_status[M] = {0};
        snprintf(sentence_check_status, "SELECT %s FROM %s WHERE public_key = %s", "account_status", ACCOUNT_TABLE_NAME, public_key);
        int ret = select_func(sentence_check_status);
        if (ret == ERROR || nrow != 1) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "no user or system error");
                *result_type == Fail;
                return;
        }

        if (((strncmp(resultp[4]), "Normal", 5) == 0) && (strlen(resultp[5]) == 5 )) {     
                *result_type = Success;
        } else {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "status abnormal");        
                *result_type == Fail;
        }
        return;
}


void check_illegal(MSG *msg, Result_Type *result_type, char *explain_msg, char *current_public_key) {
        if ((*result_type) == Fail) return;
        char selecting_value[M] = {0};
        check_status(current_public_key, result_type, explain_msg);
        if ((*result_type) == Fail)return;

        if ((*result_type) == Transfer) {
                check_status(msg->dst, result_type, explain_msg);
                if ((*result_type) == Fail) return;
        }
        *result_type = Success;
        return;
}

void opt_query(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        char sentence_query[M] = {0};
        snprintf(sentence_query, "SELECT * FROM %s WHERE public_key = %s", ACCOUNT_TABLE_NAME, public_key);
        int ret = select_func(sentence_check_status);
        if (ret == ERROR) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, errmsg);
                *result_type = Fail;
                return;
        }
        memset(explain_msg, 0, M);
        for (int i = 3; i < 6; i++) {
                strncat(explain_msg, result_type[i]);
        }
        *result_type = Success;
        return;
}

void opt_query_log(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        char sentence_history[M] = {0};
        snprintf(sentence_query, "SELECT * FROM %s WHERE public_key = %s OR dst = %s ORDER BY DESC LIMIT 50;", LOG_TABLE_NAME, public_key, public_key);

         int ret = select_func(sentence_check_status);
        if (ret == ERROR) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, errmsg);
                *result_type = Fail;
                return;
        }
        memset(explain_msg, 0, M);
        int n = 0;
        for (int i = nrow; i <= (nrow * ncolumn); i += 4) {
                strcat(explain_msg, result_type[i + 1]);
                strcat(explain_msg, result_type[i + 2]);
                strcat(explain_msg, result_type[i + 3]);
                strcat(explain_msg, "\n");
        }
        *result_type = Success;
        return;
}

void opt_deposit(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        if ((*result_type) == Fail) return;

        char sentence_deposit[M] = {0};
        snprintf(sentence_deposit, "UPDATE %s SET balance = balance + %lu WHERE public_key = %s", ACCOUNT_TABLE_NAME, msg->data, public_key);
        
        int ret_exec = sqlite3_exec(ppdb, sentence_create_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, errmsg);
                *result_type = Fail;
                return;
        }
        *result_type = Success;
        return;
}

void opt_withdraw(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_ke) {
        if ((*result_type) == Fail) return;

        char sentence_deposit[M] = {0};
        snprintf(sentence_deposit, "UPDATE %s SET balance = balance - %lu WHERE public_key = %s
                AND balance >= %lu;", ACCOUNT_TABLE_NAME, msg->data, public_key, msg->data);
        
        int ret_exec = sqlite3_exec(ppdb, sentence_create_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, errmsg);
                *result_type = Fail;
                return;
        }
        *result_type = Success;
        return;
}

char sentence_begin[S] = "START TRANSACTION;";
char sentence_commit[S] = "COMMIT;";
char sentence_rollback[S] = "ROLLBACK";

void opt_account_db(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        if ((*result_type) == Fail) return;

        switch(msg->opt_type) {
                case Query_account:
                        opt_query(msg, result_type, public_key);
                        break;
                case Query_log:
                        opt_history(msg, result_type, public_key);
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
                                memset(explain_msg, 0, M);
                                strncpy(explain_msg, errmsg);
                                *result_type = Fail;
                                return;
                        }
                        opt_withdraw(msg, result_type, public_key);
                        if ((*result_type) == Fail) {
                                int ret_exec = sqlite3_exec(ppdb, sentence_rollback, NULL, NULL, &errmsg);
                                if (ret_exec == SQLITE_ERROR) {
                                        memset(explain_msg, 0, M);
                                        strncpy(explain_msg, errmsg);
                                        *result_type = Fail;
                                        return;
                                }
                        }
                        opt_deposit(msg, result_type, msg->dst);
                        if ((*result_type) == Fail) {
                                int ret_exec = sqlite3_exec(ppdb, sentence_rollback, NULL, NULL, &errmsg);
                                if (ret_exec == SQLITE_ERROR) {
                                        memset(explain_msg, 0, M);
                                        strncpy(explain_msg, errmsg);
                                        *result_type = Fail;
                                        return;
                                }
                        }
                        int ret_exec = sqlite3_exec(ppdb, sentence_commit, NULL, NULL, &errmsg);
                        if (ret_exec == SQLITE_ERROR) {
                                memset(explain_msg, 0, M);
                                strncpy(explain_msg, errmsg);
                                *result_type = Fail;
                                return;
                        }
                        break;
                case Quit:
                        int ret = close_db();
                        if (ret != SQLITE_OK) {
                                memset(explain_msg, 0, M);
                                strncpy(explain_msg, errmsg);
                                *result_type = Fail;
                                return;
                        }
                        *result_type = Success;
                        return;
                default:
                        memset(explain_msg, 0, M);
                        strncpy(explain_msg, "operation not exist");
                        *result_type = Fail;
                        break;
        }
}

void opt_log_db(MSG *msg, char *public_key) {
        time_t now = time(NULL);
        struct tm *t = localtime(NULL);

        char time_buffer[32] = {0};
        snprintf(time_buffer, "%04d-%02d-%02d %02d:%02d:%02d",
                t->tm_year + 1900,
                t->tm_mon + 1,
                t->tm_mday,
                t->tm_hour,
                t->tm_min,
                t->tm_sec);
        char sentence_insert_log[L] = {0};
        char dst[S] = {0};
        if (msg->opt_type == Transfer) {
                strncpy(dst, msg->dst, S);
        } else {
                strncpy(dst, public_key, S);
        }
        snprintf(sentence_insert_log, "INSERT INTO %s 
                VALUES (%s, %s, %lu, %s, %s);", LOG_TABLE_NAME, public_key,
                names_opt[msg->opt_type], msg->data, dst, time_buffer);

        int ret_exec = sqlite3_exec(ppdb, sentence_insert_log, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                return;
        }
        return;
}