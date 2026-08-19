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
        Register,
        Login,
        Delete_account,
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
