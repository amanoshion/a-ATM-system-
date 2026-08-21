#include "db.h"

char *names_opt[] = {
        "Register",
        "Login",
        "Delete_account",
        "Query_self",
        "Query_log",
        "Deposit",
        "Withdraw",
        "Transfer",
        "Quit",
        "Freeze",
        "Defrost",
        "Query_log_root"
};
char *names_result[] = {"Success", "Fail"};

char *names_account_status[] = {"Normal", "Locked", "Frozen", "Root"};
unsigned long int char_to_int(char *str) {
        return strtol(str, NULL, 10);
}

void int_to_char(unsigned long int num, char *buf) {
        snprintf(buf, 32, "%ld", num);
}


void change_account_status(char *public_key, Account_Status account_status) {
        char sentence_change_account_status[L] = {0};
        snprintf(sentence_change_account_status, L, "UPDATE %s SET account_status = %d WHERE public_key = '%s'", ACCOUNT_TABLE_NAME, account_status, public_key);
        
        int ret_exec = sqlite3_exec(ppdb, sentence_change_account_status, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail: %s\n", errmsg);
                return;
        }
        return;
}

void create_root_account_if_not_exits(unsigned long int passwd) {
        unsigned char pk_bin[KEY_LEN] = {0};
        char pk_hex[KEY_HEX_LEN] = {0};
        
        unsigned char sk_bin[KEY_LEN] = {0};
        char sk_hex[KEY_HEX_LEN] = {0};

        printf("start create root\n");
        FILE *fd = fopen(ROOT_INI, "wx");
        if (fd == NULL) {
                return;
        }
        printf("creating root . . .\n");

        generate_account(passwd, sk_hex);

        int ret = fwrite(sk_hex, 1, 64, fd);
        if (ret < 0) {
                perror("fwrite fail");
        }
        // get pk_hex to change status
        sodium_hex2bin(sk_bin, 32, sk_hex, 64, NULL, NULL, NULL);
        crypto_scalarmult_base(pk_bin, sk_bin);
        sodium_bin2hex(pk_hex, sizeof(pk_hex), pk_bin, KEY_LEN);
        
        change_account_status(pk_hex, Root);

        fclose(fd);
        printf("root created\n");
}

void generate_account(unsigned long int passwd, unsigned char *explain_msg) {
        if (sodium_init() < 0) {
                fprintf(stderr, "libsodium init fail");
                return;
        }
        unsigned char seed[SEED_LEN];
        unsigned char secret_key[KEY_LEN];
        unsigned char public_key[KEY_LEN];

        randombytes_buf(seed, 32);

        crypto_box_seed_keypair(public_key, secret_key, seed);

        char pk_hex[KEY_HEX_LEN];    
        char sk_hex[KEY_HEX_LEN];    

        sodium_bin2hex(pk_hex, sizeof(pk_hex), public_key, KEY_LEN);
        sodium_bin2hex(sk_hex, sizeof(sk_hex), secret_key, KEY_LEN);
        //      write into db
        char sentence_insert_passwd[L] = {0};
        snprintf(sentence_insert_passwd, L, "INSERT INTO %s (public_key, passwd) VALUES ( '%s', %lu);", PASSWD_TABLE_NAME, pk_hex, passwd);
        int ret_exec;
        ret_exec = sqlite3_exec(ppdb, sentence_insert_passwd, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s\n", errmsg);
                return;
        }
        
        char sentence_insert_account[L] = {0};
        snprintf(sentence_insert_account, L, "INSERT INTO %s (public_key) VALUES( '%s');", ACCOUNT_TABLE_NAME, pk_hex);        
        ret_exec = sqlite3_exec(ppdb, sentence_insert_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s\n", errmsg);
                return;
        }

        strncpy(explain_msg, sk_hex, KEY_HEX_LEN);
        // change status to noraml
        change_account_status(pk_hex, 0);
        return;
}

void opt_delete_account(char *current_public_key, Result_Type *result_type, char *explain_msg) {
        char sentence_delete_passwd[L] = {0};
        snprintf(sentence_delete_passwd, L, "DELETE FROM %s WHERE public_key = '%s';", PASSWD_TABLE_NAME, current_public_key);
        int ret_exec;
        ret_exec = sqlite3_exec(ppdb, sentence_delete_passwd, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s\n", errmsg);
                *result_type = Fail;
                return;
        }
        char sentence_delete_account[L] = {0};
        snprintf(sentence_delete_account, L, "DELETE FROM %s WHERE public_key = '%s';", ACCOUNT_TABLE_NAME, current_public_key);        
        ret_exec = sqlite3_exec(ppdb, sentence_delete_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s\n", errmsg);
                *result_type = Fail;
                return;
        }
        memset(explain_msg, 0, L);
        *result_type = Success;
        return;
}

sqlite3 *ppdb;
char *errmsg;
int ini_db() {          
        printf("start ini db\n");

        int ret_open = sqlite3_open(DB_PATH, &ppdb);
        if (ret_open != SQLITE_OK) {
                printf("db open fail:%s\n", sqlite3_errmsg(ppdb));
                return ERROR;
        }
        char sentence_create_passwd[L] = {0};                // create passwd table  
        snprintf(sentence_create_passwd , L, "CREATE TABLE IF NOT EXISTS %s(id INTEGER PRIMARY KEY AUTOINCREMENT, public_key TEXT UNIQUE NOT NULL, passwd INTEGER NOT NULL);", PASSWD_TABLE_NAME);
        
        char sentence_create_account[L] = {0};
        snprintf(sentence_create_account, L, "CREATE TABLE IF NOT EXISTS %s (public_key TEXT PRIMARY KEY NOT NULL, account_status INTEGER NOT NULL DEFAULT 0, balance INTEGER NOT NULL DEFAULT 0);", ACCOUNT_TABLE_NAME);
        
        char sentence_create_log[L] = {0};
        snprintf(sentence_create_log, L, "CREATE TABLE IF NOT EXISTS %s (table_id INTEGER PRIMARY KEY AUTOINCREMENT, public_key TEXT NOT NULL, operation_type TEXT NOT NULL, data INTEGER NOT NULL, destination TEXT NOT NULL, time TEXT NOT NULL);", LOG_TABLE_NAME);
        
        int ret_exec;
        ret_exec = sqlite3_exec(ppdb, sentence_create_passwd, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite_exec fail: %s\n", errmsg);
                return ERROR;
        }
        
        ret_exec = sqlite3_exec(ppdb, sentence_create_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite_exec fail: %s\n", errmsg);
                return ERROR;
        }
        
        ret_exec = sqlite3_exec(ppdb, sentence_create_log, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite_exec fail: %s\n", errmsg);
                return ERROR;
        }

        create_root_account_if_not_exits(123456);             // create root account
        printf("ini db finish\n");
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

        if ((atoi(resultp[1]) == Normal) || (atoi(resultp[1]) == Root) || (atoi(resultp[1]) == Locked)) {     
                memset(explain_msg, 0, L);
                *result_type = Success;
        } else {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "status abnormal", L);        
                *result_type = Fail;
        }

        sqlite3_free_table(resultp);
        return;
}

void check_illegal(MSG *msg, Result_Type *result_type, char *explain_msg, char *current_public_key) {
        if ((*result_type) == Fail) return;
        check_status(current_public_key, result_type, explain_msg);
        if ((*result_type) == Fail) return;

        if ((msg->opt_type) == Transfer) {
                check_status(current_public_key, result_type, explain_msg);
                if ((*result_type) == Fail) return;
        }
        memset(explain_msg, 0, L);
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
        const char *pk     = resultp[3];
        const char *status = resultp[4];
        const char *balance= resultp[5];

        snprintf(explain_msg, L,"\npublic_key: %s\nstatus: %s\nbalance: %s", pk, names_account_status[atoi(status)], balance);
        *result_type = Success;

        sqlite3_free_table(resultp);
        return;
}

void opt_query_log(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        char sentence_log[L] = {0};
        snprintf(sentence_log, L, "SELECT * FROM %s WHERE public_key = '%s' OR destination = '%s' ORDER BY time DESC LIMIT 50;", LOG_TABLE_NAME, public_key, public_key);

        int ret = select_func(sentence_log);
        if (ret == ERROR) {
                memset(explain_msg, 0, L);
                strncpy(explain_msg, errmsg, L);
                *result_type = Fail;
                return;
        }
        memset(explain_msg, 0, M);
        for (int i = 0; i <= (nrow * ncolumn); i += ncolumn) {
                snprintf(explain_msg, L, "\nid :%s public_key:%s operation_type:%s\n data:%s destination:%s time:%s\n", resultp[i + 0], resultp[i + 1], names_opt[atoi(resultp[i + 2])], resultp[i + 3], resultp[i + 4], resultp[i + 5]);
        }
        *result_type = Success;

        sqlite3_free_table(resultp);
        return;
}

void opt_query_log_root(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        char sentence_log[L] = {0};
        snprintf(sentence_log, L, "SELECT * FROM %s LIMIT 100;", LOG_TABLE_NAME);

        int ret = select_func(sentence_log);
        if (ret == ERROR) {
                memset(explain_msg, 0, L);
                strncpy(explain_msg, errmsg, L);
                *result_type = Fail;
                return;
        }
        memset(explain_msg, 0, M);
        for (int i = 0; i <= (nrow * ncolumn); i += ncolumn) {
                strcat(explain_msg, resultp[i + 0]);
                strcat(explain_msg, resultp[i + 1]);
                strcat(explain_msg, resultp[i + 2]);
                strcat(explain_msg, resultp[i + 3]);
                strcat(explain_msg, resultp[i + 4]);
                strcat(explain_msg, resultp[i + 5]);
                strcat(explain_msg, "\n");
        }
        *result_type = Success;

        sqlite3_free_table(resultp);
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
        memset(explain_msg, 0, L);
        *result_type = Success;
        return;
}

void opt_withdraw(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        if ((*result_type) == Fail) return;

        char sentence_withdraw[L] = {0};
        snprintf(sentence_withdraw, L, "UPDATE %s SET balance = balance - %lu WHERE public_key = '%s' AND balance >= %lu;", ACCOUNT_TABLE_NAME, msg->data, public_key, msg->data);
        
        int ret_exec = sqlite3_exec(ppdb, sentence_withdraw, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR || (sqlite3_changes(ppdb) != 1)) {
                strncpy(explain_msg, "money not enough", L);
                *result_type = Fail;
                return;
        }
        *result_type = Success;
        return;
}

void opt_freeze(char *public_key) {
        change_account_status(public_key, 2);   // frozen
        return;
}

void opt_defrost(char *public_key) {
        change_account_status(public_key, 0);   // normal
        return;
}

void opt_lock(char *public_key) {
        change_account_status(public_key, 1);   // lock
        return;
}

char sentence_begin[S] = "BEGIN TRANSACTION;";
char sentence_commit[S] = "COMMIT;";
char sentence_rollback[S] = "ROLLBACK";

void opt_account_db(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key) {
        if ((*result_type) == Fail) return;

        switch(msg->opt_type) {
                case Delete_account:
                        opt_delete_account(public_key, result_type, explain_msg);
                        break;
                case Query_self:
                        opt_query_self(msg, result_type, explain_msg, public_key);
                        break;
                case Query_log:
                        opt_query_log(msg, result_type, explain_msg, public_key);
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

                        opt_deposit(msg, result_type, explain_msg, msg->dst);
                        if ((*result_type == Fail) || (sqlite3_changes(ppdb) != 1)) {
                                int ret_exec = sqlite3_exec(ppdb, sentence_rollback, NULL, NULL, &errmsg);
                                if (ret_exec == SQLITE_ERROR) {
                                        memset(explain_msg, 0, L);
                                        strncpy(explain_msg, errmsg, L);
                                        *result_type = Fail;
                                        return;
                                }
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
                        // int ret = close_db();
                        // if (ret != OK) {
                        //         memset(explain_msg, 0, L);
                        //         strncpy(explain_msg, errmsg, L);
                        //         *result_type = Fail;
                        //         return;
                        // }
                        change_account_status(public_key, Normal);
                        memset(explain_msg, 0, L);
                        *result_type = Success;
                        break;
                }

                case Freeze: 
                        opt_freeze(msg->dst);
                        break;
                case Defrost:
                        opt_defrost(msg->dst);
                        break;
                case Query_log_root:
                        opt_query_log_root(msg, result_type, explain_msg, public_key);
                        break;
                default:
                        memset(explain_msg, 0, L);
                        strncpy(explain_msg, "operation not exist", L);
                        *result_type = Fail;
                        break;
        }
}

void opt_log_db(MSG *msg, char *public_key) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

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
        snprintf(sentence_insert_log, L, "INSERT INTO %s ( public_key, operation_type, data, destination, time) VALUES ('%s', '%s', %lu, '%s', '%s');", LOG_TABLE_NAME, public_key, names_opt[msg->opt_type], msg->data, dst, time_buffer);

        int ret_exec = sqlite3_exec(ppdb, sentence_insert_log, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s\n", errmsg);
                return;
        }
        return;
}