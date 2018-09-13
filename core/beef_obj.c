
#include <sys/tree.h>
#include <string.h>

struct beef_obj_s {
    char name[32];
    void *obj_p;
    RB_ENTRY(beef_obj_s) node;
};


static RB_HEAD(obj_tree_s, beef_obj_s) ObjHead = RB_INITIALIZER(ObjHead);

static int
cmp_obj(const struct beef_obj_s *obj_0,
        const struct beef_obj_s *obj_1)
{
    return strncmp(obj_0->name, obj_1->name, sizeof(obj_0->name));
}

RB_GENERATE_STATIC(obj_tree_s, beef_obj_s, node, cmp_obj);


void *
beef_find_obj(const char *name)
{

}

void *
beef_add_obj(const char *name,
             void *obj_p,
             const char **name_pp)
{

}

void *
beef_del_obj(const char *name)
{

}


