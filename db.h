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

#define SEED_LEN 33
#define KEY_LEN 33

#define OK 0
#define ERROR -1

#define S 64
#define M 512
#define L 1024
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
