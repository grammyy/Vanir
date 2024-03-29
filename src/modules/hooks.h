#ifndef HOOKS
#define HOOKS

int hooksInit(lua_State* L);

enum dataType {
    number,
    string,
    integer,
    lua_bool,
    function
};

enum status {
    hook_awaiting,
    hook_update,
    hook_idle
};

struct callbacks {
    size_t dataSize;
    void* data;
    enum dataType dataType;
};

struct hookPool {
    struct hook *hooks;
    int count;
};

struct hook {
    const char *hookName;
    struct stack *stack;
    size_t pool;
    struct hook *address;
    void (*handle)(struct hook *, lua_State *);
    enum status status;
    struct callbacks *callback;
};

struct stack {
    const char *name;
    void (*func)(lua_State*, struct hook *instance, int, struct callbacks* callback);
    int ref;
};

// hooks ↓↓↓ hooks ///
void registerHook(struct hook hookData);
void addHook(struct hook *instance, const char *name, void (*func)(lua_State*, struct hook *instance, int, struct callbacks* callback), int ref);
void runHook(struct hook *instance, lua_State *L);
void freeHook(struct hook *instance, lua_State *L);
struct hook* findHook(const char* hookName);
// hooks ↑↑↑ hooks ///

// callbacks ↓↓↓ callbacks ///
struct callbacks* createCallback(size_t dataSize, enum dataType dataType);
void* getCallback(const struct callbacks* callback);
void setCallback(struct callbacks* callback, const void* data);
// callbacks ↑↑↑ callbacks ///

extern struct hookPool hookPool;

#endif