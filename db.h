#ifndef DB_H
#define DB_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>
#include <sodium.h>     // generating Ed25519 key, need gcc -lsodium

#define DB_PATH "./atm.db"
#define PASSWD_TABLE_NAME "passwd"
#define ACCOUNT_TABLE_NAME "account"
#define LOG_TABLE_NAME "log"
#define ROOT_INI "./root.txt"

#define SEED_LEN 32
#define KEY_LEN 32

#define OK 0
#define ERROR -1

#define S 128
#define M 256
#define L 512

// char sentence_create_passwd[L] = {0}; 
// snprintf(sentence_create_passwd , L, "CREATE TABLE IF NOT EXISTS %s
//(id INTEGER PRIMARY KEY AUTOINCREMENT, public_key TEXT UNIQUE NOT NULL, passwd INTEGER NOT NULL);", PASSWD_TABLE_NAME);

// char sentence_create_account[L] = {0};
// snprintf(sentence_create_account, L, "CREATE TABLE IF NOT EXISTS %s 
// (public_key TEXT PRIMARY KEY NOT NULL, account_status INTEGER NOT NULL DEFAULT 0, blance INTEGER NOT NULL DEFAULT 0);", ACCOUNT_TABLE_NAME);

// char sentence_create_log[L] = {0};
// snprintf(sentence_create_log, L, "CREATE TABLE IF NOT EXISTS %s 
// (table_id INTEGER PRIMARY KEY AUTOINCREMENT, public_key TEXT NOT NULL, operation_type TEXT NOT NULL, data INTEGER NOT NULL, detination TEXT NOT NULL, time TEXT NOT NULL);", LOG_TABLE_NAME);

typedef enum Opt_Type{
        Register,
        Login,
        Delete_account,
        Query_self,
        Query_log,
        Deposit,
        Withdraw,
        Transfer,
        Quit,
        Freeze,
        Defrost,
        Query_log_root
} opt_type;

typedef enum Result_Type {
        Fail = -1,
        Success = 0,
        Unknown = 1
} Result_Type;

typedef struct MSG {
        opt_type opt_type;
        unsigned char dst[S];
        unsigned long data;
} MSG;

typedef enum Account_Status {
        Normal,         // 0 idle account
        Locked,         // 1 login in account
        Frozen,         // 2 block by root
        Root            // 3 root user
} Account_Status;

unsigned long char_to_int(char *str);
void int_to_char(unsigned long int num, char *buf);

void change_account_status(char *public_key, Account_Status account_status);
void opt_freeze(char *public_key);
void opt_defrost(char *public_key);
void opt_lock(char *public_key);

void generate_account(unsigned long int passwd, unsigned char *explain_msg);
void create_root_account_if_not_exits(unsigned long int passwd);

extern sqlite3 *ppdb;
extern char *errmsg;
int ini_db();

int close_db();

extern char **resultp;
extern int nrow;
extern int ncolumn;
extern int select_func(char *select_sentence);

void check_status(char *public_key, Result_Type *result_type, char *explain_msg);
void check_illegal(MSG *msg, Result_Type *result_type, char *explain_msg, char *current_public_key);

void opt_query_self(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key);
void opt_query_log(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key);
void opt_query_log_root(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key);
void opt_deposit(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key);
void opt_withdraw(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_ke);
void opt_account_db(MSG *msg, Result_Type *result_type, char *explain_msg, char *public_key);
void opt_log_db(MSG *msg, char *public_key);

#endif
