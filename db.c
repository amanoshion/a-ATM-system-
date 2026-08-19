#include "db.h"

char *names_opt[] = {"Register", "Login", "Delete_account", "Query_self", "Query_log", "Deposit", "Withdraw", "Transfer", "Quit"};

char *names_result[] = {"Success", "Fail", "Tips"};

unsigned long char_to_int(char *str) {
        return strtol(str, NULL, 10);
}

void int_to_char(unsigned long int num, char *buf) {
        snprintf(buf, 32, "%ld", num);
}

sqlite3 *ppdb;
char *errmsg;
int ini_db() {          
        int ret_open = sqlite3_open(DB_PATH, &ppdb);
        if (ret_open != SQLITE_OK) {
                printf("db open fail:%s\n", sqlite3_errmsg(ppdb));
                return ERROR;
        }
        char sentence_create_passwd[L] = {0};                // create passwd table  
        snprintf(sentence_create_passwd , L, "CREATE TABLE IF NOT EXISTS %s(id INTEGER PRIMARY KEY AUTOINCREMENT, public_key TEXT UNIQUE NOT NULL, passwd INTEGER NOT NULL,);", PASSWD_TABLE_NAME);
        
        char sentence_create_account[L] = {0};
        snprintf(sentence_create_account, L, "CREATE TABLE IF NOT EXISTS %s (public_key TEXT PRIMARY KEY NOT NULL, account_status TEXT NOT NULL, blance INTEGER NOT NULL);", ACCOUNT_TABLE_NAME);
        
        char sentence_create_log[L] = {0};
        snprintf(sentence_create_log, L, "CREATE TABLE IF NOT EXISTS %s (table_id INTEGER PRIMARY KEY AUTOINCREMENT, public_key TEXT NOT NULL, operation_type TEXT NOT NULL, data INTEGER NOT NULL, detination TEXT NOT NULL, time TEXT NOT NULL);", LOG_TABLE_NAME);
        
        int ret_exec = sqlite3_exec(ppdb, sentence_create_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite_exec fail: %s\n", errmsg);
                return ERROR;
        }
        return OK;
}

int close_db() {
        int ret = sqlite3_close(ppdb);
        if (ret != SQLITE_OK) {
                return ERROR;
        }
        return OK;
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
        char sentence_check_status[L] = {0};
        snprintf(sentence_check_status, L, "SELECT %s FROM %s WHERE public_key = '%s'", "account_status", ACCOUNT_TABLE_NAME, public_key);
        int ret = select_func(sentence_check_status);
        if (ret == ERROR || nrow != 1) {
                memset(explain_msg, 0, L);
                strncpy(explain_msg, "no user or system error", L);
                *result_type = Fail;
                return;
        }

        if ((strncmp(resultp[4], "Normal", 6) == 0) && (strlen(resultp[5]) == 5 )) {     
                *result_type = Success;
        } else {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "status abnormal", L);        
                *result_type = Fail;
        }
        return;
}

void check_illegal(MSG *msg, Result_Type *result_type, char *explain_msg, char *current_public_key) {
        if ((*result_type) == Fail) return;
        char selecting_value[L] = {0};
        check_status(current_public_key, result_type, explain_msg);
        if ((*result_type) == Fail) return;

        if ((*result_type) == Transfer) {
                check_status(msg->dst, result_type, explain_msg);
                if ((*result_type) == Fail) return;
        }
        *result_type = Success;
        return;
}

void opt_query_self(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        char sentence_query[L] = {0};
        snprintf(sentence_query, L, "SELECT * FROM %s WHERE public_key = '%s'", ACCOUNT_TABLE_NAME, public_key);
        int ret = select_func(sentence_query);
        if (ret == ERROR) {
                memset(explain_msg, 0, L);
                strncpy(explain_msg, errmsg, L);
                *result_type = Fail;
                return;
        }
        memset(explain_msg, 0, M);
        for (int i = 3; i < 6; i++) {
                strncat(explain_msg, resultp[i], L);
        }
        *result_type = Success;
        return;
}

void opt_query_log(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        char sentence_log[L] = {0};
        snprintf(sentence_log, L, "SELECT * FROM %s WHERE public_key = '%s' OR dst = '%s' ORDER BY DESC LIMIT 50;", LOG_TABLE_NAME, public_key, public_key);

        int ret = select_func(sentence_log);
        if (ret == ERROR) {
                memset(explain_msg, 0, L);
                strncpy(explain_msg, errmsg, L);
                *result_type = Fail;
                return;
        }
        memset(explain_msg, 0, M);
        int n = 0;
        for (int i = nrow; i <= (nrow * ncolumn); i += 4) {
                strcat(explain_msg, resultp[i + 1]);
                strcat(explain_msg, resultp[i + 2]);
                strcat(explain_msg, resultp[i + 3]);
                strcat(explain_msg, "\n");
        }
        *result_type = Success;
        return;
}

void opt_deposit(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        if ((*result_type) == Fail) return;

        char sentence_deposit[L] = {0};
        snprintf(sentence_deposit, L, "UPDATE %s SET balance = balance + %lu WHERE public_key = '%s'", ACCOUNT_TABLE_NAME, msg->data, public_key);
        
        int ret_exec = sqlite3_exec(ppdb, sentence_deposit, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                memset(explain_msg, 0, L);
                strncpy(explain_msg, errmsg, L);
                *result_type = Fail;
                return;
        }
        *result_type = Success;
        return;
}

void opt_withdraw(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        if ((*result_type) == Fail) return;

        char sentence_withdraw[L] = {0};
        snprintf(sentence_withdraw, L, "UPDATE %s SET balance = balance - %lu WHERE public_key = '%s' AND balance >= %lu;", ACCOUNT_TABLE_NAME, msg->data, public_key, msg->data);
        
        int ret_exec = sqlite3_exec(ppdb, sentence_withdraw, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, errmsg, L);
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
                case Delete_account:
                        opt_delete_account(msg->dst, &result_type, explain_msg);
                        break;
                case Query_self:
                        opt_query_self(msg, result_type, explain_msg, public_key);
                        break;
                case Query_log:
                        opt_log_db(msg, public_key);
                        break;
                case Deposit:
                        opt_deposit(msg, result_type, explain_msg, public_key);
                        break;
                case Withdraw:
                        opt_withdraw(msg, result_type, explain_msg, public_key);
                        break;
                case Transfer:          
                {
                        int ret_exec = sqlite3_exec(ppdb, sentence_begin, NULL, NULL, &errmsg);
                        if (ret_exec == SQLITE_ERROR) {
                                memset(explain_msg, 0, L);
                                strncpy(explain_msg, errmsg, L);
                                *result_type = Fail;
                                return;
                        }
                        opt_withdraw(msg, result_type, explain_msg, public_key);
                        if ((*result_type) == Fail) {
                                int ret_exec = sqlite3_exec(ppdb, sentence_rollback, NULL, NULL, &errmsg);
                                if (ret_exec == SQLITE_ERROR) {
                                        memset(explain_msg, 0, M);
                                        strncpy(explain_msg, errmsg, L);
                                        *result_type = Fail;
                                        return;
                                }
                        }
                        opt_deposit(msg, result_type, explain_msg, msg->dst);
                        if ((*result_type) == Fail) {
                                int ret_exec = sqlite3_exec(ppdb, sentence_rollback, NULL, NULL, &errmsg);
                                if (ret_exec == SQLITE_ERROR) {
                                        memset(explain_msg, 0, L);
                                        strncpy(explain_msg, errmsg, L);
                                        *result_type = Fail;
                                        return;
                                }
                        }
                        ret_exec = sqlite3_exec(ppdb, sentence_commit, NULL, NULL, &errmsg);
                        if (ret_exec == SQLITE_ERROR) {
                                memset(explain_msg, 0, L);
                                strncpy(explain_msg, errmsg, L);
                                *result_type = Fail;
                                return;
                        }
                        break;
                }
                case Quit:
                {
                        int ret = close_db();
                        if (ret != OK) {
                                memset(explain_msg, 0, L);
                                strncpy(explain_msg, errmsg, L);
                                *result_type = Fail;
                                return;
                        }
                        *result_type = Success;
                        return;
                }
                default:
                        memset(explain_msg, 0, L);
                        strncpy(explain_msg, "operation not exist", L);
                        *result_type = Fail;
                        break;
        }
}

void opt_log_db(MSG *msg, char *public_key) {
        time_t now = time(NULL);
        struct tm *t = localtime(NULL);

        char time_buffer[L] = {0};
        snprintf(time_buffer, L, "%04d-%02d-%02d %02d:%02d:%02d",
                t->tm_year + 1900,
                t->tm_mon + 1,
                t->tm_mday,
                t->tm_hour,
                t->tm_min,
                t->tm_sec
        );
        char sentence_insert_log[L] = {0};
        char dst[L] = {0};
        if (msg->opt_type == Transfer) {
                strncpy(dst, msg->dst, L);
        } else {
                strncpy(dst, public_key, L);
        }
        snprintf(sentence_insert_log, L, "INSERT INTO %s VALUES ('%s', '%s', %lu, '%s', '%s');", LOG_TABLE_NAME, public_key, names_opt[msg->opt_type], msg->data, dst, time_buffer);

        int ret_exec = sqlite3_exec(ppdb, sentence_insert_log, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                return;
        }
        return;
}